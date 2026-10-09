@echo off
rem DarkStudio - build (if needed) and start the local server, then open the UI in the browser.
rem SPDX-License-Identifier: Apache-2.0
rem
rem Usage: darkstudio.bat [--rebuild] [--port 8765]
setlocal
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "EXE=%ROOT%\server\build\Release\darkstudio-server.exe"
set "PORT=8765"
set "REBUILD="

:args
if "%~1"=="" goto args_done
if "%~1"=="--rebuild" set "REBUILD=1"
if "%~1"=="--port" (set "PORT=%~2" & shift)
shift
goto args

:args_done
if defined REBUILD goto build
if not exist "%EXE%" goto build
if not exist "%ROOT%\web\dist\index.html" goto build_web
goto run

:build
echo === building the C++ server
if not exist "%ROOT%\server\build\CMakeCache.txt" (
	cmake -S "%ROOT%\server" -B "%ROOT%\server\build" -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake || exit /b 1
)
cmake --build "%ROOT%\server\build" --config Release --parallel || exit /b 2

:build_web
echo === building the web UI
pushd "%ROOT%\web"
if not exist node_modules call npm install || (popd & exit /b 3)
call npm run build || (popd & exit /b 4)
popd

:run
echo === starting DarkStudio on http://localhost:%PORT%/
"%EXE%" --root "%ROOT%" --port %PORT% --open
