# StarfallWarnings.cmake
#
# Single source of truth for compile warnings across every Starfall target.
# Called from starfall_add_module; you should not call this directly.

include_guard(GLOBAL)

function(starfall_set_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "starfall_set_warnings: '${target}' is not a target.")
    endif()

    if(MSVC)
        # /W4 high warning level; /WX warnings-as-errors in Debug.
        # /permissive- enforces stricter standard conformance.
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        # Silence noisy warnings that don't help us:
        #   C4100 unreferenced formal parameter (common with stub functions during bootstrap)
        #   C4324 structure was padded due to alignment specifier (harmless, fires on Vec2 etc.)
        target_compile_options(${target} PRIVATE /wd4100 /wd4324)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        # gcc / clang
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor
            -Wcast-align -Wunused -Woverloaded-virtual
            -Wdouble-promotion)
        # Silenced (rationale per flag):
        #   -Wno-unused-parameter           stub functions during bootstrap.
        #   -Wno-missing-field-initializers C++20 designated initializers with
        #                                   defaulted aggregate members are
        #                                   intentional (see AppConfig usage).
        # Not enabled (would fire inside third-party headers):
        #   -Wold-style-cast    SDL3 macros (SDL_min/SDL_max) expand to C-style
        #                       casts in user code.
        #   -Wconversion,
        #   -Wsign-conversion   would need wholesale audit of SDL3 macro
        #                       expansions; reconsider when we have a
        #                       reason to want them.
        target_compile_options(${target} PRIVATE
            -Wno-unused-parameter
            -Wno-missing-field-initializers)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
