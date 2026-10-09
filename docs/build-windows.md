# Building on Windows (CPU / Intel GPU machines)

These steps were tested on Windows 11 with an Intel i5-1135G7 and Iris Xe (no NVIDIA GPU), 2026-10-09.

| Component | Version used |
|---|---|
| Visual Studio 2022 Community (MSVC) | 17.14, toolset 14.44 |
| CMake | 4.1.2 |
| vcpkg | 2026-09-26, at `C:\src\vcpkg` |
| OpenCV (vcpkg) | 4.14.0, includes the DirectML feature |
| Protobuf (vcpkg) | 6.33.4, used for the Darknet ONNX export |
| Darknet | v5.1-104 + DarkStudio fixes |
| DarkHelp | v1.9.8-1 + DarkStudio fixes |

Everything installs to `C:\dev\Python_dev\draknet\build\install` (ignored by git), not `C:\Program Files`, so no admin rights are needed.

## 1. vcpkg and dependencies (one time, ~1–2 h)

```powershell
git clone https://github.com/microsoft/vcpkg C:\src\vcpkg
C:\src\vcpkg\bootstrap-vcpkg.bat -disableMetrics
C:\src\vcpkg\vcpkg.exe install "opencv[contrib,dnn,freetype,jpeg,openmp,png,webp,world]:x64-windows" protobuf:x64-windows tclap:x64-windows
```

Don't install OpenBLAS (see [the OpenBLAS note](#openblas-is-20-slower-on-windows-dont-use-it)).

> **If the vcpkg build gets interrupted** (closed terminal, sleep, power loss), a file it was writing can be left zero-filled. We hit this with `include/utf8_validity.h` (`utf8-range` package), which made protobuf fail with `'utf8_range': is not a class or namespace name`. Fix it by reinstalling the affected package:
> ```powershell
> C:\src\vcpkg\vcpkg.exe remove utf8-range:x64-windows --recurse
> C:\src\vcpkg\vcpkg.exe install utf8-range:x64-windows --no-binarycaching
> ```

## 2. Darknet (CPU-only)

> **Run CMake from inside `darknet\build`.** Darknet's `CM_version.cmake` runs `git describe` in the current directory. Started from the DarkStudio root, it reads the wrong repo and fails with `VERSION ".." format invalid`.

```powershell
cd C:\dev\Python_dev\draknet\darknet
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DDARKNET_TRY_CUDA=OFF -DDARKNET_TRY_ROCM=OFF -DDARKNET_TRY_OPENBLAS=OFF `
  -DCMAKE_INSTALL_PREFIX=C:/dev/Python_dev/draknet/build/install
cmake --build . --config Release --parallel 8
cmake --install . --config Release
```

Check it:

```powershell
C:\dev\Python_dev\draknet\build\install\bin\darknet.exe --version
# Darknet V5 "Moonlit" v5.1-... Darknet is compiled to use the CPU.  GPU is disabled.
# Protobuf 6.33.4, OpenCV 4.14.0 ... FMA & AVX2 detected.
```

## 3. DarkHelp

```powershell
cd C:\dev\Python_dev\draknet\DarkHelp
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DDarknet=C:/dev/Python_dev/draknet/build/install/lib/darknet.lib `
  -DCMAKE_INSTALL_PREFIX=C:/dev/Python_dev/draknet/build/install
cmake --build . --config Release --parallel 8
cmake --install . --config Release
```

You don't need to copy `darknet.dll` by hand (upstream's README says to). Our fork copies it next to each tool when it builds.

## 4. Smoke test

```powershell
$m = "C:\dev\Python_dev\draknet\models\pretrained"
mkdir $m -Force
Invoke-WebRequest https://github.com/hank-ai/darknet/releases/download/v2.0/yolov4-tiny.weights -OutFile $m\yolov4-tiny.weights
copy C:\dev\Python_dev\draknet\darknet\cfg\yolov4-tiny.cfg, C:\dev\Python_dev\draknet\darknet\cfg\coco.names $m
C:\dev\Python_dev\draknet\build\install\bin\DarkHelp.exe --json --keep $m\coco.names $m\yolov4-tiny.cfg $m\yolov4-tiny.weights C:\dev\Python_dev\draknet\darknet\artwork\dog.jpg
```

Expected: `dog 87%`, `truck 81%`, `bicycle 61%`.

> **Use a copy of the `.cfg`.** DarkHelp rewrites `batch=`/`subdivisions=` in the cfg file you give it (it sets them to 1 for inference). If you point it at `darknet\cfg\` directly, it modifies the submodule.

## Performance notes

### OpenBLAS is 20× slower on Windows. Don't use it

The Darknet README recommends OpenBLAS for CPU builds. With vcpkg on Windows, OpenBLAS gets compiled by MSVC, which can't build its optimized assembly kernels. The result is far slower than Darknet's own AVX2 + OpenMP GEMM:

| yolov4-tiny 416×416, i5-1135G7 (4C/8T) | Time per image |
|---|---|
| Darknet + vcpkg OpenBLAS 0.3.33 | ~2,600 ms |
| Darknet + vcpkg OpenBLAS, `OPENBLAS_NUM_THREADS=4 OMP_NUM_THREADS=1` | ~2,175 ms |
| **Darknet without OpenBLAS (AVX2 + OpenMP)** | **~130–180 ms** |

Detections are identical. Always configure with `-DDARKNET_TRY_OPENBLAS=OFF` on Windows. Leave `OMP_NUM_THREADS` unset: forcing 4 or 8 threads was slower than the default.

### Baseline for Intel GPU work

The CPU baseline is **~160–180 ms per image** (≈6 FPS) for yolov4-tiny, measured with DarkHelp including annotation. The same model on the Iris Xe through OpenVINO runs at **11.8 ms (85 FPS)**. See [intel-gpu.md](intel-gpu.md).
