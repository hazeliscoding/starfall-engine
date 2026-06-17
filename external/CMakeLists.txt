# external/CMakeLists.txt
#
# Single point of acquisition for every third-party dependency. Per the
# build-system spec, no engine module may consume a system-installed
# library — everything flows through here.

include(FetchContent)

# ---------------------------------------------------------------------------
# SDL3
# ---------------------------------------------------------------------------
# Pinned to a stable SDL3 release tag. To bump SDL3, change this one variable.
set(STARFALL_SDL3_GIT_TAG "release-3.2.16" CACHE STRING
    "SDL3 release tag to fetch and build")

# Static vs shared: STATIC is friendlier for game distribution on Steam
# (one binary, no missing-DLL support tickets). Switch to SHARED if a
# license review later demands it.
set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_STATIC ON  CACHE BOOL "Build SDL3 as a static library" FORCE)

# Trim subsystems we do not need at M0.
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL3 test library" FORCE)
set(SDL_TESTS        OFF CACHE BOOL "Build SDL3 tests"        FORCE)
set(SDL_EXAMPLES     OFF CACHE BOOL "Build SDL3 examples"     FORCE)
set(SDL_INSTALL      OFF CACHE BOOL "Generate SDL3 install rules" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${STARFALL_SDL3_GIT_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# SDL3's CMake exports SDL3::SDL3-static and SDL3::SDL3 (alias) — but the
# alias only exists when SDL_SHARED is on. Normalize so the rest of the
# project can always link against SDL3::SDL3 regardless.
if(NOT TARGET SDL3::SDL3)
    if(TARGET SDL3::SDL3-static)
        add_library(SDL3::SDL3 ALIAS SDL3-static)
    elseif(TARGET SDL3-static)
        add_library(SDL3::SDL3 ALIAS SDL3-static)
    else()
        message(FATAL_ERROR
            "external: SDL3 was fetched but no SDL3::SDL3 or SDL3::SDL3-static "
            "target was produced. Did the SDL3 release tag change its output names?")
    endif()
endif()
