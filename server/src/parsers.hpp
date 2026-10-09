// DarkStudio server - parse the text output of Darknet, ov-bench and the comparison script.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json/json.h>

#include <optional>
#include <string>
#include <vector>

namespace darkstudio::parsers
{
	enum class LogLevel { info, warning, error };

	/// Classify one output line so the UI can highlight problems (SYCL/JIT errors, CUDA-compat errors, ...).
	LogLevel classify(const std::string & line);

	const char * to_string(LogLevel level);

	/// Darknet training progress, e.g.
	/// "12: loss=1.395, avg loss=32.521, last=none, best=none, mAP=DISABLED, rate=0.00261000, load 64=89.7 milliseconds,
	///  train=1.1 seconds, 768 images, time remaining=8 seconds"
	/// Returns {iteration, loss, avg_loss, rate, train_seconds, load_ms, images, time_remaining, last_map, best_map}.
	std::optional<Json::Value> training_iteration(const std::string & line);

	/// "-> mean average precision (mAP@0.50)=100.00%"  ->  {iou: 0.5, map: 100.0}
	std::optional<Json::Value> map_result(const std::string & line);

	/// Incremental parser for ov-bench output; call add() for every line, then result().
	class OvBench
	{
		public:

			void add(const std::string & line);
			Json::Value result() const;

		private:

			Json::Value devices = Json::arrayValue;
			Json::Value images = Json::arrayValue;
			std::string device;
			std::string precision;
			double compile_ms = -1.0;
	};

	/// Incremental parser for tools/sycl-migrate/compare-cpu-sycl.ps1 output.
	class CompareCpuSycl
	{
		public:

			void add(const std::string & line);
			Json::Value result() const;

		private:

			Json::Value detections = Json::arrayValue;
			Json::Value summary = Json::objectValue;
	};

	/// Parse "darknet --version" output: version, backend line, GPU list.
	Json::Value darknet_version(const std::vector<std::string> & lines);

	/// Parse "ov-bench --list-devices" output ("openvino\t<version>" then "<device>\t<name>").
	Json::Value openvino_devices(const std::vector<std::string> & lines);

	/// Remove ANSI colour codes that Darknet adds to its output.
	std::string strip_ansi(const std::string & text);
}
