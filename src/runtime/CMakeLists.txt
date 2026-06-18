starfall_add_module(
    NAME engine_runtime
    SOURCES
        application.cpp
    DEPENDS
        engine_core
        engine_render
        engine_assets
        engine_input
        SDL3::SDL3
)
