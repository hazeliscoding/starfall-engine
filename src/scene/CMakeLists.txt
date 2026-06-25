starfall_add_module(
    NAME engine_scene
    SOURCES
        tile_set.cpp
        tilemap.cpp
        sweep.cpp
    DEPENDS
        engine_core
        engine_math
        engine_assets
)
