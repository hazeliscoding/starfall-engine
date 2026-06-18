#pragma once

#include "engine/audio/music.hpp"
#include "engine/audio/sound.hpp"
#include "engine/core/result.hpp"

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

struct MIX_Mixer;
struct MIX_Track;

namespace Engine::Audio {

// Audio system: one streaming music track + a 16-track pool for SFX
// one-shots, plus master/music/SFX volume knobs. Owned by engine_runtime.
//
// SDL3_mixer 3.2.x model:
//   - MIX_Mixer is the device. We own one.
//   - MIX_Track is a playable slot — bind a MIX_Audio to it and play.
//     We pre-allocate one dedicated music track + 16 SFX tracks.
//   - MIX_Audio is the decoded sound data. Music is loaded with
//     predecode=false (streamed); SFX is loaded with predecode=true
//     (preloaded into memory for fast triggers).
//
// Init failure does NOT crash — the system enters "disabled" state
// where Load* returns Err and Play* / Stop* / Pause* / Resume* are
// silent no-ops. The game continues running, just silent.
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&)            = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&)                 = delete;
    AudioSystem& operator=(AudioSystem&&)      = delete;

    bool IsValid() const noexcept { return enabled_; }

    // ---------- Loading ----------

    Engine::Core::Result<MusicHandle> LoadMusic(std::string_view relativePath);
    Engine::Core::Result<SoundHandle> LoadSound(std::string_view relativePath);

    // ---------- Music ----------

    void PlayMusic(const MusicHandle& music,
                   bool  loop             = true,
                   float fadeInSeconds    = 0.0f,
                   float loopPointSeconds = 0.0f);

    void StopMusic(float fadeOutSeconds = 0.0f);
    void PauseMusic();
    void ResumeMusic();
    bool IsMusicPlaying() const;

    // ---------- SFX ----------

    void PlaySound(const SoundHandle& sound);

    // ---------- Volume (all values clamped to [0, 1]) ----------

    void SetMasterVolume(float v);
    void SetMusicVolume(float v);
    void SetSfxVolume(float v);
    float GetMasterVolume() const noexcept { return master_; }
    float GetMusicVolume()  const noexcept { return music_;  }
    float GetSfxVolume()    const noexcept { return sfx_;    }

private:
    template <typename V>
    using PathMap = std::unordered_map<std::string, std::weak_ptr<V>>;

    static constexpr int kSfxTracks = 16;

    void ApplyMusicGain();  // pushes master*music to the music track
    void ApplySfxGain();    // pushes master*sfx to every SFX track

    bool        enabled_ = false;
    std::string basePath_;

    float master_ = 1.0f;
    float music_  = 1.0f;
    float sfx_    = 1.0f;

    MIX_Mixer*                       mixer_      = nullptr;
    MIX_Track*                       musicTrack_ = nullptr;
    std::array<MIX_Track*, kSfxTracks> sfxTracks_{};

    PathMap<Music> musicCache_;
    PathMap<Sound> soundCache_;
};

}  // namespace Engine::Audio
