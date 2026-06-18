## Why

M2 makes Iden walk, but the sprite is permanently the down-facing idle frame — walking up looks identical to walking down looks identical to standing still. That's a real visual bug that erodes the EarthBound feel the game is going for. GameDesign §9's roadmap jumps straight from M2 (movement) to M3 (tilemap), but living through an entire tilemap-development cycle with a broken-looking player is bad for dev morale and would have to be patched anyway before the M7 playable-slice playtests.

The TimeFantasy pack already ships everything needed: each character's `frames/chara/charaX_Y/` folder contains `{down,up,left,right}_{stand,walk1,walk2}.png` — 12 PNGs per character covering idle + walk cycle in 4 cardinal directions. DesignDoc §7.7 already lists `AnimatedSpriteComponent` as a planned engine component. This change introduces a standalone `AnimatedSprite` primitive (the future `AnimatedSpriteComponent` wraps it when the scene/component system lands in M3-M4) and uses it to give Iden directional sprites + a walk cycle.

This change inserts an **M2.5: Sprite Animation** milestone between M2 (Input & Movement) and M3 (Tilemap & Collision), updating both `docs/GameDesign.md` §9 and `docs/DesignDoc.md` §22 to reflect it.

## What Changes

- Add **`Engine::Render::AnimationClip`** — plain aggregate: `std::vector<Engine::Assets::TextureHandle> frames`, `float frameDuration` (seconds per frame, default 0.125 = 8 fps), `bool looping` (default true).
- Add **`Engine::Render::AnimatedSprite`** — owns a set of named `AnimationClip`s; tracks a current clip + timer + frame index + paused flag. Surface: `AddClip(name, clip)`, `Play(name)` (no-op if already playing that clip, preserving cycle continuity), `Pause()`, `Resume()`, `Update(dt)`, `CurrentFrame() → TextureHandle`, `CurrentClip()`, `IsPlaying()`.
- Grow **`games/my_rpg`**: `PlayerState` gains an `AnimatedSprite` member. `onStart` loads all 12 TimeFantasy frames for the chosen character (or falls back to the procedural placeholder as a single-frame "idle" clip when the paid pack isn't present), constructs 8 clips (`idle_{up,down,left,right}` + `walk_{up,down,left,right}`), and calls `Play("idle_down")` as the initial state.
- Grow **`onUpdate`**: after the existing move logic, derive a desired clip from `(isMoving, facing)` — e.g. `walk_left` when moving left, `idle_down` when standing — call `sprite.Play(desired)` and `sprite.Update(dt)`. `isMoving` is derived from "did `position` change this frame OR is any movement key held."
- Update **`onRender`** to draw `sprite.CurrentFrame()` instead of a static handle.
- Add **`tests/render/animated_sprite_tests.cpp`** — covers clip add/play, frame index advances at the right pace, looping wraps, non-looping clamps, switching clips resets the timer, Play on the current clip is a no-op, Pause freezes Update.
- Update **`docs/GameDesign.md`** §9: insert M2.5 row between M2 and M3.
- Update **`docs/DesignDoc.md`** §22: insert a "Milestone 2.5 — Sprite Animation" section between M2 and M3 (no renumbering of later milestones — they stay M3-M10).

### Non-Goals

- **No complex animation graphs.** Per DesignDoc §2.2 those are out of scope project-wide. `AnimatedSprite` is a single-track, linear-frame primitive. State machines (Idle ↔ Walk ↔ Attack with transition rules) are not part of this change.
- **No per-frame events / callbacks.** A real animation system eventually triggers "footstep SFX on walk1 frame" or similar. Deferred to the audio milestone or later.
- **No sprite blending / interpolation.** Frame-swap only (pixel-art aesthetic).
- **No animation-driven movement.** Movement still comes from `onUpdate`'s input handling; animation only reflects state.
- **No NPC animation in this change.** Iden only. NPCs animate in M6/M7 when they exist as entities.
- **No tilemap.** That's still M3.
- **No scene/component system.** `AnimatedSprite` is a standalone type used directly from game code via captured-by-reference state; future `AnimatedSpriteComponent` (DesignDoc §7.7) wraps it when the scene system lands.
- **No editor preview.** M5.
- **No Lua bindings.** M6.

## Capabilities

### New Capabilities

- `sprite-animation`: `engine_render`'s capability for named-clip animation. Owns the `AnimationClip` data shape, the `AnimatedSprite` runtime (Play/Update/CurrentFrame), and the looping vs one-shot contract.

### Modified Capabilities

None. `2d-renderer` is unchanged — `DrawSprite` already takes any `TextureHandle`, and `sprite.CurrentFrame()` returns one. `runtime-window` is unchanged — `onUpdate(dt, input)` already gives the game everything it needs to drive animation per frame.

## Impact

- **Affected modules**: `engine_render` gains new code (AnimationClip header, AnimatedSprite header + cpp). `games/my_rpg` grows in `main.cpp` (12 texture loads, 8 clip constructions, clip selection in `onUpdate`). `engine_runtime`, `engine_assets`, `engine_input`, `engine_core`, `engine_math` unchanged.
- **Dependency-rule compliance**: `engine_render` already depends on `engine_assets` (for `TextureHandle`) — no new deps, no new edges in §6.3. Game still depends only on `engine_runtime`.
- **New external dependencies**: none.
- **New serialized formats**: none. (Clips are constructed in code at M2.5; data-driven clip definitions arrive when the scene system loads them from JSON.)
- **New Lua bindings**: none.
- **Performance budget**: animation update is O(1) per sprite per frame (timer increment, possible frame-index step, modulo if looping). With one player sprite, sub-microsecond overhead. Re-evaluate at M3+ when tilemap entities + NPCs introduce dozens of `AnimatedSprite`s — at which point batched update belongs to the scene system, not `AnimatedSprite` itself.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M2.5): launching `game_my_rpg` and walking with WASD/arrow keys → Iden faces the direction of travel and her sprite cycles through the walk frames. Idle (no movement) → Iden shows the standing frame for her current facing. Tick M2.5 row in GameDesign §9 in the same change.
- **Docs**: GameDesign §9 gets a new M2.5 row inserted; DesignDoc §22 gets a new "Milestone 2.5 — Sprite Animation" subsection inserted (no renumbering — later milestones stay M3-M10). CLAUDE.md "Current Status" updated to M2.5 done + M3 next.
- **Open questions (validate at apply time):**
  - **Walk-cycle frame order**: defaulting to `[walk1, stand, walk2, stand]` (standard JRPG: foot-down, recover, opposite-foot-down, recover). Alternative `[stand, walk1, stand, walk2]` starts on a recovery pose. Game Director's call if either feels wrong.
  - **Frame duration**: defaulting to `0.125f` (8 fps) per frame, so a full 4-frame cycle takes 0.5s — roughly 2 footsteps per second, classic JRPG pace. Tunable in one place.
  - **Idle clip frame count**: idle clips are 1 frame each (just the `_stand.png`). Could be 2-3 frames for a "breathing" idle (TimeFantasy ships emote/idle animations, but not as part of the standard walk cycle). Deferred.
