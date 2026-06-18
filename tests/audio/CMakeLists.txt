starfall_add_test(
    NAME engine_audio_tests
    SOURCES
        audio_system_tests.cpp
    DEPENDS
        engine_audio
        engine_core
        SDL3::SDL3
        SDL3_mixer::SDL3_mixer
)

target_include_directories(engine_audio_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/tests")
