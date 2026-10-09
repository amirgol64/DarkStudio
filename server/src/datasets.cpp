// DarkStudio server - datasets for the annotation view.
// SPDX-License-Identifier: Apache-2.0

#include "datasets.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>


namespace darkstudio::datasets
{
	namespace
	{
		std::string utf8(const fs::path & p)
		{
			const auto u8 = p.generic_u8string();
			return std::string(u8.begin(), u8.end());
		}

		std::string relative_utf8(const fs::path & p, const fs::path & root)
		{
			return utf8(fs::relative(p, root));
		}

		/// Class names for a folder: the first *.names file in the folder or one of its parents (up to @p root).
		std::vector<std::string> find_names(const fs::path & folder, const fs::path & root)
		{
			const fs::path base = fs::weakly_canonical(root);
			for (fs::path dir = fs::weakly_canonical(folder); ; dir = dir.parent_path())
			{
				if (fs::is_directory(dir))
				{
					for (const auto & entry : fs::directory_iterator(dir))
					{
						if (entry.path().extension() == ".names")
						{
							std::vector<std::string> names;
							std::ifstream ifs(entry.path());
							std::string line;
							while (std::getline(ifs, line))
							{
								while (not line.empty() and (line.back() == '\r' or std::isspace(static_cast<unsigned char>(line.back()))))
								{
									line.pop_back();
								}
								if (not line.empty())
								{
									names.push_back(line);
								}
							}
							return names;
						}
					}
				}
				// folders always come from resolve(), so walking up reaches the datasets root
				if (dir == base or dir == dir.parent_path())
				{
					break;
				}
			}
			return {};
		}

		Json::Value names_json(const std::vector<std::string> & names)
		{
			Json::Value arr = Json::arrayValue;
			for (const auto & n : names)
			{
				arr.append(n);
			}
			return arr;
		}
	}


	bool is_image(const fs::path & p)
	{
		std::string ext = p.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return ext == ".jpg" or ext == ".jpeg" or ext == ".png" or ext == ".bmp";
	}


	fs::path resolve(const fs::path & root, const std::string & relative)
	{
		const fs::path base = fs::weakly_canonical(root);
		const fs::path full = fs::weakly_canonical(base / fs::path(std::u8string(relative.begin(), relative.end())));

		// the resolved path must be the root itself or below it
		auto b = base.begin();
		auto f = full.begin();
		for (; b != base.end(); ++ b, ++ f)
		{
			if (f == full.end() or *b != *f)
			{
				throw std::invalid_argument("path is outside of the datasets folder: " + relative);
			}
		}
		return full;
	}


	Json::Value list(const fs::path & root)
	{
		Json::Value arr = Json::arrayValue;
		if (not fs::is_directory(root))
		{
			return arr;
		}

		struct Counts { int images = 0; int labeled = 0; int with_boxes = 0; };
		std::map<fs::path, Counts> folders;
		for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied); it != fs::recursive_directory_iterator(); ++ it)
		{
			if (it.depth() > 6)
			{
				it.disable_recursion_pending();
				continue;
			}
			if (it->is_regular_file() and is_image(it->path()))
			{
				auto & counts = folders[it->path().parent_path()];
				counts.images ++;
				const fs::path txt = fs::path(it->path()).replace_extension(".txt");
				std::error_code ec;
				if (fs::exists(txt, ec))
				{
					counts.labeled ++;	// an empty .txt is a valid "negative" image without objects
					if (fs::file_size(txt, ec) > 0)
					{
						counts.with_boxes ++;
					}
				}
			}
		}

		for (const auto & [folder, counts] : folders)
		{
			Json::Value j;
			j["path"]		= relative_utf8(folder, root);
			j["images"]		= counts.images;
			j["labeled"]	= counts.labeled;
			j["with_boxes"]	= counts.with_boxes;
			j["names"]		= names_json(find_names(folder, root));
			arr.append(j);
		}
		return arr;
	}


	Json::Value images(const fs::path & root, const std::string & folder)
	{
		const fs::path dir = resolve(root, folder);
		if (not fs::is_directory(dir))
		{
			throw std::invalid_argument("not a folder: " + folder);
		}

		std::vector<fs::path> files;
		for (const auto & entry : fs::directory_iterator(dir))
		{
			if (entry.is_regular_file() and is_image(entry.path()))
			{
				files.push_back(entry.path());
			}
		}
		std::sort(files.begin(), files.end());

		Json::Value arr = Json::arrayValue;
		for (const auto & f : files)
		{
			Json::Value j;
			j["path"]		= relative_utf8(f, root);
			j["name"]		= utf8(f.filename());
			j["labeled"]	= fs::exists(fs::path(f).replace_extension(".txt"));
			arr.append(j);
		}

		Json::Value r;
		r["folder"]	= relative_utf8(dir, root);
		r["names"]	= names_json(find_names(dir, root));
		r["images"]	= arr;
		return r;
	}


	Json::Value read_labels(const fs::path & root, const std::string & image)
	{
		const fs::path img = resolve(root, image);
		if (not fs::is_regular_file(img) or not is_image(img))
		{
			throw std::invalid_argument("not an image: " + image);
		}

		Json::Value boxes = Json::arrayValue;
		std::ifstream ifs(fs::path(img).replace_extension(".txt"));
		std::string line;
		while (std::getline(ifs, line))
		{
			std::istringstream ss(line);
			int cls = -1;
			double x = 0, y = 0, w = 0, h = 0;
			if (ss >> cls >> x >> y >> w >> h)
			{
				Json::Value b;
				b["class"] = cls;
				b["x"] = x;
				b["y"] = y;
				b["w"] = w;
				b["h"] = h;
				boxes.append(b);
			}
		}

		Json::Value r;
		r["image"]	= relative_utf8(img, root);
		r["names"]	= names_json(find_names(img.parent_path(), root));
		r["boxes"]	= boxes;
		return r;
	}


	Json::Value write_labels(const fs::path & root, const std::string & image, const Json::Value & boxes)
	{
		const fs::path img = resolve(root, image);
		if (not fs::is_regular_file(img) or not is_image(img))
		{
			throw std::invalid_argument("not an image: " + image);
		}
		if (not boxes.isArray())
		{
			throw std::invalid_argument("\"boxes\" must be an array");
		}

		std::ostringstream out;
		out << std::fixed << std::setprecision(10);
		for (const auto & b : boxes)
		{
			const int cls = b["class"].asInt();
			const double x = b["x"].asDouble(), y = b["y"].asDouble(), w = b["w"].asDouble(), h = b["h"].asDouble();
			if (cls < 0 or w <= 0.0 or h <= 0.0 or w > 1.0 or h > 1.0 or x < 0.0 or x > 1.0 or y < 0.0 or y > 1.0)
			{
				throw std::invalid_argument("invalid box: class must be >= 0 and x, y, w, h must be normalized (0..1)");
			}
			out << cls << " " << x << " " << y << " " << w << " " << h << "\n";
		}

		// write to a temporary file first so a failure never leaves a half-written label file
		const fs::path txt = fs::path(img).replace_extension(".txt");
		const fs::path tmp = fs::path(txt).concat(".tmp");
		{
			std::ofstream ofs(tmp, std::ios::binary);
			ofs << out.str();
			if (not ofs)
			{
				throw std::runtime_error("cannot write " + utf8(txt));
			}
		}
		fs::rename(tmp, txt);
		return read_labels(root, image);
	}
}
