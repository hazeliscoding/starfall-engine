// tests/audio/audio_system_tests.cpp
//
// Covers AudioSystem lifecycle, volume API, and error paths. Uses SDL's
// "dummy" audio driver so the test runs on machines without audio
// hardware (future CI). Dummy driver simulates the API surface without
// producing actual sound — we don't (can't) verify audio is audible
// here; that's manual runtime smoke (Task 9 in audio-foundation tasks).

#include "engine/audio/audio_system.hpp"

#include <SDL3/SDL.h>

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

// Forces SDL to use the dummy audio driver before any AudioSystem
// constructs. Set once at process start via Catch2's main hook.
struct DummyAudioDriver {
    DummyAudioDriver() {
        SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy",
                                SDL_HINT_OVERRIDE);
    }
};

const DummyAudioDriver _force_dummy{};

}  // namespace

TEST_CASE("AudioSystem: constructs cleanly under the dummy driver",
          "[audio][lifecycle]") {
    Engine::Audio::AudioSystem audio;
    // Under the dummy driver, Mix_OpenAudio should succeed.
    CHECK(audio.IsValid());
}

TEST_CASE("AudioSystem: default volumes are all 1.0",
          "[audio][volume]") {
    Engine::Audio::AudioSystem audio;
    CHECK(audio.GetMasterVolume() == 1.0f);
    CHECK(audio.GetMusicVolume()  == 1.0f);
    CHECK(audio.GetSfxVolume()    == 1.0f);
}

TEST_CASE("AudioSystem: volume setters round-trip", "[audio][volume]") {
    Engine::Audio::AudioSystem audio;
    audio.SetMasterVolume(0.7f);
    audio.SetMusicVolume(0.5f);
    audio.SetSfxVolume(0.25f);
    CHECK(audio.GetMasterVolume() == 0.7f);
    CHECK(audio.GetMusicVolume()  == 0.5f);
    CHECK(audio.GetSfxVolume()    == 0.25f);
}

TEST_CASE("AudioSystem: volume below 0 clamps to 0", "[audio][volume]") {
    Engine::Audio::AudioSystem audio;
    audio.SetMasterVolume(-0.5f);
    CHECK(audio.GetMasterVolume() == 0.0f);
}

TEST_CASE("AudioSystem: volume above 1 clamps to 1", "[audio][volume]") {
    Engine::Audio::AudioSystem audio;
    audio.SetMasterVolume(2.0f);
    CHECK(audio.GetMasterVolume() == 1.0f);
}

TEST_CASE("AudioSystem: LoadSound with missing path returns Err",
          "[audio][loading]") {
    Engine::Audio::AudioSystem audio;
    auto result = audio.LoadSound("assets/audio/does_not_exist.wav");
    REQUIRE(result.IsErr());
    // Error message should include the path so a developer can trace it.
    CHECK(result.Error().find("does_not_exist.wav") != std::string::npos);
}

TEST_CASE("AudioSystem: LoadMusic with missing path returns Err",
          "[audio][loading]") {
    Engine::Audio::AudioSystem audio;
    auto result = audio.LoadMusic("assets/audio/does_not_exist.ogg");
    REQUIRE(result.IsErr());
    CHECK(result.Error().find("does_not_exist.ogg") != std::string::npos);
}

TEST_CASE("AudioSystem: PlayMusic with null handle is a no-op",
          "[audio][playback]") {
    Engine::Audio::AudioSystem audio;
    Engine::Audio::MusicHandle null_handle;
    REQUIRE_NOTHROW(audio.PlayMusic(null_handle));
    REQUIRE_NOTHROW(audio.StopMusic());
    CHECK_FALSE(audio.IsMusicPlaying());
}

TEST_CASE("AudioSystem: PlaySound with null handle is a no-op",
          "[audio][playback]") {
    Engine::Audio::AudioSystem audio;
    Engine::Audio::SoundHandle null_handle;
    REQUIRE_NOTHROW(audio.PlaySound(null_handle));
}

TEST_CASE("AudioSystem: Pause/Resume on no music is safe",
          "[audio][playback]") {
    Engine::Audio::AudioSystem audio;
    REQUIRE_NOTHROW(audio.PauseMusic());
    REQUIRE_NOTHROW(audio.ResumeMusic());
}
