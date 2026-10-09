// DarkStudio server - run a child process and stream its output line by line.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace darkstudio
{
	namespace fs = std::filesystem;

	/// Runs one child process.  stdout and stderr are merged and delivered one line at a time.  On Windows the child
	/// runs inside a Job Object so stop() also ends any processes it started (e.g. a PowerShell script that runs
	/// Darknet); on POSIX it runs in its own process group.
	class Process
	{
		public:

			struct Options
			{
				fs::path executable;
				std::vector<std::string> arguments;		///< UTF-8
				fs::path working_directory;
				std::vector<fs::path> extra_path;		///< prepended to PATH (e.g. oneAPI or OpenVINO DLL folders)
				std::string stdin_text;					///< written to stdin, which is then closed (e.g. "yes\n")
			};

			using LineCallback = std::function<void(const std::string & line)>;

			Process() = default;
			Process(const Process &) = delete;
			Process & operator=(const Process &) = delete;
			~Process();

			/// Start the process and block until it exits.  Returns the exit code.  Throws if it cannot be started.
			int run(const Options & options, const LineCallback & on_line);

			/// Ask a running process (and its children) to stop.  Safe to call from another thread.
			void stop();

			bool was_stopped() const { return stopped; }

		private:

			std::mutex mutex;
			std::atomic<bool> stopped = false;
#ifdef _WIN32
			void * job = nullptr;		///< HANDLE of the Job Object
			void * process = nullptr;	///< HANDLE of the child process
#else
			int pid = -1;
#endif
	};
}
