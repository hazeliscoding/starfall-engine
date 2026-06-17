starfall_add_module(
    NAME engine_editor
    TYPE executable
    SOURCES
        main.cpp
    DEPENDS
        # Empty by design (D9). Editor links engine modules once it has real
        # functionality (M5+).
)
