starfall_add_module(
    NAME engine_audio
    SOURCES
        music.cpp
        sound.cpp
        audio_system.cpp
    DEPENDS
        engine_core
        SDL3::SDL3
        SDL3_mixer::SDL3_mixer
)
