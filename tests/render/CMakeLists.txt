starfall_add_test(
    NAME engine_render_tests
    SOURCES
        renderer_tests.cpp
        animated_sprite_tests.cpp
    DEPENDS
        engine_render
        engine_assets
        engine_core
        SDL3::SDL3
        SDL3_image::SDL3_image
)

target_include_directories(engine_render_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/tests")

# AnimatedSprite "real handle" test needs the placeholder PNG to load.
# Mirror it the same way the assets test target does.
add_custom_command(TARGET engine_render_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:engine_render_tests>/assets/characters"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/games/my_rpg/assets/characters/iden_placeholder.png"
        "$<TARGET_FILE_DIR:engine_render_tests>/assets/characters/iden_placeholder.png")
