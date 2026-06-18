starfall_add_module(
    NAME engine_input
    SOURCES
        action_map.cpp
        input_state.cpp
    DEPENDS
        engine_core
        SDL3::SDL3
)
