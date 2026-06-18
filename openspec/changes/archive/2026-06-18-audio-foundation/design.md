## Context

`engine_audio` has been an empty placeholder target since M0 — wired into the dependency graph but with zero implementation. M2.75 (newly added to the roadmap on 2026-06-18) fills it in with the minimum a 2D RPG actually needs: one streamed music track at a time, polyphonic SFX one-shots, three volume knobs.

Two spec slices frame the work: the new `audio-system` capability (everything inside `engine_audio`) and a modification to `runtime-window` for the AppConfig::onStart signature change. The game-side audio behavior (silence-then-theme + footstep SFX) is implementation, not capability.

The non-goals from the M2.75 milestone spec in DesignDoc §22 are strict: no DSP graph, no spatial audio, no procedural runtime audio, no Lua bindings, no audio-device hot-swap, no save/restore of volume settings. Each deferred item has a future home (M11 UI, M6 Lua, M8 Save, etc).

## Goals / Non-Goals

**Goals:**

- The game-visible M2.75 deliverable: silence at boot, theme fades in when Iden walks outside her door, footstep SFX on each step.
- A reusable engine primitive (`AudioSystem`) that future game scenes + UI menus + combat hits + dialogue clicks consume unchanged.
- Cross-platform parity: same behavior on Windows (WASAPI via SDL) and Linux (PulseAudio / PipeWire via SDL).
- Test infrastructure works without a real audio device (SDL's `dummy` driver hint) — so the test suite still runs on the next CI host that lacks audio hardware.

**Non-Goals:**

- All M2.75 non-goals from DesignDoc §22 (DSP, spatial, procedural, Lua, hot-swap, settings persistence).
- No audio asset packer integration — that's M14.
- No streaming format beyond what SDL3_mixer's Ogg decoder gives us.
- No music crossfading or layered tracks (deferred).
- No 3D audio panning even within the 2D game.

## Decisions

### D1: SDL3_mixer for music + SFX, not raw SDL3_audio

**Choice:** Pin `SDL3_mixer` via FetchContent in `external/CMakeLists.txt`, same pattern as SDL3_image. Use it for music streaming AND SFX one-shot mixing.

**Why:** SDL3_mixer is the official vendor companion library, written by the same team as SDL3 + SDL3_image, with FetchContent that integrates the same way our existing third-party deps do. It handles Ogg + WAV decoding, fade in/out, channel pool for SFX polyphony, and the threaded audio callback locking — all things we'd otherwise implement by hand on top of raw `SDL_audio`.

**Trade-off:** SDL3_mixer is younger than SDL3 itself; its first stable release for SDL3 happened relatively recently. If its release tags are still unstable at apply time, we fall back to raw `SDL3_audio` + `stb_vorbis` (single-header) for music + SDL's built-in `SDL_LoadWAV` for SFX, and write a small channel mixer by hand. That's the open question flagged in the proposal.

**Alternative considered:** miniaudio — single-header, broader format support, very capable. Rejected for M2.75 because it's a new dependency outside the SDL family, and we're not yet using any feature it provides that SDL3_mixer doesn't. Worth reconsidering when we need spatial audio or DSP (Phase 2+).

### D2: One streaming music track, 16 SFX channels

**Choice:** `AudioSystem` reserves exactly one music slot and 16 SFX mixer channels.

**Why:** One music track at a time matches how every JRPG of the EarthBound / Chrono Trigger reference points works — there's a current area theme; transitions are sequential (fade out current, fade in next). Two simultaneous tracks would imply layered/adaptive audio, which the M2.75 non-goals exclude.

16 SFX channels is plenty for a 2D RPG. Worst-case concurrent sound count: footstep + UI click + dialogue tick + maybe an ambient bird sound + a save-confirmation sparkle ~= 5. 16 gives 3× headroom for combat (Phase 3) with overlapping hit + spell + step sounds. Bumpable to 32 if Phase 3 needs it — one constant in `AudioSystem`'s init.

### D3: MusicHandle + SoundHandle = std::shared_ptr<Music/Sound>

**Choice:** Same pattern as `engine_assets::TextureHandle`. Loading returns a `Result<Handle>`. Handles are shared, refcounted, free underlying audio data when the last handle drops. Cache by canonical path with `weak_ptr` so a re-load while a handle is live returns the cached resource.

**Why:** Consistency. Game code that already understands `TextureHandle` understands `MusicHandle` and `SoundHandle` immediately. Same shared-ownership semantics. Same eviction-on-drop behavior.

### D4: AppConfig::onStart grows a third argument (breaking change for the M2 + M2.5 game callback)

**Choice:** `onStart` signature changes from `(Renderer&, AssetLoader&)` to `(Renderer&, AssetLoader&, AudioSystem&)`. `onUpdate(dt, InputState&)` and `onRender(Renderer&)` are unchanged.

**Why:** Game code needs the audio system at startup time to load music + SFX. The alternative — capturing the audio system into the `onUpdate` and `onRender` lambdas via some shared services struct — defers the design question that we'll have to answer eventually anyway. Just take the argument now.

**Cost:** `games/my_rpg/src/main.cpp` `onStart` lambda gains a third parameter. One-line fix. No other callers exist.

**Future plan:** Once we have 5+ services (renderer, asset loader, audio, input state, scene, scripting, ...), refactor to a `Services` struct passed by reference: `onStart(Services&)`. Not now.

### D5: Volume model = linear 0..1, three knobs (master, music, sfx)

**Choice:** Each volume is a `float` in [0, 1]. Music output = `master × music × per-track-volume`. SFX output = `master × sfx × per-channel-volume`. Setters clamp out-of-range silently.

**Why:** Simple, matches what every game engine surface looks like at game-code level. Perceptual / dB-aware scaling is a refinement that belongs in M12 (Settings Menu) — there a slider may want to scale logarithmically for "feels right" perception, but the underlying engine API stays linear and the slider does the curve.

**Default = 1.0** for all three on construction. Game code or future settings UI is responsible for tuning down.

### D6: Game uses "first frame of movement" as the music-cue trigger

**Choice:** In `games/my_rpg/src/main.cpp`'s `onUpdate`, the first time `isMoving` is true and music isn't playing, call `audio.PlayMusic(theme, loop=true, fadeIn=2.0f)`. Stays a one-shot trigger via a captured `bool musicTriggered = false` flag.

**Why:** Proxy for GameDesign §5.2's "first time music plays is when the player steps outside their door." Without a real map + interior/exterior distinction (M3+), pressing any movement key is the cleanest single signal. The proxy gets refined when M3 lands and the game can ask "is Iden in an outdoor tile?" instead.

**Migration to the real trigger**: at M3-M4 when scenes have regions, the trigger becomes "Iden's tile transitions from indoor to outdoor." The audio API call shape stays identical.

### D7: Footstep SFX trigger = "moved ~16 logical px since last footstep"

**Choice:** Game accumulates distance traveled per frame. When the accumulator exceeds 16 logical pixels (one tile-worth at the 16px tile size GameDesign §5.1 calls for), reset the accumulator and `PlaySound(footstep)`.

**Why:** At 60 logical px/sec walk speed (M2's kIdenWalkSpeed), that's a footstep every ~0.27s — roughly 2 footsteps per second, matching the 8-fps walk-cycle's "2 footsteps per second" pacing from M2.5. Visually + audibly synchronized.

**Trade-off:** Hardcoded 16-px constant. If the walk speed or tile size changes, footstep cadence drifts. Acceptable for M2.75. Tunable in `kFootstepDistance` constant in `main.cpp`. A real solution at M22 (Cutscene system) connects footstep SFX to the animation system as a frame-event.

### D8: Placeholder audio = Python-generated WAVs, checked in

**Choice:** Add `tools/gen_placeholder_audio.py` (pure stdlib, uses `wave` + `struct` + `math`) that emits two WAV files:
- `embercoast_morning.wav` — ~30 seconds, 440 Hz sine drone with slow amplitude envelope, low volume. Intentionally minimal.
- `footstep.wav` — ~100ms band-limited click envelope.

Files land in `games/my_rpg/assets/audio/`. Checked in (small, ~5MB for 30s music at 22kHz mono 16-bit, ~5KB for footstep).

**Why:** Same precedent as `tools/gen_iden_placeholder.py` — reproducible from script, no external deps, replaceable when a real composer's tracks arrive. The placeholder is intentionally bland (a single sine) so nobody mistakes it for finished art.

**Pivot if needed**: if the Python WAVs don't decode correctly via SDL3_mixer for any reason, fall back to a tiny CC0-licensed placeholder pack. Decision at apply time.

**Format reality check**: Python's `wave` module writes PCM WAV only — no Ogg. So the music "stream" is actually a WAV at M2.75 (5MB for 30s at 22kHz mono). Acceptable for a placeholder; when the composer's real Ogg arrives it just becomes a longer track in a smaller file.

### D9: Audio init failure ≠ crash — degrade silently to no-op mode

**Choice:** If `AudioSystem`'s constructor can't open the audio device (no hardware, headless CI without the dummy driver hint, driver error), log `[Audio] init failed: <SDL_GetError()>` and enter an "audio disabled" state. All subsequent API calls succeed but produce no sound: LoadMusic/LoadSound return Err, PlayMusic/PlaySound are no-ops, volume setters store values but produce no audible effect.

**Why:** The game shouldn't crash because the user's audio device is yanked or their CI lacks audio. The visual experience continues. This is also how M1's `Renderer::DrawSprite` handles a null TextureHandle — degrade, log, continue.

### D10: Tests run with SDL's `dummy` audio driver

**Choice:** `tests/audio/CMakeLists.txt` test setup sets `SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE)` before constructing the AudioSystem. Tests verify the lifecycle, volume getters/setters, and error paths — NOT the actual audio output (which the dummy driver doesn't produce).

**Why:** Tests must be runnable on any machine including future CI runners without audio hardware. SDL's `dummy` driver simulates the API surface without ever producing sound — perfect for "did the code path execute without crashing" checks.

**What's NOT testable here**: that music actually fades from 0 to volume X over Y seconds in audible reality. That's manual runtime smoke (Task 9 in tasks.md).

## Risks / Trade-offs

- **[Risk] SDL3_mixer is younger than SDL3 itself; its tags may be pre-1.0 at apply time.** → Mitigation: D1 documents a fallback to raw SDL3_audio + stb_vorbis. Decision at apply time based on `git ls-remote --tags`. Worst case we lose a day reinventing what SDL3_mixer would've given us — still bounded.
- **[Risk] WAV "music" placeholder is fat (~5MB for 30s) compared to a real Ogg (~500KB).** → Acceptable for placeholder; the real composer's track lands as Ogg and the file shrinks 10×.
- **[Risk] The "first movement = music starts" trigger is a heuristic, not the real "indoor → outdoor transition" trigger GameDesign §5.2 describes.** → Documented in D6 with a migration path at M3+. The audio API call shape doesn't change — only the predicate in game code.
- **[Risk] One-music-track-at-a-time forecloses adaptive layered audio.** → Acceptable for the engine's stated scope (DesignDoc §2.2 excludes complex audio). If a future game wants this, layered playback is a non-breaking addition (`PlayMusicLayer` etc).
- **[Risk] SDL3_mixer's audio callback runs on its own thread; engine code mutating mixer state needs to respect that.** → SDL3_mixer's API is documented as thread-safe for the calls we use (channel volume, music play/stop). We don't add our own state mutation outside of those calls. If we later add custom mixing logic, we revisit thread safety.
- **[Risk] Footstep SFX cadence based on distance traveled is fragile if movement speed or tile size changes.** → Documented in D7 with a real solution path at M22.
- **[Risk] The Python-generated placeholder music is intentionally minimal and may sound unpleasant.** → Quality is explicitly placeholder; the spec is "audio system works," not "music is good." Replace ASAP when a composer is onboarded.
- **[Risk] WSL2 audio output via WSLg's PulseAudio sometimes has high latency or stutters.** → Acceptable for development; production target is native Windows + native Linux, not WSL2.
