// DarkStudio server - detect the CPU, the Darknet builds and OpenVINO devices.
// SPDX-License-Identifier: Apache-2.0

#include "system_info.hpp"

#include "events.hpp"
#include "parsers.hpp"
#include "process.hpp"

#include <chrono>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fstream>
#include <unistd.h>
#endif


namespace darkstudio
{
	namespace
	{
		std::string utf8(const fs::path & p)
		{
			const auto u8 = p.u8string();
			return std::string(u8.begin(), u8.end());
		}

		Json::Value cpu_info()
		{
			Json::Value j;
			j["threads"] = std::thread::hardware_concurrency();
#ifdef _WIN32
			wchar_t name[256] = {};
			DWORD size = sizeof(name);
			if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString",
				RRF_RT_REG_SZ, nullptr, name, &size) == ERROR_SUCCESS)
			{
				const int len = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
				std::string s(len > 0 ? len - 1 : 0, '\0');
				WideCharToMultiByte(CP_UTF8, 0, name, -1, s.data(), len, nullptr, nullptr);
				j["name"] = s;
			}
			MEMORYSTATUSEX mem{sizeof(mem)};
			if (GlobalMemoryStatusEx(&mem))
			{
				j["ram_bytes"]		= Json::UInt64(mem.ullTotalPhys);
				j["ram_free_bytes"]	= Json::UInt64(mem.ullAvailPhys);
			}
			j["os"] = "Windows";
#else
			std::ifstream cpuinfo("/proc/cpuinfo");
			std::string line;
			while (std::getline(cpuinfo, line))
			{
				if (line.rfind("model name", 0) == 0)
				{
					j["name"] = line.substr(line.find(':') + 2);
					break;
				}
			}
			const long pages = sysconf(_SC_PHYS_PAGES);
			const long page_size = sysconf(_SC_PAGE_SIZE);
			j["ram_bytes"] = Json::UInt64(static_cast<unsigned long long>(pages) * page_size);
			j["os"] = "Linux";
#endif
			return j;
		}

		struct ProbeResult
		{
			int exit_code = -1;
			std::vector<std::string> lines;
			std::string error;
			double seconds = 0.0;
		};

		ProbeResult run_probe(const fs::path & exe, const std::vector<std::string> & args, const std::vector<fs::path> & extra_path)
		{
			ProbeResult r;
			if (not fs::is_regular_file(exe))
			{
				r.error = "not found: " + utf8(exe);
				return r;
			}
			const auto start = std::chrono::steady_clock::now();
			try
			{
				Process p;
				Process::Options o;
				o.executable		= exe;
				o.arguments			= args;
				o.extra_path		= extra_path;
				o.working_directory	= exe.parent_path();
				r.exit_code = p.run(o, [&](const std::string & line) { r.lines.push_back(parsers::strip_ansi(line)); });
				if (r.exit_code != 0)
				{
					r.error = "exit code " + std::to_string(r.exit_code);
					// a missing DLL makes Windows return 0xC0000135 without any output
					if (static_cast<unsigned int>(r.exit_code) == 0xC0000135u)
					{
						r.error += " (a required DLL was not found; check the oneAPI / OpenVINO paths in Settings)";
					}
				}
			}
			catch (const std::exception & e)
			{
				r.error = e.what();
			}
			r.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
			return r;
		}

		fs::path exe(const fs::path & bin, const char * name)
		{
#ifdef _WIN32
			return bin / (std::string(name) + ".exe");
#else
			return bin / name;
#endif
		}

		Json::Value backend(const char * id, const char * name, const ProbeResult & r)
		{
			Json::Value j;
			j["id"]			= id;
			j["name"]		= name;
			j["available"]	= r.error.empty();
			j["error"]		= r.error;
			j["probe_seconds"] = r.seconds;
			Json::Value output = Json::arrayValue;
			for (const auto & line : r.lines)
			{
				output.append(line);
			}
			j["output"] = output;
			return j;
		}
	}


	SystemInfo::SystemInfo(SettingsStore & s) :
		settings(s)
	{
		cached["probing"] = true;
	}


	Json::Value SystemInfo::get() const
	{
		std::scoped_lock lock(mutex);
		Json::Value j = cached;
		j["probing"] = probing or cached.isMember("probing");
		return j;
	}


	void SystemInfo::refresh()
	{
		{
			std::scoped_lock lock(mutex);
			if (probing)
			{
				return;
			}
			probing = true;
		}
		std::thread([this]
		{
			Json::Value result = probe();
			{
				std::scoped_lock lock(mutex);
				cached = result;
				probing = false;
			}
			Json::Value event;
			event["type"] = "system";
			event["system"] = result;
			EventsSocket::broadcast(event);
		}).detach();
	}


	Json::Value SystemInfo::probe() const
	{
		const Settings s = settings.get();
		Json::Value j;
		j["cpu"] = cpu_info();
		j["backends"] = Json::arrayValue;

		{
			const auto r = run_probe(exe(s.darknet_cpu_bin, "darknet"), {"--version"}, {s.darknet_cpu_bin});
			auto b = backend("darknet-cpu", "Darknet (CPU)", r);
			b["info"] = parsers::darknet_version(r.lines);
			j["backends"].append(b);
		}
		{
			const auto r = run_probe(exe(s.darknet_sycl_bin, "darknet"), {"--version"}, {s.darknet_sycl_bin, s.oneapi_bin});
			auto b = backend("darknet-sycl", "Darknet (SYCL, Intel GPU)", r);
			b["info"] = parsers::darknet_version(r.lines);
			if (b["available"].asBool() and b["info"]["gpus"].empty())
			{
				b["available"] = false;
				b["error"] = "no SYCL GPU detected (check the Intel GPU driver)";
			}
			j["backends"].append(b);
		}
		{
			const auto r = run_probe(s.ov_bench, {"--list-devices"}, {s.openvino_bin, s.openvino_tbb_bin});
			auto b = backend("openvino", "OpenVINO", r);
			b["info"] = parsers::openvino_devices(r.lines);
			j["backends"].append(b);
		}

		// the files the benchmark and training jobs need
		Json::Value files = Json::arrayValue;
		const auto check = [&](const char * what, const fs::path & p)
		{
			Json::Value f;
			f["what"]	= what;
			f["path"]	= utf8(p);
			f["exists"]	= fs::exists(p);
			files.append(f);
		};
		check("ONNX model (OpenVINO benchmark)",	s.models_dir / "yolov4-tiny.onnx");
		check("Darknet weights",					s.models_dir / "yolov4-tiny.weights");
		check("Training data (LEGO Gears)",			s.root / "build" / "train-test" / "legogears" / "LegoGears.data");
		check("oneAPI runtime",						s.oneapi_bin);
		check("OpenVINO runtime",					s.openvino_bin);
		j["files"] = files;
		return j;
	}
}
