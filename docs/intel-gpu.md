# Intel GPU support (Iris Xe / Arc)

Upstream Darknet only accelerates on NVIDIA (CUDA) and AMD (ROCm). DarkStudio adds Intel GPUs in two steps:

| Step | What | Status |
|---|---|---|
| **M0a: inference** | Darknet `.weights` → ONNX → **OpenVINO** on the Intel GPU | ✅ Working (2026-10-09) |
| **M0b: training** | Port Darknet's CUDA kernels to **SYCL / oneAPI** | 🔨 In progress: toolchain ready, first migration clean (2026-10-09) |

See `plan.md` §3.3 for the full strategy.

## Results: yolov4-tiny 416×416 on an i5-1135G7 with Iris Xe

Averaged over the 6 Darknet sample images, 50 runs each after warm-up. "Total" includes preprocessing, inference and box decoding with NMS.

| Engine / device | Inference | Total | FPS | Speed-up |
|---|---|---|---|---|
| Darknet, CPU (AVX2 + OpenMP) | | ~130–180 ms | ~6 | 1× |
| OpenVINO, CPU (FP32) | 78.3 ms | 80.5 ms | 12 | ~2× |
| OpenVINO, **Iris Xe GPU, FP32** | 15.3 ms | 18.5 ms | 54 | ~8× |
| OpenVINO, **Iris Xe GPU, FP16** | **9.2 ms** | **11.8 ms** | **85** | **~13×** |

The detections from OpenVINO match Darknet on every image: same objects, confidences within 1%, same boxes.

**Model compile time** is a one-time cost when the model is first loaded on the GPU. It's ~12.6 s without a cache and **~0.12 s with `--cache`** (OpenVINO's model cache). DarkStudio should always enable the cache.

## Install OpenVINO (Windows)

OpenVINO is Apache-2.0. We use the official prebuilt archive, which includes the CPU, GPU and NPU plugins.

```powershell
$v    = "2026.4.1"
$name = "openvino_toolkit_windows_2026.4.1.22982.07f9c262b05_x86_64.zip"
$base = "https://storage.openvinotoolkit.org/repositories/openvino/packages/$v/windows"
Invoke-WebRequest "$base/$name" -OutFile "$env:TEMP\$name"
Invoke-WebRequest "$base/$name.sha256" -OutFile "$env:TEMP\$name.sha256"
# verify: the two hashes must be equal
(Get-FileHash "$env:TEMP\$name" -Algorithm SHA256).Hash.ToLower(); Get-Content "$env:TEMP\$name.sha256"
Expand-Archive "$env:TEMP\$name" -DestinationPath C:\src
Rename-Item C:\src\openvino_toolkit_windows_2026.4.1.22982.07f9c262b05_x86_64 openvino
```

At run time, put the OpenVINO DLLs on `PATH` (or run `C:\src\openvino\setupvars.ps1`):

```powershell
$env:PATH = "C:\src\openvino\runtime\bin\intel64\Release;C:\src\openvino\runtime\3rdparty\tbb\bin;$env:PATH"
```

The Intel GPU plugin needs a current Intel graphics driver. Tested with 32.0.101.7092.

## Export a Darknet model to ONNX

```powershell
cd C:\dev\Python_dev\draknet\models\pretrained
C:\dev\Python_dev\draknet\build\install\bin\darknet_onnx_export.exe yolov4-tiny.cfg coco.names yolov4-tiny.weights
```

The exported model (Darknet v5.1 exporter, box post-processing on by default) has this format:

| Tensor | Shape | Meaning |
|---|---|---|
| input `frame` | `[1, 3, H, W]` | RGB, float 0..1, plain resize to the network size (no letterbox) |
| output `confs` | `[1, N, classes]` | objectness × class probability (N = 2535 for yolov4-tiny 416) |
| output `boxes` | `[1, N, 1, 4]` | **normalized x1, y1, x2, y2**. Multiply by the original image width and height |

NMS isn't in the graph. Do it per class (we use IoU 0.45, like Darknet).

## ov-bench

`tools/ov-bench/` is a small C++20 tool (OpenVINO + OpenCV) that runs an exported model, prints detections and timing, and can save annotated images.

Build it:

```powershell
cd C:\dev\Python_dev\draknet
cmake -S tools/ov-bench -B tools/ov-bench/build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DOpenVINO_DIR=C:/src/openvino/runtime/cmake
cmake --build tools/ov-bench/build --config Release
```

Run it:

```powershell
$m = "C:\dev\Python_dev\draknet\models\pretrained"
tools\ov-bench\build\Release\ov-bench.exe $m\yolov4-tiny.onnx $m\coco.names darknet\artwork\dog.jpg `
  --device GPU --precision f16 --iters 50 --cache build\ov-cache --save build\ov-out
```

| Option | Default | Meaning |
|---|---|---|
| `--device` | `GPU` | `GPU`, `CPU`, `NPU` or `AUTO` |
| `--precision` | `f16` | Inference precision hint: `f16` or `f32` |
| `--iters` | `50` | Timed runs per image (after one warm-up) |
| `--threshold` | `0.5` | Confidence threshold |
| `--nms` | `0.45` | NMS IoU threshold |
| `--cache` | off | OpenVINO model cache folder. Cuts the GPU compile from ~12 s to ~0.1 s |
| `--save` | off | Write annotated images to this folder |
| `--raw` | off | Print the raw `confs` / `boxes` values above the threshold |

## Training on Intel GPU (M0b: SYCL port of Darknet)

OpenVINO can only run models. To train on the Iris Xe, Darknet's CUDA code is ported to **SYCL** (Intel oneAPI). It becomes a third GPU backend, `DARKNET_GPU_SYCL`, next to `DARKNET_GPU_CUDA` and `DARKNET_GPU_ROCM`, following the structure upstream already uses for AMD in `darknet_gpu.hpp`.

### What the Iris Xe offers (from `tools/sycl-check`)

| Property | Iris Xe (Level Zero) | Meaning for Darknet |
|---|---|---|
| Compute units | 80 | |
| Max work-group size | **512** | Matches Darknet's `BLOCK = 512`, so the kernels launch as-is |
| Sub-group sizes | 8, 16, 32 | Warp-level code (`__shfl`) maps to sub-groups of 32 |
| Local memory | 64 KiB | |
| Global memory | ~14.5 GiB (shared with the system) | |
| FP16 / FP64 | yes / **no** | Darknet's kernels use no `double`, so that's fine |

oneMKL SGEMM replaces cuBLAS. Convolutions in Darknet's GPU path are im2col + SGEMM, so this is the main cost of training:

| SGEMM (M×N×K), shapes from yolov4-tiny | Iris Xe | CPU (oneMKL via SYCL) |
|---|---|---|
| 64 × 43264 × 288 | 2.35 ms, 678 GFLOPS | 6.64 ms, 240 GFLOPS |
| 128 × 10816 × 576 | 1.84 ms, 867 GFLOPS | 6.81 ms, 234 GFLOPS |
| 256 × 2704 × 1152 | 1.65 ms, 964 GFLOPS | 8.56 ms, 186 GFLOPS |
| 512 × 676 × 2304 | 2.36 ms, 674 GFLOPS | 8.24 ms, 194 GFLOPS |
| 2048 × 2048 × 2048 | 12.97 ms, **1,325 GFLOPS** | 67.44 ms, 255 GFLOPS |

So the GPU is **~3.5–4.5× the CPU on the same oneMKL code**. Darknet's own CPU GEMM is slower than oneMKL, so the expected training speed-up over today's Darknet CPU build is **≥ 4×**.

### First SYCLomatic migration

`tools/sycl-migrate/migrate.sh` converts all 10 `.cu` files (6,302 lines, 154 kernels), configured as GPU + CUDA API with **no cuDNN** (like the ROCm build). Result: **0 errors, 10 `.dp.cpp` files, 330 warnings**:

| Warning | Count | Where | Action |
|---|---|---|---|
| DPCT1049 | 150 | all | Work-group may exceed the device limit. Darknet uses 512 = the Iris Xe limit. **OK** |
| DPCT1010 | 127 | all | `cudaPeekAtLastError` removed, since SYCL uses exceptions. **OK** |
| DPCT1101/1065 | 20 | blas, im2col | Constant-size hints and barrier fences. Performance only |
| DPCT1007/1082 | 17 | im2col | `nvcuda::wmma` Tensor Core **binary (XNOR)** kernels. Not used by YOLO. **Disable under SYCL** |
| DPCT1119 | 6 | network | CUDA Graphs (optional speed-up). **Disable first**, then try SYCL graphs |
| DPCT1110 | 4 | blas, im2col | Large private arrays mean register pressure. Performance check |
| DPCT1118/1121 | 3 | im2col | Group functions in divergent control flow. **Needs a correctness review** |
| DPCT1000/1001/1124 | 3 | dark_cuda.hpp, blas | Error-check macro and async memcpy. Small manual edits |

### Toolchain setup

| Tool | Version | Where | Notes |
|---|---|---|---|
| Intel oneAPI: DPC++/C++ compiler, oneMKL, oneDNN, TBB, oneDPL | 2026.1 | `C:\Program Files (x86)\Intel\oneAPI\2026.1` | Online installer, components `intel.oneapi.win.cpp-dpcpp-common`, `intel.oneapi.win.mkl.devel`, `intel.oneapi.win.dnnl`. Needs admin |
| SYCLomatic (`c2s`) | 20260928 (clang 21) | WSL Ubuntu `~/sdk/syclomatic` | No Windows build is published anymore, and it's no longer in the oneAPI toolkit |
| CUDA headers | 12.9.2 (headers only) | WSL `~/sdk/cuda-12.9-headers` | SYCLomatic supports CUDA ≤ 12.9. No NVIDIA GPU or driver needed |
| C/C++ std headers | Ubuntu libstdc++-15 / glibc 2.43 | WSL `~/sdk/sysroot` | Unpacked from `.deb` files, no root |

Install oneAPI (silent, with the components we use):

```powershell
$u = "https://registrationcenter-download.intel.com/akdlm/IRC_NAS/4144bec3-82ce-4672-bd71-5c93a79cd5e7/intel-oneapi-toolkit-2026.1.0.191.exe"
Invoke-WebRequest $u -OutFile $env:TEMP\oneapi.exe
& $env:TEMP\oneapi.exe -s -x -f $env:TEMP\oneapi            # extract (signed by Intel Corporation)
Start-Process $env:TEMP\oneapi\bootstrapper.exe -Verb RunAs -Wait -ArgumentList `
  '-s','--action','install','--eula','accept','-p=NEED_VS2022_INTEGRATION=1', `
  '--components','intel.oneapi.win.cpp-dpcpp-common:intel.oneapi.win.mkl.devel:intel.oneapi.win.dnnl'
```

Set up the migration tools in WSL (once), then migrate:

```powershell
wsl -d Ubuntu -- bash /mnt/c/dev/Python_dev/draknet/tools/sycl-migrate/setup-wsl.sh
wsl -d Ubuntu -- bash /mnt/c/dev/Python_dev/draknet/tools/sycl-migrate/migrate.sh
```

Check the device and oneMKL: `tools\sycl-check\build.bat`.

> **oneAPI environment pitfalls (Windows):**
> - oneAPI 2026 uses a single unified folder. Use `2026.1\oneapi-vars.bat`, not the top-level `setvars.bat`.
> - `oneapi-vars.bat` needs `vswhere.exe` on `PATH` (`C:\Program Files (x86)\Microsoft Visual Studio\Installer`).
> - It fails with `'vars.bat' is not recognized` if `NoDefaultCurrentDirectoryInExePath` is set. Clear it first, as `tools\sycl-check\build.bat` does.
> - `icx` on Windows takes MSVC-style options: `/std:c++20 /EHsc /Qmkl /Fe:out.exe`.

## Next

- M0b: add `DARKNET_TRY_SYCL` / `DARKNET_GPU_SYCL` to the darknet fork, bring in the migrated kernels, port the host side (memory, streams, cuBLAS → oneMKL), and get inference matching the CPU first, then training.
- Add OpenVINO as a backend in DarkHelp (`IBackend`, M2), so the tools and the server can use the Intel GPU directly.
- Try INT8 quantization and the NPU on newer Intel CPUs (M6).
