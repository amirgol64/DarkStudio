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
| **Training: Intel GPU** | Port Darknet's CUDA kernels to **SYCL (oneAPI DPC++)**: use **SYCLomatic** for the first draft, then use oneMKL for GEMM and oneDNN for conv. Adds a `DARKNET_TRY_SYCL` CMake option alongside CUDA/ROCm | Apache-2.0 (w/ LLVM exception) | **M0b** (moved up, weeks 5–12) |

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
- [x] Audit Darknet's GPU code for FP64 use: **no `double` in any kernel**. The Iris Xe (no FP64) is fine.
- [x] `tools/sycl-check`: the Iris Xe has max work-group size 512 (= Darknet `BLOCK`) and sub-groups 8/16/32. **oneMKL SGEMM reaches 670–960 GFLOPS on yolov4-tiny shapes** (1,325 peak), ~4× the CPU.
- [x] Run SYCLomatic on `darknet/src-lib/*.cu` (`tools/sycl-migrate/migrate.sh`): 10 files, **0 errors**, 330 warnings, triaged in `docs/intel-gpu.md`.
- [ ] Review the 3 correctness warnings (DPCT1118/1121, group functions in divergent code in `im2col_kernels`).
- [ ] Add a `DARKNET_TRY_SYCL` CMake option to the darknet fork: find icx/oneMKL and define `DARKNET_GPU` + `DARKNET_GPU_SYCL` (no cuDNN, like ROCm).
- [ ] Bring the migrated kernels into the fork as `src-lib/sycl/*.dp.cpp`, built only when `DARKNET_GPU_SYCL` is set. Disable `wmma` (XNOR Tensor Core) kernels and CUDA Graphs under SYCL.
- [ ] Port the host side (`dark_cuda.cpp`, `darknet_gpu.hpp`): device selection, memory, streams → SYCL queue, cuBLAS SGEMM → oneMKL, cuRAND → oneMKL RNG, error checks → exceptions.
- [ ] Later: cuDNN-style conv with oneDNN, and FP16 (the Iris Xe supports fp16).
- [ ] Inference on the Iris Xe via SYCL gives the same detections as the CPU on the sample images.
- [ ] Training check: train yolov4-tiny on a small dataset on CPU and on SYCL. The loss curve and mAP should match.
- [ ] Benchmark training (iterations per second) on the Iris Xe against the CPU. Document it in `docs/`.
- [ ] Offer it upstream to Hank.ai Darknet.

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
- [ ] App shell with routing, dark/light theme, i18n scaffold (EN plus HE with RTL), and keyboard shortcuts.
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
4. **M0b:** SYCL port of Darknet for Iris Xe training. *In progress:* the toolchain is ready, the first migration is clean, and oneMKL SGEMM runs at ~1 TFLOPS. Next: the `DARKNET_TRY_SYCL` CMake option and the host-side port.
5. Scaffold `server/` (Drogon hello-world plus SQLite) and `web/` (Vite + React + TS + Konva).
6. Set up CI with the build matrix and the license gate.
7. Start the M1 labeling canvas.
