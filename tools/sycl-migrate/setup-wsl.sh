#!/bin/bash
# DarkStudio - one-time setup for the SYCLomatic migration of Darknet's CUDA kernels (M0b).
# SPDX-License-Identifier: Apache-2.0
#
# Run inside WSL Ubuntu as a normal user. Nothing is installed system-wide and no root is needed:
#   ~/sdk/syclomatic          SYCLomatic (c2s), Linux build
#   ~/sdk/cuda-12.9-headers   CUDA 12.9 headers only (SYCLomatic supports CUDA <= 12.9)
#   ~/sdk/sysroot             C/C++ standard headers unpacked from Ubuntu .deb packages
set -eo pipefail

SYCLOMATIC_RELEASE="${SYCLOMATIC_RELEASE:-20260928}"
cuda="$HOME/sdk/cuda-12.9-headers"
base="https://developer.download.nvidia.com/compute/cuda/redist"

echo "== SYCLomatic $SYCLOMATIC_RELEASE"
mkdir -p ~/sdk/syclomatic
curl -fsSL "https://github.com/oneapi-src/SYCLomatic/releases/download/$SYCLOMATIC_RELEASE/linux_release.tgz" | tar -xz -C ~/sdk/syclomatic
~/sdk/syclomatic/bin/c2s --version | head -1

echo "== CUDA 12.9 headers"
mkdir -p "$cuda"
for p in \
  cuda_cudart/linux-x86_64/cuda_cudart-linux-x86_64-12.9.79-archive.tar.xz \
  cuda_nvcc/linux-x86_64/cuda_nvcc-linux-x86_64-12.9.86-archive.tar.xz \
  cuda_cccl/linux-x86_64/cuda_cccl-linux-x86_64-12.9.27-archive.tar.xz \
  libcurand/linux-x86_64/libcurand-linux-x86_64-10.3.10.19-archive.tar.xz \
  libcublas/linux-x86_64/libcublas-linux-x86_64-12.9.2.10-archive.tar.xz
do
  echo "   $p"
  curl -fsSL "$base/$p" | tar -xJ -C "$cuda" --strip-components=1 --wildcards '*/include/*'
done
grep -m1 'define CUDART_VERSION' "$cuda/include/cuda_runtime_api.h"

echo "== C/C++ standard headers (sysroot)"
mkdir -p ~/sdk/debs ~/sdk/sysroot
cd ~/sdk/debs
apt-get download libstdc++-15-dev libc6-dev linux-libc-dev libgcc-15-dev libcrypt-dev
for d in *.deb; do dpkg -x "$d" ~/sdk/sysroot; done
echo "done: $(du -sh ~/sdk | cut -f1) in ~/sdk"
