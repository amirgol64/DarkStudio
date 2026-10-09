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
- **darknet development happens on branch `sycl`** of the fork (the SYCL / Intel GPU backend). Keep `master` equal to upstream. Put fixes that help upstream in their own commits.
- The SYCL build: `tools\sycl-migrate\build-darknet-sycl.bat` → `build/install-sycl/`. It needs `C:\Program Files (x86)\Intel\oneAPI\2026.1\bin` on PATH at run time. Check correctness with `tools\sycl-migrate\compare-cpu-sycl.ps1` after kernel changes. `src-lib/sycl/*.dp.cpp` are maintained by hand now; re-running `import.sh` overwrites them.
- Windows deps come from vcpkg at `C:\src\vcpkg` (triplet `x64-windows`). Build from a VS 2022 x64 Developer environment.
- Follow [docs/build-windows.md](docs/build-windows.md): run CMake from inside `darknet/build`, use `-DDARKNET_TRY_OPENBLAS=OFF`, install to `build/install/`, and point DarkHelp at a **copy** of a `.cfg` in `models/pretrained/` (it rewrites cfg files).
- oneAPI 2026.1 lives at `C:\Program Files (x86)\Intel\oneAPI\2026.1`. Use `2026.1\oneapi-vars.bat` with `vswhere` on PATH and `NoDefaultCurrentDirectoryInExePath` cleared (see `tools\sycl-check\build.bat`). `icx` takes MSVC-style flags on Windows.
- SYCLomatic runs in WSL **Ubuntu** (`~/sdk`, no root). Don't touch the `Nvidia_SDKM_*` WSL distros. Use `tools/sycl-migrate/*.sh`.
- **DarkStudio app**:
  - `server/` (C++20, Drogon from vcpkg, MSVC + VS generator) and `web/` (React + TS, Vite). See [docs/darkstudio-ui.md](docs/darkstudio-ui.md).
  - After server changes, run `server\build\Release\darkstudio-tests.exe`. After UI changes, run `npm run lint` and `npm run build` in `web/`.
  - Stop a running `darkstudio-server.exe` before rebuilding (the exe is locked).
  - Verify the UI with headless Edge screenshots (`msedge --headless=new --screenshot=... --virtual-time-budget=8000 http://localhost:8765/#/<page>`).
  - New UI text needs both `en` and `he` strings in `web/src/i18n.ts`. Store selectors must return state slices, never new arrays.
- OpenVINO 2026.4.1 lives at `C:\src\openvino` (see [docs/intel-gpu.md](docs/intel-gpu.md)). Always enable the model cache (`build/ov-cache`) and use the Iris Xe at FP16 by default.
