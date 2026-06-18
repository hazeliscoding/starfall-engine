starfall_add_module(
    NAME engine_render
    SOURCES
        renderer.cpp
        animated_sprite.cpp
    DEPENDS
        engine_core
        engine_math
        engine_assets
        SDL3::SDL3
)
