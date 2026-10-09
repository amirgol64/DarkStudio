// DarkStudio server - datasets for the annotation view: list images, read and write YOLO labels.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json/json.h>

#include <filesystem>
#include <string>

namespace darkstudio::datasets
{
	namespace fs = std::filesystem;

	/// Resolve @p relative inside @p root.  Throws std::invalid_argument if the result would be outside of @p root
	/// (e.g. "../../Windows"), so the API can only touch files below the datasets folder.
	fs::path resolve(const fs::path & root, const std::string & relative);

	/// Folders below @p root that contain images: [{path, images, labeled, with_boxes, names}].
	/// "labeled" counts images with a .txt file (possibly empty = no objects), "with_boxes" those with at least one box.
	Json::Value list(const fs::path & root);

	/// Images in one folder: [{path, name, labeled}], plus the class names found for that folder.
	Json::Value images(const fs::path & root, const std::string & folder);

	/// YOLO labels of one image: {image, width, height, names, boxes: [{class, x, y, w, h}]} (normalized centre/size).
	Json::Value read_labels(const fs::path & root, const std::string & image);

	/// Validate and write YOLO labels for one image (the .txt file next to the image).
	Json::Value write_labels(const fs::path & root, const std::string & image, const Json::Value & boxes);

	bool is_image(const fs::path & p);
}
