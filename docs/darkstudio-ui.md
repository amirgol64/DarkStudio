# DarkStudio UI v0 (M0c)

The first slice of the DarkStudio app: a local C++ server plus a web UI. Use it to see what works on this machine, run benchmarks and training, follow logs and errors, and label images. It's the foundation for M1. The labeling canvas, the job runner and the live charts carry over.

| View | What it shows |
|---|---|
| **Devices** | CPU, RAM, and each backend: Darknet CPU, Darknet SYCL (Intel GPU), OpenVINO devices. Versions, availability, probe output and errors, plus the files the jobs need |
| **Benchmarks** | Run the OpenVINO benchmark (GPU/CPU, FP16/FP32) or the Darknet CPU-vs-SYCL check. Results table, ms-per-image chart, per-image detections, annotated images |
| **Training** | Prepare LEGO Gears, start or stop training on the Iris Xe or the CPU, a **live loss / average loss / mAP chart**, seconds per iteration, time remaining, one-click mAP evaluation |
| **Logs & errors** | Every job's output, live. Errors and warnings are highlighted and filterable, with search |
| **Annotation** | YOLO boxes on images in `datasets/`. Draw, move, resize, delete and change class. It saves Darknet `.txt` labels next to the images |
| **Settings** | Tool paths, default OpenVINO device and precision, the OpenVINO model cache, theme (system/light/dark), language (English / Hebrew RTL) |

## Run it

```bat
darkstudio.bat            :: builds the server and the UI the first time, then opens http://localhost:8765/
darkstudio.bat --rebuild  :: rebuild both
```

Prerequisites: everything from [build-windows.md](build-windows.md) and [intel-gpu.md](intel-gpu.md) (vcpkg, the Darknet CPU and SYCL builds, OpenVINO, `ov-bench`), plus **Drogon** from vcpkg and **Node.js** for the UI build:

```powershell
C:\src\vcpkg\vcpkg.exe install "drogon[core,orm,sqlite3]:x64-windows"   # ~20 min, mostly OpenSSL
```

### Development

Run the C++ server, then the Vite dev server with hot reload. It forwards `/api` and `/ws` to the C++ server:

```powershell
server\build\Release\darkstudio-server.exe --root C:\dev\Python_dev\draknet
cd web; npm run dev          # http://localhost:5173/
```

Build and test:

```powershell
cmake -S server -B server/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build server/build --config Release
server\build\Release\darkstudio-tests.exe      # 65 checks: parsers (real tool output) + dataset path safety
cd web; npm run lint; npm run build
```

## Architecture

```
Browser (React + TS, Vite)  ──REST /api/*──►  darkstudio-server (C++20, Drogon, 127.0.0.1:8765)
        ▲                                        │  serves web/dist, settings, jobs, datasets
        └──────── WebSocket /ws/events ◄─────────┤  job runner ──► darknet.exe (CPU | SYCL)
                  job / log / metric / system    │                 ov-bench.exe (OpenVINO)
                                                 │                 compare-cpu-sycl.ps1, prepare-legogears.ps1
                                                 └► build/darkstudio/{settings.json, jobs/<id>/{job.json, log.txt, …}}
```

| Server module (`server/src/`) | Role |
|---|---|
| `main.cpp` | Routes, static files, startup |
| `settings.*` | Paths and preferences, stored in `build/darkstudio/settings.json` |
| `process.*` | Child processes with line-by-line output and stdin. A Windows Job Object (or a POSIX process group) lets **Stop** end the whole process tree |
| `jobs.*` | One job at a time (benchmarks and training would distort each other). Each job's folder holds `job.json`, `log.txt`, weights and images, and the history is reloaded at startup |
| `parsers.*` | Turn Darknet, ov-bench and compare output into JSON (training iterations, mAP, benchmark numbers). Log lines are classified as info, warning or error |
| `system_info.*` | Probes `darknet --version` (CPU and SYCL) and `ov-bench --list-devices`, plus CPU and RAM |
| `datasets.*` | Lists folders and images, and reads/writes YOLO labels. **Every path is resolved and must stay inside `datasets/`** |
| `events.*` | WebSocket broadcast |

### REST API

| Method + path | Purpose |
|---|---|
| `GET /api/health` | `{ok, version}` |
| `GET /api/system`, `POST /api/system/refresh` | Device and backend probe (cached / re-run) |
| `GET /api/settings`, `PUT /api/settings` | Read and update settings (partial JSON) |
| `GET /api/jobs`, `POST /api/jobs` `{kind, params}` | List / start. Returns **409** while another job runs and **400** for invalid parameters |
| `GET /api/jobs/{id}`, `GET …/log?since=N`, `GET …/metrics`, `POST …/stop`, `GET …/image?name=` | One job: summary, log lines, training metrics, stop, annotated benchmark image |
| `GET /api/datasets`, `GET …/images?folder=`, `GET …/file?path=`, `GET/PUT …/labels?image=` | Annotation |

Job kinds: `ov-bench` {device, precision, iterations} · `compare-cpu-sycl` {repeat} · `prepare-legogears` {max_batches} · `train` {backend: sycl|cpu, data, cfg, weights?, map} · `map` {backend, data, cfg, weights, iou}.

WebSocket `/ws/events` messages: `{"type":"job","job":{…}}`, `{"type":"log","job_id","n","line","level"}`, `{"type":"metric","job_id","metric":{…}}`, `{"type":"system","system":{…}}`.

## Security (v0)

- The server listens on **127.0.0.1 only** and has **no login**. Accounts and roles come with multi-user mode (M5).
- Dataset and image APIs can't leave the `datasets/` folder (`../` is rejected; there's a unit test for it).
- Jobs run only the configured tools with validated parameters. There's no arbitrary command execution.

## Licenses of the UI dependencies

All are MIT: React, React Router, i18next / react-i18next, Recharts, Konva / react-konva, Vite. Drogon is MIT, jsoncpp MIT, OpenSSL Apache-2.0, SQLite public domain.

## Tested (2026-10-09, i5-1135G7 + Iris Xe)

- All 3 backends detected. The required-files check passes.
- OpenVINO GPU FP16 job through the API: **10.5 ms per image (95 FPS)**, the same detections as before. The UI received 54 live events (8 job updates and 46 log lines).
- Training through the API: prepare (30 iterations), then SYCL training, with 30 live loss points plus mAP streamed to the UI. All metrics are stored and served by `/metrics`.
- A second job while busy returns 409. Invalid parameters return 400. Job history survives a server restart.
- Screenshots of every page in dark theme, and Hebrew RTL, checked with headless Edge.

## Known limitations / next

- **Job titles are generated by the server in English** (the UI text itself is translated).
- The **annotation view handles boxes only**. Polygons, the SAM2 assist and review states come in M1/M4.
- Settings and job history are JSON files. SQLite (already in the Drogon build) arrives with M1's projects and datasets.
- Single user and localhost only (see Security).
- There's no GPU memory or utilization graph yet. Level Zero Sysman could provide it.
