starfall_add_module(
    NAME engine_assets
    SOURCES
        texture.cpp
        asset_loader.cpp
    DEPENDS
        engine_core
        SDL3::SDL3
        SDL3_image::SDL3_image
)
