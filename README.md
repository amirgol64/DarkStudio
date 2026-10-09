# DarkStudio

DarkStudio is a free, open-source computer-vision toolchain. It lets you label, train, run inference and export from one web app. It is built on [Darknet](https://codeberg.org/CCodeRun/darknet) and [DarkHelp](https://codeberg.org/CCodeRun/DarkHelp).

> Status: early planning. See [plan.md](plan.md) for the roadmap.

## Repository layout

| Path | What it is | License |
|---|---|---|
| `darknet/` | Submodule: fork of Hank.ai Darknet (training and inference engine). It follows branch **`sycl`**, which adds the Intel GPU (SYCL) backend | Apache-2.0 |
| `DarkHelp/` | Submodule: fork of DarkHelp (inference API) | MIT |
| `DarkMark/` | Submodule: fork of DarkMark, **kept for reference only and not built into DarkStudio** | GPL-3.0 |
| `tools/ov-bench/` | C++ tool: run Darknet ONNX models with OpenVINO (Intel GPU/CPU), benchmark and save annotated images | Apache-2.0 |
| `tools/sycl-check/` | C++/SYCL tool: show the SYCL device and benchmark oneMKL SGEMM (Intel GPU training check) | Apache-2.0 |
| `tools/sycl-migrate/` | Scripts to convert Darknet's CUDA kernels to SYCL with SYCLomatic (WSL), import them into the fork, build Darknet with SYCL (`build-darknet-sycl.bat`), and compare CPU vs SYCL results (`compare-cpu-sycl.ps1`) | Apache-2.0 |
| `docs/` | [Windows build](docs/build-windows.md), [Intel GPU](docs/intel-gpu.md) | Apache-2.0 |
| `server/` | C++ backend (Drogon). Planned | Apache-2.0 |
| `web/` | React + TypeScript web UI. Planned | Apache-2.0 |

## Hardware support

| Hardware | Inference | Training | Status |
|---|---|---|---|
| CPU (x64 / ARM) | Darknet (AVX2 + OpenMP), OpenVINO, ONNX Runtime | Darknet (AVX2 + OpenMP) | **Working on Windows**: yolov4-tiny ≈ 160–180 ms per image on an i5-1135G7 |
| **Intel GPU** (Iris Xe, Arc) | **OpenVINO** (Darknet → ONNX), ONNX Runtime OpenVINO/DirectML EP | Darknet **SYCL / oneAPI** port | **Inference working**: yolov4-tiny at 11.8 ms / 85 FPS on Iris Xe (FP16), ~13× faster than CPU ([docs](docs/intel-gpu.md)) · **Native Darknet on Iris Xe via SYCL works** (darknet fork, branch `sycl`): identical detections to the CPU, 27.5 ms vs 180 ms per image · Training: next |
| NVIDIA | Darknet CUDA/cuDNN, TensorRT | Darknet CUDA | Upstream |
| AMD | Darknet ROCm, DirectML | Darknet ROCm | Upstream |
| Apple | CoreML | CPU | Planned |

## Languages

DarkStudio is **C++ first**. The engine, server and tools are C++20. The browser UI is TypeScript, with heavy work in C++ compiled to WebAssembly. Python is only used for one-time model conversion to ONNX. See [plan.md §3.4](plan.md#34-c-first).

## Getting the code

```sh
git clone --recurse-submodules https://github.com/amirgol64/DarkStudio.git
```

If you already cloned without submodules:

```sh
git submodule update --init
```

## Syncing the forks with upstream

Each submodule has two remotes. `origin` is the GitHub fork, where you push your work. `upstream` is the original project on Codeberg, and it is fetch-only.

```sh
cd darknet
git fetch upstream
git rebase upstream/master   # or: git merge upstream/master
git push origin master       # use --force-with-lease after a rebase
cd ..
git add darknet && git commit -m "Bump darknet"
```

A fresh clone only has `origin`. Add the upstream remote once per submodule:

```sh
git -C darknet  remote add upstream https://codeberg.org/CCodeRun/darknet.git
git -C DarkHelp remote add upstream https://codeberg.org/CCodeRun/DarkHelp.git
git -C DarkMark remote add upstream https://codeberg.org/CCodeRun/DarkMark.git
```

## Building

- **Windows:** [docs/build-windows.md](docs/build-windows.md) covers vcpkg, Darknet (CPU), DarkHelp, a smoke test and performance notes. Dependencies come from vcpkg at `C:\src\vcpkg`, and the build installs to `build/install/`.
  - Configure Darknet with `-DDARKNET_TRY_OPENBLAS=OFF` on Windows. vcpkg's OpenBLAS is about 20× slower than Darknet's built-in AVX2 code.
- **Intel GPU (OpenVINO):** [docs/intel-gpu.md](docs/intel-gpu.md) covers installing OpenVINO, exporting to ONNX, and building and running `ov-bench`.
- **Darknet on Intel GPU (oneAPI / SYCL):** [docs/intel-gpu.md](docs/intel-gpu.md#training-on-intel-gpu-m0b-sycl-port-of-darknet) covers installing oneAPI 2026.1, SYCLomatic in WSL, and building with `tools\sycl-migrate\build-darknet-sycl.bat` (installs to `build/install-sycl/`, needs `oneAPI\2026.1\bin` on `PATH`).
- Our forks carry these Windows build fixes (to be offered upstream):
  - **darknet:** the ONNX protobuf header is generated before the library compiles (fixes parallel build error C1083), plus fixes for clang-based compilers (clang-cl, Intel icx) and `DARKNET_TRY_ONNX`.
  - **DarkHelp:** runtime DLLs install without copying `darknet.dll` by hand.

## Contributing

Read [CLAUDE.md](CLAUDE.md) for the project rules. Every change has to tick its task in `plan.md` and update `README.md` and `docs/` when it adds a feature, platform or dependency.

## License

DarkStudio's own code is licensed under [Apache-2.0](LICENSE). Each submodule keeps its own license. The planned model registry marks every model with its license, and models that are AGPL or not allowed for commercial use are offered only as optional plugins. See [plan.md](plan.md#2-licensing-policy).
