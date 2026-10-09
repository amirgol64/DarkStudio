// DarkStudio server - WebSocket that pushes live events (jobs, log lines, training metrics) to the UI.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <drogon/WebSocketController.h>
#include <json/json.h>

#include <mutex>
#include <unordered_set>

namespace darkstudio
{
	/// Endpoint: ws://<host>/ws/events.  Every message is a JSON object with a "type" field:
	///   {"type":"job",    "job":{...}}                       job created / status changed / finished
	///   {"type":"log",    "job_id":"...", "line":"...", "level":"info|warning|error", "n":123}
	///   {"type":"metric", "job_id":"...", "metric":{...}}    one training iteration or mAP result
	///   {"type":"system", "system":{...}}                    device probe finished
	class EventsSocket : public drogon::WebSocketController<EventsSocket>
	{
		public:

			void handleNewMessage(const drogon::WebSocketConnectionPtr &, std::string &&, const drogon::WebSocketMessageType &) override;
			void handleNewConnection(const drogon::HttpRequestPtr &, const drogon::WebSocketConnectionPtr &) override;
			void handleConnectionClosed(const drogon::WebSocketConnectionPtr &) override;

			WS_PATH_LIST_BEGIN
			WS_PATH_ADD("/ws/events");
			WS_PATH_LIST_END

			/// Send @p event to every connected client.  Safe to call from any thread.
			static void broadcast(const Json::Value & event);

		private:

			static std::mutex & mutex();
			static std::unordered_set<drogon::WebSocketConnectionPtr> & connections();
	};
}
