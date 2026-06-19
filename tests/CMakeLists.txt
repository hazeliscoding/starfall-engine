# tests/CMakeLists.txt
#
# Per-module test targets live in tests/<module>/CMakeLists.txt. List them
# here as they come online; the IF guard keeps this resilient if a module's
# tests get temporarily removed.

foreach(_test_dir IN ITEMS input assets render audio scene)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_test_dir}/CMakeLists.txt")
        add_subdirectory(${_test_dir})
    endif()
endforeach()
