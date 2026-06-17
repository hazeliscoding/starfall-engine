starfall_add_module(
    NAME engine_runtime
    SOURCES
        application.cpp
    DEPENDS
        engine_core
        engine_render
        engine_assets
        SDL3::SDL3
)
