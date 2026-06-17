# StarfallModuleTest.cmake
#
# Configure-time self-test for the rule table in StarfallModule.cmake.
#
# Invoked via `cmake -P` from the root CMakeLists.txt when
# STARFALL_RUN_CMAKE_SELFTESTS is ON. This script does NOT create any targets;
# it loads StarfallModule.cmake and calls _starfall_check_edge directly to
# verify that forbidden combinations are flagged and allowed ones are not.

cmake_minimum_required(VERSION 3.24)

# Locate StarfallModule.cmake relative to this script so it works regardless
# of where cmake -P is invoked from.
get_filename_component(_self_dir "${CMAKE_SCRIPT_MODE_FILE}" DIRECTORY)
include("${_self_dir}/../StarfallModule.cmake")

set(_failures 0)

function(_expect_violation from dep label)
    _starfall_check_edge("${from}" "${dep}")
    if(_starfall_violation_msg STREQUAL "")
        message(SEND_ERROR
            "SELFTEST FAIL [${label}]: expected (${from} -> ${dep}) to be flagged as forbidden, but rule table allowed it.")
        math(EXPR _failures "${_failures} + 1")
        set(_failures "${_failures}" PARENT_SCOPE)
    else()
        message(STATUS "SELFTEST ok   [${label}]: ${from} -> ${dep} correctly flagged (${_starfall_violation_msg})")
    endif()
endfunction()

function(_expect_allowed from dep label)
    _starfall_check_edge("${from}" "${dep}")
    if(NOT _starfall_violation_msg STREQUAL "")
        message(SEND_ERROR
            "SELFTEST FAIL [${label}]: expected (${from} -> ${dep}) to be allowed, but rule table flagged: ${_starfall_violation_msg}")
        math(EXPR _failures "${_failures} + 1")
        set(_failures "${_failures}" PARENT_SCOPE)
    else()
        message(STATUS "SELFTEST ok   [${label}]: ${from} -> ${dep} correctly allowed")
    endif()
endfunction()

# Forbidden combinations (must trigger).
_expect_violation(engine_core   engine_render   "core->render forbidden")
_expect_violation(engine_core   engine_input    "core->input forbidden")
_expect_violation(engine_core   game_my_rpg     "core->game forbidden")
_expect_violation(engine_render game_my_rpg     "engine->game forbidden")
_expect_violation(engine_render engine_editor   "engine->editor forbidden")
_expect_violation(engine_runtime engine_editor  "runtime->editor forbidden")
_expect_violation(game_my_rpg   engine_editor   "game->editor forbidden")

# Allowed combinations (must NOT trigger).
_expect_allowed(engine_runtime  engine_core      "runtime->core allowed")
_expect_allowed(engine_render   engine_core      "render->core allowed")
_expect_allowed(engine_render   engine_math      "render->math allowed")
_expect_allowed(engine_runtime  SDL3::SDL3       "runtime->SDL3 allowed (third-party)")
_expect_allowed(game_my_rpg     engine_runtime   "game->runtime allowed")

if(NOT _failures EQUAL 0)
    message(FATAL_ERROR "StarfallModule selftest: ${_failures} failure(s).")
endif()
message(STATUS "StarfallModule selftest: all rules verified.")
