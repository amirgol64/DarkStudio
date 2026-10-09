# Intel GPU support (Iris Xe / Arc)

Upstream Darknet only accelerates on NVIDIA (CUDA) and AMD (ROCm). DarkStudio adds Intel GPUs in two steps:

| Step | What | Status |
|---|---|---|
| **M0a: inference** | Darknet `.weights` → ONNX → **OpenVINO** on the Intel GPU | ✅ Working (2026-10-09) |
| **M0b: training** | Port Darknet's CUDA kernels to **SYCL / oneAPI** | Next |

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

## Next

- Add OpenVINO as a backend in DarkHelp (`IBackend`, M2), so the tools and the server can use the Intel GPU directly.
- M0b: train on the Iris Xe through a SYCL port of Darknet.
- Try INT8 quantization (OpenVINO NNCF) and the NPU on newer Intel CPUs (M6).
