// DarkStudio - ov-bench: run Darknet ONNX models with OpenVINO and measure speed.
// SPDX-License-Identifier: Apache-2.0
//
// Expects an ONNX file made by darknet_onnx_export with box post-processing enabled (the default):
//   input  "frame" [1, 3, H, W]  RGB, float 0..1, plain resize (no letterbox)
//   output "confs" [1, N, C]     objectness x class probability
//   output "boxes" [1, N, 1, 4]  normalized x1, y1, x2, y2
//
// Usage: ov-bench <model.onnx> <names> <image> [image...] [--device GPU] [--precision f16|f32]
//                 [--iters 50] [--threshold 0.5] [--nms 0.45] [--cache <dir>] [--save <dir>] [--raw]

#include <openvino/openvino.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

namespace
{
	using Clock = std::chrono::steady_clock;

	double ms_since(const Clock::time_point & start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}

	struct Options
	{
		std::string model;
		std::string names;
		std::vector<std::string> images;
		std::string device		= "GPU";
		std::string precision	= "f16";
		int iterations			= 50;
		float threshold			= 0.5f;
		float nms				= 0.45f;
		std::string cache_dir;
		std::string save_dir;
		bool raw				= false;
	};

	struct Detection
	{
		int class_id;
		float confidence;
		cv::Rect box;
	};

	Options parse(int argc, char * argv[])
	{
		Options opt;
		std::vector<std::string> positional;
		for (int i = 1; i < argc; i ++)
		{
			const std::string arg = argv[i];
			auto next = [&]() -> std::string
			{
				if (i + 1 >= argc)
				{
					throw std::invalid_argument("missing value after " + arg);
				}
				return argv[++ i];
			};

			if		(arg == "--device"		) opt.device		= next();
			else if	(arg == "--precision"	) opt.precision		= next();
			else if	(arg == "--iters"		) opt.iterations	= std::stoi(next());
			else if	(arg == "--threshold"	) opt.threshold		= std::stof(next());
			else if	(arg == "--nms"			) opt.nms			= std::stof(next());
			else if	(arg == "--cache"		) opt.cache_dir		= next();
			else if	(arg == "--save"		) opt.save_dir		= next();
			else if	(arg == "--raw"			) opt.raw			= true;
			else positional.push_back(arg);
		}

		if (positional.size() < 3)
		{
			throw std::invalid_argument("usage: ov-bench <model.onnx> <names> <image> [image...] [--device GPU|CPU|AUTO] "
				"[--precision f16|f32] [--iters N] [--threshold 0.5] [--nms 0.45] [--cache dir] [--save dir] [--raw]");
		}
		opt.model	= positional[0];
		opt.names	= positional[1];
		opt.images	.assign(positional.begin() + 2, positional.end());
		return opt;
	}

	std::vector<std::string> load_names(const std::string & filename)
	{
		std::vector<std::string> names;
		std::ifstream ifs(filename);
		std::string line;
		while (std::getline(ifs, line))
		{
			if (not line.empty() and line.back() == '\r')
			{
				line.pop_back();
			}
			if (not line.empty())
			{
				names.push_back(line);
			}
		}
		return names;
	}

	/// Darknet-style preprocessing: BGR->RGB, plain resize to the network size, scale to 0..1, NCHW.
	void preprocess(const cv::Mat & bgr, ov::Tensor & tensor)
	{
		const auto shape	= tensor.get_shape();	// [1, 3, H, W]
		const int h			= static_cast<int>(shape[2]);
		const int w			= static_cast<int>(shape[3]);

		cv::Mat resized;
		cv::resize(bgr, resized, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);
		cv::Mat rgb;
		cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

		float * data = tensor.data<float>();
		std::vector<cv::Mat> planes =
		{
			cv::Mat(h, w, CV_32FC1, data + 0 * h * w),
			cv::Mat(h, w, CV_32FC1, data + 1 * h * w),
			cv::Mat(h, w, CV_32FC1, data + 2 * h * w),
		};
		cv::Mat rgb_float;
		rgb.convertTo(rgb_float, CV_32FC3, 1.0 / 255.0);
		cv::split(rgb_float, planes);	// writes straight into the tensor memory
	}

	std::vector<Detection> decode(const ov::Tensor & confs, const ov::Tensor & boxes, const cv::Size & image_size, const Options & opt)
	{
		const auto cshape		= confs.get_shape();	// [1, N, C]
		const size_t n			= cshape[1];
		const size_t classes	= cshape[2];
		const float * c			= confs.data<const float>();
		const float * b			= boxes.data<const float>();

		// per-class candidates, then per-class NMS (same idea as Darknet's do_nms_sort)
		std::map<int, std::vector<cv::Rect>> class_boxes;
		std::map<int, std::vector<float>> class_scores;

		for (size_t i = 0; i < n; i ++)
		{
			const float * row = c + i * classes;
			const float * box = b + i * 4;
			for (size_t k = 0; k < classes; k ++)
			{
				if (row[k] < opt.threshold)
				{
					continue;
				}
				const int x1 = static_cast<int>(std::round(box[0] * image_size.width));
				const int y1 = static_cast<int>(std::round(box[1] * image_size.height));
				const int x2 = static_cast<int>(std::round(box[2] * image_size.width));
				const int y2 = static_cast<int>(std::round(box[3] * image_size.height));
				const cv::Rect rect = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)) & cv::Rect(cv::Point(0, 0), image_size);
				class_boxes[static_cast<int>(k)].push_back(rect);
				class_scores[static_cast<int>(k)].push_back(row[k]);

				if (opt.raw)
				{
					std::cout << std::setprecision(4) << "  raw #" << i << " class=" << k << " conf=" << row[k]
						<< " box=[" << box[0] << ", " << box[1] << ", " << box[2] << ", " << box[3] << "]" << std::endl;
				}
			}
		}

		std::vector<Detection> detections;
		for (const auto & [class_id, rects] : class_boxes)
		{
			std::vector<int> keep;
			cv::dnn::NMSBoxes(rects, class_scores[class_id], opt.threshold, opt.nms, keep);
			for (const int idx : keep)
			{
				detections.push_back({class_id, class_scores[class_id][idx], rects[idx]});
			}
		}
		std::sort(detections.begin(), detections.end(), [](const auto & lhs, const auto & rhs) { return lhs.confidence > rhs.confidence; });
		return detections;
	}

	struct Stats
	{
		double mean;
		double median;
		double min;
		double p95;
	};

	Stats stats(std::vector<double> v)
	{
		std::sort(v.begin(), v.end());
		const double mean = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
		return {mean, v[v.size() / 2], v.front(), v[std::min(v.size() - 1, (v.size() * 95) / 100)]};
	}
}


int main(int argc, char * argv[])
{
	try
	{
		const Options opt	= parse(argc, argv);
		const auto names	= load_names(opt.names);

		ov::Core core;
		if (not opt.cache_dir.empty())
		{
			core.set_property(ov::cache_dir(opt.cache_dir));
		}

		std::cout << "OpenVINO " << ov::get_openvino_version() << std::endl;
		for (const auto & device : core.get_available_devices())
		{
			std::cout << "  device " << std::left << std::setw(6) << device << " "
				<< core.get_property(device, ov::device::full_name) << std::endl;
		}

		auto start = Clock::now();
		auto model = core.read_model(opt.model);
		const double read_ms = ms_since(start);

		ov::AnyMap config = {ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY)};
		config.emplace(ov::hint::inference_precision.name(), opt.precision == "f32" ? ov::element::f32 : ov::element::f16);

		start = Clock::now();
		auto compiled = core.compile_model(model, opt.device, config);
		const double compile_ms = ms_since(start);

		auto request = compiled.create_infer_request();
		auto input = request.get_input_tensor();

		std::cout
			<< "model     " << opt.model << " (read " << std::fixed << std::setprecision(0) << read_ms << " ms)" << std::endl
			<< "device    " << opt.device << ", precision hint " << opt.precision
			<< ", compile " << compile_ms << " ms" << (opt.cache_dir.empty() ? "" : " (cache enabled)") << std::endl
			<< "input     " << input.get_shape() << std::endl;

		for (const auto & filename : opt.images)
		{
			const cv::Mat image = cv::imread(filename);
			if (image.empty())
			{
				std::cerr << "cannot read image " << filename << std::endl;
				continue;
			}

			// warm-up (the first GPU inference is always slower)
			preprocess(image, input);
			request.infer();

			std::vector<double> pre_ms, infer_ms, post_ms, total_ms;
			std::vector<Detection> detections;
			for (int i = 0; i < opt.iterations; i ++)
			{
				const auto t0 = Clock::now();
				preprocess(image, input);
				const auto t1 = Clock::now();
				request.infer();
				const auto t2 = Clock::now();
				Options decode_opt = opt;
				decode_opt.raw = (opt.raw and i == 0);
				detections = decode(request.get_tensor("confs"), request.get_tensor("boxes"), image.size(), decode_opt);
				const auto t3 = Clock::now();

				pre_ms	.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
				infer_ms.push_back(std::chrono::duration<double, std::milli>(t2 - t1).count());
				post_ms	.push_back(std::chrono::duration<double, std::milli>(t3 - t2).count());
				total_ms.push_back(std::chrono::duration<double, std::milli>(t3 - t0).count());
			}

			const auto inf = stats(infer_ms);
			const auto tot = stats(total_ms);
			std::cout << std::endl << std::filesystem::path(filename).filename().string()
				<< " " << image.cols << "x" << image.rows << std::endl
				<< std::setprecision(1)
				<< "  infer   mean " << inf.mean << " ms, median " << inf.median << ", min " << inf.min << ", p95 " << inf.p95 << std::endl
				<< "  total   mean " << tot.mean << " ms (pre " << stats(pre_ms).mean << " + infer + post " << stats(post_ms).mean
				<< ") = " << 1000.0 / tot.mean << " FPS" << std::endl;

			for (const auto & d : detections)
			{
				const std::string name = d.class_id < static_cast<int>(names.size()) ? names[d.class_id] : std::to_string(d.class_id);
				std::cout << "  " << std::left << std::setw(10) << name << std::right << std::setprecision(0)
					<< std::setw(3) << d.confidence * 100.0f << "%  "
					<< "x=" << d.box.x << " y=" << d.box.y << " w=" << d.box.width << " h=" << d.box.height << std::endl;
			}

			if (not opt.save_dir.empty())
			{
				cv::Mat annotated = image.clone();
				for (const auto & d : detections)
				{
					const std::string name = d.class_id < static_cast<int>(names.size()) ? names[d.class_id] : std::to_string(d.class_id);
					cv::rectangle(annotated, d.box, cv::Scalar(0, 200, 255), 2);
					cv::putText(annotated, name + " " + std::to_string(static_cast<int>(d.confidence * 100.0f)) + "%",
						d.box.tl() + cv::Point(2, 16), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 200, 255), 2);
				}
				std::filesystem::create_directories(opt.save_dir);
				const auto out = std::filesystem::path(opt.save_dir) / (std::filesystem::path(filename).stem().string() + "_" + opt.device + ".jpg");
				cv::imwrite(out.string(), annotated);
			}
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
