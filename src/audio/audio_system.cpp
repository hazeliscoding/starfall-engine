#include "engine/audio/audio_system.hpp"

#include "engine/core/logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <utility>

namespace Engine::Audio {

namespace {

std::string CaptureBasePath() {
    const char* base = SDL_GetBasePath();
    if (base == nullptr) {
        SF_LOG_WARN("Audio", "SDL_GetBasePath returned null: %s", SDL_GetError());
        return {};
    }
    return std::string(base);
}

float Clamp01(float v) noexcept {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

}  // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

AudioSystem::AudioSystem() : basePath_(CaptureBasePath()) {
    if (!MIX_Init()) {
        SF_LOG_ERROR("Audio", "MIX_Init failed (audio disabled): %s", SDL_GetError());
        return;
    }

    // MIX_CreateMixerDevice opens an SDL audio device and creates a mixer
    // for it. SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK is the right "default
    // device" sentinel (NOT 0 — 0 is "invalid"). spec=nullptr means use
    // the device's preferred format. MIX_CreateMixer (no device arg)
    // would be a different operation and *requires* a non-null spec.
    mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (mixer_ == nullptr) {
        SF_LOG_ERROR("Audio",
            "MIX_CreateMixerDevice failed (audio disabled): %s", SDL_GetError());
        MIX_Quit();
        return;
    }

    musicTrack_ = MIX_CreateTrack(mixer_);
    if (musicTrack_ == nullptr) {
        SF_LOG_ERROR("Audio", "MIX_CreateTrack(music) failed: %s", SDL_GetError());
        MIX_DestroyMixer(mixer_);
        mixer_ = nullptr;
        MIX_Quit();
        return;
    }

    for (int i = 0; i < kSfxTracks; ++i) {
        sfxTracks_[i] = MIX_CreateTrack(mixer_);
        if (sfxTracks_[i] == nullptr) {
            SF_LOG_ERROR("Audio",
                "MIX_CreateTrack(sfx %d) failed: %s", i, SDL_GetError());
            // Partial-init cleanup.
            for (int j = 0; j < i; ++j) MIX_DestroyTrack(sfxTracks_[j]);
            MIX_DestroyTrack(musicTrack_);
            MIX_DestroyMixer(mixer_);
            mixer_ = nullptr;
            musicTrack_ = nullptr;
            MIX_Quit();
            return;
        }
    }

    enabled_ = true;
    SF_LOG_INFO("Audio", "AudioSystem ready (%d SFX tracks)", kSfxTracks);
}

AudioSystem::~AudioSystem() {
    if (!enabled_) return;

    if (musicTrack_) MIX_StopTrack(musicTrack_, /*fade out frames*/ 0);
    for (auto* t : sfxTracks_) if (t) MIX_StopTrack(t, 0);

    musicCache_.clear();
    soundCache_.clear();

    for (auto* t : sfxTracks_) if (t) MIX_DestroyTrack(t);
    if (musicTrack_) MIX_DestroyTrack(musicTrack_);
    if (mixer_) MIX_DestroyMixer(mixer_);
    MIX_Quit();
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

Engine::Core::Result<MusicHandle>
AudioSystem::LoadMusic(std::string_view relativePath) {
    if (!enabled_) {
        return Engine::Core::Result<MusicHandle>::Err(
            "AudioSystem is disabled (audio init failed)");
    }

    std::string key(relativePath);

    if (auto it = musicCache_.find(key); it != musicCache_.end()) {
        if (auto shared = it->second.lock()) {
            return Engine::Core::Result<MusicHandle>::Ok(std::move(shared));
        }
        musicCache_.erase(it);
    }

    std::string fullPath = basePath_ + key;
    // predecode=false → streamed during playback (right call for long music).
    MIX_Audio* raw = MIX_LoadAudio(mixer_, fullPath.c_str(), /*predecode*/ false);
    if (raw == nullptr) {
        std::string err = "MIX_LoadAudio (music) failed for '" + fullPath
                          + "': " + SDL_GetError();
        SF_LOG_ERROR("Audio", "%s", err.c_str());
        return Engine::Core::Result<MusicHandle>::Err(std::move(err));
    }

    auto handle = std::make_shared<Music>(raw);
    musicCache_[key] = handle;
    SF_LOG_INFO("Audio", "Loaded music '%s'", fullPath.c_str());
    return Engine::Core::Result<MusicHandle>::Ok(std::move(handle));
}

Engine::Core::Result<SoundHandle>
AudioSystem::LoadSound(std::string_view relativePath) {
    if (!enabled_) {
        return Engine::Core::Result<SoundHandle>::Err(
            "AudioSystem is disabled (audio init failed)");
    }

    std::string key(relativePath);

    if (auto it = soundCache_.find(key); it != soundCache_.end()) {
        if (auto shared = it->second.lock()) {
            return Engine::Core::Result<SoundHandle>::Ok(std::move(shared));
        }
        soundCache_.erase(it);
    }

    std::string fullPath = basePath_ + key;
    // predecode=true → fully loaded into memory (fast trigger for SFX).
    MIX_Audio* raw = MIX_LoadAudio(mixer_, fullPath.c_str(), /*predecode*/ true);
    if (raw == nullptr) {
        std::string err = "MIX_LoadAudio (sound) failed for '" + fullPath
                          + "': " + SDL_GetError();
        SF_LOG_ERROR("Audio", "%s", err.c_str());
        return Engine::Core::Result<SoundHandle>::Err(std::move(err));
    }

    auto handle = std::make_shared<Sound>(raw);
    soundCache_[key] = handle;
    SF_LOG_INFO("Audio", "Loaded sound '%s'", fullPath.c_str());
    return Engine::Core::Result<SoundHandle>::Ok(std::move(handle));
}

// ---------------------------------------------------------------------------
// Music playback
// ---------------------------------------------------------------------------

void AudioSystem::PlayMusic(const MusicHandle& music,
                            bool  loop,
                            float fadeInSeconds,
                            float loopPointSeconds) {
    if (!enabled_ || !music || music->RawHandle() == nullptr) return;

    // Stop the current track first (StopTrack on an idle track is a no-op).
    MIX_StopTrack(musicTrack_, 0);

    if (!MIX_SetTrackAudio(musicTrack_, music->RawHandle())) {
        SF_LOG_ERROR("Audio", "MIX_SetTrackAudio (music) failed: %s", SDL_GetError());
        return;
    }

    // Loops: -1 = infinite, 0 = play once, n>0 = play (n+1) times.
    MIX_SetTrackLoops(musicTrack_, loop ? -1 : 0);

    SDL_PropertiesID opts = SDL_CreateProperties();
    if (fadeInSeconds > 0.0f) {
        SDL_SetNumberProperty(opts, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER,
                              static_cast<int>(fadeInSeconds * 1000.0f));
    }
    if (loopPointSeconds > 0.0f) {
        SDL_SetNumberProperty(opts, MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER,
                              static_cast<int>(loopPointSeconds * 1000.0f));
    }

    if (!MIX_PlayTrack(musicTrack_, opts)) {
        SF_LOG_ERROR("Audio", "MIX_PlayTrack (music) failed: %s", SDL_GetError());
    }
    SDL_DestroyProperties(opts);

    // Apply current music gain (in case it was changed before playback).
    ApplyMusicGain();
}

void AudioSystem::StopMusic(float fadeOutSeconds) {
    if (!enabled_ || musicTrack_ == nullptr) return;

    // StopTrack's fade is in *sample frames*, not seconds. Convert.
    Sint64 fadeFrames = 0;
    if (fadeOutSeconds > 0.0f) {
        fadeFrames = MIX_TrackMSToFrames(musicTrack_,
                                         static_cast<Sint64>(fadeOutSeconds * 1000.0f));
    }
    MIX_StopTrack(musicTrack_, fadeFrames);
}

void AudioSystem::PauseMusic() {
    if (!enabled_ || musicTrack_ == nullptr) return;
    MIX_PauseTrack(musicTrack_);
}

void AudioSystem::ResumeMusic() {
    if (!enabled_ || musicTrack_ == nullptr) return;
    MIX_ResumeTrack(musicTrack_);
}

bool AudioSystem::IsMusicPlaying() const {
    if (!enabled_ || musicTrack_ == nullptr) return false;
    return MIX_TrackPlaying(musicTrack_);
}

// ---------------------------------------------------------------------------
// SFX playback
// ---------------------------------------------------------------------------

void AudioSystem::PlaySound(const SoundHandle& sound) {
    if (!enabled_ || !sound || sound->RawHandle() == nullptr) return;

    // Find a free SFX track (one not currently playing).
    MIX_Track* track = nullptr;
    for (auto* t : sfxTracks_) {
        if (!MIX_TrackPlaying(t)) { track = t; break; }
    }
    if (track == nullptr) {
        static bool warned_once = false;
        if (!warned_once) {
            SF_LOG_WARN("Audio",
                "PlaySound dropped — all %d SFX tracks busy (further occurrences suppressed)",
                kSfxTracks);
            warned_once = true;
        }
        return;
    }

    if (!MIX_SetTrackAudio(track, sound->RawHandle())) {
        SF_LOG_ERROR("Audio", "MIX_SetTrackAudio (sfx) failed: %s", SDL_GetError());
        return;
    }
    MIX_SetTrackLoops(track, 0);
    MIX_SetTrackGain(track, master_ * sfx_);

    // Fire-and-forget — no options, no fade.
    if (!MIX_PlayTrack(track, 0)) {
        SF_LOG_ERROR("Audio", "MIX_PlayTrack (sfx) failed: %s", SDL_GetError());
    }
}

// ---------------------------------------------------------------------------
// Volume
// ---------------------------------------------------------------------------

void AudioSystem::SetMasterVolume(float v) {
    master_ = Clamp01(v);
    if (enabled_) {
        ApplyMusicGain();
        ApplySfxGain();
    }
}

void AudioSystem::SetMusicVolume(float v) {
    music_ = Clamp01(v);
    if (enabled_) ApplyMusicGain();
}

void AudioSystem::SetSfxVolume(float v) {
    sfx_ = Clamp01(v);
    if (enabled_) ApplySfxGain();
}

void AudioSystem::ApplyMusicGain() {
    if (musicTrack_ == nullptr) return;
    MIX_SetTrackGain(musicTrack_, master_ * music_);
}

void AudioSystem::ApplySfxGain() {
    const float g = master_ * sfx_;
    for (auto* t : sfxTracks_) {
        if (t) MIX_SetTrackGain(t, g);
    }
}

}  // namespace Engine::Audio
