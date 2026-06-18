## 1. Roadmap Doc Updates (Land Up Front)

- [ ] 1.1 Insert M2.5 row in `docs/GameDesign.md` §9 between the M2 and M3 rows: `| M2.5 Sprite Animation | Iden faces direction of travel. Walk cycle plays when moving, idle frame when standing. |` (unchecked initially; ✅ when shipped per task 10.1). Touches: GameDesign §9.
- [ ] 1.2 Insert a "Milestone 2.5 — Sprite Animation" section in `docs/DesignDoc.md` §22, after the M2 block and before the M3 block. Mirror the format of the other milestone sections (Goals + Deliverable). No renumbering of M3-M10. Touches: DesignDoc §22.

## 2. engine_render — AnimationClip + AnimatedSprite

- [ ] 2.1 Add `include/engine/render/animation_clip.hpp`: `Engine::Render::AnimationClip` aggregate — `std::vector<Engine::Assets::TextureHandle> frames`, `float frameDuration = 0.125f`, `bool looping = true`. Header-only. Touches: `engine_render`.
- [ ] 2.2 Add `include/engine/render/animated_sprite.hpp`: `Engine::Render::AnimatedSprite` class — internal `StringKeyedMap<AnimationClip>` (re-use the StringHash transparent comparator pattern from `engine_input`; declare it locally in animated_sprite.hpp since `StringKeyedMap` isn't currently a shared utility), `std::string current_`, `float timer_`, `std::size_t frameIndex_`, `bool paused_`. Surface: `AddClip(std::string name, AnimationClip clip)`, `Play(std::string_view name)`, `Pause()`, `Resume()`, `Update(float dt)`, `CurrentFrame() → TextureHandle`, `CurrentClip() → const std::string&`, `IsPlaying() → bool`. Touches: `engine_render`.
- [ ] 2.3 Add `src/render/animated_sprite.cpp` implementing the methods per design D3 (Play-on-current is no-op; Play-on-different resets index+timer), D4 (frameDuration default 0.125f via AnimationClip), D5 (frame order is the caller's responsibility — the engine just plays frames in order). Update advances timer; while `timer_ >= frameDuration`, subtract and increment index (wrapping or clamping per `looping`). Play with unknown name logs once-per-process `[Render]` warning and clears `current_`. Touches: `engine_render`.
- [ ] 2.4 Update `src/render/CMakeLists.txt` SOURCES list to add `animated_sprite.cpp`. Deps unchanged. Touches: `engine_render`.

## 3. Game Side — Iden Walks With Animation

- [ ] 3.1 Update `games/my_rpg/src/main.cpp` `PlayerState`: add `Engine::Render::AnimatedSprite sprite;` field. Remove the existing single-TextureHandle `sprite` field (replaced by what's inside `AnimatedSprite`). Touches: `game_my_rpg`.
- [ ] 3.2 In `onStart`: attempt to load all 12 TimeFantasy frames for `chara/chara2_1/` — for each direction in `{down, up, left, right}`, load `{<dir>_stand.png, <dir>_walk1.png, <dir>_walk2.png}`. If ALL 12 succeed, construct 8 clips: `idle_<dir>` with frames `[stand]` (single-frame), `walk_<dir>` with frames `[walk1, stand, walk2, stand]` per design D5. If ANY of the 12 fails, fall back to building all 8 clips with a single-frame placeholder per design D7 (load `assets/characters/iden_placeholder.png` once, use it as the sole frame of every clip). Call `player.sprite.Play("idle_down")` as the initial state. Touches: `game_my_rpg`.
- [ ] 3.3 In `onUpdate`, after the existing direction-resolution + position-update logic: derive `isMoving` per design D6 (`any move key held` covers the M2.5 no-collision case; the position-diff term lands when M3 collision arrives). Compose the desired clip name as `(isMoving ? "walk_" : "idle_") + directionName(player.facing)` where `directionName` maps `Up/Down/Left/Right → "up"/"down"/"left"/"right"`. Call `player.sprite.Play(desired)` then `player.sprite.Update(dt)`. Touches: `game_my_rpg`.
- [ ] 3.4 Update `onRender` to draw `player.sprite.CurrentFrame()` instead of the old fixed handle. Skip if `CurrentFrame()` is null (fallback edge case where nothing loaded). Re-center on first non-null frame's width/height after `onStart` (the centering logic should move into the post-load block since we don't have a stable single sprite to size against anymore). Touches: `game_my_rpg`.

## 4. Tests

- [ ] 4.1 Add `tests/render/animated_sprite_tests.cpp` (sits in the existing `tests/render/CMakeLists.txt` SOURCES list, no new test target needed). Add `animated_sprite_tests.cpp` to that SOURCES list. Touches: tests CMakeLists.
- [ ] 4.2 Write the timing/index tests (no SDL fixture needed — use empty `AnimationClip`s with placeholder null TextureHandles where appropriate): clip add/play sets current clip; Update advances index at frameDuration intervals; looping wraps; non-looping clamps; Pause freezes Update; Resume continues; Play with current-clip name is a no-op (frame index unchanged); Play with different name resets index + timer to 0; Play with unknown name returns null CurrentFrame. Touches: tests.
- [ ] 4.3 Add ONE test that uses the existing `SdlWindowAndRendererFixture` + a real `AssetLoader` to load the existing placeholder PNG twice (frame index 0 and 1 of a 2-frame clip), and verify `CurrentFrame()` returns the correct handle by `.get()` equality before vs after a frame advance. Touches: tests + real-handle smoke.

## 5. Verification (Both Platforms)

- [ ] 5.1 Windows (VS Dev Shell): `cmake --build --preset debug-windows && ctest --preset debug-windows`. All new + existing tests pass (17+ total now). Touches: end-to-end Windows.
- [ ] 5.2 Linux (WSL2 g++-10): same flow. Touches: end-to-end Linux.
- [ ] 5.3 Runtime smoke (both platforms): launch `game_my_rpg`, walk in each cardinal direction, confirm: (a) Iden's sprite faces the direction of travel, (b) the walk cycle visibly plays (~2 footsteps/sec), (c) releasing all keys reverts to the idle frame for whatever direction was last faced. Touches: end-to-end runtime.
- [ ] 5.4 Fallback verification: rename `games/my_rpg/assets/timefantasy_characters/` to disable the paid pack. Confirm `game_my_rpg` still launches, shows the placeholder, and movement still works (just visually static — the placeholder is the only frame in every clip). Revert the rename. Touches: public-contributor experience.
- [ ] 5.5 Rule audit (`check-deps`): no new edges introduced; confirm clean. Touches: rule audit.

## 6. Docs (Wrap-Up)

- [ ] 6.1 Mark the GameDesign §9 M2.5 row ✅ with date + sprite/animation summary. Touches: GameDesign §9.
- [ ] 6.2 Update `CLAUDE.md` "Current Status": M2.5 done; AnimatedSprite primitive landed; next target M3 (Tilemap & Collision). Touches: project root CLAUDE.md.
- [ ] 6.3 Keep `README.md` unchanged unless something in the run-the-game story changed (e.g. controls). No change expected. Touches: README (skip if unchanged).
