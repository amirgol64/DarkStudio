#!/bin/bash
# DarkStudio - migrate Darknet's CUDA kernels to SYCL with SYCLomatic (M0b).
# SPDX-License-Identifier: Apache-2.0
#
# Usage (inside WSL Ubuntu, after setup-wsl.sh):
#   bash tools/sycl-migrate/migrate.sh [output-dir]
# The default output is build/sycl-migration/ (git-ignored). The darknet submodule itself is not modified.
# Configuration matches the planned DARKNET_GPU_SYCL build: GPU code, CUDA API, no cuDNN.
set -o pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
src="$root/darknet/src-lib"
out="${1:-$root/build/sycl-migration}"
cuda="$HOME/sdk/cuda-12.9-headers"
vcpkg_inc="${VCPKG_INCLUDE:-/mnt/c/src/vcpkg/installed/x64-windows/include}"

source "$HOME/sdk/syclomatic/setvars.sh" > /dev/null
rm -rf "$out"; mkdir -p "$out"
cd "$src"
c2s --cuda-include-path="$cuda/include" \
    --in-root="$src" --out-root="$out" \
    --report-type=all --report-file-prefix=report \
    --extra-arg="-DDARKNET_GPU" --extra-arg="-DDARKNET_GPU_CUDA" \
    --extra-arg="-I$src" --extra-arg="-I$src/../src-onnx" \
    --extra-arg="-I$vcpkg_inc" --extra-arg="-I$vcpkg_inc/opencv4" \
    --extra-arg="-std=c++17" --extra-arg="--sysroot=$HOME/sdk/sysroot" \
    *.cu > "$out/c2s.log" 2>&1
echo "c2s exit=$?"
echo "outputs:  $(ls "$out" | grep -c '\.dp\.cpp$') .dp.cpp files"
echo "errors:   $(grep -c ' error:' "$out/c2s.log" || true)"
echo "warnings: $(grep -c 'warning: DPCT' "$out/c2s.log" || true)"
grep -o 'warning: DPCT[0-9]*' "$out/c2s.log" | sort | uniq -c | sort -rn
