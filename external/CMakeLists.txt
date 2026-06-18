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

# ---------------------------------------------------------------------------
# SDL3_image
# ---------------------------------------------------------------------------
# Official SDL companion library for image loading (PNG/JPEG/etc).
# Pinned to a stable release; bump this one variable to upgrade.
set(STARFALL_SDL3_IMAGE_GIT_TAG "release-3.2.4" CACHE STRING
    "SDL3_image release tag to fetch and build")

# SDL3_image's user options use the SDLIMAGE_ prefix (no "3" — yes, it's
# inconsistent with the library name; check the upstream CMakeLists.txt).
# Static linking matches SDL3 (one binary for distribution).
set(SDLIMAGE_SAMPLES         OFF CACHE BOOL "Build SDL3_image samples"          FORCE)
set(SDLIMAGE_VENDORED        ON  CACHE BOOL "Use SDL3_image's vendored libpng/libjpeg" FORCE)
set(SDLIMAGE_INSTALL         OFF CACHE BOOL "Generate SDL3_image install rules" FORCE)
set(SDLIMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "Build SDL3_image shared"         FORCE)
# Only enable formats we actually use at M1; turning off the rest trims
# binary size and configure time. Enable more as milestones need them.
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BMP  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_GIF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_LBM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PCX  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG  ON  CACHE BOOL "" FORCE)
set(SDLIMAGE_PNM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_QOI  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SVG  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TGA  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XCF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XPM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XV   OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        ${STARFALL_SDL3_IMAGE_GIT_TAG}
    GIT_SHALLOW    TRUE
)

# SDL3_image's CMakeLists reads BUILD_SHARED_LIBS to decide which SDL3
# target name to bind (SDL3::SDL3 vs SDL3::SDL3-shared). Force it OFF for
# the SDL3_image scope only, save/restore so we don't perturb the rest of
# the project. Without this, SDL3_image may fail to find SDL3::SDL3-shared
# (which we don't build) and fall back to find_package(SDL3) — which fails
# because we don't install SDL3.
set(_starfall_saved_BSL "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(SDL3_image)
set(BUILD_SHARED_LIBS "${_starfall_saved_BSL}")
unset(_starfall_saved_BSL)

# Same alias-normalization dance as SDL3.
if(NOT TARGET SDL3_image::SDL3_image)
    if(TARGET SDL3_image::SDL3_image-static)
        add_library(SDL3_image::SDL3_image ALIAS SDL3_image-static)
    elseif(TARGET SDL3_image-static)
        add_library(SDL3_image::SDL3_image ALIAS SDL3_image-static)
    elseif(TARGET SDL3_image)
        add_library(SDL3_image::SDL3_image ALIAS SDL3_image)
    else()
        message(FATAL_ERROR
            "external: SDL3_image was fetched but no SDL3_image::SDL3_image or "
            "SDL3_image-static target was produced. Did the release tag change "
            "its output names?")
    endif()
endif()
