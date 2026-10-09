// DarkStudio server - jobs: run tools (benchmarks, comparisons, training) and keep their logs, metrics and results.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "process.hpp"
#include "settings.hpp"

#include <json/json.h>

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace darkstudio
{
	struct LogLine
	{
		size_t n;			///< 1-based line number within the job
		std::string text;
		std::string level;	///< info, warning, error
	};

	/// One run of a tool.  All members are protected by @p mutex.
	struct Job
	{
		std::mutex mutex;
		std::string id;
		std::string kind;			///< ov-bench, compare-cpu-sycl, train, prepare-legogears, map
		std::string title;
		Json::Value params;
		std::string status = "queued";	///< queued, running, succeeded, failed, stopped
		std::string created;
		std::string started;
		std::string finished;
		int exit_code = 0;
		std::string error;			///< why the job failed to start or finished unsuccessfully
		fs::path dir;				///< <work_dir>/jobs/<id>
		size_t line_count = 0;
		size_t errors = 0;
		size_t warnings = 0;
		std::deque<LogLine> log;	///< the most recent lines (the full log is in <dir>/log.txt)
		Json::Value metrics = Json::arrayValue;	///< training iterations / mAP results
		Json::Value result = Json::objectValue;	///< parsed summary (benchmark numbers, mAP, ...)
		int progress_current = 0;
		int progress_total = 0;
		std::shared_ptr<Process> process;

		/// Summary without the log and metrics (sent with every "job" event).  Caller must hold @p mutex.
		Json::Value summary_locked() const;
	};

	class JobManager
	{
		public:

			explicit JobManager(SettingsStore & settings);
			~JobManager();

			/// Validate @p params, then start the job in a background thread.  Throws std::invalid_argument for bad
			/// parameters and std::runtime_error if another job is still running.
			Json::Value start(const std::string & kind, const Json::Value & params);

			bool stop(const std::string & id);
			Json::Value list() const;
			Json::Value get(const std::string & id) const;
			Json::Value log(const std::string & id, size_t since) const;
			Json::Value metrics(const std::string & id) const;

		private:

			std::shared_ptr<Job> find(const std::string & id) const;
			void run(std::shared_ptr<Job> job, Process::Options options);
			void on_line(Job & job, const std::string & raw);
			void load_history();
			void save(Job & job);	///< write <dir>/job.json; caller must hold job.mutex
			void announce(Job & job);

			Process::Options prepare(Job & job, const Settings & s);

			SettingsStore & settings;
			mutable std::mutex mutex;
			std::map<std::string, std::shared_ptr<Job>> jobs;	///< ordered by id, which starts with the date and time
			std::shared_ptr<Job> active;
			std::vector<std::thread> threads;
	};
}
