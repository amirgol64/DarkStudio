// DarkStudio server - entry point: REST API, WebSocket events, and the web UI.
// SPDX-License-Identifier: Apache-2.0
//
// Usage: darkstudio-server [--root <DarkStudio folder>] [--port 8765] [--open]
//
// The server listens on 127.0.0.1 only: v0 has no user accounts yet (those come with the multi-user mode, M5).

#include "datasets.hpp"
#include "events.hpp"
#include "jobs.hpp"
#include "settings.hpp"
#include "system_info.hpp"

#include <drogon/drogon.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

using namespace drogon;
namespace fs = std::filesystem;

namespace
{
	constexpr const char * kVersion = "0.1.0";

	using Callback = std::function<void(const HttpResponsePtr &)>;

	HttpResponsePtr json_response(const Json::Value & body, HttpStatusCode code = k200OK)
	{
		auto resp = HttpResponse::newHttpJsonResponse(body);
		resp->setStatusCode(code);
		return resp;
	}

	HttpResponsePtr error_response(HttpStatusCode code, const std::string & message)
	{
		Json::Value j;
		j["error"] = message;
		return json_response(j, code);
	}

	/// Run @p fn and turn exceptions into JSON errors: invalid input -> 400, busy/conflict -> 409, anything else -> 500.
	void guarded(const Callback & callback, const std::function<HttpResponsePtr()> & fn)
	{
		try
		{
			callback(fn());
		}
		catch (const std::invalid_argument & e)
		{
			callback(error_response(k400BadRequest, e.what()));
		}
		catch (const std::runtime_error & e)
		{
			callback(error_response(k409Conflict, e.what()));
		}
		catch (const std::exception & e)
		{
			callback(error_response(k500InternalServerError, e.what()));
		}
	}

	Json::Value body_json(const HttpRequestPtr & req)
	{
		const auto json = req->getJsonObject();
		if (not json)
		{
			throw std::invalid_argument("the request body must be JSON");
		}
		return *json;
	}

	struct Arguments
	{
		fs::path root;
		uint16_t port = 8765;
		bool open_browser = false;
	};

	Arguments parse_arguments(int argc, char * argv[])
	{
		Arguments a;
		for (int i = 1; i < argc; i ++)
		{
			const std::string arg = argv[i];
			if (arg == "--root" and i + 1 < argc)
			{
				a.root = argv[++ i];
			}
			else if (arg == "--port" and i + 1 < argc)
			{
				a.port = static_cast<uint16_t>(std::stoi(argv[++ i]));
			}
			else if (arg == "--open")
			{
				a.open_browser = true;
			}
			else if (arg == "--help" or arg == "-h")
			{
				std::cout << "Usage: darkstudio-server [--root <DarkStudio folder>] [--port 8765] [--open]" << std::endl;
				std::exit(0);
			}
			else
			{
				throw std::invalid_argument("unknown argument: " + arg);
			}
		}
		if (a.root.empty())
		{
			a.root = darkstudio::find_root(fs::current_path());
		}
		return a;
	}
}


int main(int argc, char * argv[])
{
	try
	{
		const Arguments args = parse_arguments(argc, argv);

		auto settings	= std::make_shared<darkstudio::SettingsStore>(args.root);
		auto jobs		= std::make_shared<darkstudio::JobManager>(*settings);
		auto system		= std::make_shared<darkstudio::SystemInfo>(*settings);
		const auto s	= settings->get();

		std::cout
			<< "DarkStudio server " << kVersion << std::endl
			<< "  root:     " << s.root.string() << std::endl
			<< "  settings: " << settings->file().string() << std::endl
			<< "  web UI:   " << s.web_dir.string() << (fs::exists(s.web_dir / "index.html") ? "" : "  (not built yet: cd web && npm run build)") << std::endl
			<< "  open:     http://localhost:" << args.port << "/" << std::endl;

		// --- system ---
		app().registerHandler("/api/health", [](const HttpRequestPtr &, Callback && cb)
		{
			Json::Value j;
			j["ok"] = true;
			j["version"] = kVersion;
			cb(json_response(j));
		}, {Get});

		app().registerHandler("/api/system", [system](const HttpRequestPtr &, Callback && cb)
		{
			cb(json_response(system->get()));
		}, {Get});

		app().registerHandler("/api/system/refresh", [system](const HttpRequestPtr &, Callback && cb)
		{
			system->refresh();
			cb(json_response(system->get(), k202Accepted));
		}, {Post});

		// --- settings ---
		app().registerHandler("/api/settings", [settings, system](const HttpRequestPtr & req, Callback && cb)
		{
			guarded(cb, [&]
			{
				if (req->method() == Get)
				{
					return json_response(settings->get().to_json());
				}
				const auto updated = settings->update(body_json(req));
				system->refresh();	// paths may have changed
				return json_response(updated.to_json());
			});
		}, {Get, Put});

		// --- jobs ---
		app().registerHandler("/api/jobs", [jobs](const HttpRequestPtr & req, Callback && cb)
		{
			guarded(cb, [&]
			{
				if (req->method() == Get)
				{
					return json_response(jobs->list());
				}
				const auto body = body_json(req);
				return json_response(jobs->start(body["kind"].asString(), body["params"]), k201Created);
			});
		}, {Get, Post});

		app().registerHandler("/api/jobs/{1}", [jobs](const HttpRequestPtr &, Callback && cb, const std::string & id)
		{
			const auto job = jobs->get(id);
			cb(job.isNull() ? error_response(k404NotFound, "no such job") : json_response(job));
		}, {Get});

		app().registerHandler("/api/jobs/{1}/log", [jobs](const HttpRequestPtr & req, Callback && cb, const std::string & id)
		{
			size_t since = 0;
			try { since = std::stoull(req->getParameter("since")); } catch (...) {}
			const auto log = jobs->log(id, since);
			cb(log.isNull() ? error_response(k404NotFound, "no such job") : json_response(log));
		}, {Get});

		app().registerHandler("/api/jobs/{1}/metrics", [jobs](const HttpRequestPtr &, Callback && cb, const std::string & id)
		{
			const auto m = jobs->metrics(id);
			cb(m.isNull() ? error_response(k404NotFound, "no such job") : json_response(m));
		}, {Get});

		app().registerHandler("/api/jobs/{1}/stop", [jobs](const HttpRequestPtr &, Callback && cb, const std::string & id)
		{
			cb(jobs->stop(id) ? json_response(jobs->get(id), k202Accepted) : error_response(k404NotFound, "no such job"));
		}, {Post});

		// annotated images written by a benchmark job (<job dir>/images/<name>)
		app().registerHandler("/api/jobs/{1}/image", [jobs](const HttpRequestPtr & req, Callback && cb, const std::string & id)
		{
			guarded(cb, [&]
			{
				const auto job = jobs->get(id);
				if (job.isNull())
				{
					return error_response(k404NotFound, "no such job");
				}
				const std::string dir = job["dir"].asString();
				const fs::path file = darkstudio::datasets::resolve(fs::path(std::u8string(dir.begin(), dir.end())) / "images", req->getParameter("name"));
				if (not fs::is_regular_file(file))
				{
					return error_response(k404NotFound, "no such image");
				}
				return HttpResponse::newFileResponse(file.string());
			});
		}, {Get});

		// --- datasets / annotation ---
		app().registerHandler("/api/datasets", [settings](const HttpRequestPtr &, Callback && cb)
		{
			guarded(cb, [&] { return json_response(darkstudio::datasets::list(settings->get().datasets_dir)); });
		}, {Get});

		app().registerHandler("/api/datasets/images", [settings](const HttpRequestPtr & req, Callback && cb)
		{
			guarded(cb, [&] { return json_response(darkstudio::datasets::images(settings->get().datasets_dir, req->getParameter("folder"))); });
		}, {Get});

		app().registerHandler("/api/datasets/file", [settings](const HttpRequestPtr & req, Callback && cb)
		{
			guarded(cb, [&]
			{
				const fs::path file = darkstudio::datasets::resolve(settings->get().datasets_dir, req->getParameter("path"));
				if (not fs::is_regular_file(file) or not darkstudio::datasets::is_image(file))
				{
					return error_response(k404NotFound, "no such image");
				}
				auto resp = HttpResponse::newFileResponse(file.string());
				resp->addHeader("Cache-Control", "max-age=3600");
				return resp;
			});
		}, {Get});

		app().registerHandler("/api/datasets/labels", [settings](const HttpRequestPtr & req, Callback && cb)
		{
			guarded(cb, [&]
			{
				const auto root = settings->get().datasets_dir;
				const std::string image = req->getParameter("image");
				if (req->method() == Get)
				{
					return json_response(darkstudio::datasets::read_labels(root, image));
				}
				return json_response(darkstudio::datasets::write_labels(root, image, body_json(req)["boxes"]));
			});
		}, {Get, Put});

		// --- web UI (built with "npm run build" in web/) ---
		app().setDocumentRoot(s.web_dir.string());
		app().setHomePage("index.html");

		app().addListener("127.0.0.1", args.port);
		app().setThreadNum(4);
		app().setClientMaxBodySize(16 * 1024 * 1024);
		app().setLogLevel(trantor::Logger::kWarn);

		system->refresh();

		if (args.open_browser)
		{
#ifdef _WIN32
			const std::wstring url = L"http://localhost:" + std::to_wstring(args.port) + L"/";
			ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
		}

		app().run();
		return 0;
	}
	catch (const std::exception & e)
	{
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}
