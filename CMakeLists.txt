cmake_minimum_required(VERSION 3.24)

# ---------------------------------------------------------------------------
# Windows: require the Visual Studio developer environment.
#
# Without it, cl.exe may be on PATH but the LINKER's LIB env var lacks the
# Windows SDK paths (kernel32.lib etc.), producing a confusing LNK1104.
# Fail loud and tell the user what to do.
# ---------------------------------------------------------------------------
if(CMAKE_HOST_WIN32 AND "$ENV{VSINSTALLDIR}" STREQUAL "")
    message(FATAL_ERROR
        "Visual Studio developer environment is not loaded.\n"
        "  cmake configure on Windows needs INCLUDE / LIB / PATH set up by\n"
        "  VsDevCmd.bat (otherwise the linker cannot find kernel32.lib).\n"
        "\n"
        "  Pick one:\n"
        "    - Open 'Developer PowerShell for VS' from the Start menu and re-run.\n"
        "    - Run scripts/dev-shell.ps1 from this repo to enter a dev shell here.\n"
        "    - In an existing PowerShell, run:\n"
        "        & 'C:/Program Files/Microsoft Visual Studio/<ver>/<edition>/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64\n"
        "\n"
        "  Then re-run: cmake --preset debug-windows")
endif()

project(StarfallEngine
    VERSION 0.1.0
    DESCRIPTION "Modular C++20 2D RPG game engine"
    LANGUAGES CXX)

# C++20 across the whole project, no compiler extensions.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put binaries together so a developer can find game_my_rpg without hunting.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Project options.
option(STARFALL_RUN_CMAKE_SELFTESTS
    "Run the StarfallModule rule-table self-test at configure time" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(StarfallModule)

# Mirror compile_commands.json to the repo root so editors (Zed/clangd, VS Code
# C/C++ extension, etc.) find it without having to be told the build dir.
# Symlink when possible; fall back to a copy (Windows without dev mode).
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COPY_ON_ERROR SYMBOLIC)
endif()

# ---------------------------------------------------------------------------
# Rule-table self-test (design D4 — fail-loud at configure time).
# ---------------------------------------------------------------------------
if(STARFALL_RUN_CMAKE_SELFTESTS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/tests/StarfallModuleTest.cmake"
        RESULT_VARIABLE _selftest_result
        OUTPUT_VARIABLE _selftest_output
        ERROR_VARIABLE  _selftest_output)
    if(NOT _selftest_result EQUAL 0)
        message(FATAL_ERROR
            "StarfallModule selftest failed:\n${_selftest_output}")
    endif()
    message(STATUS "StarfallModule selftest passed.")
endif()

# ---------------------------------------------------------------------------
# Third-party.
# ---------------------------------------------------------------------------
add_subdirectory(external)

# ---------------------------------------------------------------------------
# Engine modules. Order does not matter for correctness (CMake resolves), but
# we follow the dependency-graph order (DesignDoc §6.2) for readability.
# ---------------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/math)
add_subdirectory(src/assets)
add_subdirectory(src/render)
add_subdirectory(src/input)
add_subdirectory(src/audio)
add_subdirectory(src/scene)
add_subdirectory(src/scripting)
add_subdirectory(src/runtime)

# ---------------------------------------------------------------------------
# Executables.
# ---------------------------------------------------------------------------
add_subdirectory(src/editor)
add_subdirectory(games/my_rpg)
