starfall_add_test(
    NAME engine_input_tests
    SOURCES
        action_map_tests.cpp
        input_state_tests.cpp
    DEPENDS
        engine_input
        SDL3::SDL3
)
