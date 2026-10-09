@echo off
rem DarkStudio - build and run sycl-check with the Intel oneAPI DPC++ compiler (Windows).
rem SPDX-License-Identifier: Apache-2.0
setlocal

rem oneapi-vars.bat runs "call vars.bat" after pushd, which fails if this is set (some shells set it)
set "NoDefaultCurrentDirectoryInExePath="
rem oneapi-vars.bat needs vswhere.exe to find Visual Studio
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
if not defined DARKSTUDIO_ONEAPI set "DARKSTUDIO_ONEAPI=C:\Program Files (x86)\Intel\oneAPI\2026.1"
call "%DARKSTUDIO_ONEAPI%\oneapi-vars.bat" intel64 vs2022 >nul || exit /b 1

if not exist "%~dp0build" mkdir "%~dp0build"
cd /d "%~dp0build"
icx -fsycl /O2 /std:c++20 /EHsc /W4 /Qmkl ..\sycl_check.cpp /Fe:sycl-check.exe || exit /b 2
sycl-check.exe
