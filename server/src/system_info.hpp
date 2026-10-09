// DarkStudio server - detect the CPU, the Darknet builds and OpenVINO devices.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "settings.hpp"

#include <json/json.h>

#include <mutex>

namespace darkstudio
{
	/// Runs the probes (which take a few seconds: they start Darknet and OpenVINO) and caches the result.
	class SystemInfo
	{
		public:

			explicit SystemInfo(SettingsStore & settings);

			/// The last probe result; {"probing": true} until the first probe has finished.
			Json::Value get() const;

			/// Probe again in a background thread and broadcast a "system" event when done.
			void refresh();

		private:

			Json::Value probe() const;

			SettingsStore & settings;
			mutable std::mutex mutex;
			Json::Value cached;
			bool probing = false;
	};
}
