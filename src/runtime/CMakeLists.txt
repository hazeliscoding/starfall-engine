starfall_add_module(
    NAME engine_runtime
    SOURCES
        application.cpp
    DEPENDS
        engine_core
        SDL3::SDL3
)
