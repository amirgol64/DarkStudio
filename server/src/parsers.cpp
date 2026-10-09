// DarkStudio server - parse the text output of Darknet, ov-bench and the comparison script.
// SPDX-License-Identifier: Apache-2.0

#include "parsers.hpp"

#include <algorithm>
#include <cctype>
#include <regex>


namespace darkstudio::parsers
{
	namespace
	{
		std::string lower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}

		/// "100.00%" -> 100.0, "none" -> null
		Json::Value percent_or_null(const std::string & text)
		{
			try
			{
				if (not text.empty() and (std::isdigit(static_cast<unsigned char>(text[0])) or text[0] == '.'))
				{
					return std::stod(text);
				}
			}
			catch (...)
			{
			}
			return Json::nullValue;
		}
	}


	std::string strip_ansi(const std::string & text)
	{
		static const std::regex ansi("\x1B\\[[0-9;]*[A-Za-z]");
		return std::regex_replace(text, ansi, "");
	}


	LogLevel classify(const std::string & line)
	{
		const std::string l = lower(line);

		// lines that contain the words but are not problems
		if (l.find("error=0") != std::string::npos or l.find("errors: 0") != std::string::npos or
			l.find("max err") != std::string::npos or l.find("class mismatches: 0") != std::string::npos)
		{
			return LogLevel::info;
		}

		static const std::regex error_words(R"((^|[^a-z])(error|exception|fatal|failed|failure|abort(ed)?|segmentation fault|access violation|not supported|exceeds limit|nan\b))");
		if (std::regex_search(l, error_words))
		{
			return LogLevel::error;
		}
		if (l.find("warning") != std::string::npos or l.find("cannot") != std::string::npos)
		{
			return LogLevel::warning;
		}
		return LogLevel::info;
	}


	const char * to_string(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::error:	return "error";
			case LogLevel::warning:	return "warning";
			default:				return "info";
		}
	}


	std::optional<Json::Value> training_iteration(const std::string & line)
	{
		static const std::regex rx(
			R"(^\s*(\d+):\s*loss=([-\d.naif]+),\s*avg loss=([-\d.naif]+),\s*last=([^,]+),\s*best=([^,]+),\s*mAP=([^,]+),\s*rate=([\d.eE+-]+),\s*load\s+(\d+)=([\d.]+)\s*(milliseconds|seconds),\s*train=([\d.]+)\s*(milliseconds|seconds),\s*(\d+)\s+images,\s*time remaining=(.*)$)");
		std::smatch m;
		if (not std::regex_search(line, m, rx))
		{
			return std::nullopt;
		}

		const auto to_double = [](const std::string & s) -> Json::Value
		{
			try { return std::stod(s); } catch (...) { return Json::nullValue; }
		};

		Json::Value j;
		j["iteration"]		= std::stoi(m[1]);
		j["loss"]			= to_double(m[2]);
		j["avg_loss"]		= to_double(m[3]);
		j["last_map"]		= percent_or_null(m[4]);
		j["best_map"]		= percent_or_null(m[5]);
		j["rate"]			= to_double(m[7]);
		j["batch"]			= std::stoi(m[8]);
		const double load	= std::stod(m[9]);
		j["load_ms"]		= m[10] == "seconds" ? load * 1000.0 : load;
		const double train	= std::stod(m[11]);
		j["train_seconds"]	= m[12] == "milliseconds" ? train / 1000.0 : train;
		j["images"]			= Json::Int64(std::stoll(m[13]));
		j["time_remaining"]	= std::string(m[14]);
		return j;
	}


	std::optional<Json::Value> map_result(const std::string & line)
	{
		static const std::regex rx(R"(mean average precision \(mAP@([\d.]+)\)\s*=\s*([\d.]+)\s*%)");
		std::smatch m;
		if (not std::regex_search(line, m, rx))
		{
			return std::nullopt;
		}
		Json::Value j;
		j["iou"] = std::stod(m[1]);
		j["map"] = std::stod(m[2]);
		return j;
	}


	void OvBench::add(const std::string & raw)
	{
		const std::string line = strip_ansi(raw);
		static const std::regex device_rx(R"(^\s+device\s+(\S+)\s+(.+)$)");
		static const std::regex config_rx(R"(^device\s+(\S+), precision hint (\S+), compile (\d+) ms)");
		static const std::regex image_rx(R"(^(\S+\.(?:jpg|jpeg|png|bmp))\s+(\d+)x(\d+)$)", std::regex::icase);
		static const std::regex infer_rx(R"(^\s+infer\s+mean ([\d.]+) ms, median ([\d.]+), min ([\d.]+), p95 ([\d.]+))");
		static const std::regex total_rx(R"(^\s+total\s+mean ([\d.]+) ms \(pre ([\d.]+) \+ infer \+ post ([\d.]+)\) = ([\d.]+) FPS)");
		static const std::regex det_rx(R"(^\s+(.+?)\s+(\d+)%\s+x=(-?\d+) y=(-?\d+) w=(\d+) h=(\d+)$)");

		std::smatch m;
		if (std::regex_search(line, m, device_rx))
		{
			Json::Value d;
			d["id"] = std::string(m[1]);
			d["name"] = std::string(m[2]);
			devices.append(d);
		}
		else if (std::regex_search(line, m, config_rx))
		{
			device		= m[1];
			precision	= m[2];
			compile_ms	= std::stod(m[3]);
		}
		else if (std::regex_search(line, m, image_rx))
		{
			Json::Value img;
			img["image"]		= std::string(m[1]);
			img["width"]		= std::stoi(m[2]);
			img["height"]		= std::stoi(m[3]);
			img["detections"]	= Json::arrayValue;
			images.append(img);
		}
		else if (not images.empty() and std::regex_search(line, m, infer_rx))
		{
			auto & img = images[images.size() - 1];
			img["infer_mean_ms"]	= std::stod(m[1]);
			img["infer_median_ms"]	= std::stod(m[2]);
			img["infer_min_ms"]		= std::stod(m[3]);
			img["infer_p95_ms"]		= std::stod(m[4]);
		}
		else if (not images.empty() and std::regex_search(line, m, total_rx))
		{
			auto & img = images[images.size() - 1];
			img["total_mean_ms"]	= std::stod(m[1]);
			img["pre_ms"]			= std::stod(m[2]);
			img["post_ms"]			= std::stod(m[3]);
			img["fps"]				= std::stod(m[4]);
		}
		else if (not images.empty() and std::regex_search(line, m, det_rx))
		{
			Json::Value d;
			d["name"]		= std::string(m[1]);
			d["confidence"]	= std::stoi(m[2]);
			d["x"]			= std::stoi(m[3]);
			d["y"]			= std::stoi(m[4]);
			d["w"]			= std::stoi(m[5]);
			d["h"]			= std::stoi(m[6]);
			images[images.size() - 1]["detections"].append(d);
		}
	}


	Json::Value OvBench::result() const
	{
		Json::Value r;
		r["device"]		= device;
		r["precision"]	= precision;
		r["compile_ms"]	= compile_ms;
		r["devices"]	= devices;
		r["images"]		= images;

		double infer = 0.0, total = 0.0;
		int n = 0;
		for (const auto & img : images)
		{
			if (img.isMember("infer_mean_ms") and img.isMember("total_mean_ms"))
			{
				infer += img["infer_mean_ms"].asDouble();
				total += img["total_mean_ms"].asDouble();
				n ++;
			}
		}
		if (n > 0)
		{
			r["infer_mean_ms"]	= infer / n;
			r["total_mean_ms"]	= total / n;
			r["fps"]			= 1000.0 / (total / n);
		}
		return r;
	}


	void CompareCpuSycl::add(const std::string & raw)
	{
		const std::string line = strip_ansi(raw);
		static const std::regex det_rx(R"(^(\S+)\s+(\S+)\s+cpu\s+([\d.]+)%\s+sycl\s+([\d.]+)%\s+diff\s+([\d.]+)\s+box diff\s+(\d+)px)");
		static const std::regex count_rx(R"(^(\S+)\s+DIFFERENT COUNT: cpu=(\d+) sycl=(\d+))");
		static const std::regex summary_rx(R"(class mismatches: (\d+), worst probability difference: ([\d.]+), worst box difference: (\d+)px)");
		static const std::regex steady_rx(R"(steady state: cpu ([\d.,]+) ms, sycl ([\d.,]+) ms per image)");
		static const std::regex first_rx(R"(first image:\s+cpu (.+?), sycl (.+)$)");

		const auto number = [](std::string s)
		{
			s.erase(std::remove(s.begin(), s.end(), ','), s.end());
			return std::stod(s);
		};

		std::smatch m;
		if (std::regex_search(line, m, det_rx))
		{
			Json::Value d;
			d["image"]		= std::string(m[1]);
			d["name"]		= std::string(m[2]);
			d["cpu"]		= std::stod(m[3]);
			d["sycl"]		= std::stod(m[4]);
			d["diff"]		= std::stod(m[5]);
			d["box_diff"]	= std::stoi(m[6]);
			detections.append(d);
		}
		else if (std::regex_search(line, m, count_rx))
		{
			Json::Value d;
			d["image"]		= std::string(m[1]);
			d["count_cpu"]	= std::stoi(m[2]);
			d["count_sycl"]	= std::stoi(m[3]);
			detections.append(d);
		}
		else if (std::regex_search(line, m, summary_rx))
		{
			summary["class_mismatches"]		= std::stoi(m[1]);
			summary["worst_probability_diff"] = std::stod(m[2]);
			summary["worst_box_diff"]		= std::stoi(m[3]);
		}
		else if (std::regex_search(line, m, steady_rx))
		{
			summary["cpu_ms"]	= number(m[1]);
			summary["sycl_ms"]	= number(m[2]);
			summary["speedup"]	= number(m[1]) / number(m[2]);
		}
		else if (std::regex_search(line, m, first_rx))
		{
			summary["cpu_first"]	= std::string(m[1]);
			summary["sycl_first"]	= std::string(m[2]);
		}
	}


	Json::Value CompareCpuSycl::result() const
	{
		Json::Value r = summary;
		r["detections"] = detections;
		return r;
	}


	Json::Value darknet_version(const std::vector<std::string> & raw_lines)
	{
		static const std::regex version_rx(R"rx(Darknet V\d+ "([^"]+)" (v[\w.\-]+))rx");
		static const std::regex sycl_rx(R"(^SYCL ([\d.]+))");
		static const std::regex gpu_rx(R"(^=> (\d+): (.+?) \[(.+)\], (.+)$)");
		static const std::regex cpu_rx(R"(compiled to use the CPU)");

		Json::Value j;
		j["gpus"] = Json::arrayValue;
		j["backend"] = "unknown";
		Json::Value details = Json::arrayValue;

		for (const auto & raw : raw_lines)
		{
			const std::string line = strip_ansi(raw);
			if (line.empty())
			{
				continue;
			}
			details.append(line);
			std::smatch m;
			if (std::regex_search(line, m, version_rx))
			{
				j["codename"]	= std::string(m[1]);
				j["version"]	= std::string(m[2]);
			}
			else if (std::regex_search(line, m, sycl_rx))
			{
				j["backend"]		= "sycl";
				j["sycl_version"]	= std::string(m[1]);
			}
			else if (std::regex_search(line, m, cpu_rx))
			{
				j["backend"] = "cpu";
			}
			else if (std::regex_search(line, m, gpu_rx))
			{
				Json::Value g;
				g["index"]	= std::stoi(m[1]);
				g["name"]	= std::string(m[2]);
				g["info"]	= std::string(m[3]);
				g["memory"]	= std::string(m[4]);
				j["gpus"].append(g);
			}
		}
		j["details"] = details;
		return j;
	}


	Json::Value openvino_devices(const std::vector<std::string> & lines)
	{
		Json::Value j;
		j["devices"] = Json::arrayValue;
		for (const auto & line : lines)
		{
			const auto tab = line.find('\t');
			if (tab == std::string::npos)
			{
				continue;
			}
			const std::string key = line.substr(0, tab);
			const std::string value = line.substr(tab + 1);
			if (key == "openvino")
			{
				j["version"] = value;
			}
			else
			{
				Json::Value d;
				d["id"] = key;
				d["name"] = value;
				j["devices"].append(d);
			}
		}
		return j;
	}
}
