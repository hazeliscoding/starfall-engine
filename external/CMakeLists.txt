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
set(STARFALL_SDL3_GIT_TAG "release-3.4.10" CACHE STRING
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

# SDL3 3.4.x added a required dep on libxtst-dev for XTest. We don't
# use XTest (it's for synthesizing X11 input events from a remote
# session — irrelevant for a game runtime). Disable so contributors
# don't have to apt-install yet another -dev package.
set(SDL_X11_XTEST OFF CACHE BOOL "Build SDL3 with X11 XTest support" FORCE)

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
set(STARFALL_SDL3_IMAGE_GIT_TAG "release-3.4.4" CACHE STRING
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

# ---------------------------------------------------------------------------
# Catch2 v3 (test framework, CTest integration)
# ---------------------------------------------------------------------------
set(STARFALL_CATCH2_GIT_TAG "v3.7.1" CACHE STRING
    "Catch2 release tag to fetch and build")

# Static linking; don't build the framework's own self-tests or docs.
set(CATCH_INSTALL_DOCS    OFF CACHE BOOL "Install Catch2 docs"     FORCE)
set(CATCH_INSTALL_EXTRAS  OFF CACHE BOOL "Install Catch2 extras"   FORCE)
set(CATCH_BUILD_TESTING   OFF CACHE BOOL "Build Catch2's own tests" FORCE)
set(CATCH_BUILD_EXAMPLES  OFF CACHE BOOL "Build Catch2 examples"   FORCE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        ${STARFALL_CATCH2_GIT_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# Catch2 ships a CMake module with catch_discover_tests; make it findable.
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)

# ---------------------------------------------------------------------------
# SDL3_mixer (audio: music streaming + SFX channels)
# ---------------------------------------------------------------------------
# Pinned to release-3.2.4 — same release line as SDL3_image, confirmed
# stable via `git ls-remote --tags` (2026-06-18). If SDL3_mixer ever
# goes ABI-incompatible with our pinned SDL3 (3.2.16), bump both at once.
set(STARFALL_SDL3_MIXER_GIT_TAG "release-3.2.4" CACHE STRING
    "SDL3_mixer release tag to fetch and build")

# Static + vendored dependencies (one binary for distribution).
# SDL3_mixer uses SDLMIXER_ prefix for its options (not SDL3MIXER_).
set(SDLMIXER_VENDORED ON  CACHE BOOL "Use SDL3_mixer's vendored decoders" FORCE)
set(SDLMIXER_INSTALL  OFF CACHE BOOL "Generate SDL3_mixer install rules"  FORCE)
set(SDLMIXER_SAMPLES  OFF CACHE BOOL "Build SDL3_mixer samples"           FORCE)

# Trim format support to what M2.75 needs: Ogg Vorbis (music) + WAV (SFX).
# Disabled formats keep their decoders out of the binary entirely.
set(SDLMIXER_FLAC    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3     OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS    OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS  ON  CACHE BOOL "" FORCE)
set(SDLMIXER_WAVE    ON  CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME     OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG        ${STARFALL_SDL3_MIXER_GIT_TAG}
    GIT_SHALLOW    TRUE
)

# Same BUILD_SHARED_LIBS scope trick as SDL3_image — SDL3_mixer's
# CMakeLists also keys its target naming off BUILD_SHARED_LIBS.
set(_starfall_saved_BSL2 "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(SDL3_mixer)
set(BUILD_SHARED_LIBS "${_starfall_saved_BSL2}")
unset(_starfall_saved_BSL2)

# Normalize alias the same way we do for SDL3 + SDL3_image.
if(NOT TARGET SDL3_mixer::SDL3_mixer)
    if(TARGET SDL3_mixer::SDL3_mixer-static)
        add_library(SDL3_mixer::SDL3_mixer ALIAS SDL3_mixer-static)
    elseif(TARGET SDL3_mixer-static)
        add_library(SDL3_mixer::SDL3_mixer ALIAS SDL3_mixer-static)
    elseif(TARGET SDL3_mixer)
        add_library(SDL3_mixer::SDL3_mixer ALIAS SDL3_mixer)
    else()
        message(FATAL_ERROR
            "external: SDL3_mixer was fetched but no SDL3_mixer::SDL3_mixer "
            "or SDL3_mixer-static target was produced. Did the release tag "
            "change its output names?")
    endif()
endif()
