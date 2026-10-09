# DarkStudio — Project Plan

DarkStudio is a free, open-source toolchain for computer vision. It brings labeling, training, inference and export together in one web app, built on Hank.ai **Darknet**, **DarkHelp**, and ideas from **DarkMark**.

- **Owner:** solo developer, full-time (roughly a 14-month roadmap after moving Intel GPU work up to M0a/M0b)
- **Workspace:** `C:\dev\Python_dev\draknet`
- **Upstream clones** (each is a fork that we rebase regularly):

| Folder | Upstream | License | Role in DarkStudio |
|---|---|---|---|
| `darknet/` | github.com/hank-ai/darknet | Apache-2.0 | Native training and inference engine; we add a segmentation head |
| `DarkHelp/` | codeberg.org/CCodeRun/DarkHelp | MIT | Inference API; we add a pluggable ONNX Runtime backend |
| `DarkMark/` | github.com/stephanecharette/DarkMark | GPL-3 (JUCE) | **Reference only.** We copy no code from it, and the new UI is written from scratch |

---

## 1. Decisions (from the Q&A)

| # | Topic | Decision |
|---|---|---|
| 1 | Goal | One unified product covering label → train → infer → export |
| 2 | License strategy | **Permissive core (Apache-2.0/MIT).** Models that are AGPL or non-commercial are only optional plugins, they show a warning, and we never bundle them |
| 3 | Segmentation | Instance: RTMDet-Ins (core) and YOLO-seg (AGPL plugin). Semantic: DeepLabV3 (core) and SegFormer (non-commercial plugin). Promptable: SAM2 / MobileSAM (core) |
| 4 | Model families | Darknet native, the DETR family (RT-DETR / RF-DETR / D-FINE), YOLOX / RTMDet, and Ultralytics YOLOv8–v11 (plugin) |
| 5 | Purpose badges | Every model shows its task (OD / Seg / Cls / Pose / OBB / Track) and a license badge |
| 6 | Inference backend | ONNX Runtime added as a pluggable backend inside DarkHelp, next to Darknet |
| 7 | Training | Native C++. Phase 1 adds an instance-segmentation head and mask loss to Darknet |
| 8 | Tasks | Detection, segmentation, classification, pose, OBB, tracking |
| 9 | Platforms | Windows, Linux, macOS, Jetson/ARM |
| 10 | Accelerators | **Intel GPU and CPU (OpenVINO, oneDNN, SYCL)** first, then CUDA/cuDNN/TensorRT, ROCm, DirectML and CoreML/Metal |
| 11 | UI stack | Web UI written from scratch: **React + TypeScript + Vite + Konva**, Tailwind/shadcn |
| 12 | Old DarkMark | Kept only as a reference. No code is copied, so the new UI stays Apache-2.0 |
| 13 | API | REST for CRUD and WebSocket for live streams (training metrics, inference) |
| 14 | Backend framework | **Drogon** (C++, MIT) |
| 15 | Deployment | Local single-user (localhost) and a multi-user server with accounts |
| 16 | Auth | Local accounts with JWT. Roles are admin, annotator and reviewer |
| 17 | Storage | SQLite for local use, PostgreSQL for multi-user (same schema). Images stay on disk or in object storage |
| 18 | UI features | Polygon/mask tools, training dashboard, model-zoo browser, modern theme and UX |
| 19 | Dataset formats | Import and export Darknet/YOLO txt (plus YOLO-seg polygons), COCO JSON, Pascal VOC, CVAT, Label Studio and LabelMe |
| 20 | Assisted labeling | SAM2 click-to-mask, pre-annotation with the trained model, video propagation, active learning |
| 21 | Optimization | Inference (FP16/INT8, TensorRT), training speed, UI responsiveness, edge footprint |
| 22 | Export targets | ONNX, TensorRT, OpenVINO IR, CoreML, TFLite |
| 23 | Upstream | Forks with regular rebases; we send fixes back upstream |
| 24 | Quality | Full CI matrix: Win/Linux/macOS builds, unit tests, an mAP regression check, Playwright E2E |
| 25 | MVP | Web labeling (boxes and polygons) → Darknet detection training → inference through DarkHelp |
| 26 | i18n | react-i18next, English plus Hebrew (full RTL) |
| 27 | Name | **DarkStudio** |
| 28 | Dev hardware | Windows 11, **Intel Iris Xe iGPU** (i5-1135G7, 32 GB), **no CUDA**. Intel GPU support is a priority (see §3.3) |
| 29 | Language | **C++ first** (C++20) for the engine, backend and tools. TypeScript only for the browser UI, with C++ compiled to WebAssembly for heavy client-side work. Python only for one-time model conversion |
| 30 | Hosting | Everything on GitHub (`amirgol64/*`). Forks sync from Codeberg `CCodeRun/*` as `upstream` |
| 31 | CPU math on Windows | **No OpenBLAS.** Darknet's built-in AVX2 + OpenMP GEMM measured ~20× faster than vcpkg's MSVC-built OpenBLAS. CPU baseline for yolov4-tiny: ~160–180 ms per image |
| 32 | Intel GPU inference | **OpenVINO 2026.4.1** (C++ runtime, `C:\src\openvino`). Iris Xe at FP16 is the default device, with the model cache always on. yolov4-tiny runs at 11.8 ms (85 FPS) |
| 33 | Intel GPU training toolchain | **oneAPI 2026.1** (icx, oneMKL, oneDNN) on Windows. **SYCLomatic in WSL Ubuntu** (CUDA 12.9 headers, no root). SYCL becomes a 3rd Darknet GPU backend (`DARKNET_GPU_SYCL`) without cuDNN, like ROCm |
| 34 | UI v0 | Built as the real app (Drogon server + React UI), not a throwaway dashboard. Local only (127.0.0.1, no login until M5), one job at a time, job history as JSON files (SQLite with M1 projects), hash routing |

---

## 2. Licensing policy

### 2.1 License cheat-sheet

| License | Personal | Commercial | Must you release source? |
|---|---|---|---|
| MIT / BSD | ✅ | ✅ | No (keep the notice) |
| Apache-2.0 | ✅ | ✅ | No (keep the notice, state your changes; includes a patent grant) |
| LGPL-3 | ✅ | ✅ | Only for changes to the library itself |
| GPL-3 | ✅ | ✅ | Yes, for the whole program, if you distribute it |
| AGPL-3 | ✅ | ✅ or buy a commercial license | Yes, **even if you only offer it as a network service** |
| Non-commercial (CC-BY-NC, NVIDIA SCL) | ✅ | ❌ | n/a. This is not open source |

### 2.2 Rules

1. **Core** (backend, web UI, the Darknet and DarkHelp changes) is **Apache-2.0**. It may only depend on Apache, MIT, BSD, zlib, or (dynamically linked) LGPL code.
2. **No DarkMark/JUCE code goes into the core.** Reading it for ideas is fine. Copying it is not.
3. **Plugins** (Ultralytics YOLO, SegFormer, YOLO-NAS, and similar): the user downloads them through the model zoo. Before download a modal shows the license and states whether commercial use is allowed. The plugin is tagged with its license in the project metadata, and exported models inherit a warning.
4. A CI job runs a license scanner (e.g. `reuse lint` plus `scancode` or `licensee`) and fails the build if the core picks up a GPL, AGPL or non-commercial dependency.
5. Write `THIRD_PARTY_LICENSES.md`, generated automatically.

### 2.3 Model registry (shown in the UI)

| Model | Tasks | License | Commercial OK | Tier |
|---|---|---|---|---|
| Darknet YOLOv4/v7 (+ tiny), Hank.ai | OD, Cls, (Seg in phase 3) | Apache-2.0 | ✅ | Core |
| YOLOX | OD | Apache-2.0 | ✅ | Core |
| RTMDet / RTMDet-Ins (MMDetection) | OD, Instance Seg | Apache-2.0 | ✅ | Core |
| RTMPose | Pose | Apache-2.0 | ✅ | Core |
| RT-DETR / RF-DETR / D-FINE / DEIM | OD | Apache-2.0 | ✅ | Core |
| DeepLabV3 (torchvision) | Semantic Seg | BSD-3 | ✅ | Core |
| SAM / SAM2 / MobileSAM / EfficientSAM | Promptable Seg, video | Apache-2.0 | ✅ | Core (labeling) |
| ByteTrack / OC-SORT | Tracking | MIT | ✅ | Core |
| Ultralytics YOLOv8–v11 | OD, Seg, Cls, Pose, OBB | AGPL-3.0 | ⚠️ AGPL or paid license | Plugin |
| SegFormer (NVIDIA weights) | Semantic Seg | NVIDIA SCL (NC) | ❌ | Plugin |
| YOLO-NAS weights | OD | Non-commercial | ❌ | Plugin |

> Check each license again before release. Licenses change.

---

## 3. Architecture

```
┌──────────────────────── Browser ────────────────────────┐
│ React + TS + Vite · Konva canvas · Tailwind/shadcn       │
│ Labeling │ Datasets │ Training dashboard │ Model zoo │ … │
└───────────────▲──────────────────────▲──────────────────┘
          REST (JSON)            WebSocket (metrics, progress, frames)
┌───────────────┴──────────────────────┴──────────────────┐
│ darkstudio-server (C++20, Drogon)                       │
│  auth/JWT · projects · datasets · jobs · model registry │
│  ORM → SQLite (local) / PostgreSQL (multi-user)         │
│  Job runner (training/inference/export workers)         │
└───────┬──────────────────────┬──────────────────────────┘
        │                      │
┌───────▼────────┐   ┌─────────▼──────────────────────────┐
│ Darknet (fork) │   │ DarkHelp (fork) – unified inference│
│ train/infer    │   │  IBackend: Darknet | ONNX Runtime  │
│ + seg head     │   │  EPs: OpenVINO/DirectML/CUDA/TRT/  │
│ + ONNX export  │   │       CoreML/ROCm/CPU              │
│ + SYCL (Intel) │   │                                    │
└────────────────┘   │  Task heads: det/seg/cls/pose/obb  │
                     └────────────────────────────────────┘
```

### 3.1 Repository layout (target)

```
DarkStudio/  (C:\dev\Python_dev\draknet)
├── CLAUDE.md           # project rules: keep plan/README/docs in sync, C++ first
├── darknet/            # submodule → amirgol64/darknet  (Apache-2.0)
├── DarkHelp/           # submodule → amirgol64/DarkHelp (MIT)
├── DarkMark/           # submodule → amirgol64/DarkMark (GPL-3, reference only)
├── server/             # Drogon backend (Apache-2.0)
│   ├── src/{api,auth,db,jobs,formats,registry}
│   └── migrations/
├── web/                # React frontend (Apache-2.0)
│   └── src/{features/{label,datasets,train,zoo,review},components,i18n}
├── models/registry.yaml  # model metadata: task, license, URLs, pre/post-proc
├── models/pretrained/  # downloaded weights/cfg/onnx (git-ignored)
├── tools/              # C++ CLI tools
│   ├── ov-bench/       # OpenVINO runner and benchmark for Darknet ONNX models (M0a)
│   ├── sycl-check/     # SYCL device info + oneMKL SGEMM benchmark (M0b)
│   ├── train-test/     # dataset prep for CPU vs Intel GPU training comparisons (M0b)
│   └── sycl-migrate/   # SYCLomatic setup and migration scripts, run in WSL (M0b)
├── docs/
└── .github/workflows/
```

### 3.2 Key internal interfaces

- **DarkHelp `IBackend`**: `load(model_spec)`, `predict(batch) -> Results`. `Results` holds boxes, masks (RLE or polygon), class scores, keypoints and rotated boxes.
- **`ModelSpec`** (from `registry.yaml`): input size, normalization, letterbox, output decoder (`yolo_v8`, `rtdetr`, `rtmdet_ins`, `deeplab`, `sam2_encoder/decoder`, …), task, license.
- **Annotation model** (DB): `Image`, `Annotation{type: box|polygon|mask_rle|keypoints|obb|class, class_id, data, source: human|model|sam, confidence, reviewed}`.
- **Format adapters** (`server/src/formats/`): one importer and one exporter per format, sharing a canonical internal model.

### 3.3 Intel GPU strategy (Iris Xe / Arc)

Upstream Darknet only accelerates on NVIDIA CUDA and AMD ROCm. On Intel hardware it runs on the CPU. The plan:

| Workload | Intel path | License | When |
|---|---|---|---|
| **Inference: Darknet models** | Darknet → ONNX (`src-onnx`) → **OpenVINO** (GPU plugin on Iris Xe, CPU plugin as fallback) | Apache-2.0 | ✅ **M0a**: 85 FPS on Iris Xe (FP16) |
| **Inference: all other models** | DarkHelp `IBackend` → **ONNX Runtime + OpenVINO EP** (alternatively DirectML EP) | MIT / Apache-2.0 | M2 |
| **Training: CPU** | Darknet CPU build with **AVX2 + OpenMP** (built-in GEMM, ~20× faster than vcpkg OpenBLAS on Windows), later oneDNN for conv/GEMM | Apache-2.0 | ✅ builds (M0) |
| **Training: Intel GPU** | Port Darknet's CUDA kernels to **SYCL (oneAPI DPC++)**: use **SYCLomatic** for the first draft, then use oneMKL for GEMM and oneDNN for conv. Adds a `DARKNET_TRY_SYCL` CMake option alongside CUDA/ROCm | Apache-2.0 (w/ LLVM exception) | **M0b**: inference ✅ (identical to CPU, 6.5× faster), training ✅ (~27× faster than CPU) |

What to expect on Iris Xe (~2 TFLOPS FP32, shared memory): inference on yolov4-tiny-class models should be real-time through OpenVINO FP16. Training on the iGPU will be several times faster than CPU but far slower than a discrete NVIDIA GPU, so for small datasets and tiny models it's practical. Big training runs will still want a CUDA/ROCm machine or the multi-user GPU server (M5).

### 3.4 C++ first

| Part | Language | Notes |
|---|---|---|
| Darknet, DarkHelp, backends, decoders | C++20 (CUDA/HIP/SYCL kernels) | Already C++ upstream |
| Server (REST, WebSocket, jobs, DB, formats) | C++20 + Drogon | No Python in the runtime path |
| CLI tools (dataset convert, benchmark, export) | C++20 | |
| Browser UI | TypeScript + React | A browser can only run JS/WASM |
| Heavy client ops (polygon simplify, mask RLE, tiling) | C++ → **WebAssembly** (Emscripten) | Shares code with the server |
| Model conversion (PyTorch → ONNX) | Python, offline, one-time | Users can skip this by downloading pre-exported ONNX from the model zoo |
| Python bindings | Optional | DarkHelp `src-python`, for scripting only |

---

## 4. Roadmap

Milestones assume one developer working full time. Each one ends with a release you can demo.

### M0 — Foundations (weeks 1–3)
- [x] Clone darknet, DarkHelp and DarkMark.
- [x] Fork the 3 repos to GitHub (`amirgol64/*`). Set `origin` to the fork and `upstream` to Codeberg (fetch-only).
- [x] Create the `DarkStudio` repo with the 3 forks as submodules, and push the first commit.
- [x] Write `README.md` with the layout, cloning steps and the upstream-sync procedure.
- [x] Write `CLAUDE.md` with project rules (docs-sync, C++ first, license gate, Intel hardware).
- [x] Install vcpkg at `C:\src\vcpkg`.
- [x] Use vcpkg to build OpenCV 4.14 (with DirectML), protobuf, TCLAP and OpenBLAS (x64-windows). We recovered from a corrupted `utf8-range` header left by an interrupted build.
- [x] Build Darknet on Windows, CPU-only with AVX2 + OpenMP + ONNX export (no CUDA on the dev PC).
- [x] Fix in the darknet fork: generate the ONNX protobuf header before `darknetobjlib` compiles (parallel-build race, C1083).
- [x] Build DarkHelp on Windows against Darknet, installed to `build/install`.
- [x] Fix in the DarkHelp fork: install runtime DLLs on Windows without copying `darknet.dll` by hand.
- [x] Smoke test: pretrained yolov4-tiny on the Darknet sample images (CPU). The detections are correct.
- [x] Benchmark CPU: vcpkg OpenBLAS was ~2,600 ms per image against **~130–180 ms** with Darknet's built-in AVX2 GEMM. **Decided:** build with `-DDARKNET_TRY_OPENBLAS=OFF` on Windows.
- [x] Write `docs/build-windows.md` with the exact steps, pitfalls and performance notes.
- [ ] Offer both build fixes upstream (Codeberg PRs), and report the OpenBLAS slowdown to the Darknet docs.
- [ ] Upstream fix: make `CM_version.cmake` run `git describe` in `${CMAKE_CURRENT_SOURCE_DIR}` (today it fails unless CMake is started from inside `darknet/build`).
- [ ] Remove OpenBLAS from vcpkg on the dev PC (it's unused now) and drop it from the docs' dependency list in CI.
- [ ] Linux build steps (`docs/build-linux.md`).
- [ ] Create the rest of the monorepo skeleton (`server/`, `web/`, CMake superbuild, vcpkg manifest).
- [ ] CI: build matrix (Windows, Ubuntu, macOS), license scanner, clang-format/clang-tidy, ESLint/Prettier.
- [x] Add `LICENSE` (Apache-2.0).
- [ ] Write `THIRD_PARTY_LICENSES.md`, `CONTRIBUTING.md` and `CODE_OF_CONDUCT.md`.

### M0a — Intel GPU inference with OpenVINO (week 4)
- [x] Install the OpenVINO 2026.4.1 C++ runtime (Apache-2.0, prebuilt archive, SHA256 verified) at `C:\src\openvino`, including the GPU plugin for the Iris Xe. Documented in `docs/intel-gpu.md`.
- [x] Export yolov4-tiny to ONNX with `darknet_onnx_export`. The output format (normalized x1,y1,x2,y2 boxes plus confs) was confirmed empirically and documented.
- [x] `tools/ov-bench` (C++20): loads ONNX in OpenVINO, runs on GPU/CPU at FP16/FP32, decodes the boxes with per-class NMS, clips them, times each stage, and saves annotated images.
- [x] Detections match Darknet on all 6 sample images (same objects, confidences within 1%).
- [x] Benchmark: **Iris Xe FP16 11.8 ms (85 FPS, ~13× Darknet CPU)**, Iris Xe FP32 18.5 ms, OpenVINO CPU 80.5 ms. The model cache cuts the GPU compile from 12.6 s to 0.12 s. Results are in `docs/intel-gpu.md`.

### M0b — Darknet training on Intel GPU via SYCL (weeks 5–12, moved up from "M3b")
Moved up on 2026-10-09: the dev PC has only an Iris Xe, so GPU training on it speeds up all later work. Expect ~3–5× faster than CPU for tiny models.
- [x] Install Intel oneAPI 2026.1 (DPC++/C++ compiler, oneMKL, oneDNN, TBB, oneDPL). `sycl-ls` sees the Iris Xe through Level Zero.
- [x] SYCLomatic isn't in oneAPI 2026 and has no Windows build, so set it up in WSL Ubuntu with CUDA 12.9 headers and a no-root sysroot (`tools/sycl-migrate/setup-wsl.sh`).
- [x] Audit Darknet's GPU code for FP64 use: no kernel declares a `double`, **but some use double literals** (e.g. `.001*x`, which promotes the math to FP64). Inference kernels run fine. See the follow-up task below.
- [x] `tools/sycl-check`: the Iris Xe has max work-group size 512 (= Darknet `BLOCK`) and sub-groups 8/16/32. **oneMKL SGEMM reaches 670–960 GFLOPS on yolov4-tiny shapes** (1,325 peak), ~4× the CPU.
- [x] Run SYCLomatic on `darknet/src-lib/*.cu` (`tools/sycl-migrate/migrate.sh`): 10 files, **0 errors**, 330 warnings, triaged in `docs/intel-gpu.md`.
- [ ] Review the 3 correctness warnings (DPCT1118/1121, group functions in divergent code in `im2col_kernels`).
- [x] Add a `DARKNET_TRY_SYCL` CMake option to the darknet fork (branch **`sycl`**). It requires icx/icpx + oneMKL and defines `DARKNET_GPU` + `DARKNET_GPU_SYCL`, with no cuDNN (like ROCm). It also has optional `DARKNET_SYCL_TARGETS` for AOT, IntelLLVM detection so AVX stays on, and leaves the oneAPI DLLs out of the install (Intel license).
- [x] Bring the migrated kernels into the fork as `src-lib/sycl/*.dp.cpp` (`tools/sycl-migrate/import.sh`): `dpct` → `dn_sycl::` helpers (no dpct dependency), `CHECK_CUDA(0)` → `cudaPeekAtLastError()`. `wmma` is off (`DN_SYCL_CUDA_ARCH=0`) and CUDA graphs are stubs.
- [x] Fix: 19 kernel lambdas captured the whole `Layer`/`NetworkState` (3.3 KB > the Intel GPU's 2 KB argument limit). They now capture only the fields they use.
- [x] Port the host side with a **CUDA-on-SYCL compatibility layer** (`darknet_sycl.hpp/.cpp`, ~40 functions). `dark_cuda.cpp` and all layers compile **unchanged**. Covers USM memory, in-order queues as streams, cuBLAS → oneMKL GEMM, cuRAND → oneMKL RNG, and exceptions → `cudaError_t`.
- [x] Upstream build fixes found along the way: SYCL headers must come before Darknet's global `node`/`list` types; `gemm.cpp` `__m256` indexing for clang; a `getopt.h` forward declaration; and the ONNX tool now respects `DARKNET_TRY_ONNX`.
- [x] Inference on the Iris Xe via SYCL gives **identical detections to the CPU** on all 6 sample images (0 class mismatches, 0.0000 probability difference, 0 px box difference) at **27.5 ms vs 180 ms per image (6.5×)**. Checked with `tools/sycl-migrate/compare-cpu-sycl.ps1`.
- [x] Make kernel literals single precision: the `-Wdouble-promotion` audit found **23 kernel sites**, all now `float` literals. Inference is still identical to the CPU.
- [x] AOT for Iris Xe: **not possible with oneAPI 2026** (`ocloc` dropped `tgllp`). Not needed either: the driver's kernel cache gives a 0.7 s process start after the one-time ~1.7 s JIT. AOT is documented for Arc (`intel_gpu_acm_g10`).
- [x] Training smoke test (LEGO Gears, 20 iterations, batch 64): **Iris Xe 1.1 s/iteration vs CPU 30 s/iteration (~27×)**, with no errors, NaNs or FP64 issues. `tools/train-test/prepare-legogears.ps1` prepares the data.
- [ ] Build DarkHelp against the SYCL Darknet, so DarkHelp/DarkStudio can use the Iris Xe directly.
- [ ] Later: cuDNN-style conv with oneDNN, and FP16 (the Iris Xe supports fp16).
- [x] Training check: LEGO Gears (yolov4-tiny-style network), **1,000 iterations on Iris Xe in 19.6 min**, final loss 0.09, **mAP@0.50 100% / mAP@0.75 84.6%**, checked on the CPU build (the author's 3,000-iteration weights: 100%/100%). CPU and GPU loss curves match over the 20-iteration smoke test. GPU-trained weights work on the CPU build.
- [x] Benchmark training on the Iris Xe against the CPU: **1.1 s vs 30 s per iteration (batch 64, 224×160), ~27×**. Documented in `docs/intel-gpu.md`.
- [ ] Offer it upstream to Hank.ai Darknet.

### M0c — DarkStudio UI v0: see functionality, performance and errors (after the first Iris Xe training run)
Decided 2026-10-09: build the first slice of the **real** app (Drogon C++ server + React/TypeScript UI) instead of a throwaway dashboard, so it becomes the M1 foundation. It starts once the M0b training test works, so there's real training to show.
- [x] `server/` (C++20, Drogon 1.9 via vcpkg, MIT): REST + WebSocket (`/ws/events`) and a job runner (Windows Job Object / POSIX process group, so stop kills the whole tree; one job at a time; per-job folder with `job.json` + `log.txt`; history reloaded at startup). Listens on 127.0.0.1 only. See `docs/darkstudio-ui.md`.
- [x] Output parsers for Darknet training/mAP/version, ov-bench, and compare-cpu-sycl, plus a log-level classifier. **65 unit tests** use real captured output and the dataset path-safety check (`darkstudio-tests`).
- [x] `ov-bench --list-devices` for the device probe.
- [x] `web/`: Vite + React 19 + TS app shell, HashRouter, dark/light/system theme, i18n **EN + HE with RTL**, and a live store fed by the WebSocket (auto-reconnect). All dependencies are MIT.
- [x] **Devices & backends** view: CPU/RAM, and Darknet CPU, Darknet SYCL (Iris Xe) and OpenVINO with versions, devices, errors and probe output, plus a required-files check.
- [x] **Benchmarks** view: OpenVINO (device/precision/iterations) and CPU-vs-SYCL jobs from the UI, a results table, a ms-per-image chart, per-image detections and annotated images.
- [x] **Live training** view: prepare dataset, start/stop on SYCL or CPU, a live loss / average loss / mAP chart, s/iteration, remaining time, and a one-click mAP evaluation. (GPU memory isn't shown yet; it needs Level Zero Sysman.)
- [x] **Logs & errors** view: all jobs, live log, errors and warnings highlighted, problems-only filter, search, follow. The sidebar badge counts failed jobs.
- [x] **Annotation** view (first version of the M1 canvas, Konva): YOLO boxes on any `datasets/` folder, draw/move/resize/delete, class keys 1–9, ←/→, Ctrl+S, and auto-save on image change. **Polygons are deferred to M1.**
- [x] **Settings** view: all tool paths, the default OpenVINO device and precision, the model cache, theme and language (saved on the server; browsers without their own choice follow it).
- [x] `darkstudio.bat`: one command that builds the server and UI if needed, then starts and opens the browser.
- [x] End-to-end test: API jobs (OpenVINO 10.5 ms/95 FPS on Iris Xe, SYCL training with live metrics), 409/400 handling, history after restart, and headless-Edge screenshots of every page (dark + Hebrew RTL).
- [ ] Translate the server-generated job titles (currently English).
- [ ] GPU memory and utilization in the training view (Level Zero Sysman).

### M1 — MVP: label → train → infer (weeks 13–21)
**Backend**
- [ ] Drogon server with SQLite. Migrations for users, projects, images, classes, annotations and jobs.
- [ ] Local JWT auth (a single admin account in local mode).
- [ ] Image ingest: upload a folder or a server path, generate thumbnails, store EXIF.
- [ ] Import and export Darknet/YOLO txt, YOLO-seg polygons and COCO JSON.
- [ ] Training job: generate `.cfg`, `.data` and `train.txt` (port DarkMark's ideas, not its code). Spawn Darknet, parse the log, stream loss/mAP over WebSocket, support stop and resume.
- [ ] Inference job through DarkHelp with the Darknet backend. Results can be saved as pre-annotations.
- [ ] Device picker for training and inference jobs: CPU, Intel GPU (SYCL / OpenVINO from M0a/M0b), and CUDA/ROCm when present.

**Frontend**
- [x] App shell with routing, dark/light theme, i18n (EN plus HE with RTL), and keyboard shortcuts (done in M0c).
- [ ] Labeling canvas (Konva): box and polygon tools, zoom/pan, class palette, undo/redo, auto-save.
- [ ] Image grid with virtualization (handles 100k+ images) and filters (unlabeled, class, reviewed).
- [ ] Training dashboard: live loss and mAP charts, GPU usage, ETA, and checkpoint list.
- [ ] Inference viewer: run the model on an image, a folder or a video, with a confidence slider.

**Exit criterion:** label 200 images in the browser, train a YOLOv4-tiny, and view predictions, all without touching the CLI.

### M2 — Multi-model inference via ONNX Runtime (weeks 22–27)
- [ ] Add `IBackend` to DarkHelp and keep the existing Darknet path working (send this upstream if the maintainer agrees).
- [ ] ONNX Runtime backend with execution providers **OpenVINO (Intel GPU/CPU/NPU) first**, then DirectML, CUDA, TensorRT, CoreML, ROCm and CPU.
- [ ] Decoders for YOLOX, RT-DETR/RF-DETR/D-FINE, RTMDet/RTMDet-Ins, DeepLabV3, RTMPose and YOLO-seg (plugin).
- [ ] `models/registry.yaml` and a **Model Zoo page** with task badges, license badges, a "commercial OK?" flag, and the warning modal for plugins.
- [ ] Render masks, keypoints and OBBs in the viewer.
- [ ] Python bindings (DarkHelp already has `src-python`) for scripting.

### M3 — Darknet native instance segmentation (weeks 28–39)
This is the biggest research item. Code lives in `darknet/src-lib/`.
- [ ] Design: YOLACT/YOLOv8-seg style. A prototype mask branch (`[proto]` layer) plus mask coefficients added to the `[yolo]` layer outputs.
- [ ] New layer `seg_proto_layer.cpp`, and `yolo_layer.cpp` extended with N mask coefficients per anchor.
- [ ] Mask loss (BCE plus dice on the cropped prototype combination). CPU, SYCL (Intel) and CUDA kernels.
- [ ] Data loader: read YOLO-seg polygons, rasterize to masks, and keep augmentation consistent (mosaic, flip, scale).
- [ ] cfg templates: `yolov4-tiny-seg.cfg` and `yolov7-tiny-seg.cfg`.
- [ ] Mask mAP evaluation (COCO-style) in `darknet detector map`.
- [ ] Extend `src-onnx` export to include the seg head, and add a DarkHelp decoder for it.
- [ ] Benchmark against RTMDet-Ins and YOLOv8-seg on a public dataset.
- [ ] Later: native classification UI flow, then OBB/pose heads.

### M4 — Assisted labeling (weeks 34–43, overlaps M3)
- [ ] SAM2 / MobileSAM: run the encoder once per image (server-side, cached) and run the decoder on click/box. Convert the mask to a simplified polygon.
- [ ] Pre-annotate the whole dataset with any model from the registry. Reviewers accept or reject.
- [ ] Video: frame extraction, SAM2 video propagation, ByteTrack/OC-SORT ID tracking.
- [ ] Active learning: rank unlabeled images by uncertainty (entropy, low margin) and diversity.
- [ ] Magic wand and brush/eraser for masks.

### M5 — Multi-user and collaboration (weeks 44–49)
- [ ] PostgreSQL backend (same ORM models), plus S3/MinIO storage as an option.
- [ ] Roles (admin, annotator, reviewer). Task assignment and a review queue with accept, reject and comment.
- [ ] Audit log and per-user stats.
- [ ] Docker Compose (server, Postgres, MinIO, GPU worker) and Helm as a later option.

### M6 — Export and optimization (weeks 50–57)
- [ ] Export: ONNX, TensorRT engine (FP16/INT8 with a calibration dataset), OpenVINO IR, CoreML, TFLite.
- [ ] Inference: batching, engine caching, async pipelines, a benchmarking page (FPS/latency per EP).
- [ ] Training: mixed precision in Darknet (FP16 where safe), faster data loader (threaded decode, cache), multi-GPU review.
- [ ] UI: tiled rendering for very large images, web workers for polygon ops, lazy thumbnails.
- [ ] Edge: a minimal build for Jetson/ARM (DarkHelp, TensorRT, a headless CLI).

### M7 — Formats, polish, release 1.0 (weeks 58–61)
- [ ] Pascal VOC, CVAT XML, Label Studio JSON and LabelMe import/export.
- [ ] Packaged installers for Windows (MSI) and Linux (.deb/AppImage), plus macOS as a stretch goal.
- [ ] Docs site, tutorials, sample datasets, and a demo video.

---

## 5. Testing and quality

| Layer | Tooling | What's covered |
|---|---|---|
| Darknet / DarkHelp | GoogleTest (upstream style), CTest | Layers, decoders, NMS, mask utils |
| Server | GoogleTest plus HTTP integration tests | API, auth, format round-trips |
| Regression | Small fixed dataset in CI | Train N iterations, assert loss drops and mAP stays above a threshold |
| Frontend | Vitest plus Playwright | Components, labeling flows, RTL layout |
| License | reuse / scancode | No GPL, AGPL or NC in the core |
| Perf | Benchmark job (nightly, self-hosted GPU runner) | FPS/latency per backend |

---

## 6. Risks

| Risk | Mitigation |
|---|---|
| The native seg head in Darknet is research-heavy | Start with the tiny models. ONNX seg models (M2) ship earlier, so users aren't blocked |
| Licenses accidentally get mixed | CI license gate, the clean-room rule for DarkMark, and a plugin-only policy for AGPL/NC |
| Upstream drift (Hank.ai Darknet moves fast) | Keep patches small and isolated, rebase monthly, contribute upstream |
| Too much work for one person | MVP first, then strict milestone scope. M5 and M7 can slip |
| Cross-platform GPU stack complexity | Lean on ONNX Runtime EPs and keep Darknet native training on CUDA/ROCm/CPU |
| The dev PC has no CUDA (Intel Iris Xe only) | CPU training plus OpenVINO inference from day one. The SYCL port (M0b) comes right after OpenVINO. CUDA features get tested in CI or on a rented GPU |
| The SYCL port is large and may diverge from upstream | Keep it behind `DARKNET_TRY_SYCL`, generate it mechanically with SYCLomatic, and offer it upstream |
| macOS / Apple GPU training | Inference only via CoreML. No native training on Metal in 1.0 |

---

## 7. Immediate next steps

1. ~~Create the forks and set the remotes.~~ Done.
2. ~~Build Darknet and DarkHelp on this Windows machine and record the steps in `docs/build-windows.md`.~~ Done. CPU baseline is ~160–180 ms per image.
3. ~~**M0a:** install OpenVINO and run the first Intel GPU inference.~~ Done: 85 FPS on Iris Xe, ~13× the Darknet CPU.
4. **M0b:** SYCL port of Darknet. **Inference on the Iris Xe is identical to the CPU (6.5× faster) and training works (~27× faster than CPU, mAP@0.50 100% on LEGO Gears).** Left: the DPCT1118 review, DarkHelp on SYCL, the training benchmark row, and the upstream offer.
5. ~~**M0c:** DarkStudio UI v0.~~ Done: `darkstudio.bat` opens it at http://localhost:8765/ (see `docs/darkstudio-ui.md`).
6. Set up CI with the build matrix and the license gate.
7. Continue the M1 labeling canvas from the M0c annotation view.
