// DarkStudio server - training setup: the base models that can be trained, and generation of the Darknet files
// (.data, .names, train/valid lists, .cfg) for a labeled folder in datasets/.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "settings.hpp"

#include <json/json.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace darkstudio::training
{
	namespace fs = std::filesystem;

	/// One trainable network from the built-in model list.
	struct Model
	{
		std::string id;				///< e.g. "yolov4-tiny"
		std::string name;			///< shown in the UI
		std::string cfg;			///< template, relative to the DarkStudio root
		std::string weights;		///< COCO weights file name in the models folder ("" = none known)
		std::string weights_url;	///< where to download @p weights
		int partial_layers = 0;		///< "darknet partial" layer count for transfer learning (0 = no pretrained start)
		int width = 416;			///< default network size
		int height = 416;
		std::string license;
	};

	const std::vector<Model> & models();
	const Model * find_model(const std::string & id);

	/// The model list for the UI, with what is available on this machine (cfg, weights, partial weights).
	Json::Value models_json(const Settings & s);

	/// "yolov4-tiny.conv.29" for a model with partial_layers.
	fs::path partial_weights_path(const Settings & s, const Model & m);

	/// Settings that change the template cfg.  Unset optionals keep the template's value.
	struct CfgOptions
	{
		int classes = 1;
		int width = 416;
		int height = 416;
		int max_batches = 6000;
		int batch = 64;
		int subdivisions = 1;
		std::optional<double> learning_rate;
		std::optional<bool> mosaic;
		std::optional<bool> flip;
		std::optional<double> hue;
		std::optional<double> saturation;
		std::optional<double> exposure;
		std::optional<double> angle;
	};

	/// Rewrite a Darknet cfg: [net] size, batch, iterations, burn_in, steps (80 % / 90 %) and the options above;
	/// classes= in every [yolo] and filters=(classes + 5) * masks in the last [convolutional] before each [yolo].
	std::string make_cfg(const std::string & template_text, const CfgOptions & o);

	/// Split sorted images into train and validation lists: every n-th image (n = 100 / valid_percent) is used for
	/// validation.  With 0 % the validation list is the training list (mAP is then measured on seen images).
	void split(const std::vector<fs::path> & images, int valid_percent, std::vector<fs::path> & train, std::vector<fs::path> & valid);

	/// Folders below @p datasets_root that hold labeled images, directly or in sub-folders:
	/// [{path, labeled, with_boxes, names}], with counts that include the sub-folders.
	Json::Value datasets_json(const fs::path & datasets_root);

	/// A dataset folder from the UI: a path relative to the datasets folder (checked to stay inside it), or an absolute
	/// path anywhere (picked with the file browser).  @p names_root is where the .names search stops: the datasets
	/// folder, or the parent of an outside folder.  Throws std::invalid_argument if it isn't a folder.
	struct DatasetFolder
	{
		fs::path folder;
		fs::path names_root;
	};
	DatasetFolder resolve_dataset(const Settings & s, const std::string & text);

	/// What a dataset folder holds, counting sub-folders: {path, labeled, with_boxes, names}.
	Json::Value dataset_info(const Settings & s, const std::string & text);

	/// Labeled images (with a .txt file next to them) below @p folder, sorted.
	std::vector<fs::path> labeled_images(const fs::path & folder);

	struct Prepared
	{
		fs::path data;
		fs::path cfg;
		fs::path weights;	///< empty = start from scratch
		int classes = 0;
		size_t train_images = 0;
		size_t valid_images = 0;
		std::string name;
	};

	/// Write <dir>/{name.data, name.names, train.txt, valid.txt, name.cfg} for job parameters
	/// {dataset, model, width, height, max_batches, batch, subdivisions, valid_percent, pretrained, learning_rate, mosaic,
	/// flip, hue, saturation, exposure, angle}.  Creates the partial pretrained weights with "darknet partial" the
	/// first time they are needed.  Throws std::invalid_argument for bad parameters or a dataset that can't be trained.
	Prepared prepare(const Settings & s, const Json::Value & params, const fs::path & dir);
}
