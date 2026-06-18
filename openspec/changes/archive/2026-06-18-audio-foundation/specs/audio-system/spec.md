## ADDED Requirements

### Requirement: Audio System Lifecycle

`engine_audio` SHALL provide an `Engine::Audio::AudioSystem` type whose lifetime is managed by `engine_runtime`. It SHALL initialize an audio backend (default device), expose load + playback primitives, and shut down cleanly when destroyed.

#### Scenario: Audio system initializes against default device

- **WHEN** `engine_runtime` constructs an `AudioSystem`
- **THEN** the underlying audio backend opens the default output device, 16 SFX mixer channels are reserved, and the system reports ready (or `IsValid() == false` + a `[Audio]` error logged on failure)

#### Scenario: Audio init failure does not crash

- **WHEN** the backend fails to open (no audio device, headless CI without the dummy driver hint, driver fault)
- **THEN** the `AudioSystem` constructor MUST NOT throw or terminate; it MUST log an `[Audio]` error and enter an "audio disabled" state in which all subsequent calls are silent no-ops (LoadMusic/LoadSound return Err; PlayMusic/PlaySound do nothing; volume setters accept the value but produce no audible change)

#### Scenario: Clean shutdown reverses init

- **WHEN** the `AudioSystem` is destroyed
- **THEN** all cached SFX + the streaming music are released BEFORE the audio backend is closed, and destruction is safe to call exactly once

### Requirement: Music Streaming

`AudioSystem::LoadMusic(path) → Result<MusicHandle>` SHALL load a music file (Ogg Vorbis preferred; WAV acceptable as fallback per design.md) and return a `MusicHandle = std::shared_ptr<Music>`. Only ONE music track plays at a time — `PlayMusic` SHALL stop any currently-playing track first.

#### Scenario: PlayMusic on a valid handle starts playback

- **WHEN** game code calls `LoadMusic("assets/audio/embercoast_morning.ogg")` successfully, then `PlayMusic(handle, loop=true, fadeIn=0.0f, loopPoint=0.0f)`
- **THEN** the track begins playing immediately and `IsMusicPlaying()` returns true

#### Scenario: PlayMusic with fade-in ramps volume from 0 to the music-volume setting

- **WHEN** `PlayMusic(handle, loop=true, fadeIn=2.0f, ...)` is called
- **THEN** the track starts at silence, ramps to the configured music volume over 2 seconds, and reaches full volume at t=2s (verified by `SetMusicVolume(0.5)` before the call: at t=2s the output level matches 0.5 × master)

#### Scenario: PlayMusic on a new track stops the previous one

- **WHEN** music A is playing and game code calls `PlayMusic(B, ...)`
- **THEN** A is stopped before B starts; `IsMusicPlaying()` returns true (now for B)

#### Scenario: StopMusic with fade-out ramps to silence then stops

- **WHEN** music is playing at volume 0.8 and `StopMusic(fadeOut=1.0f)` is called
- **THEN** the track ramps to silence over 1 second and `IsMusicPlaying()` returns false after the fade completes

#### Scenario: PauseMusic / ResumeMusic preserve playback position

- **WHEN** a track is playing at position T, `PauseMusic()` is called, then ~5 seconds later `ResumeMusic()` is called
- **THEN** playback resumes from approximately position T (not from start, not from T+5s)

#### Scenario: Loop with non-zero loop point

- **WHEN** a 90-second music track is played with `loop=true, loopPoint=10.0f`
- **THEN** when playback reaches the end of the track, playback restarts at the 10-second mark (the first 10 seconds are an intro that does not repeat)

#### Scenario: Missing music file returns Err

- **WHEN** `LoadMusic` is called with a path that does not exist
- **THEN** the result is `Err` with the path in the message; a `[Audio]` error is logged; no crash

### Requirement: SFX One-Shot Playback

`AudioSystem::LoadSound(path) → Result<SoundHandle>` SHALL load a WAV file fully into memory and return a `SoundHandle`. `AudioSystem::PlaySound(handle)` SHALL play the sound on the next available channel of a 16-channel pool. Polyphony — concurrent playback of the same sound or different sounds on different channels — MUST work without one cutting off another below the channel limit.

#### Scenario: PlaySound on a valid handle is audible

- **WHEN** `LoadSound("assets/audio/footstep.wav")` succeeds and `PlaySound(handle)` is called
- **THEN** the sound plays once at the configured SFX × master volume

#### Scenario: Polyphony allows overlap

- **WHEN** `PlaySound(footstep)` is called five times within 100ms
- **THEN** all five plays are heard (overlap); none preempts the others

#### Scenario: Same sound concurrent

- **WHEN** `PlaySound(footstep)` is called while a previous PlaySound(footstep) is still playing
- **THEN** the second play starts on a different channel and runs concurrently

#### Scenario: Channel exhaustion drops the new sound (does not crash)

- **WHEN** 16 PlaySound calls are made within a single frame (more than the channel count)
- **THEN** the first 16 play; the 17th MAY be silently dropped (or replace the oldest, implementation choice — both are spec-compliant); no crash

#### Scenario: Missing sound file returns Err

- **WHEN** `LoadSound` is called with a path that does not exist
- **THEN** the result is `Err` with the path in the message; a `[Audio]` error is logged; no crash

#### Scenario: SoundHandle caching

- **WHEN** `LoadSound("footstep.wav")` is called twice
- **THEN** both calls MUST return handles that resolve to the same underlying decoded sound data (no second decode), via the same weak-ptr cache pattern as `engine_assets::TextureHandle`

### Requirement: Volume Controls

`AudioSystem` SHALL expose `SetMasterVolume(0..1)`, `SetMusicVolume(0..1)`, `SetSfxVolume(0..1)`, and matching getters. Music output level is `master × music`; SFX output level is `master × sfx`. Volume values outside [0, 1] SHALL be clamped to that range without error.

#### Scenario: Round-trip getter/setter

- **WHEN** game code calls `SetMusicVolume(0.7f)` and then `GetMusicVolume()`
- **THEN** the returned value is approximately 0.7 (within float precision)

#### Scenario: Out-of-range volume is clamped

- **WHEN** game code calls `SetMasterVolume(-0.5f)` or `SetMasterVolume(2.0f)`
- **THEN** the stored value clamps to 0.0 or 1.0 respectively; no error logged

#### Scenario: Master scales both music and SFX

- **WHEN** master is 0.5, music is 1.0, SFX is 1.0, and a PlayMusic + PlaySound are issued
- **THEN** both the music and the sound play at half output level

#### Scenario: SFX volume changes apply to newly-started AND already-playing channels

- **WHEN** a sound is mid-playback and game code calls `SetSfxVolume(0.2f)`
- **THEN** the in-flight sound's volume drops to 0.2 × master within the next audio callback tick (no abrupt zipper noise — SDL3_mixer's per-channel volume change handles the ramp internally)

### Requirement: Default Volume State

A freshly-constructed `AudioSystem` SHALL start with `master = 1.0`, `music = 1.0`, `sfx = 1.0` — all audio at full volume by default. Game code or future settings UI is responsible for tuning down.

#### Scenario: Defaults on construction

- **WHEN** an `AudioSystem` is freshly constructed and `GetMasterVolume / GetMusicVolume / GetSfxVolume` are called before any setter
- **THEN** all three return 1.0
