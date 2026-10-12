// DarkStudio server - file browser.
// SPDX-License-Identifier: Apache-2.0

#include "files.hpp"

#include "datasets.hpp"

#include <algorithm>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif


namespace darkstudio::files
{
	namespace
	{
		namespace fs = std::filesystem;

		constexpr size_t kMaxEntries = 5000;

		std::string utf8(const fs::path & p)
		{
			const auto u8 = p.u8string();
			return std::string(u8.begin(), u8.end());
		}

		fs::path path_from(const std::string & s)
		{
			return fs::path(std::u8string(s.begin(), s.end()));
		}

		bool hidden(const fs::path & p)
		{
			const auto name = p.filename().wstring();
			if (name.empty() or name[0] == L'.' or name[0] == L'$')
			{
				return true;
			}
#ifdef _WIN32
			const DWORD attr = GetFileAttributesW(p.c_str());
			return attr != INVALID_FILE_ATTRIBUTES and (attr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
#else
			return false;
#endif
		}

		std::wstring lower(std::wstring s)
		{
			std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
			return s;
		}

		Json::Value shortcuts(const Settings & s)
		{
			Json::Value arr = Json::arrayValue;
			auto add = [&](const char * id, const fs::path & p)
			{
				std::error_code ec;
				if (not p.empty() and fs::is_directory(p, ec))
				{
					Json::Value j;
					j["id"]		= id;
					j["path"]	= utf8(fs::weakly_canonical(p, ec).make_preferred());
					arr.append(j);
				}
			};
			add("datasets",	s.datasets_dir);
			add("models",	s.models_dir);
			add("runs",		s.work_dir / "jobs");
			add("root",		s.root);
#ifdef _WIN32
			wchar_t home[MAX_PATH];
			if (const DWORD n = GetEnvironmentVariableW(L"USERPROFILE", home, MAX_PATH); n > 0 and n < MAX_PATH) { add("home", fs::path(home)); }
#else
			if (const char * home = std::getenv("HOME")) { add("home", path_from(home)); }
#endif
			return arr;
		}
	}


	Json::Value list(const Settings & s, const std::string & text)
	{
		Json::Value r;
		r["shortcuts"]	= shortcuts(s);
		r["entries"]	= Json::arrayValue;

		if (text.empty())
		{
			r["path"]	= "";
			r["parent"]	= "";
#ifdef _WIN32
			const DWORD drives = GetLogicalDrives();
			for (int i = 0; i < 26; i ++)
			{
				if (drives & (1u << i))
				{
					const std::string root = std::string(1, static_cast<char>('A' + i)) + ":\\";
					Json::Value e;
					e["name"]	= root;
					e["path"]	= root;
					e["dir"]	= true;
					e["size"]	= 0;
					r["entries"].append(e);
				}
			}
#else
			Json::Value e;
			e["name"] = "/"; e["path"] = "/"; e["dir"] = true; e["size"] = 0;
			r["entries"].append(e);
#endif
			return r;
		}

		fs::path dir = path_from(text);
		if (not dir.is_absolute())
		{
			throw std::invalid_argument("not an absolute path: " + text);
		}
		std::error_code ec;
		dir = fs::weakly_canonical(dir, ec).make_preferred();
		if (not fs::is_directory(dir, ec))
		{
			throw std::invalid_argument("not a folder: " + text);
		}

		struct Entry { fs::path path; bool dir; uintmax_t size; std::wstring key; };
		std::vector<Entry> entries;
		int images = 0;
		int labeled = 0;
		Json::Value names = Json::arrayValue;
		for (auto it = fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec); not ec and it != fs::directory_iterator(); it.increment(ec))
		{
			const fs::path p = it->path();
			if (hidden(p))
			{
				continue;
			}
			std::error_code e2;
			const bool is_dir = it->is_directory(e2);
			if (not is_dir and not it->is_regular_file(e2))
			{
				continue;
			}
			if (not is_dir)
			{
				if (datasets::is_image(p))
				{
					images ++;
					labeled += fs::exists(fs::path(p).replace_extension(".txt"), e2) ? 1 : 0;
				}
				else if (p.extension() == ".names")
				{
					names.append(utf8(p.filename()));
				}
			}
			if (entries.size() < kMaxEntries)
			{
				entries.push_back({p, is_dir, is_dir ? 0 : it->file_size(e2), lower(p.filename().wstring())});
			}
		}
		if (ec)
		{
			throw std::invalid_argument("can't read " + text + ": " + ec.message());
		}

		std::sort(entries.begin(), entries.end(), [](const Entry & a, const Entry & b)
		{
			return a.dir != b.dir ? a.dir : a.key < b.key;
		});
		for (const auto & e : entries)
		{
			Json::Value j;
			j["name"]	= utf8(e.path.filename());
			j["path"]	= utf8(e.path);
			j["dir"]	= e.dir;
			j["size"]	= Json::UInt64(e.size);
			r["entries"].append(j);
		}

		r["path"]		= utf8(dir);
		r["parent"]		= dir.has_relative_path() ? utf8(dir.parent_path()) : "";	// "" = the drive list
		r["truncated"]	= entries.size() >= kMaxEntries;
		r["summary"]["images"]	= images;
		r["summary"]["labeled"]	= labeled;
		r["summary"]["names"]	= names;
		return r;
	}
}
