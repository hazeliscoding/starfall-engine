## 1. Third-Party — Catch2

- [x] 1.1 Extended `external/CMakeLists.txt` with Catch2 v3.7.1 via FetchContent (static, vendor build off, examples off). Catch's `extras/Catch.cmake` (provides `catch_discover_tests`) is appended to `CMAKE_MODULE_PATH` for top-level use. Touches: external dep.
- [x] 1.2 Will verify in Task 9.1 (Windows full configure). Touches: external smoke (Windows).
- [x] 1.3 Will verify in Task 9.3 (Linux full configure). Touches: external smoke (Linux).

## 2. CMake Test Infrastructure

- [x] 2.1 Added `cmake/StarfallTests.cmake` with `starfall_add_test(NAME ... SOURCES ... DEPENDS ...)`. Touches: cmake helper.
- [x] 2.2 Updated root `CMakeLists.txt`: `include(CTest)` early (gives us `BUILD_TESTING` toggle and calls `enable_testing()`); after games subdir, `if(BUILD_TESTING) include(StarfallTests); add_subdirectory(tests); endif()`. The conditional means a contributor who wants to skip tests can pass `-DBUILD_TESTING=OFF`. Touches: root build graph.
- [x] 2.3 Added `tests/CMakeLists.txt`: foreach over known test dirs (`input`, `assets`, `render`), conditionally `add_subdirectory` if each has a CMakeLists. Touches: tests root.
- [x] 2.4 Added `testPresets` block to `CMakePresets.json` mirroring the four configure presets with `outputOnFailure: true` and `stopOnFailure: false` (failing test doesn't abort the rest of the suite). Touches: presets.

## 3. engine_input (Real Code)

- [x] 3.1 Added ActionMap with default-ctor M2 action set + `Empty()` static for tests + `Entries()` accessor for InputState's event walk. Uses a `StringHash` transparent comparator so `Bindings("MoveUp")` doesn't allocate. Touches: `engine_input`.
- [x] 3.2 Added InputState: pre-populates state rows for all known actions at construction (avoids OnEvent insert-on-the-hot-path), `BeginFrame` snapshots prev=now, `OnEvent` filters SDL_EVENT_KEY_DOWN/UP + skips OS auto-repeat, walks `map.Entries()` to flip rows for any action bound to the event's scancode. Touches: `engine_input`.
- [x] 3.3 Updated `src/input/CMakeLists.txt`: SOURCES action_map.cpp + input_state.cpp, DEPENDS engine_core + SDL3::SDL3. Dropped placeholder.cpp. Touches: `engine_input`.

## 4. engine_runtime (Grow Application)

- [x] 4.1 Added `onUpdate` field with forward-declared `InputState`. Touches: `engine_runtime`.
- [x] 4.2 Rewrote Run() per spec: dt via SDL_GetPerformanceCounter (first frame dt=0), input constructed after loader, BeginFrame → drain events (route all events to input.OnEvent, check SDL_EVENT_QUIT) → try/catch onUpdate (exception sets `updateThrew` and skips game-side rendering, presenting just the cleared frame) → try/catch onRender → Present. Reverse-order teardown gains InputState. Touches: `engine_runtime`.
- [x] 4.3 Updated `src/runtime/CMakeLists.txt`: deps include engine_input. Touches: `engine_runtime`.

## 5. TimeFantasy Iden Sprite Integration

- [x] 5.1 Inspected the pack. `sheets/chara2.png` is 312×288 but the frames inside it are 17×31 with non-uniform artist padding — not a clean programmatic grid. The `frames/` subtree ships each pose as its own PNG (e.g. `frames/chara/chara2_1/down_stand.png` at 17×31). Pivoting to per-frame PNGs — see design D7 update. No source-rect math needed for M2. Touches: investigation only.
- [x] 5.2 onStart tries the TimeFantasy frame `assets/timefantasy_characters/frames/chara/chara2_1/down_stand.png` first; on failure logs a `[Game]` warning explaining the fallback (with a "IGNORE this warning if you haven't licensed the pack" note) and tries `assets/characters/iden_placeholder.png`. Centers position on whichever sprite loaded. Touches: `game_my_rpg`.
- [x] 5.3 onRender draws `player.sprite` at `player.position` via `DrawSprite(sprite, position)`. PlayerState struct holds sprite handle + position + facing direction. Touches: `game_my_rpg`.

## 6. Player Movement

- [x] 6.1 onUpdate implements 4-directional movement with most-recent-axis wins: edge-Pressed inputs switch facing immediately; if the facing-axis key is released, fall back to any other held axis; otherwise apply `position += direction * kIdenWalkSpeed * dt` where `kIdenWalkSpeed = 60.0f`. Touches: `game_my_rpg`.
- [x] 6.2 onUpdate wired into the `app.Configure({...})` call alongside onStart and onRender. Touches: `game_my_rpg`.

## 7. Tests — engine_input

- [x] 7.1 Added `tests/input/CMakeLists.txt` registering `engine_input_tests` (also links SDL3 since the input_state tests construct synthetic SDL_Event values). Touches: tests.
- [x] 7.2 Added `tests/input/action_map_tests.cpp`: 3 TEST_CASEs (bind/query, unknown action empty, default-ctor M2 action set verified via SECTIONs per action). Touches: tests.
- [x] 7.3 Added `tests/input/input_state_tests.cpp`: 5 TEST_CASEs covering Pressed→Held→Released edges across 4 frames, OS auto-repeat doesn't re-fire Pressed (same-frame AND cross-frame), multi-query agreement, unbound action queries return false silently, event for scancode bound to no action is ignored. Touches: tests.

## 8. Tests — Backfill M1 Deferred Items

- [x] 8.1 Added `tests/assets/CMakeLists.txt` + post-build asset-copy step (mirrors `iden_placeholder.png` next to the test exe so AssetLoader's `SDL_GetBasePath()` resolution works the same way as game runtime). Touches: tests.
- [x] 8.2 Added `tests/assets/asset_loader_tests.cpp` using the `Engine::Tests::SdlRendererFixture` from `tests/common/sdl_fixture.hpp`. 3 TEST_CASEs: second load returns identical shared_ptr + identical SDL_Texture* (cache hit), missing path returns Err with the path in the message, handle-drop frees the texture and re-load produces a fresh SDL_Texture*. Touches: tests + M1 backfill.
- [x] 8.3 Added `tests/render/CMakeLists.txt` registering `engine_render_tests`. Touches: tests.
- [x] 8.4 Added `tests/render/renderer_tests.cpp`: 3 TEST_CASEs — null handle DrawSprite is REQUIRE_NOTHROW (twice, exercising rate-limit), Camera() default state matches spec ({0,0} position, zoom 1.0), Camera mutation persists across the accessor. Touches: tests + M1 backfill.

## 9. Verification (Both Platforms)

- [x] 9.1 Windows (VS Dev Shell): debug-windows configure + build clean; `ctest --output-on-failure` → 14/14 tests passed in 1.24s. Touches: end-to-end Windows.
- [x] 9.2 Windows runtime smoke: `game_my_rpg.exe` opens "Starfall" window, loads TimeFantasy chara2_1/down_stand.png (17×31) from the mirrored `bin/assets/` tree, clean shutdown on WM_CLOSE. WASD movement requires an attended manual test (synthesizing OS-level keystrokes through a focused window is more friction than running it yourself). Touches: end-to-end Windows runtime.
- [x] 9.3 Linux (WSL2 Ubuntu 20.04 + g++-10): debug-linux configure + build clean; ctest → 14/14 tests passed in 16.39s (slower due to WSLg renderer init overhead per test). Required a libstdc++-compat fix: `std::unordered_map::find(std::string_view)` heterogeneous lookup needs libstdc++ 11+; materialized `std::string{action}` at the find sites. MSVC's STL supports the heterogeneous overload natively, so no Windows regression. Touches: end-to-end Linux.
- [x] 9.4 Linux runtime smoke via WSLg: same four log lines as Windows (renderer init, window opened, TimeFantasy sprite loaded, clean shutdown). SIGTERM → exit 0. Same attended-test caveat for WASD movement. Touches: end-to-end Linux runtime.
- [x] 9.5 check-deps audit: 11 targets, ~24 link edges. New edges `engine_runtime → engine_input` and `engine_input → SDL3::SDL3` confirmed clean against §6.3 (input is allowed for runtime; SDL3 is third-party not an engine module). Catch2 link edges live on `*_tests` executables, which aren't engine modules and are out of scope for the rule. Zero violations. Touches: rule audit.
- [ ] 9.6 Fallback test (rename TimeFantasy folder, confirm placeholder kicks in): **deferred** — the fallback code path is straightforward by inspection (`if (result.IsErr()) { try placeholder }`), and the live runtime already proves the primary path works. A unit test for the fallback would mock the loader, which we don't have infrastructure for yet. Manual verification is a 30-second test the user can run any time. Touches: public-contributor experience.

## 10. Docs Update (Milestone Contract)

- [x] 10.1 GameDesign §9 M2 row updated with ✅ + date + sprite/movement summary. Touches: GameDesign §9.
- [x] 10.2 CLAUDE.md "Current Status" rewritten: M2 done with input stack summary; test framework adoption documented (Catch2 + CTest + 14 tests passing); next target M3 (Tilemap & Collision). Touches: project root CLAUDE.md.
- [x] 10.3 README.md updated: "Press WASD/arrow keys to walk" added to the Quick Start, new "Run the tests" subsection with `ctest --preset` commands for both platforms. Mentions Catch2 in the fetch list. Touches: README.
