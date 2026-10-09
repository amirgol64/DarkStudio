// DarkStudio server - unit tests for the output parsers and the datasets path check.
// SPDX-License-Identifier: Apache-2.0
//
// The sample lines are real output captured from Darknet, ov-bench and compare-cpu-sycl.ps1 on 2026-10-09.

#include "datasets.hpp"
#include "parsers.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	int failures = 0;
	int checks = 0;

	void check(bool ok, const std::string & what, int line)
	{
		checks ++;
		if (not ok)
		{
			failures ++;
			std::cerr << "FAILED (line " << line << "): " << what << std::endl;
		}
	}

	#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)
	#define CHECK_NEAR(a, b) check(std::abs((a) - (b)) < 1e-6, #a " == " #b, __LINE__)

	using namespace darkstudio;

	void test_training_line()
	{
		const auto j = parsers::training_iteration(
			"12: loss=1.395, avg loss=32.521, last=none, best=none, mAP=DISABLED, rate=0.00261000, load 64=89.7 milliseconds, "
			"train=1.1 seconds, 768 images, time remaining=8 seconds");
		CHECK(j.has_value());
		CHECK((*j)["iteration"].asInt() == 12);
		CHECK_NEAR((*j)["loss"].asDouble(), 1.395);
		CHECK_NEAR((*j)["avg_loss"].asDouble(), 32.521);
		CHECK_NEAR((*j)["rate"].asDouble(), 0.00261);
		CHECK_NEAR((*j)["train_seconds"].asDouble(), 1.1);
		CHECK_NEAR((*j)["load_ms"].asDouble(), 89.7);
		CHECK((*j)["last_map"].isNull());
		CHECK((*j)["time_remaining"].asString() == "8 seconds");

		const auto k = parsers::training_iteration(
			"1000: loss=0.095, avg loss=0.092, last=100.00%, best=100.00%, mAP=1033, rate=0.00002610, load 64=181.5 milliseconds, "
			"train=1.2 seconds, 64000 images, time remaining=unknown");
		CHECK(k.has_value());
		CHECK_NEAR((*k)["last_map"].asDouble(), 100.0);
		CHECK((*k)["images"].asInt64() == 64000);

		CHECK(not parsers::training_iteration("Saving weights to C:/x/LegoGears_final.weights").has_value());
	}

	void test_map_line()
	{
		const auto m = parsers::map_result("-> mean average precision (mAP@0.50)=100.00%");
		CHECK(m.has_value());
		CHECK_NEAR((*m)["iou"].asDouble(), 0.5);
		CHECK_NEAR((*m)["map"].asDouble(), 100.0);
		CHECK(not parsers::map_result("Calculating mAP (mean average precision) with threshold 0.25").has_value());
	}

	void test_classify()
	{
		CHECK(parsers::classify("error: Total size of kernel arguments exceeds limit!") == parsers::LogLevel::error);
		CHECK(parsers::classify("Exception caught at file:maxpool_layer_kernels.dp.cpp, line:270") == parsers::LogLevel::error);
		CHECK(parsers::classify("class mismatches: 0, worst probability difference: 0.0000, worst box difference: 0px") == parsers::LogLevel::info);
		CHECK(parsers::classify("  sgemm  1024 x  1024 x  1024      1.62 ms   1325 GFLOPS  max err 1.1e-05") == parsers::LogLevel::info);
		CHECK(parsers::classify("warning: loop not vectorized") == parsers::LogLevel::warning);
		CHECK(parsers::classify("12: loss=1.395, avg loss=32.521") == parsers::LogLevel::info);
		CHECK(parsers::classify("12: loss=nan, avg loss=nan") == parsers::LogLevel::error);
	}

	void test_ov_bench()
	{
		parsers::OvBench p;
		for (const char * line : {
			"  device CPU    11th Gen Intel(R) Core(TM) i5-1135G7 @ 2.40GHz",
			"  device GPU    Intel(R) Iris(R) Xe Graphics (iGPU)",
			"device    GPU, precision hint f16, compile 129 ms (cache enabled)",
			"",
			"dog.jpg 768x576",
			"  infer   mean 9.8 ms, median 9.1, min 8.8, p95 11.4",
			"  total   mean 13.0 ms (pre 1.9 + infer + post 1.4) = 77.0 FPS",
			"  dog        87%  x=137 y=205 w=182 h=337",
			"  truck      82%  x=465 y=79 w=239 h=92",
			"giraffe.jpg 500x500",
			"  infer   mean 8.6 ms, median 8.5, min 8.2, p95 9.0",
			"  total   mean 11.0 ms (pre 1.0 + infer + post 1.4) = 90.9 FPS",
			"  giraffe    85%  x=170 y=0 w=250 h=430" })
		{
			p.add(line);
		}
		const auto r = p.result();
		CHECK(r["device"].asString() == "GPU");
		CHECK(r["precision"].asString() == "f16");
		CHECK_NEAR(r["compile_ms"].asDouble(), 129.0);
		CHECK(r["devices"].size() == 2);
		CHECK(r["images"].size() == 2);
		CHECK(r["images"][0]["detections"].size() == 2);
		CHECK(r["images"][0]["detections"][0]["name"].asString() == "dog");
		CHECK(r["images"][0]["detections"][0]["confidence"].asInt() == 87);
		CHECK_NEAR(r["total_mean_ms"].asDouble(), 12.0);
		CHECK_NEAR(r["infer_mean_ms"].asDouble(), 9.2);
	}

	void test_compare()
	{
		parsers::CompareCpuSycl p;
		for (const char * line : {
			"dog.jpg      dog      cpu  87.0%  sycl  87.0%  diff 0.0000  box diff 0px",
			"horses.jpg   horse    cpu  45.8%  sycl  45.8%  diff 0.0000  box diff 0px",
			"",
			"class mismatches: 0, worst probability difference: 0.0000, worst box difference: 0px",
			"first image:  cpu 134.919 milliseconds, sycl 1.735 seconds",
			"steady state: cpu 122.4 ms, sycl 26.4 ms per image" })
		{
			p.add(line);
		}
		const auto r = p.result();
		CHECK(r["detections"].size() == 2);
		CHECK(r["class_mismatches"].asInt() == 0);
		CHECK_NEAR(r["cpu_ms"].asDouble(), 122.4);
		CHECK_NEAR(r["sycl_ms"].asDouble(), 26.4);
		CHECK(r["speedup"].asDouble() > 4.6 and r["speedup"].asDouble() < 4.7);
		CHECK(r["sycl_first"].asString() == "1.735 seconds");
	}

	void test_darknet_version()
	{
		const auto j = parsers::darknet_version({
			"Darknet V5 \"Moonlit\" v5.1-105-g4c15e64c-dirty [sycl] [Oct  9 2026]",
			"SYCL 9.0.0 (Intel oneAPI), oneMKL BLAS and RNG, cuDNN is not used",
			"=> 0: Intel(R) Iris(R) Xe Graphics [Level Zero, 80 compute units, fp16], 14.5 GiB",
			"OpenCV 4.14.0, Windows 11 Home" });
		CHECK(j["version"].asString() == "v5.1-105-g4c15e64c-dirty");
		CHECK(j["codename"].asString() == "Moonlit");
		CHECK(j["backend"].asString() == "sycl");
		CHECK(j["sycl_version"].asString() == "9.0.0");
		CHECK(j["gpus"].size() == 1);
		CHECK(j["gpus"][0]["name"].asString() == "Intel(R) Iris(R) Xe Graphics");
		CHECK(j["gpus"][0]["memory"].asString() == "14.5 GiB");

		const auto cpu = parsers::darknet_version({
			"Darknet V5 \"Moonlit\" v5.1-104-gf684e1d7 [Oct  9 2026]",
			"Darknet is compiled to use the CPU.  GPU is disabled." });
		CHECK(cpu["backend"].asString() == "cpu");
		CHECK(cpu["gpus"].empty());
	}

	void test_openvino_devices()
	{
		const auto j = parsers::openvino_devices({
			"openvino\t2026.4.1-22982-e213a147257-releases/2026/4",
			"CPU\t11th Gen Intel(R) Core(TM) i5-1135G7 @ 2.40GHz",
			"GPU\tIntel(R) Iris(R) Xe Graphics (iGPU)" });
		CHECK(j["version"].asString().rfind("2026.4.1", 0) == 0);
		CHECK(j["devices"].size() == 2);
		CHECK(j["devices"][1]["id"].asString() == "GPU");
	}

	void test_strip_ansi()
	{
		CHECK(parsers::strip_ansi("\x1B[1;32mIris Xe\x1B[0m ok") == "Iris Xe ok");
	}

	void test_datasets()
	{
		namespace fs = std::filesystem;
		const fs::path root = fs::temp_directory_path() / "darkstudio-tests" / "datasets";
		fs::create_directories(root / "set1");
		std::ofstream(root / "set1" / "a.jpg") << "not really a jpeg";
		std::ofstream(root / "set1" / "a.txt") << "1 0.5 0.5 0.25 0.25\n";
		std::ofstream(root / "set1" / "x.names") << "zero\none\n";

		bool rejected = false;
		try { datasets::resolve(root, "../../outside.txt"); } catch (const std::invalid_argument &) { rejected = true; }
		CHECK(rejected);
		CHECK(datasets::resolve(root, "set1/a.jpg") == fs::weakly_canonical(root / "set1" / "a.jpg"));

		const auto list = datasets::list(root);
		CHECK(list.size() == 1);
		CHECK(list[0]["images"].asInt() == 1);
		CHECK(list[0]["labeled"].asInt() == 1);
		CHECK(list[0]["with_boxes"].asInt() == 1);
		CHECK(list[0]["names"].size() == 2);

		const auto labels = datasets::read_labels(root, "set1/a.jpg");
		CHECK(labels["boxes"].size() == 1);
		CHECK(labels["boxes"][0]["class"].asInt() == 1);

		Json::Value boxes = Json::arrayValue;
		Json::Value b;
		b["class"] = 0; b["x"] = 0.25; b["y"] = 0.75; b["w"] = 0.1; b["h"] = 0.2;
		boxes.append(b);
		const auto written = datasets::write_labels(root, "set1/a.jpg", boxes);
		CHECK(written["boxes"].size() == 1);
		CHECK_NEAR(written["boxes"][0]["x"].asDouble(), 0.25);

		bool invalid = false;
		b["w"] = 1.5;
		Json::Value bad = Json::arrayValue;
		bad.append(b);
		try { datasets::write_labels(root, "set1/a.jpg", bad); } catch (const std::invalid_argument &) { invalid = true; }
		CHECK(invalid);

		fs::remove_all(root.parent_path());
	}
}


int main()
{
	test_training_line();
	test_map_line();
	test_classify();
	test_ov_bench();
	test_compare();
	test_darknet_version();
	test_openvino_devices();
	test_strip_ansi();
	test_datasets();

	std::cout << (failures == 0 ? "OK" : "FAILED") << ": " << (checks - failures) << "/" << checks << " checks passed" << std::endl;
	return failures == 0 ? 0 : 1;
}
