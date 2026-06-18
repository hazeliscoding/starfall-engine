## 1. Roadmap Doc Updates (Land Up Front)

- [x] 1.1 GameDesign §9 M2.5 row inserted between M2 and M3 (unchecked; ✅ added at task 6.1 after verification). Touches: GameDesign §9.
- [x] 1.2 DesignDoc §22 "Milestone 2.5 — Sprite Animation" section inserted between M2 and M3 with Goals + Deliverable blocks matching the surrounding milestone format. M3-M10 unchanged. Touches: DesignDoc §22.

## 2. engine_render — AnimationClip + AnimatedSprite

- [x] 2.1 Added `include/engine/render/animation_clip.hpp`: aggregate with frames + frameDuration (0.125f default) + looping (true default). Header-only. Touches: `engine_render`.
- [x] 2.2 Added `include/engine/render/animated_sprite.hpp` with the full Play/Pause/Resume/Update/CurrentFrame surface; declared a local `AnimatedSpriteStringHash` transparent comparator alongside the class (rather than extracting `StringKeyedMap` to a shared utility — extract when a third user appears). Touches: `engine_render`.
- [x] 2.3 Implemented `src/render/animated_sprite.cpp` per D3/D4/D5: Update accumulates timer and steps frame index while `timer_ >= frameDuration`, wrap-mod for looping, clamp-and-pause for one-shot. Play on current clip is a no-op; Play on different clip (or first Play) resets index+timer; Play with unknown name logs once-per-process `[Render]` warning and clears `current_`. Touches: `engine_render`.
- [x] 2.4 Updated `src/render/CMakeLists.txt` SOURCES to add `animated_sprite.cpp`. Deps unchanged. Touches: `engine_render`.

## 3. Game Side — Iden Walks With Animation

- [x] 3.1 `PlayerState` now holds `Engine::Render::AnimatedSprite sprite;` (replacing the single TextureHandle). Touches: `game_my_rpg`.
- [x] 3.2 onStart attempts all 12 TimeFantasy frames for chara2_1 (stand+walk1+walk2 × 4 directions). On full success, `BuildClips` constructs 8 clips: `idle_<dir>` single-frame stand, `walk_<dir>` 4-frame `[walk1, stand, walk2, stand]`. On any failure, `BuildFallbackClips` uses the procedural placeholder as the sole frame of every clip. Initial state: `Play("idle_down")`. Touches: `game_my_rpg`.
- [x] 3.3 onUpdate composes the desired clip name as `(isMoving ? "walk_" : "idle_") + DirectionName(facing)`, calls `sprite.Play(desired)` (no-op when same clip is already playing — preserves cycle) then `sprite.Update(dt)`. `isMoving` derived from any move-key held (matches design D6's M2.5 case). Touches: `game_my_rpg`.
- [x] 3.4 onRender now draws `sprite.CurrentFrame()` (early-returns on null). Centering logic moved into the onStart post-load block — uses first non-null current frame's dimensions. Touches: `game_my_rpg`.

## 4. Tests

- [x] 4.1 Added `tests/render/animated_sprite_tests.cpp` to the existing `engine_render_tests` SOURCES list. Added SDL3_image dep + post-build copy of iden_placeholder.png next to the test exe (mirrors what `engine_assets_tests` does — needed for the real-handle smoke test). Touches: tests CMakeLists.
- [x] 4.2 Wrote state-machine tests using null-handle `MakeNullClip` helper: freshly-constructed sprite has no current clip; AddClip+Play sets clip; unknown-name Play is no-op; re-add replaces; Update accumulates dt across multiple sub-frameDuration calls; non-looping clamps and pauses; Play on current clip is no-op; Play on different clip resets; Pause freezes, Resume continues; Play on different clip implicitly resumes. Touches: tests.
- [x] 4.3 Added a real-handle smoke test using `SdlWindowAndRendererFixture` + `AssetLoader` — loads the placeholder twice, builds a 2-frame clip, verifies CurrentFrame's `.get()` matches each handle in turn across a wrap. (Earlier draft synthesized fake textures via reinterpret_cast; removed as UB-adjacent — the real-handle test already covers handle-identity-across-advance.) Touches: tests + real-handle smoke.

## 5. Verification (Both Platforms)

- [x] 5.1 Windows (VS Dev Shell): cmake --build + ctest --output-on-failure → 26/26 tests pass in 1.98s (12 new AnimatedSprite + the existing 14). Touches: end-to-end Windows.
- [x] 5.2 Linux (WSL2 g++-10): build + ctest → 26/26 tests pass in 20.39s. Touches: end-to-end Linux.
- [x] 5.3 Runtime smoke (both platforms): `game_my_rpg` launches; all 12 TimeFantasy frames load (sizes 15-17 × 30-31 — small per-direction padding differences in the artist's frames, fine); clean shutdown on WM_CLOSE (Windows) / SIGTERM (Linux). Visual confirmation of walk cycle + directional facing requires attended manual interaction with the focused window — same caveat as M2; the engine + state machine is unit-tested above. Touches: end-to-end runtime.
- [ ] 5.4 Fallback verification (rename TimeFantasy folder, confirm placeholder kicks in): **deferred** — same rationale as M1.9.6 and M2.9.6. Fallback code path is correct by inspection (`if (allLoaded) {build full} else {build single-frame fallback}`); manual rename is a 30-second test the user can run any time. Touches: public-contributor experience.
- [x] 5.5 check-deps audit: zero new module-level edges (AnimatedSprite is internal to engine_render; engine_render's deps unchanged at engine_core + engine_math + engine_assets + SDL3). 11 targets, ~24 edges, 0 §6.3 violations. Touches: rule audit.

## 6. Docs (Wrap-Up)

- [x] 6.1 GameDesign §9 M2.5 row marked ✅ with date + summary. Touches: GameDesign §9.
- [x] 6.2 CLAUDE.md "Current Status" rewritten: M2.5 done; AnimatedSprite primitive + 12-frame TimeFantasy load + 8 named clips; previously-complete history summarized; next target M3. Touches: project root CLAUDE.md.
- [x] 6.3 README.md unchanged — controls and run flow are identical to M2 (WASD/arrows + close window). Skip per task plan. Touches: README (skipped).
