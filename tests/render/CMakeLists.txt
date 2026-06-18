starfall_add_test(
    NAME engine_render_tests
    SOURCES
        renderer_tests.cpp
    DEPENDS
        engine_render
        engine_assets
        engine_core
        SDL3::SDL3
)

target_include_directories(engine_render_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/tests")
