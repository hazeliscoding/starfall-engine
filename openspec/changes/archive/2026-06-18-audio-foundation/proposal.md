## Why

`engine_audio` has been a placeholder since M0. GameDesign §5.2 designs explicitly around audio — *"Music: acoustic-leaning chiptune, ~90s loops, role of silence is large. The first time music plays is when the player steps outside their door."* That moment is the M2.75 deliverable per GameDesign §9 (newly added 2026-06-18) and is impossible without a working audio system. This change graduates `engine_audio` from placeholder to a working module that handles music streaming + SFX one-shots, surfaces it through the runtime to game code, and adds the first audio behavior to `game_my_rpg` (silence at boot, theme fade-in when Iden leaves her starting position, footstep SFX on each step).

The roadmap addition in DesignDoc §22 places M2.75 between M2.5 and M3 with explicit non-goals (no DSP graph, no spatial audio, no procedural audio, no Lua bindings yet). This change implements exactly that scope.

## What Changes

- Add **`Engine::Audio::AudioSystem`** owned by `engine_runtime`. Surface: `LoadMusic(path) → Result<MusicHandle>`, `LoadSound(path) → Result<SoundHandle>`, `PlayMusic(handle, loop, fadeInSeconds, loopPointSeconds)`, `StopMusic(fadeOutSeconds)`, `PauseMusic()`, `ResumeMusic()`, `IsMusicPlaying()`, `PlaySound(handle)`, `SetMasterVolume(0..1)`, `SetMusicVolume(0..1)`, `SetSfxVolume(0..1)`, plus matching getters.
- Add **`Engine::Audio::Music`** (streamed Ogg Vorbis, one at a time) and **`Engine::Audio::Sound`** (cached WAV, polyphonic via 16 mixer channels) as opaque types behind `MusicHandle = std::shared_ptr<Music>` and `SoundHandle = std::shared_ptr<Sound>` — same shape as `TextureHandle`.
- Pin **`SDL3_mixer`** (release-3.0.0 or latest stable for SDL3) via `external/CMakeLists.txt`, same FetchContent + static pattern as SDL3_image. Enable Ogg + WAV decoders; disable everything else (FLAC/MP3/MOD/MID/etc.) to trim binary size.
- Grow **`Engine::Runtime::Application`**: owns an `AudioSystem` instance, constructed after the asset loader, torn down before it.
- Grow **`Engine::Runtime::AppConfig::onStart`** signature: add `AudioSystem&` as a third argument. (Breaking change to the M2.5 game callback; the migration is one line in `games/my_rpg/src/main.cpp`.) `onUpdate` and `onRender` signatures unchanged.
- Add **`tools/gen_placeholder_audio.py`** — pure-stdlib Python that emits two placeholder files:
  - `embercoast_morning.ogg` (~30s gentle ambient loop) — actually a `.wav` written via `wave` stdlib since pure-stdlib Python can't write Ogg; spec'd as a `.wav` for now and renamed when a real composer's track lands.
  - `footstep.wav` (~0.1s soft click).
  - **Pivot if needed**: if Python's stdlib `wave` module produces something that SDL3_mixer can't read at the format we want, fall back to a tiny CC0-licensed placeholder pack checked in directly. Decision at apply time.
- Grow **`games/my_rpg/src/main.cpp`**: `onStart` receives the audio system, loads the placeholder music + footstep handles, does NOT auto-play (per GameDesign §5.2's "30-60s of silence at the start"). `onUpdate` calls `audio.PlayMusic(theme, loop=true, fadeIn=2.0f)` the first frame the player moves (proxy for "Iden walks outside her door" — refined when M3 adds map regions); calls `audio.PlaySound(footstep)` each time a movement-axis frame produces ~16 logical pixels of travel (one footstep per ~0.27s at 60px/s — matches the walk cycle visually).
- Add **`tests/audio/`** test target — covers `AudioSystem` lifecycle (construct + destruct cleanly with SDL's `"dummy"` audio driver hint so the test runs without a real audio device), volume getters/setters round-trip, and `LoadSound` with a missing file returns Err.
- Update **`docs/GameDesign.md`** §9 to ✅ the M2.75 row when shipped; **`CLAUDE.md`** "Current Status" to reflect audio-foundation done + M3 next.

### Non-Goals

- **No DSP graph, no reverb/EQ, no audio buses beyond music/sfx/master.** Per the M2.75 milestone spec in DesignDoc §22. Add a third bus when something needs it (UI sounds typically share the SFX bus).
- **No spatial audio.** The game is top-down 2D; positional audio is overkill.
- **No procedural / synthesized audio runtime.** The Python placeholder generator is build-time only and gets replaced by a composer's tracks.
- **No Lua bindings.** Per the milestone spec — Lua bindings land in the M6 binding pass which already has `Audio:PlaySound` and `Audio:PlayMusic` on its list (DesignDoc §13.5).
- **No streaming audio support beyond what SDL3_mixer provides natively for Ogg.** No custom decoders.
- **No music crossfading between two simultaneous tracks.** One music track at a time; `StopMusic(fadeOut)` then `PlayMusic(fadeIn)` is the sequencing primitive.
- **No music seek / scrubbing API.** Linear playback only.
- **No audio device hot-swap handling.** SDL3_mixer's default device selection at init; revisit if a user reports issues.
- **No save/restore of volume settings.** That belongs to M8 (Save System) and M12 (Settings Menu).

## Capabilities

### New Capabilities

- `audio-system`: `engine_audio`'s capability for music streaming, SFX one-shot playback, the master/music/SFX volume model, and the asset-loading + handle ownership contract.

### Modified Capabilities

- `runtime-window`: `Application` now owns an `AudioSystem` alongside the renderer + asset loader. `AppConfig::onStart`'s signature grows from `(Renderer&, AssetLoader&)` to `(Renderer&, AssetLoader&, AudioSystem&)`. `onUpdate` and `onRender` are unchanged. Teardown order extends to include the audio system in reverse construction order.

## Impact

- **Affected modules**: `engine_audio` graduates from placeholder to real code. `engine_runtime` grows to own + dispatch the audio system. `games/my_rpg` updates its `onStart` signature + adds the first music + SFX behavior. `engine_core`, `engine_math`, `engine_assets`, `engine_render`, `engine_input` unchanged.
- **Dependency-rule compliance**: `engine_audio → engine_core + SDL3::SDL3 + SDL3_mixer::SDL3_mixer`. `engine_runtime → engine_core + engine_render + engine_assets + engine_input + engine_audio + SDL3::SDL3` (new: + `engine_audio`). Within §6.3.
- **New external dependencies**: SDL3_mixer (pinned tag, static, vendored Ogg/Vorbis decoder via FetchContent's vendored=ON convention).
- **New serialized formats**: none introduced by this change. (Volume settings persistence belongs to M8.)
- **New Lua bindings**: none (deferred to M6).
- **Performance budget**: SDL3_mixer runs the audio callback on its own thread; engine code mutates state via the SDL3_mixer API which handles locking. Per-frame engine cost is sub-microsecond (a couple of state queries at most). One streamed Ogg + 16 cached WAV channels well within budget.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M2.75): launching `game_my_rpg` plays NO music for the opening seconds; pressing a movement key (proxy for "Iden walks outside her door") fades in the placeholder Embercoast theme; each step produces a footstep SFX. Tick M2.75 row in GameDesign §9 in the same change.
- **Docs**: GameDesign §9 M2.75 row ticked. CLAUDE.md "Current Status" updated. No DesignDoc edits — M2.75 was added in the 2026-06-18 roadmap update commit and this change implements that spec.
- **Open questions (validate at apply time):**
  - **Library availability**: SDL3_mixer's release tag for SDL3 might still be pre-1.0 at the time of apply. If the latest tag is unstable, options are (a) pin to a known-good commit on `main`, (b) fall back to raw SDL3_audio + stb_vorbis for music + WAV via SDL_LoadWAV. Decision at apply time based on what `git ls-remote --tags` shows.
  - **Music format**: Ogg if SDL3_mixer's Ogg decoder works out of the box; fall back to WAV (larger files, but lossless) if Ogg is finicky on either platform.
  - **Placeholder audio quality**: The Python-generated ambient + footstep are intentionally minimal — a sine drone and a click. If they're actively unpleasant, fall back to silence-with-stub-music or check in a CC0 pack.
  - **Footstep trigger heuristic**: "one footstep per 16 logical px of travel" is a guess. May feel too sparse or too dense once heard; tune the constant during the apply runtime smoke.
