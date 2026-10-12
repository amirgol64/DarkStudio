// DarkStudio server - training setup.
// SPDX-License-Identifier: Apache-2.0

#include "training.hpp"

#include "datasets.hpp"
#include "process.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>


namespace darkstudio::training
{
	namespace
	{
		std::string utf8(const fs::path & p)
		{
			const auto u8 = p.u8string();
			return std::string(u8.begin(), u8.end());
		}

		std::string trim(const std::string & s)
		{
			const auto b = s.find_first_not_of(" \t\r\n");
			if (b == std::string::npos)
			{
				return "";
			}
			const auto e = s.find_last_not_of(" \t\r\n");
			return s.substr(b, e - b + 1);
		}

		std::string number(double v)
		{
			std::ostringstream ss;
			ss << v;
			return ss.str();
		}

		/// The key of a "key = value" cfg line, or "" for comments, blank lines and section headers.
		std::string key_of(const std::string & line)
		{
			const std::string t = trim(line);
			if (t.empty() or t[0] == '#' or t[0] == ';' or t[0] == '[')
			{
				return "";
			}
			const auto eq = t.find('=');
			return eq == std::string::npos ? "" : trim(t.substr(0, eq));
		}

		std::string value_of(const std::string & line)
		{
			const auto eq = line.find('=');
			return eq == std::string::npos ? "" : trim(line.substr(eq + 1));
		}

		struct Section
		{
			std::string name;				///< "net", "convolutional", "yolo", ... ("" for lines before the first header)
			std::vector<std::string> lines;	///< including the header line
		};

		std::vector<Section> parse_sections(const std::string & text)
		{
			std::vector<Section> sections(1);
			std::istringstream ss(text);
			std::string line;
			while (std::getline(ss, line))
			{
				if (not line.empty() and line.back() == '\r')
				{
					line.pop_back();
				}
				const std::string t = trim(line);
				if (t.size() > 2 and t.front() == '[' and t.back() == ']')
				{
					sections.push_back({t.substr(1, t.size() - 2), {}});
				}
				sections.back().lines.push_back(line);
			}
			return sections;
		}

		std::string get_key(const Section & s, const std::string & key)
		{
			for (const auto & line : s.lines)
			{
				if (key_of(line) == key)
				{
					return value_of(line);
				}
			}
			return "";
		}

		/// Replace the first "key=..." line, or add one after the last setting of the section.
		void set_key(Section & s, const std::string & key, const std::string & value)
		{
			for (auto & line : s.lines)
			{
				if (key_of(line) == key)
				{
					line = key + "=" + value;
					return;
				}
			}
			size_t pos = s.lines.size();
			while (pos > 1 and trim(s.lines[pos - 1]).empty())
			{
				pos --;
			}
			s.lines.insert(s.lines.begin() + static_cast<std::ptrdiff_t>(pos), key + "=" + value);
		}

		/// Number of entries in a comma-separated list such as "mask = 3,4,5".
		int count_list(const std::string & value)
		{
			if (trim(value).empty())
			{
				return 0;
			}
			return static_cast<int>(std::count(value.begin(), value.end(), ',')) + 1;
		}

		std::string safe_name(const std::string & text)
		{
			std::string out;
			for (unsigned char c : text)
			{
				out += (std::isalnum(c) or c == '-' or c == '_') ? static_cast<char>(c) : '_';
			}
			return out.empty() ? "dataset" : out;
		}

		std::string read_file(const fs::path & p)
		{
			std::ifstream ifs(p, std::ios::binary);
			std::ostringstream ss;
			ss << ifs.rdbuf();
			return ss.str();
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

		std::optional<double> double_param(const Json::Value & p, const char * key, double min, double max)
		{
			if (not p.isMember(key) or p[key].isNull())
			{
				return std::nullopt;
			}
			if (not p[key].isNumeric())
			{
				throw std::invalid_argument(std::string(key) + " must be a number");
			}
			const double v = p[key].asDouble();
			if (v < min or v > max)
			{
				throw std::invalid_argument(std::string(key) + " must be between " + number(min) + " and " + number(max));
			}
			return v;
		}

		std::optional<bool> bool_option(const Json::Value & p, const char * key)
		{
			if (p.isMember(key) and p[key].isBool())
			{
				return p[key].asBool();
			}
			return std::nullopt;
		}

		void write_lines(const fs::path & file, const std::vector<std::string> & lines)
		{
			std::ofstream ofs(file, std::ios::binary);
			for (const auto & l : lines)
			{
				ofs << l << "\n";
			}
		}

		/// Run "darknet partial" once to cut the COCO weights down to the layers that are reused for transfer learning.
		void make_partial(const Settings & s, const Model & m, const fs::path & out)
		{
			const fs::path weights = s.models_dir / m.weights;
			if (not fs::is_regular_file(weights))
			{
				throw std::invalid_argument("pretrained weights not found: " + utf8(weights) + " (download " + m.weights_url +
					" into the models folder, or turn off \"start from pretrained weights\")");
			}
#ifdef _WIN32
			const fs::path exe = s.darknet_cpu_bin / "darknet.exe";
#else
			const fs::path exe = s.darknet_cpu_bin / "darknet";
#endif
			if (not fs::is_regular_file(exe))
			{
				throw std::invalid_argument("Darknet CPU not found (needed once to prepare the pretrained weights): " + utf8(exe));
			}

			// the COCO weights belong to the plain yolov4-tiny network; its first layers are shared by the -3l variant
			const fs::path cfg = s.root / "darknet" / "cfg" / (m.weights.substr(0, m.weights.rfind('.')) + ".cfg");
			Process process;
			Process::Options o;
			o.executable = exe;
			o.arguments = {"partial", utf8(cfg), utf8(weights), utf8(out), std::to_string(m.partial_layers)};
			o.working_directory = s.models_dir;
			o.extra_path = {s.darknet_cpu_bin};
			std::string output;
			const int rc = process.run(o, [&](const std::string & line) { output += line + "\n"; });
			if (rc != 0 or not fs::is_regular_file(out))
			{
				throw std::invalid_argument("darknet partial failed (exit code " + std::to_string(rc) + "):\n" + output);
			}
		}
	}


	const std::vector<Model> & models()
	{
		static const std::vector<Model> list =
		{
			{"yolov4-tiny",		"YOLOv4-tiny",		"darknet/cfg/yolov4-tiny.cfg",		"yolov4-tiny.weights",
				"https://github.com/hank-ai/darknet/releases/download/v2.0/yolov4-tiny.weights", 29, 416, 416, "Apache-2.0 (Darknet)"},
			{"yolov4-tiny-3l",	"YOLOv4-tiny-3L",	"darknet/cfg/yolov4-tiny-3l.cfg",	"yolov4-tiny.weights",
				"https://github.com/hank-ai/darknet/releases/download/v2.0/yolov4-tiny.weights", 29, 416, 416, "Apache-2.0 (Darknet)"},
			{"yolov7-tiny",		"YOLOv7-tiny",		"darknet/cfg/yolov7-tiny.cfg",		"", "", 0, 416, 416, "Apache-2.0 (Darknet)"},
		};
		return list;
	}


	const Model * find_model(const std::string & id)
	{
		for (const auto & m : models())
		{
			if (m.id == id)
			{
				return &m;
			}
		}
		return nullptr;
	}


	fs::path partial_weights_path(const Settings & s, const Model & m)
	{
		const std::string stem = m.weights.substr(0, m.weights.rfind('.'));
		return s.models_dir / (stem + ".conv." + std::to_string(m.partial_layers));
	}


	Json::Value models_json(const Settings & s)
	{
		Json::Value arr = Json::arrayValue;
		for (const auto & m : models())
		{
			Json::Value j;
			j["id"]				= m.id;
			j["name"]			= m.name;
			j["cfg"]			= m.cfg;
			j["cfg_found"]		= fs::is_regular_file(s.root / m.cfg);
			j["width"]			= m.width;
			j["height"]			= m.height;
			j["license"]		= m.license;
			j["pretrained"]		= m.partial_layers > 0;
			j["weights"]		= m.weights;
			j["weights_url"]	= m.weights_url;
			j["weights_found"]	= not m.weights.empty() and fs::is_regular_file(s.models_dir / m.weights);
			j["partial_found"]	= m.partial_layers > 0 and fs::is_regular_file(partial_weights_path(s, m));
			arr.append(j);
		}
		return arr;
	}


	std::string make_cfg(const std::string & template_text, const CfgOptions & o)
	{
		auto sections = parse_sections(template_text);

		bool has_net = false;
		for (auto & s : sections)
		{
			if (s.name != "net" and s.name != "network")
			{
				continue;
			}
			has_net = true;
			set_key(s, "batch",			std::to_string(o.batch));
			set_key(s, "subdivisions",	std::to_string(o.subdivisions));
			set_key(s, "width",			std::to_string(o.width));
			set_key(s, "height",		std::to_string(o.height));
			set_key(s, "max_batches",	std::to_string(o.max_batches));
			set_key(s, "burn_in",		std::to_string(std::clamp(o.max_batches / 5, 1, 1000)));
			set_key(s, "steps",			std::to_string(std::max(1, o.max_batches * 8 / 10)) + "," + std::to_string(std::max(1, o.max_batches * 9 / 10)));
			if (get_key(s, "policy").empty())
			{
				set_key(s, "policy", "steps");
			}
			if (get_key(s, "scales").empty())
			{
				set_key(s, "scales", ".1,.1");
			}
			if (o.learning_rate)	{ set_key(s, "learning_rate",	number(*o.learning_rate)); }
			if (o.mosaic)			{ set_key(s, "mosaic",			*o.mosaic ? "1" : "0"); }
			if (o.flip)				{ set_key(s, "flip",			*o.flip ? "1" : "0"); }
			if (o.hue)				{ set_key(s, "hue",				number(*o.hue)); }
			if (o.saturation)		{ set_key(s, "saturation",		number(*o.saturation)); }
			if (o.exposure)			{ set_key(s, "exposure",		number(*o.exposure)); }
			if (o.angle)			{ set_key(s, "angle",			number(*o.angle)); }
			break;
		}
		if (not has_net)
		{
			throw std::invalid_argument("the cfg has no [net] section");
		}

		int yolo_layers = 0;
		Section * last_conv = nullptr;
		for (auto & s : sections)
		{
			if (s.name == "convolutional")
			{
				last_conv = &s;
			}
			else if (s.name == "yolo" or s.name == "Gaussian_yolo")
			{
				if (last_conv == nullptr)
				{
					throw std::invalid_argument("the cfg has a [yolo] section without a [convolutional] before it");
				}
				int masks = count_list(get_key(s, "mask"));
				if (masks == 0)
				{
					masks = std::max(1, std::atoi(get_key(s, "num").c_str()));
				}
				set_key(s, "classes", std::to_string(o.classes));
				set_key(*last_conv, "filters", std::to_string((o.classes + 5) * masks));
				yolo_layers ++;
			}
		}
		if (yolo_layers == 0)
		{
			throw std::invalid_argument("the cfg has no [yolo] section");
		}

		std::string out;
		for (const auto & s : sections)
		{
			for (const auto & line : s.lines)
			{
				out += line + "\n";
			}
		}
		return out;
	}


	void split(const std::vector<fs::path> & images, int valid_percent, std::vector<fs::path> & train, std::vector<fs::path> & valid)
	{
		train.clear();
		valid.clear();
		if (valid_percent <= 0)
		{
			train = images;
			valid = images;
			return;
		}
		const size_t every = std::max<size_t>(2, static_cast<size_t>(std::lround(100.0 / valid_percent)));
		for (size_t i = 0; i < images.size(); i ++)
		{
			((i % every) == every - 1 ? valid : train).push_back(images[i]);
		}
		if (valid.empty() and not train.empty())
		{
			// too few images for the split: measure on the last training image rather than not at all
			valid.push_back(train.back());
		}
	}


	Json::Value datasets_json(const fs::path & datasets_root)
	{
		Json::Value arr = Json::arrayValue;
		if (not fs::is_directory(datasets_root))
		{
			return arr;
		}
		const fs::path root = fs::weakly_canonical(datasets_root);
		struct Counts { int labeled = 0; int with_boxes = 0; };
		std::map<fs::path, Counts> folders;
		for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied); it != fs::recursive_directory_iterator(); ++ it)
		{
			if (it.depth() > 6)
			{
				it.disable_recursion_pending();
				continue;
			}
			if (not it->is_regular_file() or not datasets::is_image(it->path()))
			{
				continue;
			}
			const fs::path txt = fs::path(it->path()).replace_extension(".txt");
			std::error_code ec;
			if (not fs::exists(txt, ec))
			{
				continue;
			}
			const bool boxes = fs::file_size(txt, ec) > 0;
			// count the image for its folder and every parent up to (not including) the datasets root
			for (fs::path dir = it->path().parent_path(); dir != root and dir.has_relative_path() and dir != dir.parent_path(); dir = dir.parent_path())
			{
				auto & c = folders[dir];
				c.labeled ++;
				c.with_boxes += boxes ? 1 : 0;
			}
		}
		for (const auto & [folder, c] : folders)
		{
			const auto rel = fs::relative(folder, root).generic_u8string();
			Json::Value j;
			j["path"]		= std::string(rel.begin(), rel.end());
			j["labeled"]	= c.labeled;
			j["with_boxes"]	= c.with_boxes;
			Json::Value names = Json::arrayValue;
			for (const auto & n : datasets::find_names(folder, root))
			{
				names.append(n);
			}
			j["names"] = names;
			arr.append(j);
		}
		return arr;
	}


	DatasetFolder resolve_dataset(const Settings & s, const std::string & text)
	{
		if (text.empty())
		{
			throw std::invalid_argument("choose a dataset folder");
		}
		DatasetFolder d;
		const fs::path p(std::u8string(text.begin(), text.end()));
		if (p.is_absolute())
		{
			std::error_code ec;
			d.folder = fs::weakly_canonical(p, ec);
			const fs::path root = fs::weakly_canonical(s.datasets_dir, ec);
			const auto rel = fs::relative(d.folder, root, ec);
			const bool inside = not ec and not rel.empty() and *rel.begin() != "..";
			d.names_root = inside ? root : d.folder.parent_path();
		}
		else
		{
			d.folder = datasets::resolve(s.datasets_dir, text);
			d.names_root = s.datasets_dir;
		}
		if (not fs::is_directory(d.folder))
		{
			throw std::invalid_argument("dataset folder not found: " + text);
		}
		return d;
	}


	Json::Value dataset_info(const Settings & s, const std::string & text)
	{
		const auto d = resolve_dataset(s, text);
		int labeled = 0;
		int with_boxes = 0;
		for (const auto & img : labeled_images(d.folder))
		{
			labeled ++;
			std::error_code ec;
			with_boxes += fs::file_size(fs::path(img).replace_extension(".txt"), ec) > 0 ? 1 : 0;
		}
		Json::Value j;
		j["path"]		= utf8(fs::path(d.folder).make_preferred());
		j["labeled"]	= labeled;
		j["with_boxes"]	= with_boxes;
		Json::Value names = Json::arrayValue;
		for (const auto & n : datasets::find_names(d.folder, d.names_root))
		{
			names.append(n);
		}
		j["names"] = names;
		return j;
	}


	std::vector<fs::path> labeled_images(const fs::path & folder)
	{
		std::vector<fs::path> out;
		for (auto it = fs::recursive_directory_iterator(folder, fs::directory_options::skip_permission_denied); it != fs::recursive_directory_iterator(); ++ it)
		{
			if (it.depth() > 6)
			{
				it.disable_recursion_pending();
				continue;
			}
			if (it->is_regular_file() and datasets::is_image(it->path()) and fs::exists(fs::path(it->path()).replace_extension(".txt")))
			{
				out.push_back(it->path());
			}
		}
		std::sort(out.begin(), out.end());
		return out;
	}


	Prepared prepare(const Settings & s, const Json::Value & p, const fs::path & dir)
	{
		const std::string model_id = (p.isMember("model") and p["model"].isString()) ? p["model"].asString() : "yolov4-tiny";
		const Model * model = find_model(model_id);
		if (model == nullptr)
		{
			throw std::invalid_argument("unknown model \"" + model_id + "\"");
		}
		const fs::path template_cfg = s.root / model->cfg;
		if (not fs::is_regular_file(template_cfg))
		{
			throw std::invalid_argument("model cfg not found: " + utf8(template_cfg));
		}

		const std::string dataset = p.isMember("dataset") and p["dataset"].isString() ? p["dataset"].asString() : "";
		const auto resolved = resolve_dataset(s, dataset);
		const fs::path folder = resolved.folder;

		const auto names = datasets::find_names(folder, resolved.names_root);
		if (names.empty())
		{
			throw std::invalid_argument("no .names file in " + dataset + " or its parent folders: add one with a class name per line (see docs/how_to_use.md)");
		}
		const int classes = static_cast<int>(names.size());

		CfgOptions o;
		o.classes		= classes;
		o.width			= int_param(p, "width", model->width, 32, 2048);
		o.height		= int_param(p, "height", model->height, 32, 2048);
		o.max_batches	= int_param(p, "max_batches", std::max(6000, 2000 * classes), 1, 2000000);
		o.batch			= int_param(p, "batch", 64, 1, 256);
		o.subdivisions	= int_param(p, "subdivisions", 1, 1, 256);
		if (o.width % 32 != 0 or o.height % 32 != 0)
		{
			throw std::invalid_argument("width and height must be multiples of 32");
		}
		if (o.batch % o.subdivisions != 0)
		{
			throw std::invalid_argument("batch must be divisible by subdivisions");
		}
		o.learning_rate	= double_param(p, "learning_rate", 1e-6, 0.1);
		o.hue			= double_param(p, "hue", 0.0, 0.5);
		o.saturation	= double_param(p, "saturation", 1.0, 3.0);
		o.exposure		= double_param(p, "exposure", 1.0, 3.0);
		o.angle			= double_param(p, "angle", 0.0, 180.0);
		o.mosaic		= bool_option(p, "mosaic");
		o.flip			= bool_option(p, "flip");
		const int valid_percent = int_param(p, "valid_percent", 10, 0, 50);

		// every labeled image, with the class ids checked against the names file
		const auto images = labeled_images(folder);
		if (images.size() < 2)
		{
			throw std::invalid_argument("dataset " + dataset + " has " + std::to_string(images.size()) + " labeled images; label some on the Annotation page first");
		}
		size_t boxes = 0;
		for (const auto & img : images)
		{
			std::ifstream ifs(fs::path(img).replace_extension(".txt"));
			std::string line;
			while (std::getline(ifs, line))
			{
				if (trim(line).empty())
				{
					continue;
				}
				const int cls = std::atoi(trim(line).c_str());
				if (cls < 0 or cls >= classes)
				{
					throw std::invalid_argument("class " + std::to_string(cls) + " in " + utf8(fs::path(img).replace_extension(".txt")) +
						" is not in the names file (" + std::to_string(classes) + " classes)");
				}
				boxes ++;
			}
		}
		if (boxes == 0)
		{
			throw std::invalid_argument("dataset " + dataset + " has no boxes yet");
		}

		std::vector<fs::path> train, valid;
		split(images, valid_percent, train, valid);

		Prepared r;
		r.name			= safe_name(folder.filename().string());
		r.classes		= classes;
		r.train_images	= train.size();
		r.valid_images	= valid.size();
		r.data			= dir / (r.name + ".data");
		r.cfg			= dir / (r.name + ".cfg");

		std::vector<std::string> lines;
		for (const auto & i : train) { lines.push_back(utf8(i)); }
		write_lines(dir / "train.txt", lines);
		lines.clear();
		for (const auto & i : valid) { lines.push_back(utf8(i)); }
		write_lines(dir / "valid.txt", lines);
		write_lines(dir / (r.name + ".names"), names);
		write_lines(r.data, {
			"classes = " + std::to_string(classes),
			"train = " + utf8(dir / "train.txt"),
			"valid = " + utf8(dir / "valid.txt"),
			"names = " + utf8(dir / (r.name + ".names")),
			"backup = " + utf8(dir)});
		std::ofstream(r.cfg, std::ios::binary) << make_cfg(read_file(template_cfg), o);

		const bool pretrained = not (p.isMember("pretrained") and p["pretrained"].isBool() and not p["pretrained"].asBool());
		if (pretrained and model->partial_layers > 0)
		{
			r.weights = partial_weights_path(s, *model);
			if (not fs::is_regular_file(r.weights))
			{
				make_partial(s, *model, r.weights);
			}
		}
		return r;
	}
}
