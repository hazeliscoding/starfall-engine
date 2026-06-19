starfall_add_test(
    NAME engine_scene_tests
    SOURCES
        tilemap_tests.cpp
    DEPENDS
        engine_scene
        engine_assets
        engine_core
        SDL3::SDL3
)

target_include_directories(engine_scene_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/tests")
