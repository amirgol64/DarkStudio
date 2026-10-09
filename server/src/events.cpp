// DarkStudio server - WebSocket that pushes live events to the UI.
// SPDX-License-Identifier: Apache-2.0

#include "events.hpp"


namespace darkstudio
{
	std::mutex & EventsSocket::mutex()
	{
		static std::mutex m;
		return m;
	}


	std::unordered_set<drogon::WebSocketConnectionPtr> & EventsSocket::connections()
	{
		static std::unordered_set<drogon::WebSocketConnectionPtr> c;
		return c;
	}


	void EventsSocket::handleNewMessage(const drogon::WebSocketConnectionPtr & conn, std::string &&, const drogon::WebSocketMessageType & type)
	{
		// the UI only listens; answer pings so proxies keep the connection open
		if (type == drogon::WebSocketMessageType::Ping)
		{
			conn->send("", drogon::WebSocketMessageType::Pong);
		}
	}


	void EventsSocket::handleNewConnection(const drogon::HttpRequestPtr &, const drogon::WebSocketConnectionPtr & conn)
	{
		std::scoped_lock lock(mutex());
		connections().insert(conn);
	}


	void EventsSocket::handleConnectionClosed(const drogon::WebSocketConnectionPtr & conn)
	{
		std::scoped_lock lock(mutex());
		connections().erase(conn);
	}


	void EventsSocket::broadcast(const Json::Value & event)
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "";
		const std::string text = Json::writeString(builder, event);

		std::scoped_lock lock(mutex());
		for (const auto & conn : connections())
		{
			if (conn->connected())
			{
				conn->send(text);	// queued on the connection's event loop, so it is safe from worker threads
			}
		}
	}
}
