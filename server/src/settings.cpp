// DarkStudio server - settings.
// SPDX-License-Identifier: Apache-2.0

#include "settings.hpp"

#include <fstream>
#include <stdexcept>


namespace darkstudio
{
	namespace
	{
		std::string to_utf8(const fs::path & p)
		{
			const auto u8 = p.u8string();
			return std::string(u8.begin(), u8.end());
		}

		fs::path from_utf8(const std::string & s)
		{
			return fs::path(std::u8string(s.begin(), s.end()));
		}

		void read_path(const Json::Value & json, const char * key, fs::path & value)
		{
			if (json.isMember(key) and json[key].isString())
			{
				value = from_utf8(json[key].asString());
			}
		}

		void read_string(const Json::Value & json, const char * key, std::string & value)
		{
			if (json.isMember(key) and json[key].isString())
			{
				value = json[key].asString();
			}
		}
	}


	Settings Settings::defaults(const fs::path & root)
	{
		Settings s;
		s.root				= root;
		s.work_dir			= root / "build" / "darkstudio";
		s.web_dir			= root / "web" / "dist";
		s.darknet_cpu_bin	= root / "build" / "install" / "bin";
		s.darknet_sycl_bin	= root / "build" / "install-sycl" / "bin";
		s.models_dir		= root / "models" / "pretrained";
		s.datasets_dir		= root / "datasets";
		s.compare_script	= root / "tools" / "sycl-migrate" / "compare-cpu-sycl.ps1";
#ifdef _WIN32
		s.oneapi_bin		= "C:/Program Files (x86)/Intel/oneAPI/2026.1/bin";
		s.openvino_bin		= "C:/src/openvino/runtime/bin/intel64/Release";
		s.openvino_tbb_bin	= "C:/src/openvino/runtime/3rdparty/tbb/bin";
		s.ov_bench			= root / "tools" / "ov-bench" / "build" / "Release" / "ov-bench.exe";
#else
		s.oneapi_bin		= "/opt/intel/oneapi/2026.1/lib";
		s.openvino_bin		= "/opt/intel/openvino/runtime/lib/intel64";
		s.openvino_tbb_bin	= "/opt/intel/openvino/runtime/3rdparty/tbb/lib";
		s.ov_bench			= root / "tools" / "ov-bench" / "build" / "ov-bench";
#endif
		return s;
	}


	Json::Value Settings::to_json() const
	{
		Json::Value j;
		j["root"]				= to_utf8(root);
		j["work_dir"]			= to_utf8(work_dir);
		j["web_dir"]			= to_utf8(web_dir);
		j["darknet_cpu_bin"]	= to_utf8(darknet_cpu_bin);
		j["darknet_sycl_bin"]	= to_utf8(darknet_sycl_bin);
		j["oneapi_bin"]			= to_utf8(oneapi_bin);
		j["openvino_bin"]		= to_utf8(openvino_bin);
		j["openvino_tbb_bin"]	= to_utf8(openvino_tbb_bin);
		j["ov_bench"]			= to_utf8(ov_bench);
		j["compare_script"]		= to_utf8(compare_script);
		j["models_dir"]			= to_utf8(models_dir);
		j["datasets_dir"]		= to_utf8(datasets_dir);
		j["default_device"]		= default_device;
		j["default_precision"]	= default_precision;
		j["openvino_cache"]		= openvino_cache;
		j["language"]			= language;
		j["theme"]				= theme;
		return j;
	}


	void Settings::update_from_json(const Json::Value & json)
	{
		// root and work_dir are fixed for the lifetime of the server
		read_path(json, "web_dir",			web_dir);
		read_path(json, "darknet_cpu_bin",	darknet_cpu_bin);
		read_path(json, "darknet_sycl_bin",	darknet_sycl_bin);
		read_path(json, "oneapi_bin",		oneapi_bin);
		read_path(json, "openvino_bin",		openvino_bin);
		read_path(json, "openvino_tbb_bin",	openvino_tbb_bin);
		read_path(json, "ov_bench",			ov_bench);
		read_path(json, "compare_script",	compare_script);
		read_path(json, "models_dir",		models_dir);
		read_path(json, "datasets_dir",		datasets_dir);
		read_string(json, "default_device",		default_device);
		read_string(json, "default_precision",	default_precision);
		read_string(json, "language",			language);
		read_string(json, "theme",				theme);
		if (json.isMember("openvino_cache") and json["openvino_cache"].isBool())
		{
			openvino_cache = json["openvino_cache"].asBool();
		}

		if (default_precision != "f16" and default_precision != "f32")
		{
			throw std::invalid_argument("default_precision must be \"f16\" or \"f32\"");
		}
	}


	SettingsStore::SettingsStore(const fs::path & root) :
		settings(Settings::defaults(root))
	{
		fs::create_directories(settings.work_dir);
		if (fs::exists(file()))
		{
			std::ifstream ifs(file());
			Json::Value json;
			Json::CharReaderBuilder builder;
			std::string errors;
			if (Json::parseFromStream(builder, ifs, &json, &errors))
			{
				settings.update_from_json(json);
			}
		}
		else
		{
			save_locked();
		}
	}


	Settings SettingsStore::get() const
	{
		std::scoped_lock lock(mutex);
		return settings;
	}


	Settings SettingsStore::update(const Json::Value & json)
	{
		std::scoped_lock lock(mutex);
		Settings copy = settings;
		copy.update_from_json(json);	// throws on invalid values, leaving the current settings untouched
		settings = copy;
		save_locked();
		return settings;
	}


	fs::path SettingsStore::file() const
	{
		return settings.work_dir / "settings.json";
	}


	void SettingsStore::save_locked() const
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "\t";
		std::ofstream ofs(file());
		ofs << Json::writeString(builder, settings.to_json()) << std::endl;
	}


	fs::path find_root(fs::path start)
	{
		start = fs::absolute(start);
		for (auto dir = start; not dir.empty(); dir = dir.parent_path())
		{
			if (fs::exists(dir / "plan.md") and fs::exists(dir / "darknet"))
			{
				return dir;
			}
			if (dir == dir.parent_path())
			{
				break;
			}
		}
		throw std::runtime_error("cannot find the DarkStudio root (plan.md + darknet/) above " + start.string() + "; use --root");
	}
}
