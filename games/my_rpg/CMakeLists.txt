starfall_add_module(
    NAME game_my_rpg
    TYPE executable
    SOURCES
        src/main.cpp
    DEPENDS
        engine_runtime
)

# Mirror source assets into the runtime output dir alongside the
# executable, after every build (texture-assets spec). The runtime resolves
# asset paths against SDL_GetBasePath() so this dev layout matches the
# packaged layout from M9.
add_custom_command(TARGET game_my_rpg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:game_my_rpg>/assets"
    COMMENT "Mirroring game assets to $<TARGET_FILE_DIR:game_my_rpg>/assets")
