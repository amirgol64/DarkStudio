// DarkStudio server - file browser for the UI's "Browse…" window: list one folder (names, sizes, a short summary of
// the images in it) so the user can pick a dataset folder or a .data / .cfg / .weights file.
// SPDX-License-Identifier: Apache-2.0
//
// It only lists names; it never returns file contents.  The server listens on 127.0.0.1 only (see
// docs/darkstudio-ui.md, Security); multi-user mode (M5) has to restrict it.

#pragma once

#include "settings.hpp"

#include <json/json.h>

#include <string>

namespace darkstudio::files
{
	/// List the folder @p path (absolute, UTF-8): {path, parent, entries: [{name, path, dir, size}], summary:
	/// {images, labeled, names}, shortcuts: [{id, path}]}.  Folders come first, both sorted by name; hidden and
	/// system entries are skipped.  An empty @p path lists the drives (Windows) or "/" (POSIX), with parent "".
	/// Throws std::invalid_argument for a relative path or one that isn't a folder.
	Json::Value list(const Settings & s, const std::string & path);
}
