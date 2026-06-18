## 1. Third-Party — SDL3_mixer

- [ ] 1.1 Inspect upstream tags at apply time (`git ls-remote --tags https://github.com/libsdl-org/SDL_mixer.git`) and pick the latest release tag for SDL3 (e.g. `release-3.0.0` or whatever's stable). Document the pinned tag + reason in `external/CMakeLists.txt` next to the existing SDL3 + SDL3_image pins. Touches: investigation.
- [ ] 1.2 If no stable SDL3_mixer tag exists, execute design D1's fallback: skip 1.3-1.4, instead vendor stb_vorbis as a single-header into `external/stb/` and use raw `SDL3_audio` + `SDL_LoadWAV` + a small hand-rolled mixer in `engine_audio`. Update design D1 to record the fallback was taken. Touches: investigation outcome — defines path forward.
- [ ] 1.3 (SDL3_mixer path) Extend `external/CMakeLists.txt` with a `FetchContent_Declare(SDL3_mixer ...)` block. Set `SDLMIXER_VENDORED=ON`, `SDLMIXER_BUILD_SHARED_LIBS=OFF`, `SDLMIXER_INSTALL=OFF`, `SDLMIXER_SAMPLES=OFF`. Enable Ogg + WAV decoders, disable everything else (MP3/FLAC/MOD/MID/Opus etc.) to trim binary + configure time. Alias normalized to `SDL3_mixer::SDL3_mixer` post-fetch. Apply the same `BUILD_SHARED_LIBS` scope guard we used for SDL3_image (the libstdc++-10/BUILD_SHARED_LIBS issue may recur). Touches: external dep.
- [ ] 1.4 Verify SDL3_mixer builds clean on Windows (MSVC + Ninja) via the next configure. Verified by Task 9.1. Touches: smoke (Windows).
- [ ] 1.5 Verify SDL3_mixer builds clean on Linux (WSL2 + g++-10). Verified by Task 9.3. Touches: smoke (Linux).

## 2. Placeholder Audio Assets

- [ ] 2.1 Add `tools/gen_placeholder_audio.py`: pure stdlib (uses `wave`, `struct`, `math`). Emits `embercoast_morning.wav` (~30s, 22kHz mono 16-bit, 440 Hz sine drone with slow amplitude envelope rising/falling ±0.15 around a 0.3 base, intentionally minimal) and `footstep.wav` (~100ms band-limited click at the same sample rate). Output path: `games/my_rpg/assets/audio/`. Touches: tooling + game assets.
- [ ] 2.2 Run the script. Verify both WAVs play in any system audio player (Windows Media Player, ffplay, etc.) and are ≤ 6 MB combined. Commit both the script and the WAV outputs (size is fine; we're a single-developer indie repo, not a shared mono). Touches: assets.

## 3. engine_audio — Real Code

- [ ] 3.1 Add `include/engine/audio/music.hpp` + `include/engine/audio/sound.hpp`: opaque types (PIMPL or thin wrappers around the SDL3_mixer types) with `MusicHandle = std::shared_ptr<Music>` and `SoundHandle = std::shared_ptr<Sound>` aliases. Destructors release the SDL3_mixer resources. Non-copyable, non-movable. Touches: `engine_audio`.
- [ ] 3.2 Add `include/engine/audio/audio_system.hpp` + `src/audio/audio_system.cpp`: `Engine::Audio::AudioSystem` with the full surface from the `audio-system` spec (Load*/Play*/Stop*/Pause*/Resume*, volume getters + setters, IsMusicPlaying, IsValid). Construct opens the mixer device + reserves 16 SFX channels; destructor reverses. On init failure, log `[Audio]` error + enter disabled state (D9) where all subsequent calls are silent no-ops + Load* returns Err. Touches: `engine_audio`.
- [ ] 3.3 Implement `LoadMusic`/`LoadSound` with the same path-keyed `weak_ptr` cache as `engine_assets::AssetLoader` — use `SDL_GetBasePath()` snapshot for exe-relative resolution. (Don't share the AssetLoader; AudioSystem has its own resolver — same shape, different cache.) Touches: `engine_audio`.
- [ ] 3.4 Implement `PlayMusic(handle, loop, fadeInSeconds, loopPointSeconds)` via SDL3_mixer's `Mix_PlayMusic` + `Mix_FadeInMusicPos`. `StopMusic(fadeOutSeconds)` via `Mix_FadeOutMusic`. `PauseMusic` / `ResumeMusic` via `Mix_PauseMusic` / `Mix_ResumeMusic`. Track current music handle so `IsMusicPlaying` can return without polling the mixer + Stop properly releases. Touches: `engine_audio`.
- [ ] 3.5 Implement `PlaySound(handle)` via `Mix_PlayChannel` (channel = -1 to pick any free channel; -1 return = exhausted, log once and drop per spec scenario). Touches: `engine_audio`.
- [ ] 3.6 Implement volume controls: store `master_`, `music_`, `sfx_` as floats; setters clamp to [0,1]; on any volume change, compute `master_ * music_` and pass to `Mix_VolumeMusic`, `master_ * sfx_` and pass to `Mix_Volume(-1, ...)` (all channels). Touches: `engine_audio`.
- [ ] 3.7 Update `src/audio/CMakeLists.txt`: `starfall_add_module(NAME engine_audio SOURCES audio_system.cpp music.cpp sound.cpp DEPENDS engine_core SDL3::SDL3 SDL3_mixer::SDL3_mixer)`. Drop `placeholder.cpp`. Touches: `engine_audio`.

## 4. engine_runtime — Wire In the Audio System

- [ ] 4.1 Update `include/engine/runtime/app_config.hpp`: change `onStart`'s signature from `std::function<void(Render::Renderer&, Assets::AssetLoader&)>` to `std::function<void(Render::Renderer&, Assets::AssetLoader&, Audio::AudioSystem&)>`. Forward-declare `Engine::Audio::AudioSystem`. Touches: `engine_runtime`.
- [ ] 4.2 Update `src/runtime/application.cpp` `Run()`: construct `Engine::Audio::AudioSystem` as a `std::unique_ptr` after the AssetLoader and before the InputState. Pass it as the third arg to `onStart`. Teardown reverses: input → audio → assets → renderer → window. The onStart exception path tears down audio along with everything else. Touches: `engine_runtime`.
- [ ] 4.3 Update `src/runtime/CMakeLists.txt`: add `engine_audio` to DEPENDS. Touches: `engine_runtime`.

## 5. Game Side — Silence, Theme, and Footsteps

- [ ] 5.1 Update `games/my_rpg/src/main.cpp` `PlayerState`: add `Engine::Audio::MusicHandle theme;`, `Engine::Audio::SoundHandle footstep;`, `bool musicTriggered = false;`, `float footstepDistanceAccum = 0.0f;`. Add an `Engine::Audio::AudioSystem* audio = nullptr;` reference-holder for use from onUpdate (captured-by-ref from onStart's parameter via the lambda capture). Touches: `game_my_rpg`.
- [ ] 5.2 Update `onStart` signature to accept the audio system. Load `assets/audio/embercoast_morning.wav` into `player.theme` and `assets/audio/footstep.wav` into `player.footstep`. On failure (degraded audio system), the handles are null and subsequent PlayMusic/PlaySound silently no-op. Capture `&audio` into player state so onUpdate can call it. Touches: `game_my_rpg`.
- [ ] 5.3 Update `onUpdate`: at the end of the existing direction-resolve + position-update + animation block, add:
  - **Music trigger**: if `!player.musicTriggered && isMoving && player.theme`, call `player.audio->PlayMusic(player.theme, /*loop*/true, /*fadeIn*/2.0f, /*loopPoint*/0.0f)` and set `musicTriggered = true`. (Design D6 — proxy for "outside her door" until M3 lands real regions.)
  - **Footstep trigger**: if `isMoving`, accumulate `step` (= kIdenWalkSpeed * dt — already computed) into `player.footstepDistanceAccum`. When it crosses `kFootstepDistance = 16.0f` logical px, subtract 16 and `player.audio->PlaySound(player.footstep)`. (Design D7.)
  Touches: `game_my_rpg`.

## 6. Tests — engine_audio

- [ ] 6.1 Add `tests/audio/CMakeLists.txt` with `starfall_add_test(NAME engine_audio_tests SOURCES audio_system_tests.cpp DEPENDS engine_audio engine_core SDL3::SDL3 SDL3_mixer::SDL3_mixer)`. Include the `tests/common/` dir for SDL fixture access if we add an audio-driver fixture later. Touches: tests.
- [ ] 6.2 Update `tests/CMakeLists.txt`'s foreach loop to add `audio` to the per-module test-dir list. Touches: tests root.
- [ ] 6.3 Add `tests/audio/audio_system_tests.cpp`: set `SDL_HINT_AUDIO_DRIVER=dummy` (via `SDL_SetHintWithPriority` with OVERRIDE priority) before constructing AudioSystem. Tests:
  - lifecycle: construct + destruct cleanly, IsValid returns true under dummy driver
  - volume round-trip: SetMasterVolume(0.7) → GetMasterVolume ≈ 0.7
  - volume clamping: SetMasterVolume(-0.5) → 0.0; SetMasterVolume(2.0) → 1.0
  - default volumes: fresh AudioSystem has all three at 1.0
  - LoadSound with missing path returns Err with path in message
  - LoadMusic with missing path returns Err with path in message
  - PlayMusic / PlaySound on null handle is a no-op (no crash)
  - (We don't verify audio is audible — dummy driver suppresses output.)
  Touches: tests.

## 7. Docs Update (Milestone Contract)

- [ ] 7.1 In `docs/GameDesign.md` §9, mark M2.75 row complete: `✅ M2.75 Audio (YYYY-MM-DD)` with summary (placeholder WAV theme + footstep SFX + silence-first opening). Touches: GameDesign §9.
- [ ] 7.2 Update `CLAUDE.md` "Current Status": M2.75 done; AudioSystem primitive landed; next target M3 (Tilemap & Collision). Roadmap-shape paragraph unchanged. Touches: project root CLAUDE.md.
- [ ] 7.3 README.md status table: tick M2.75 to ✅; bump 🎯 Next to M3. Touches: README.

## 8. Rule Audit

- [ ] 8.1 Run `check-deps` skill against the updated module graph. New edges: `engine_audio → SDL3::SDL3 + SDL3_mixer::SDL3_mixer`, `engine_runtime → engine_audio`. Confirm zero §6.3 violations. Touches: rule audit.

## 9. Verification (Both Platforms)

- [ ] 9.1 Windows (VS Dev Shell): full configure + build + `ctest --preset debug-windows`. SDL3_mixer fetches + builds. All tests pass (existing 26 + new audio tests). Touches: end-to-end Windows.
- [ ] 9.2 Windows runtime smoke: launch `bin\game_my_rpg.exe`. Window opens in silence. Press W (or any movement key) — music begins fading in over ~2s; footstep SFX plays roughly twice per second matching the walk-cycle pace. Close window → exit 0. Touches: end-to-end Windows runtime.
- [ ] 9.3 Linux (WSL2 g++-10 + WSLg): same flow with `debug-linux`. WSLg's PulseAudio routing should deliver the same audio. Touches: end-to-end Linux.
- [ ] 9.4 Linux runtime smoke via WSLg: same as 9.2; tolerate higher-latency audio per design risk note. SIGTERM → exit 0. Touches: end-to-end Linux runtime.
- [ ] 9.5 Footstep cadence tuning: count footsteps per second by ear during 9.2. If significantly off (< 1.5 or > 3 per second), tune `kFootstepDistance` and re-verify before declaring done. Touches: feel tuning.
- [ ] 9.6 Audio-disabled-mode smoke (manual): set `SDL_AUDIODRIVER=dummy` env var before launching `game_my_rpg`. Confirm the game still runs (window opens, Iden walks, no crash), just silent. (Catches the D9 degraded-mode path on the real binary, not just unit tests.) Touches: degraded-mode validation.
