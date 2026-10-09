@echo off
rem DarkStudio - configure, build and install Darknet with the SYCL backend (Intel GPUs) on Windows.
rem SPDX-License-Identifier: Apache-2.0
rem
rem Usage: tools\sycl-migrate\build-darknet-sycl.bat [configure|build|install|all]   (default: all)
rem Output: darknet\build-sycl (build tree), build\install-sycl (install tree)
rem
rem The ONNX export tool is left out: the vcpkg protobuf DLL is built with MSVC and one of its inline functions does not
rem link with icx.  Use the ONNX exporter from the regular CPU build instead (docs/build-windows.md).
setlocal
set "STEP=%~1"
if "%STEP%"=="" set "STEP=all"

set "ROOT=%~dp0..\.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

rem oneapi-vars.bat runs "call vars.bat" after pushd, which fails if this is set (some shells set it)
set "NoDefaultCurrentDirectoryInExePath="
rem oneapi-vars.bat needs vswhere.exe to find Visual Studio
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
if not defined DARKSTUDIO_ONEAPI set "DARKSTUDIO_ONEAPI=C:\Program Files (x86)\Intel\oneAPI\2026.1"
call "%DARKSTUDIO_ONEAPI%\oneapi-vars.bat" intel64 vs2022 >nul || exit /b 1

if not exist "%ROOT%\darknet\build-sycl" mkdir "%ROOT%\darknet\build-sycl"
rem Darknet's CM_version.cmake runs "git describe" in the current directory, so configure from inside the build tree
cd /d "%ROOT%\darknet\build-sycl"

if "%STEP%"=="configure" goto configure
if "%STEP%"=="all" goto configure
goto build

:configure
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release ^
	-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icx ^
	-DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ^
	-DDARKNET_TRY_SYCL=ON -DDARKNET_TRY_CUDA=OFF -DDARKNET_TRY_ROCM=OFF -DDARKNET_TRY_OPENBLAS=OFF ^
	-DDARKNET_TRY_ONNX=OFF ^
	-DCMAKE_INSTALL_PREFIX="%ROOT%/build/install-sycl" %DARKNET_SYCL_CMAKE_ARGS% || exit /b 2
if "%STEP%"=="configure" exit /b 0

:build
if "%STEP%"=="install" goto install
cmake --build . --parallel %NUMBER_OF_PROCESSORS% || exit /b 3
if "%STEP%"=="build" exit /b 0

:install
cmake --install . || exit /b 4
exit /b 0
