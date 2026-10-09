// DarkStudio server - jobs.
// SPDX-License-Identifier: Apache-2.0

#include "jobs.hpp"

#include "events.hpp"
#include "parsers.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>


namespace darkstudio
{
	namespace
	{
		constexpr size_t kMaxLogLinesInMemory = 5000;

		std::string utf8(const fs::path & p)
		{
			const auto u8 = p.u8string();
			return std::string(u8.begin(), u8.end());
		}

		fs::path path_from(const std::string & s)
		{
			return fs::path(std::u8string(s.begin(), s.end()));
		}

		std::string now_iso()
		{
			const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm{};
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			localtime_r(&t, &tm);
#endif
			std::ostringstream ss;
			ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
			return ss.str();
		}

		std::string new_job_id()
		{
			const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm{};
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			localtime_r(&t, &tm);
#endif
			static std::mt19937 rng(std::random_device{}());
			std::ostringstream ss;
			ss << std::put_time(&tm, "%Y%m%d-%H%M%S") << "-" << std::hex << std::setw(4) << std::setfill('0') << (rng() & 0xffff);
			return ss.str();
		}

		std::string string_param(const Json::Value & p, const char * key, const std::string & fallback)
		{
			return (p.isMember(key) and p[key].isString() and not p[key].asString().empty()) ? p[key].asString() : fallback;
		}

		int int_param(const Json::Value & p, const char * key, int fallback, int min, int max)
		{
			const int v = (p.isMember(key) and p[key].isNumeric()) ? p[key].asInt() : fallback;
			if (v < min or v > max)
			{
				throw std::invalid_argument(std::string(key) + " must be between " + std::to_string(min) + " and " + std::to_string(max));
			}
			return v;
		}

		bool bool_param(const Json::Value & p, const char * key, bool fallback)
		{
			return (p.isMember(key) and p[key].isBool()) ? p[key].asBool() : fallback;
		}

		fs::path require_file(const std::string & text, const char * what)
		{
			const fs::path p = path_from(text);
			if (not fs::is_regular_file(p))
			{
				throw std::invalid_argument(std::string(what) + " not found: " + text);
			}
			return p;
		}

		fs::path executable(const fs::path & bin, const char * name)
		{
#ifdef _WIN32
			return bin / (std::string(name) + ".exe");
#else
			return bin / name;
#endif
		}

		/// "max_batches=1000" from a Darknet .cfg file, or 0.
		int read_max_batches(const fs::path & cfg)
		{
			std::ifstream ifs(cfg);
			std::string line;
			while (std::getline(ifs, line))
			{
				if (line.rfind("max_batches", 0) == 0)
				{
					const auto eq = line.find('=');
					if (eq != std::string::npos)
					{
						try { return std::stoi(line.substr(eq + 1)); } catch (...) {}
					}
				}
			}
			return 0;
		}

		/// Copy a Darknet .data file, pointing "backup" at @p backup so each job keeps its own weights.
		fs::path copy_data_file(const fs::path & data, const fs::path & dir)
		{
			std::ifstream ifs(data);
			const fs::path out = dir / data.filename();
			std::ofstream ofs(out);
			std::string line;
			bool has_backup = false;
			while (std::getline(ifs, line))
			{
				std::string key = line.substr(0, line.find('='));
				key.erase(std::remove_if(key.begin(), key.end(), [](unsigned char c) { return std::isspace(c); }), key.end());
				if (key == "backup")
				{
					ofs << "backup = " << utf8(dir) << "\n";
					has_backup = true;
				}
				else
				{
					ofs << line << "\n";
				}
			}
			if (not has_backup)
			{
				ofs << "backup = " << utf8(dir) << "\n";
			}
			return out;
		}

		std::string powershell()
		{
#ifdef _WIN32
			return "pwsh";
#else
			return "/usr/bin/pwsh";
#endif
		}
	}


	Json::Value Job::summary_locked() const
	{
		Json::Value j;
		j["id"]			= id;
		j["kind"]		= kind;
		j["title"]		= title;
		j["params"]		= params;
		j["status"]		= status;
		j["created"]	= created;
		j["started"]	= started;
		j["finished"]	= finished;
		j["exit_code"]	= exit_code;
		j["error"]		= error;
		j["dir"]		= utf8(dir);
		j["lines"]		= Json::UInt64(line_count);
		j["errors"]		= Json::UInt64(errors);
		j["warnings"]	= Json::UInt64(warnings);
		j["result"]		= result;
		j["progress"]["current"]	= progress_current;
		j["progress"]["total"]		= progress_total;
		return j;
	}


	JobManager::JobManager(SettingsStore & s) :
		settings(s)
	{
		load_history();
	}


	JobManager::~JobManager()
	{
		{
			std::scoped_lock lock(mutex);
			if (active and active->process)
			{
				active->process->stop();
			}
		}
		for (auto & t : threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
	}


	Process::Options JobManager::prepare(Job & job, const Settings & s)
	{
		Process::Options o;
		o.working_directory = job.dir;
		const auto & p = job.params;

		if (job.kind == "ov-bench")
		{
			const std::string device	= string_param(p, "device", s.default_device);
			const std::string precision	= string_param(p, "precision", s.default_precision);
			const int iterations		= int_param(p, "iterations", 50, 1, 1000);
			if (device != "GPU" and device != "CPU" and device != "NPU" and device != "AUTO")
			{
				throw std::invalid_argument("device must be GPU, CPU, NPU or AUTO");
			}
			if (precision != "f16" and precision != "f32")
			{
				throw std::invalid_argument("precision must be f16 or f32");
			}

			const fs::path model = require_file(string_param(p, "model", utf8(s.models_dir / "yolov4-tiny.onnx")), "model");
			const fs::path names = require_file(string_param(p, "names", utf8(s.models_dir / "coco.names")), "names file");
			if (not fs::is_regular_file(s.ov_bench))
			{
				throw std::invalid_argument("ov-bench not found: " + utf8(s.ov_bench) + " (build tools/ov-bench, see docs/intel-gpu.md)");
			}

			o.executable = s.ov_bench;
			o.arguments = {utf8(model), utf8(names)};
			std::vector<fs::path> images;
			for (const auto & entry : fs::directory_iterator(s.root / "darknet" / "artwork"))
			{
				if (entry.path().extension() == ".jpg")
				{
					images.push_back(entry.path());
				}
			}
			std::sort(images.begin(), images.end());
			for (const auto & img : images)
			{
				o.arguments.push_back(utf8(img));
			}
			o.arguments.insert(o.arguments.end(), {"--device", device, "--precision", precision, "--iters", std::to_string(iterations), "--save", utf8(job.dir / "images")});
			if (s.openvino_cache)
			{
				o.arguments.insert(o.arguments.end(), {"--cache", utf8(s.work_dir / "ov-cache")});
			}
			o.extra_path = {s.openvino_bin, s.openvino_tbb_bin};
			job.title = "OpenVINO " + device + " " + precision;
			job.progress_total = static_cast<int>(images.size());
		}
		else if (job.kind == "compare-cpu-sycl")
		{
			const int repeat = int_param(p, "repeat", 3, 2, 20);
			require_file(utf8(s.compare_script), "compare script");
			o.executable = powershell();
			o.arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", utf8(s.compare_script), "-Repeat", std::to_string(repeat), "-OneApiBin", utf8(s.oneapi_bin)};
			job.title = "Darknet CPU vs SYCL (Iris Xe)";
		}
		else if (job.kind == "prepare-legogears")
		{
			const int max_batches = int_param(p, "max_batches", 1000, 1, 1000000);
			const fs::path script = s.root / "tools" / "train-test" / "prepare-legogears.ps1";
			require_file(utf8(script), "prepare script");
			o.executable = powershell();
			o.arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", utf8(script), "-MaxBatches", std::to_string(max_batches)};
			job.title = "Prepare LEGO Gears (" + std::to_string(max_batches) + " iterations)";
		}
		else if (job.kind == "train" or job.kind == "map")
		{
			const std::string backend = string_param(p, "backend", "sycl");
			if (backend != "cpu" and backend != "sycl")
			{
				throw std::invalid_argument("backend must be cpu or sycl");
			}
			const fs::path default_dir = s.root / "build" / "train-test" / "legogears";
			const fs::path data = require_file(string_param(p, "data", utf8(default_dir / "LegoGears.data")), "data file");
			const fs::path cfg_src = require_file(string_param(p, "cfg", utf8(default_dir / "LegoGears.cfg")), "cfg file");

			const fs::path bin = backend == "cpu" ? s.darknet_cpu_bin : s.darknet_sycl_bin;
			o.executable = executable(bin, "darknet");
			if (not fs::is_regular_file(o.executable))
			{
				throw std::invalid_argument("Darknet not found: " + utf8(o.executable));
			}
			if (backend == "sycl")
			{
				o.extra_path = {s.darknet_sycl_bin, s.oneapi_bin};
			}
			else
			{
				o.extra_path = {s.darknet_cpu_bin};
				o.stdin_text = "yes\n";	// Darknet asks for confirmation before training on a CPU
			}

			// copy the cfg so later changes (e.g. a new "prepare" job) do not affect this run
			const fs::path cfg = job.dir / cfg_src.filename();
			fs::copy_file(cfg_src, cfg, fs::copy_options::overwrite_existing);
			const fs::path data_copy = copy_data_file(data, job.dir);

			if (job.kind == "train")
			{
				o.arguments = {"detector", "train", utf8(data_copy), utf8(cfg)};
				const std::string weights = string_param(p, "weights", "");
				if (not weights.empty())
				{
					o.arguments.push_back(utf8(require_file(weights, "weights")));
				}
				o.arguments.push_back("-dont_show");
				if (bool_param(p, "map", true))
				{
					o.arguments.push_back("-map");
				}
				job.progress_total = read_max_batches(cfg);
				job.title = std::string("Train ") + cfg.stem().string() + " on " + (backend == "cpu" ? "CPU" : "Iris Xe (SYCL)");
			}
			else
			{
				const fs::path weights = require_file(string_param(p, "weights", ""), "weights");
				const std::string iou = string_param(p, "iou", "0.50");
				o.arguments = {"detector", "map", utf8(data_copy), utf8(cfg), utf8(weights), "-iou_thresh", iou};
				job.title = "mAP@" + iou + " " + weights.filename().string() + " (" + backend + ")";
			}
		}
		else
		{
			throw std::invalid_argument("unknown job kind \"" + job.kind + "\"");
		}
		return o;
	}


	Json::Value JobManager::start(const std::string & kind, const Json::Value & params)
	{
		const Settings s = settings.get();

		auto job = std::make_shared<Job>();
		job->id			= new_job_id();
		job->kind		= kind;
		job->params		= params.isObject() ? params : Json::Value(Json::objectValue);
		job->created	= now_iso();
		job->dir		= s.work_dir / "jobs" / job->id;
		fs::create_directories(job->dir);

		Process::Options options;
		try
		{
			options = prepare(*job, s);
		}
		catch (...)
		{
			std::error_code ec;
			fs::remove_all(job->dir, ec);
			throw;
		}

		Json::Value summary;
		{
			std::scoped_lock lock(mutex);
			if (active)
			{
				throw std::runtime_error("another job is running (" + active->title + "); stop it first or wait until it finishes");
			}
			active = job;
			jobs[job->id] = job;
			threads.emplace_back(&JobManager::run, this, job, options);
		}
		{
			std::scoped_lock lock(job->mutex);
			summary = job->summary_locked();
		}
		return summary;
	}


	void JobManager::run(std::shared_ptr<Job> job, Process::Options options)
	{
		parsers::OvBench ov;
		parsers::CompareCpuSycl compare;
		Json::Value last_map;

		{
			std::scoped_lock lock(job->mutex);
			job->status		= "running";
			job->started	= now_iso();
			job->process	= std::make_shared<Process>();
			save(*job);
		}
		announce(*job);

		int exit_code = -1;
		std::string error;
		try
		{
			exit_code = job->process->run(options, [&](const std::string & raw)
			{
				// progress output uses '\r' to redraw a line; keep only the final text
				std::string line = raw;
				const auto cr = line.find_last_of('\r');
				if (cr != std::string::npos)
				{
					line = line.substr(cr + 1);
				}
				line = parsers::strip_ansi(line);

				if (job->kind == "ov-bench")
				{
					ov.add(line);
				}
				else if (job->kind == "compare-cpu-sycl")
				{
					compare.add(line);
				}
				on_line(*job, line);
			});
		}
		catch (const std::exception & e)
		{
			error = e.what();
		}

		{
			std::scoped_lock lock(job->mutex);
			job->exit_code	= exit_code;
			job->finished	= now_iso();
			if (job->kind == "ov-bench")
			{
				job->result = ov.result();
			}
			else if (job->kind == "compare-cpu-sycl")
			{
				job->result = compare.result();
			}

			if (job->process->was_stopped())
			{
				job->status = "stopped";
			}
			else if (not error.empty())
			{
				job->status = "failed";
				job->error	= error;
			}
			else if (exit_code != 0)
			{
				job->status = "failed";
				job->error	= "exit code " + std::to_string(exit_code);
			}
			else
			{
				job->status = "succeeded";
			}
			save(*job);
		}
		announce(*job);

		std::scoped_lock lock(mutex);
		if (active == job)
		{
			active.reset();
		}
	}


	void JobManager::on_line(Job & job, const std::string & line)
	{
		const auto level = parsers::classify(line);
		Json::Value metric;
		bool has_metric = false;
		bool progress_changed = false;
		size_t n = 0;

		{
			std::scoped_lock lock(job.mutex);
			n = ++ job.line_count;
			job.log.push_back({n, line, parsers::to_string(level)});
			if (job.log.size() > kMaxLogLinesInMemory)
			{
				job.log.pop_front();
			}
			if (level == parsers::LogLevel::error)
			{
				job.errors ++;
			}
			else if (level == parsers::LogLevel::warning)
			{
				job.warnings ++;
			}

			std::ofstream(job.dir / "log.txt", std::ios::app) << line << "\n";

			if (job.kind == "train")
			{
				if (auto it = parsers::training_iteration(line))
				{
					metric = *it;
					metric["type"] = "iteration";
					job.progress_current = metric["iteration"].asInt();
					progress_changed = true;
					has_metric = true;
				}
			}
			if (job.kind == "train" or job.kind == "map")
			{
				if (auto m = parsers::map_result(line))
				{
					metric = *m;
					metric["type"] = "map";
					metric["iteration"] = job.progress_current;
					job.result["last_map"] = (*m)["map"];
					job.result["iou"] = (*m)["iou"];
					has_metric = true;
				}
			}
			if (job.kind == "ov-bench" and line.find(".jpg ") != std::string::npos)
			{
				job.progress_current ++;
				progress_changed = true;
			}
			if (has_metric)
			{
				job.metrics.append(metric);
			}
		}

		Json::Value event;
		event["type"]	= "log";
		event["job_id"]	= job.id;
		event["n"]		= Json::UInt64(n);
		event["line"]	= line;
		event["level"]	= parsers::to_string(level);
		EventsSocket::broadcast(event);

		if (has_metric)
		{
			Json::Value m;
			m["type"]	= "metric";
			m["job_id"]	= job.id;
			m["metric"]	= metric;
			EventsSocket::broadcast(m);
		}
		if (progress_changed)
		{
			announce(job);
		}
	}


	void JobManager::announce(Job & job)
	{
		Json::Value event;
		event["type"] = "job";
		{
			std::scoped_lock lock(job.mutex);
			event["job"] = job.summary_locked();
		}
		EventsSocket::broadcast(event);
	}


	void JobManager::save(Job & job)
	{
		Json::Value j = job.summary_locked();
		j["metrics"] = job.metrics;
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "\t";
		std::ofstream(job.dir / "job.json") << Json::writeString(builder, j) << std::endl;
	}


	void JobManager::load_history()
	{
		const fs::path dir = settings.get().work_dir / "jobs";
		if (not fs::is_directory(dir))
		{
			return;
		}
		for (const auto & entry : fs::directory_iterator(dir))
		{
			const fs::path file = entry.path() / "job.json";
			if (not fs::is_regular_file(file))
			{
				continue;
			}
			std::ifstream ifs(file);
			Json::Value j;
			Json::CharReaderBuilder builder;
			std::string errors;
			if (not Json::parseFromStream(builder, ifs, &j, &errors))
			{
				continue;
			}
			auto job = std::make_shared<Job>();
			job->id					= j["id"].asString();
			job->kind				= j["kind"].asString();
			job->title				= j["title"].asString();
			job->params				= j["params"];
			job->status				= j["status"].asString();
			job->created			= j["created"].asString();
			job->started			= j["started"].asString();
			job->finished			= j["finished"].asString();
			job->exit_code			= j["exit_code"].asInt();
			job->error				= j["error"].asString();
			job->dir				= entry.path();
			job->line_count			= j["lines"].asUInt64();
			job->errors				= j["errors"].asUInt64();
			job->warnings			= j["warnings"].asUInt64();
			job->result				= j["result"];
			job->metrics			= j.isMember("metrics") ? j["metrics"] : Json::Value(Json::arrayValue);
			job->progress_current	= j["progress"]["current"].asInt();
			job->progress_total		= j["progress"]["total"].asInt();
			if (job->status == "running" or job->status == "queued")
			{
				job->status	= "failed";
				job->error	= "the server stopped while this job was running";
			}
			jobs[job->id] = job;
		}
	}


	std::shared_ptr<Job> JobManager::find(const std::string & id) const
	{
		std::scoped_lock lock(mutex);
		const auto it = jobs.find(id);
		return it == jobs.end() ? nullptr : it->second;
	}


	bool JobManager::stop(const std::string & id)
	{
		auto job = find(id);
		if (not job)
		{
			return false;
		}
		std::scoped_lock lock(job->mutex);
		if (job->process and job->status == "running")
		{
			job->process->stop();
		}
		return true;
	}


	Json::Value JobManager::list() const
	{
		std::vector<std::shared_ptr<Job>> copy;
		{
			std::scoped_lock lock(mutex);
			for (const auto & [id, job] : jobs)
			{
				copy.push_back(job);
			}
		}
		Json::Value arr = Json::arrayValue;
		for (auto it = copy.rbegin(); it != copy.rend(); ++ it)	// newest first
		{
			std::scoped_lock lock((*it)->mutex);
			arr.append((*it)->summary_locked());
		}
		return arr;
	}


	Json::Value JobManager::get(const std::string & id) const
	{
		auto job = find(id);
		if (not job)
		{
			return Json::nullValue;
		}
		std::scoped_lock lock(job->mutex);
		return job->summary_locked();
	}


	Json::Value JobManager::log(const std::string & id, size_t since) const
	{
		auto job = find(id);
		if (not job)
		{
			return Json::nullValue;
		}

		std::scoped_lock lock(job->mutex);
		if (job->log.empty() and job->line_count > 0)
		{
			// a job from a previous server run: read the most recent lines back from log.txt
			std::ifstream ifs(job->dir / "log.txt");
			std::string text;
			size_t n = 0;
			while (std::getline(ifs, text))
			{
				n ++;
				job->log.push_back({n, text, parsers::to_string(parsers::classify(text))});
				if (job->log.size() > kMaxLogLinesInMemory)
				{
					job->log.pop_front();
				}
			}
		}

		Json::Value lines = Json::arrayValue;
		for (const auto & l : job->log)
		{
			if (l.n > since)
			{
				Json::Value j;
				j["n"]		= Json::UInt64(l.n);
				j["text"]	= l.text;
				j["level"]	= l.level;
				lines.append(j);
			}
		}
		Json::Value r;
		r["job_id"]	= id;
		r["total"]	= Json::UInt64(job->line_count);
		r["lines"]	= lines;
		return r;
	}


	Json::Value JobManager::metrics(const std::string & id) const
	{
		auto job = find(id);
		if (not job)
		{
			return Json::nullValue;
		}
		std::scoped_lock lock(job->mutex);
		return job->metrics;
	}
}
