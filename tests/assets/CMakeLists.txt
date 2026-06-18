starfall_add_test(
    NAME engine_assets_tests
    SOURCES
        asset_loader_tests.cpp
    DEPENDS
        engine_assets
        engine_core
        SDL3::SDL3
        SDL3_image::SDL3_image
)

# Tests need access to tests/common/sdl_fixture.hpp.
target_include_directories(engine_assets_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/tests")

# Place a fixture PNG next to the test executable. AssetLoader resolves
# paths against the exe directory (SDL_GetBasePath()), so the test can
# load "assets/characters/iden_placeholder.png" exactly the way game code
# does in production.
add_custom_command(TARGET engine_assets_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:engine_assets_tests>/assets/characters"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/games/my_rpg/assets/characters/iden_placeholder.png"
        "$<TARGET_FILE_DIR:engine_assets_tests>/assets/characters/iden_placeholder.png")
