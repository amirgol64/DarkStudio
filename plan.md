# DarkStudio — Project Plan

DarkStudio is a free, open-source toolchain for computer vision. It brings labeling, training, inference and export together in one web app, built on Hank.ai **Darknet**, **DarkHelp**, and ideas from **DarkMark**.

- **Owner:** solo developer, full-time (roughly a 6–12 month roadmap)
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
| 10 | Accelerators | CUDA/cuDNN/TensorRT, ROCm, DirectML, OpenVINO/oneDNN on CPU, CoreML/Metal |
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
│ + seg head     │   │  EPs: CUDA/TRT/DirectML/OpenVINO/  │
│ + ONNX export  │   │       CoreML/ROCm/CPU              │
└────────────────┘   │  Task heads: det/seg/cls/pose/obb  │
                     └────────────────────────────────────┘
```

### 3.1 Repository layout (target)

```
draknet/
├── darknet/            # fork (Apache-2.0)
├── DarkHelp/           # fork (MIT)
├── DarkMark/           # reference only (GPL-3) – not built into DarkStudio
├── server/             # Drogon backend (Apache-2.0)
│   ├── src/{api,auth,db,jobs,formats,registry}
│   └── migrations/
├── web/                # React frontend (Apache-2.0)
│   └── src/{features/{label,datasets,train,zoo,review},components,i18n}
├── models/registry.yaml  # model metadata: task, license, URLs, pre/post-proc
├── tools/              # converters, scripts
├── docs/
└── .github/workflows/
```

### 3.2 Key internal interfaces

- **DarkHelp `IBackend`**: `load(model_spec)`, `predict(batch) -> Results`. `Results` holds boxes, masks (RLE or polygon), class scores, keypoints and rotated boxes.
- **`ModelSpec`** (from `registry.yaml`): input size, normalization, letterbox, output decoder (`yolo_v8`, `rtdetr`, `rtmdet_ins`, `deeplab`, `sam2_encoder/decoder`, …), task, license.
- **Annotation model** (DB): `Image`, `Annotation{type: box|polygon|mask_rle|keypoints|obb|class, class_id, data, source: human|model|sam, confidence, reviewed}`.
- **Format adapters** (`server/src/formats/`): one importer and one exporter per format, sharing a canonical internal model.

---

## 4. Roadmap

Milestones assume one developer working full time. Each one ends with a release you can demo.

### M0 — Foundations (weeks 1–3)
- [ ] Fork the 3 repos on GitHub/Codeberg and set the `upstream` remotes. Add a `docs/upstream-sync.md` procedure.
- [ ] Build all three on Windows (MSVC + vcpkg) and Linux. Write down the exact steps.
- [ ] Create the monorepo skeleton (`server/`, `web/`, CMake superbuild, vcpkg manifest).
- [ ] CI: build matrix (Windows, Ubuntu, macOS), license scanner, clang-format/clang-tidy, ESLint/Prettier.
- [ ] Write `LICENSE` (Apache-2.0), `THIRD_PARTY_LICENSES.md`, `CONTRIBUTING.md`, and `CODE_OF_CONDUCT.md`.

### M1 — MVP: label → train → infer (weeks 4–12)
**Backend**
- [ ] Drogon server with SQLite. Migrations for users, projects, images, classes, annotations and jobs.
- [ ] Local JWT auth (a single admin account in local mode).
- [ ] Image ingest: upload a folder or a server path, generate thumbnails, store EXIF.
- [ ] Import and export Darknet/YOLO txt, YOLO-seg polygons and COCO JSON.
- [ ] Training job: generate `.cfg`, `.data` and `train.txt` (port DarkMark's ideas, not its code). Spawn Darknet, parse the log, stream loss/mAP over WebSocket, support stop and resume.
- [ ] Inference job through DarkHelp with the Darknet backend. Results can be saved as pre-annotations.

**Frontend**
- [ ] App shell with routing, dark/light theme, i18n scaffold (EN plus HE with RTL), and keyboard shortcuts.
- [ ] Labeling canvas (Konva): box and polygon tools, zoom/pan, class palette, undo/redo, auto-save.
- [ ] Image grid with virtualization (handles 100k+ images) and filters (unlabeled, class, reviewed).
- [ ] Training dashboard: live loss and mAP charts, GPU usage, ETA, and checkpoint list.
- [ ] Inference viewer: run the model on an image, a folder or a video, with a confidence slider.

**Exit criterion:** label 200 images in the browser, train a YOLOv4-tiny, and view predictions, all without touching the CLI.

### M2 — Multi-model inference via ONNX Runtime (weeks 13–18)
- [ ] Add `IBackend` to DarkHelp and keep the existing Darknet path working (send this upstream if the maintainer agrees).
- [ ] ONNX Runtime backend with execution providers CUDA, TensorRT, DirectML, OpenVINO, CoreML, ROCm and CPU.
- [ ] Decoders for YOLOX, RT-DETR/RF-DETR/D-FINE, RTMDet/RTMDet-Ins, DeepLabV3, RTMPose and YOLO-seg (plugin).
- [ ] `models/registry.yaml` and a **Model Zoo page** with task badges, license badges, a "commercial OK?" flag, and the warning modal for plugins.
- [ ] Render masks, keypoints and OBBs in the viewer.
- [ ] Python bindings (DarkHelp already has `src-python`) for scripting.

### M3 — Darknet native instance segmentation (weeks 19–30)
This is the biggest research item. Code lives in `darknet/src-lib/`.
- [ ] Design: YOLACT/YOLOv8-seg style. A prototype mask branch (`[proto]` layer) plus mask coefficients added to the `[yolo]` layer outputs.
- [ ] New layer `seg_proto_layer.cpp`, and `yolo_layer.cpp` extended with N mask coefficients per anchor.
- [ ] Mask loss (BCE plus dice on the cropped prototype combination). CPU and CUDA kernels.
- [ ] Data loader: read YOLO-seg polygons, rasterize to masks, and keep augmentation consistent (mosaic, flip, scale).
- [ ] cfg templates: `yolov4-tiny-seg.cfg` and `yolov7-tiny-seg.cfg`.
- [ ] Mask mAP evaluation (COCO-style) in `darknet detector map`.
- [ ] Extend `src-onnx` export to include the seg head, and add a DarkHelp decoder for it.
- [ ] Benchmark against RTMDet-Ins and YOLOv8-seg on a public dataset.
- [ ] Later: native classification UI flow, then OBB/pose heads.

### M4 — Assisted labeling (weeks 25–34, overlaps M3)
- [ ] SAM2 / MobileSAM: run the encoder once per image (server-side, cached) and run the decoder on click/box. Convert the mask to a simplified polygon.
- [ ] Pre-annotate the whole dataset with any model from the registry. Reviewers accept or reject.
- [ ] Video: frame extraction, SAM2 video propagation, ByteTrack/OC-SORT ID tracking.
- [ ] Active learning: rank unlabeled images by uncertainty (entropy, low margin) and diversity.
- [ ] Magic wand and brush/eraser for masks.

### M5 — Multi-user and collaboration (weeks 35–40)
- [ ] PostgreSQL backend (same ORM models), plus S3/MinIO storage as an option.
- [ ] Roles (admin, annotator, reviewer). Task assignment and a review queue with accept, reject and comment.
- [ ] Audit log and per-user stats.
- [ ] Docker Compose (server, Postgres, MinIO, GPU worker) and Helm as a later option.

### M6 — Export and optimization (weeks 41–48)
- [ ] Export: ONNX, TensorRT engine (FP16/INT8 with a calibration dataset), OpenVINO IR, CoreML, TFLite.
- [ ] Inference: batching, engine caching, async pipelines, a benchmarking page (FPS/latency per EP).
- [ ] Training: mixed precision in Darknet (FP16 where safe), faster data loader (threaded decode, cache), multi-GPU review.
- [ ] UI: tiled rendering for very large images, web workers for polygon ops, lazy thumbnails.
- [ ] Edge: a minimal build for Jetson/ARM (DarkHelp, TensorRT, a headless CLI).

### M7 — Formats, polish, release 1.0 (weeks 49–52)
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
| macOS / Apple GPU training | Inference only via CoreML. No native training on Metal in 1.0 |

---

## 7. Immediate next steps

1. Create your GitHub/Codeberg forks and repoint the `origin` and `upstream` remotes in the three folders.
2. Build Darknet and DarkHelp on this Windows machine (CUDA if a GPU is present) and record the steps in `docs/build-windows.md`.
3. Scaffold `server/` (Drogon hello-world plus SQLite) and `web/` (Vite + React + TS + Konva).
4. Set up CI with the build matrix and the license gate.
5. Start the M1 labeling canvas.
