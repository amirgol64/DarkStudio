// DarkStudio server - run a child process and stream its output line by line.
// SPDX-License-Identifier: Apache-2.0

#include "process.hpp"

#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#endif


namespace darkstudio
{
	namespace
	{
		/// Split @p buffer into complete lines, keep the incomplete tail, and strip CR.
		void deliver_lines(std::string & buffer, const Process::LineCallback & on_line)
		{
			size_t start = 0;
			for (size_t pos = buffer.find('\n'); pos != std::string::npos; pos = buffer.find('\n', start))
			{
				std::string line = buffer.substr(start, pos - start);
				if (not line.empty() and line.back() == '\r')
				{
					line.pop_back();
				}
				on_line(line);
				start = pos + 1;
			}
			buffer.erase(0, start);
		}

#ifdef _WIN32
		std::wstring widen(const std::string & s)
		{
			if (s.empty())
			{
				return {};
			}
			const int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
			std::wstring w(len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), len);
			return w;
		}

		/// Quote one argument following the rules of CommandLineToArgvW.
		std::wstring quote(const std::wstring & arg)
		{
			if (not arg.empty() and arg.find_first_of(L" \t\"") == std::wstring::npos)
			{
				return arg;
			}
			std::wstring out = L"\"";
			size_t backslashes = 0;
			for (const wchar_t c : arg)
			{
				if (c == L'\\')
				{
					backslashes ++;
					continue;
				}
				if (c == L'"')
				{
					out.append(backslashes * 2 + 1, L'\\');
				}
				else
				{
					out.append(backslashes, L'\\');
				}
				backslashes = 0;
				out += c;
			}
			out.append(backslashes * 2, L'\\');
			out += L'"';
			return out;
		}

		/// Copy of the current environment block with @p extra prepended to PATH.
		std::wstring make_environment(const std::vector<fs::path> & extra)
		{
			std::wstring prefix;
			for (const auto & p : extra)
			{
				prefix += p.wstring() + L";";
			}

			std::wstring block;
			bool found_path = false;
			wchar_t * env = GetEnvironmentStringsW();
			for (const wchar_t * entry = env; *entry; entry += wcslen(entry) + 1)
			{
				std::wstring var(entry);
				if (_wcsnicmp(var.c_str(), L"PATH=", 5) == 0)
				{
					var = L"PATH=" + prefix + var.substr(5);
					found_path = true;
				}
				block += var;
				block += L'\0';
			}
			FreeEnvironmentStringsW(env);
			if (not found_path)
			{
				block += L"PATH=" + prefix;
				block += L'\0';
			}
			block += L'\0';
			return block;
		}
#endif
	}


	Process::~Process()
	{
#ifdef _WIN32
		if (job)
		{
			CloseHandle(job);
		}
#endif
	}


#ifdef _WIN32

	int Process::run(const Options & options, const LineCallback & on_line)
	{
		SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
		HANDLE out_read = nullptr, out_write = nullptr, in_read = nullptr, in_write = nullptr;
		if (not CreatePipe(&out_read, &out_write, &sa, 0) or not CreatePipe(&in_read, &in_write, &sa, 0))
		{
			throw std::runtime_error("CreatePipe() failed");
		}
		SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(in_write, HANDLE_FLAG_INHERIT, 0);

		std::wstring cmd = quote(options.executable.wstring());
		for (const auto & arg : options.arguments)
		{
			cmd += L" " + quote(widen(arg));
		}

		STARTUPINFOW si{};
		si.cb			= sizeof(si);
		si.dwFlags		= STARTF_USESTDHANDLES;
		si.hStdOutput	= out_write;
		si.hStdError	= out_write;
		si.hStdInput	= in_read;

		PROCESS_INFORMATION pi{};
		auto env = make_environment(options.extra_path);
		const std::wstring cwd = options.working_directory.empty() ? std::wstring() : options.working_directory.wstring();

		{
			std::scoped_lock lock(mutex);
			job = CreateJobObjectW(nullptr, nullptr);
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
			limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));

			const BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE,
				CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW | CREATE_SUSPENDED,
				env.data(), cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);
			CloseHandle(out_write);
			CloseHandle(in_read);
			if (not ok)
			{
				const DWORD err = GetLastError();
				CloseHandle(out_read);
				CloseHandle(in_write);
				throw std::runtime_error("cannot start " + options.executable.string() + " (Windows error " + std::to_string(err) + ")");
			}
			AssignProcessToJobObject(job, pi.hProcess);
			process = pi.hProcess;
			ResumeThread(pi.hThread);
			CloseHandle(pi.hThread);
		}

		if (not options.stdin_text.empty())
		{
			DWORD written = 0;
			WriteFile(in_write, options.stdin_text.data(), static_cast<DWORD>(options.stdin_text.size()), &written, nullptr);
		}
		CloseHandle(in_write);

		std::string buffer;
		char chunk[4096];
		DWORD bytes = 0;
		while (ReadFile(out_read, chunk, sizeof(chunk), &bytes, nullptr) and bytes > 0)
		{
			buffer.append(chunk, bytes);
			deliver_lines(buffer, on_line);
		}
		if (not buffer.empty())
		{
			buffer += '\n';
			deliver_lines(buffer, on_line);
		}
		CloseHandle(out_read);

		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exit_code = 0;
		GetExitCodeProcess(pi.hProcess, &exit_code);

		std::scoped_lock lock(mutex);
		CloseHandle(pi.hProcess);
		process = nullptr;
		return static_cast<int>(exit_code);
	}


	void Process::stop()
	{
		std::scoped_lock lock(mutex);
		stopped = true;
		if (job)
		{
			TerminateJobObject(job, 1);
		}
	}

#else // POSIX

	int Process::run(const Options & options, const LineCallback & on_line)
	{
		int out_pipe[2];
		int in_pipe[2];
		if (pipe(out_pipe) != 0 or pipe(in_pipe) != 0)
		{
			throw std::runtime_error("pipe() failed");
		}

		std::string path_env = std::getenv("PATH") ? std::getenv("PATH") : "";
		for (auto it = options.extra_path.rbegin(); it != options.extra_path.rend(); ++ it)
		{
			path_env = it->string() + ":" + path_env;
		}

		std::vector<std::string> args{options.executable.string()};
		args.insert(args.end(), options.arguments.begin(), options.arguments.end());
		std::vector<char *> argv;
		for (auto & a : args)
		{
			argv.push_back(a.data());
		}
		argv.push_back(nullptr);

		const pid_t child = fork();
		if (child < 0)
		{
			throw std::runtime_error("fork() failed");
		}
		if (child == 0)
		{
			setpgid(0, 0);
			dup2(in_pipe[0], STDIN_FILENO);
			dup2(out_pipe[1], STDOUT_FILENO);
			dup2(out_pipe[1], STDERR_FILENO);
			close(out_pipe[0]); close(out_pipe[1]); close(in_pipe[0]); close(in_pipe[1]);
			setenv("PATH", path_env.c_str(), 1);
			setenv("LD_LIBRARY_PATH", (path_env + ":" + (std::getenv("LD_LIBRARY_PATH") ? std::getenv("LD_LIBRARY_PATH") : "")).c_str(), 1);
			if (not options.working_directory.empty() and chdir(options.working_directory.c_str()) != 0)
			{
				_exit(126);
			}
			execv(argv[0], argv.data());
			_exit(127);
		}

		{
			std::scoped_lock lock(mutex);
			pid = child;
		}
		close(out_pipe[1]);
		close(in_pipe[0]);
		if (not options.stdin_text.empty())
		{
			[[maybe_unused]] auto ignored = write(in_pipe[1], options.stdin_text.data(), options.stdin_text.size());
		}
		close(in_pipe[1]);

		std::string buffer;
		char chunk[4096];
		ssize_t bytes = 0;
		while ((bytes = read(out_pipe[0], chunk, sizeof(chunk))) > 0)
		{
			buffer.append(chunk, static_cast<size_t>(bytes));
			deliver_lines(buffer, on_line);
		}
		if (not buffer.empty())
		{
			buffer += '\n';
			deliver_lines(buffer, on_line);
		}
		close(out_pipe[0]);

		int status = 0;
		waitpid(child, &status, 0);
		std::scoped_lock lock(mutex);
		pid = -1;
		return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
	}


	void Process::stop()
	{
		std::scoped_lock lock(mutex);
		stopped = true;
		if (pid > 0)
		{
			kill(-pid, SIGTERM);
		}
	}

#endif
}
