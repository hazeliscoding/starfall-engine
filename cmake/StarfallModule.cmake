# StarfallModule.cmake
#
# Provides starfall_add_module(): the single entry point through which every
# engine module, game, and editor target is declared. Enforces the dependency
# rules from docs/DesignDoc.md §6.3 at configure time.
#
# If §6.3 changes, update FORBIDDEN_EDGES below in the same commit.

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/StarfallWarnings.cmake)

# ---------------------------------------------------------------------------
# Forbidden-dependency rules (mirror of DesignDoc §6.3).
#
# Format: each entry is "<from>:<to-regex>:<human-readable-rule>".
# A target named <from> may not declare any DEPENDS that matches <to-regex>.
# ---------------------------------------------------------------------------
set(STARFALL_FORBIDDEN_EDGES
    "engine_core:^engine_(render|input|audio|scene|scripting|runtime)$:engine_core MUST NOT depend on render/input/audio/scene/scripting/runtime"
    "engine_math:^engine_(render|input|audio|scene|scripting|runtime)$:engine_math is a leaf utility and MUST NOT depend on higher-level engine modules"
    "engine_assets:^engine_(render|input|audio|scene|scripting|runtime)$:engine_assets MUST NOT depend on higher-level engine modules"
    # Engine modules may never depend on game or editor code.
    "engine_core:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_math:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_assets:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_render:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_input:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_audio:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_scene:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_scripting:^(game_|engine_editor).*:engine_* targets MUST NOT depend on game_* or engine_editor"
    "engine_runtime:^(game_|engine_editor).*:engine_runtime MUST NOT depend on game_* or engine_editor"
    # Game executables may not link the editor.
    "game_my_rpg:^engine_editor$:game executables MUST NOT depend on engine_editor (editor is optional at runtime)"
)

# Internal helper: validates one (from, dep) pair against STARFALL_FORBIDDEN_EDGES.
# Sets _starfall_violation_msg in parent scope on violation, empty otherwise.
function(_starfall_check_edge from dep)
    set(_starfall_violation_msg "" PARENT_SCOPE)
    foreach(edge IN LISTS STARFALL_FORBIDDEN_EDGES)
        string(REGEX MATCH "^([^:]+):([^:]+):(.+)$" _m "${edge}")
        if(NOT _m)
            continue()
        endif()
        set(rule_from "${CMAKE_MATCH_1}")
        set(rule_to_regex "${CMAKE_MATCH_2}")
        set(rule_msg "${CMAKE_MATCH_3}")
        if(NOT from STREQUAL rule_from)
            continue()
        endif()
        if(dep MATCHES "${rule_to_regex}")
            set(_starfall_violation_msg "${rule_msg} (target '${from}' may not depend on '${dep}')" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Internal helper: returns the expected /src/<suffix> for an engine module.
function(_starfall_module_src_dir module out_var)
    if(module MATCHES "^engine_(.+)$")
        set(${out_var} "${CMAKE_SOURCE_DIR}/src/${CMAKE_MATCH_1}" PARENT_SCOPE)
    elseif(module MATCHES "^game_(.+)$")
        set(${out_var} "${CMAKE_SOURCE_DIR}/games/${CMAKE_MATCH_1}/src" PARENT_SCOPE)
    elseif(module STREQUAL "engine_editor")
        set(${out_var} "${CMAKE_SOURCE_DIR}/src/editor" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

# starfall_add_module(
#     NAME    <target-name>      # e.g. engine_runtime, game_my_rpg, engine_editor
#     TYPE    <library|executable>  # defaults to library for engine_*, executable for game_*/engine_editor
#     SOURCES <files...>
#     PUBLIC_INCLUDES <dirs...>
#     DEPENDS <targets...>
# )
function(starfall_add_module)
    set(options)
    set(one_value_args NAME TYPE)
    set(multi_value_args SOURCES PUBLIC_INCLUDES DEPENDS)
    cmake_parse_arguments(SF "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT SF_NAME)
        message(FATAL_ERROR "starfall_add_module: NAME is required")
    endif()

    # Validate name prefix.
    if(NOT (SF_NAME MATCHES "^engine_" OR SF_NAME MATCHES "^game_" OR SF_NAME STREQUAL "engine_editor"))
        message(FATAL_ERROR
            "starfall_add_module: target name '${SF_NAME}' must start with 'engine_' or 'game_' "
            "(or be exactly 'engine_editor').")
    endif()

    # Infer type from naming convention if not provided.
    if(NOT SF_TYPE)
        if(SF_NAME MATCHES "^engine_" AND NOT SF_NAME STREQUAL "engine_editor")
            set(SF_TYPE "library")
        else()
            set(SF_TYPE "executable")
        endif()
    endif()
    if(NOT (SF_TYPE STREQUAL "library" OR SF_TYPE STREQUAL "executable"))
        message(FATAL_ERROR "starfall_add_module: TYPE must be 'library' or 'executable', got '${SF_TYPE}'.")
    endif()

    # For engine static libraries we require a matching /src/<suffix> dir.
    if(SF_TYPE STREQUAL "library" AND NOT STARFALL_SKIP_SRC_DIR_CHECK)
        _starfall_module_src_dir("${SF_NAME}" expected_src)
        if(expected_src AND NOT IS_DIRECTORY "${expected_src}")
            message(FATAL_ERROR
                "starfall_add_module(${SF_NAME}): expected source directory '${expected_src}' does not exist. "
                "Module targets MUST own one /src/<name>/ subdirectory (build-system spec).")
        endif()
    endif()

    # Enforce DesignDoc §6.3 dependency rules.
    foreach(dep IN LISTS SF_DEPENDS)
        _starfall_check_edge("${SF_NAME}" "${dep}")
        if(NOT _starfall_violation_msg STREQUAL "")
            message(FATAL_ERROR
                "starfall_add_module(${SF_NAME}): forbidden dependency on '${dep}'.\n"
                "  Rule: ${_starfall_violation_msg}\n"
                "  See docs/DesignDoc.md §6.3.")
        endif()
    endforeach()

    # Create the target.
    if(SF_TYPE STREQUAL "library")
        add_library(${SF_NAME} STATIC ${SF_SOURCES})
    else()
        add_executable(${SF_NAME} ${SF_SOURCES})
    endif()

    # Public include dirs default to /include for engine modules so that
    # consumers do `#include <engine/runtime/application.hpp>`.
    if(NOT SF_PUBLIC_INCLUDES AND SF_NAME MATCHES "^engine_" AND NOT SF_NAME STREQUAL "engine_editor")
        set(SF_PUBLIC_INCLUDES "${CMAKE_SOURCE_DIR}/include")
    endif()
    foreach(inc IN LISTS SF_PUBLIC_INCLUDES)
        target_include_directories(${SF_NAME} PUBLIC ${inc})
    endforeach()

    # Wire dependencies (already validated).
    if(SF_DEPENDS)
        target_link_libraries(${SF_NAME} PUBLIC ${SF_DEPENDS})
    endif()

    # Standard + warnings.
    target_compile_features(${SF_NAME} PUBLIC cxx_std_20)
    starfall_set_warnings(${SF_NAME})

    set_target_properties(${SF_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "${SF_NAME}")
endfunction()
