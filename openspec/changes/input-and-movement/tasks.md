## 1. Third-Party — Catch2

- [ ] 1.1 Extend `external/CMakeLists.txt` to pin Catch2 v3.x (latest stable release tag, e.g. `v3.7.1` — verify at apply time). Use the same FetchContent + static pattern as SDL3. Set `CATCH_BUILD_TESTING=OFF`, `CATCH_INSTALL_DOCS=OFF`, `CATCH_INSTALL_EXTRAS=OFF`. Surface `Catch2::Catch2WithMain` as the link target. Touches: external dep.
- [ ] 1.2 Verify Catch2 builds on Windows (MSVC, Ninja) via the next configure. Touches: external smoke (Windows).
- [ ] 1.3 Verify Catch2 builds on Linux (WSL2, g++-10, Ninja). Touches: external smoke (Linux).

## 2. CMake Test Infrastructure

- [ ] 2.1 Add `cmake/StarfallTests.cmake` with `starfall_add_test(NAME ... SOURCES ... DEPENDS ...)` per design D10: creates an executable, links `Catch2::Catch2WithMain` + the listed deps, calls `catch_discover_tests(<target>)`, applies the shared warnings policy from `cmake/StarfallWarnings.cmake`. Include from root `CMakeLists.txt` after the existing `StarfallModule` include. Touches: cmake helper.
- [ ] 2.2 Update root `CMakeLists.txt`: `enable_testing()` (call before any `add_test`/`catch_discover_tests`), `include(CTest)` once, and `add_subdirectory(tests)` AFTER the engine + game subdirs. Touches: root build graph.
- [ ] 2.3 Add `tests/CMakeLists.txt` that conditionally `add_subdirectory`s each per-module test dir if its CMakeLists exists (so adding tests later is one new directory, not edits in two places). Touches: tests root.
- [ ] 2.4 Add a `testPresets` block to `CMakePresets.json`: one entry per configure preset, each pointing at the corresponding `build/<preset>` build dir, so `ctest --preset debug-windows` and `ctest --preset debug-linux` work without flags. Touches: presets.

## 3. engine_input (Real Code)

- [ ] 3.1 Add `include/engine/input/action_map.hpp` + `src/input/action_map.cpp`: `Engine::Input::ActionMap` with `Bind(std::string action, SDL_Scancode key)` and `Bindings(std::string_view action) → std::vector<SDL_Scancode>` (heterogeneous lookup via `std::unordered_map<std::string, ..., std::hash<std::string>, std::equal_to<>>`). Default ctor populates the M2 game action set (MoveUp/Down/Left/Right on WASD+arrows, Confirm on Enter+Space, Cancel on Escape) per the `input-actions` spec. Touches: `engine_input`.
- [ ] 3.2 Add `include/engine/input/input_state.hpp` + `src/input/input_state.cpp`: `Engine::Input::InputState` constructed with an `ActionMap` (stored by value). Maintains internal `unordered_map<std::string, struct{ bool now; bool prev; }>`. Exposes `OnEvent(const SDL_Event&)` (handles SDL_EVENT_KEY_DOWN / SDL_EVENT_KEY_UP, including `event.key.repeat` filtering so a held key doesn't generate spurious Pressed edges every frame), `BeginFrame()` (snapshots prev = now), `IsHeld(action)`, `IsPressed(action)`, `IsReleased(action)` per design D2. Touches: `engine_input`.
- [ ] 3.3 Update `src/input/CMakeLists.txt`: `starfall_add_module(NAME engine_input SOURCES action_map.cpp input_state.cpp DEPENDS engine_core SDL3::SDL3)`. Drop `placeholder.cpp`. Touches: `engine_input`.

## 4. engine_runtime (Grow Application)

- [ ] 4.1 Extend `include/engine/runtime/app_config.hpp`: add `std::function<void(float dt, Engine::Input::InputState&)> onUpdate;` (forward-declared `InputState`). Touches: `engine_runtime`.
- [ ] 4.2 Update `src/runtime/application.cpp` `Run()` per design D5 + spec: construct an `Engine::Input::InputState` after the AssetLoader; capture `SDL_GetPerformanceCounter()` at the top of each frame; pass `dt` to `onUpdate`. Frame body becomes `BeginFrame → drain events (route key events to input.OnEvent, check SDL_EVENT_QUIT) → onUpdate(dt, input) (try/catch) → Clear → onRender (try/catch) → Present`. `onUpdate` exception logs `[Runtime]` and skips the rest of the frame's draw (cleared frame still presented). Teardown gains InputState in reverse order. Touches: `engine_runtime`.
- [ ] 4.3 Update `src/runtime/CMakeLists.txt`: depends on `engine_core`, `engine_render`, `engine_assets`, `engine_input`, `SDL3::SDL3`. Touches: `engine_runtime`.

## 5. TimeFantasy Iden Sprite Integration

- [ ] 5.1 Inspect `games/my_rpg/assets/timefantasy_characters/sheets/chara2.png` dimensions to confirm the standard 3×4 frames-per-character layout and the per-frame size (likely 16×16). Document the inferred grid in a code comment in `main.cpp` so future contributors know. Touches: investigation only.
- [ ] 5.2 In `games/my_rpg/src/main.cpp`'s `onStart`: attempt `loader.LoadTexture("assets/timefantasy_characters/sheets/chara2.png")`. If successful, store its handle as the primary Iden sprite + remember the source-rect for the down-facing idle frame. If failed (paid asset not present — public contributors), fall back to the existing `iden_placeholder.png` with no source-rect. Touches: `game_my_rpg`.
- [ ] 5.3 In `onRender`, draw the active sprite at `player.position` using the appropriate source rect (or no rect for the placeholder fallback). The position originates from a `PlayerState { Math::Vec2 position{160.0f - frameW/2.0f, 90.0f - frameH/2.0f}; }` initialized to logical center. Touches: `game_my_rpg`.

## 6. Player Movement

- [ ] 6.1 Add an `onUpdate` lambda in `main.cpp`: read `input.IsHeld("MoveUp/Down/Left/Right")`; resolve a 4-directional cardinal direction (most-recently-pressed wins on diagonal — maintain a small `lastAxisPressed` state captured in the lambda); apply `player.position += direction * kIdenWalkSpeed * dt` where `kIdenWalkSpeed = 60.0f` logical px/sec (design D8). Touches: `game_my_rpg`.
- [ ] 6.2 Wire `onUpdate` into the `app.Configure({...})` call alongside the existing `onStart` and `onRender`. Touches: `game_my_rpg`.

## 7. Tests — engine_input

- [ ] 7.1 Add `tests/input/CMakeLists.txt` with `starfall_add_test(NAME engine_input_tests SOURCES action_map_tests.cpp input_state_tests.cpp DEPENDS engine_input)`. Touches: tests.
- [ ] 7.2 Add `tests/input/action_map_tests.cpp`: bind/query a scancode, query an unknown action returns empty, default-constructed ActionMap contains the documented M2 action set per the spec's default-action-set scenario. Touches: tests.
- [ ] 7.3 Add `tests/input/input_state_tests.cpp`: synthesize SDL_Event KEY_DOWN/UP for a bound scancode, verify Pressed→Held→Released edges, verify BeginFrame correctly transitions prev=now, verify a second KEY_DOWN with `event.key.repeat=true` does NOT re-fire Pressed (repeat filtering), verify multiple queries within a frame return the same value. Touches: tests.

## 8. Tests — Backfill M1 Deferred Items

- [ ] 8.1 Add `tests/assets/CMakeLists.txt` with `starfall_add_test(NAME engine_assets_tests SOURCES asset_loader_tests.cpp DEPENDS engine_assets SDL3::SDL3 SDL3_image::SDL3_image)`. Touches: tests.
- [ ] 8.2 Add `tests/assets/asset_loader_tests.cpp` with a small SDL fixture: `SDL_Init(SDL_INIT_VIDEO)` + create a hidden window + create an SDL_Renderer in test setup; teardown reverses it. Tests: load `iden_placeholder.png` twice → second call returns the same shared_ptr (cache hit), no second `[Assets] Loaded` log line. Load a known-missing path → `Result` is Err with the path in the message. Touches: tests + M1 backfill.
- [ ] 8.3 Add `tests/render/CMakeLists.txt` with `starfall_add_test(NAME engine_render_tests SOURCES renderer_tests.cpp DEPENDS engine_render engine_assets SDL3::SDL3)`. Touches: tests.
- [ ] 8.4 Add `tests/render/renderer_tests.cpp`: same SDL fixture shape. Test: `DrawSprite` with a null `TextureHandle` returns without crashing AND without throwing — exercise the rate-limited-warning path. Touches: tests + M1 backfill.

## 9. Verification (Both Platforms)

- [ ] 9.1 Windows (VS Dev Shell): `cmake --preset debug-windows && cmake --build --preset debug-windows && ctest --preset debug-windows`. All tests pass. Touches: end-to-end Windows.
- [ ] 9.2 Windows runtime smoke: launch `bin\game_my_rpg.exe`. Press W → Iden moves up. Press D → Iden moves right (not diagonal because most-recent-axis wins). Release all keys → Iden stops immediately (no inertia). Close window → exit 0. Touches: end-to-end Windows runtime.
- [ ] 9.3 Linux (WSL2 g++-10): same flow with `debug-linux`. Touches: end-to-end Linux.
- [ ] 9.4 Linux runtime smoke via WSLg: verify Iden moves with WASD; SIGTERM → exit 0. Touches: end-to-end Linux runtime.
- [ ] 9.5 Rule audit (`check-deps`): new edges `engine_runtime → engine_input`, `engine_input → SDL3::SDL3` confirmed clean against §6.3. Touches: rule audit.
- [ ] 9.6 Fallback test: temporarily rename `games/my_rpg/assets/timefantasy_characters/` so the paid sheet is missing. Confirm `game_my_rpg` falls back to the placeholder without error (just a warning log). Revert. Touches: public-contributor experience.

## 10. Docs Update (Milestone Contract)

- [ ] 10.1 In `docs/GameDesign.md` §9, mark M2 row complete: `✅ M2 Input & Movement (YYYY-MM-DD)` with brief annotation. Touches: GameDesign §9.
- [ ] 10.2 Update `CLAUDE.md` "Current Status": M2 done, next target M3 (Tilemap & Collision). Note the new test framework + ctest command. Touches: project root CLAUDE.md.
- [ ] 10.3 Update `README.md` "Quick Start" with a new `ctest --preset debug-<platform>` line under the build commands. Touches: README.
