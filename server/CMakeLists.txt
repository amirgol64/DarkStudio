# DarkStudio server (C++20, Drogon): REST API + WebSocket events + serves the web UI.
# SPDX-License-Identifier: Apache-2.0
#
# Windows:
#   cmake -S server -B server/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake
#   cmake --build server/build --config Release

CMAKE_MINIMUM_REQUIRED (VERSION 3.24)
PROJECT (darkstudio-server VERSION 0.1.0 LANGUAGES CXX)

SET (CMAKE_CXX_STANDARD 20)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)

FIND_PACKAGE (Drogon CONFIG REQUIRED)

ADD_LIBRARY (darkstudio_core STATIC
	src/datasets.cpp
	src/events.cpp
	src/jobs.cpp
	src/parsers.cpp
	src/process.cpp
	src/settings.cpp
	src/system_info.cpp)
TARGET_INCLUDE_DIRECTORIES (darkstudio_core PUBLIC src)
TARGET_LINK_LIBRARIES (darkstudio_core PUBLIC Drogon::Drogon)

ADD_EXECUTABLE (darkstudio-server src/main.cpp)
TARGET_LINK_LIBRARIES (darkstudio-server PRIVATE darkstudio_core)

IF (MSVC)
	TARGET_COMPILE_OPTIONS (darkstudio_core PUBLIC /W4 /utf-8 /permissive-)
	TARGET_COMPILE_DEFINITIONS (darkstudio_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
ELSE ()
	TARGET_COMPILE_OPTIONS (darkstudio_core PUBLIC -Wall -Wextra)
ENDIF ()

# unit tests for the parsers and dataset path checks (no GPU or Darknet needed)
ENABLE_TESTING ()
ADD_EXECUTABLE (darkstudio-tests tests/tests.cpp)
TARGET_LINK_LIBRARIES (darkstudio-tests PRIVATE darkstudio_core)
ADD_TEST (NAME darkstudio-tests COMMAND darkstudio-tests)

INSTALL (TARGETS darkstudio-server RUNTIME DESTINATION bin)
