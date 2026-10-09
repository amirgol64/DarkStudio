# DarkStudio: rules for Claude

These rules apply to every session in this project.

## Keep the docs in sync (mandatory)

1. **`plan.md` is the source of truth for progress.** When you finish a task, tick it (`- [x]`) in `plan.md` in the same change. If work starts that the plan doesn't cover, add a task for it first, then tick it when done.
2. **Every new feature, platform, backend, dependency or model has to show up everywhere it applies.** That includes the decision table and roadmap in `plan.md`, the model registry and license table if it's a model, `README.md`, and any file under `docs/`. Nothing gets added to the code without being documented.
3. Check these before every commit. A commit that changes behavior without updating `plan.md`/`README.md`/`docs/` is incomplete.

## Engineering principles

- **C++ first.** Engine, backend and tools are C++20. Python is only for one-time model conversion (PyTorch → ONNX) and optional bindings. The browser UI is TypeScript, and heavy client-side work goes to C++ compiled to WebAssembly.
- **License gate:** the core (`server/`, `web/`, our changes to darknet/DarkHelp) is Apache-2.0. Don't copy code from `DarkMark/` (GPL-3). AGPL or non-commercial models are offered only as warned plugins (see `plan.md` §2).
- **Hardware:** the dev machine is Windows 11 with an Intel Iris Xe iGPU (i5-1135G7, 32 GB) and **no NVIDIA/CUDA**. Every feature has to work on CPU and on Intel GPU (OpenVINO / oneAPI). CUDA, ROCm, and the rest are additions on top.

## Layout and workflow

- `darknet/`, `DarkHelp/`, `DarkMark/` are submodules. `origin` is github.com/amirgol64/<repo> and `upstream` is codeberg.org/CCodeRun/<repo> (fetch only).
- Commit changes inside a submodule to that fork first, then commit the updated submodule pointer in DarkStudio.
- Windows deps come from vcpkg at `C:\src\vcpkg` (triplet `x64-windows`). Build from a VS 2022 x64 Developer environment.
