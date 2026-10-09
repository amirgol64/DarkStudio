// DarkStudio server - settings (paths to tools, models, datasets, and UI preferences).
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json/json.h>

#include <filesystem>
#include <mutex>
#include <string>

namespace darkstudio
{
	namespace fs = std::filesystem;

	/// Everything the server needs to know about the local installation.  Stored as JSON in
	/// <work_dir>/settings.json; missing values fall back to defaults derived from the DarkStudio root directory.
	struct Settings
	{
		fs::path root;					///< DarkStudio repository root
		fs::path work_dir;				///< server output: settings, job folders, results (default build/darkstudio)
		fs::path web_dir;				///< built web UI (default web/dist)

		fs::path darknet_cpu_bin;		///< Darknet CPU install (build/install/bin)
		fs::path darknet_sycl_bin;		///< Darknet SYCL install (build/install-sycl/bin)
		fs::path oneapi_bin;			///< oneAPI runtime DLLs needed by the SYCL build
		fs::path openvino_bin;			///< OpenVINO runtime DLLs
		fs::path openvino_tbb_bin;		///< TBB DLLs used by OpenVINO
		fs::path ov_bench;				///< tools/ov-bench executable
		fs::path compare_script;		///< tools/sycl-migrate/compare-cpu-sycl.ps1

		fs::path models_dir;			///< downloaded/converted models (models/pretrained)
		fs::path datasets_dir;			///< datasets root used by the annotation view (datasets)

		std::string default_device		= "GPU";	///< OpenVINO device for benchmarks
		std::string default_precision	= "f16";	///< OpenVINO precision hint
		bool openvino_cache				= true;		///< use OpenVINO's model cache (12.6 s -> 0.12 s GPU compile)
		std::string language			= "en";		///< UI language (en, he)
		std::string theme				= "system";	///< UI theme (system, light, dark)

		/// Defaults for a DarkStudio checkout at @p root (see docs/build-windows.md and docs/intel-gpu.md).
		static Settings defaults(const fs::path & root);

		Json::Value to_json() const;

		/// Apply the values present in @p json; unknown keys are ignored.
		void update_from_json(const Json::Value & json);
	};

	/// Thread-safe holder that loads and saves the settings file.
	class SettingsStore
	{
		public:

			explicit SettingsStore(const fs::path & root);

			Settings get() const;
			Settings update(const Json::Value & json);	///< apply, save, and return the new settings
			fs::path file() const;

		private:

			void save_locked() const;

			mutable std::mutex mutex;
			Settings settings;
	};

	/// Find the DarkStudio root by walking up from @p start until plan.md and the darknet directory are found.
	fs::path find_root(fs::path start);
}
