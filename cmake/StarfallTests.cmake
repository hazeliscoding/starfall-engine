# StarfallTests.cmake
#
# Provides starfall_add_test(): the single entry point for registering
# Catch2-based unit tests. Mirrors starfall_add_module's shape so adding
# a test target feels familiar.
#
# Usage:
#   starfall_add_test(
#       NAME engine_input_tests
#       SOURCES action_map_tests.cpp input_state_tests.cpp
#       DEPENDS engine_input
#   )

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/StarfallWarnings.cmake)
include(Catch)  # catch_discover_tests — surfaced by external/CMakeLists.txt

function(starfall_add_test)
    set(options)
    set(one_value_args NAME)
    set(multi_value_args SOURCES DEPENDS)
    cmake_parse_arguments(SF "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT SF_NAME)
        message(FATAL_ERROR "starfall_add_test: NAME is required")
    endif()
    if(NOT SF_SOURCES)
        message(FATAL_ERROR "starfall_add_test(${SF_NAME}): SOURCES is required")
    endif()

    add_executable(${SF_NAME} ${SF_SOURCES})
    target_compile_features(${SF_NAME} PRIVATE cxx_std_20)
    target_link_libraries(${SF_NAME} PRIVATE Catch2::Catch2WithMain ${SF_DEPENDS})

    # Tests need to see the engine's public headers.
    target_include_directories(${SF_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")

    starfall_set_warnings(${SF_NAME})

    set_target_properties(${SF_NAME} PROPERTIES FOLDER "tests")

    # Register every TEST_CASE as a separate CTest entry.
    catch_discover_tests(${SF_NAME})
endfunction()
