## Context

M1 produces a static sprite on a blue background. M2 (per DesignDoc §22 and GameDesign §9) needs Iden to walk and the player's input to drive that movement — and along the way, picks up two pieces of foundation work that everything after M2 will lean on: a real test framework, and a proper engine→game update hook.

Three spec slices frame the work:

- New `input-actions` — `engine_input`'s action map + edge-triggered state queries + SDL_Event dispatch contract.
- Modified `runtime-window` — `AppConfig` grows an `onUpdate(dt, InputState&)` callback, `Application` owns an `InputState` and dispatches events into it.
- Modified `build-system` — adds Catch2 via FetchContent, CTest integration, the `starfall_add_test` helper, and a per-preset test-runner contract.

The TimeFantasy character-sprite swap and the M1 deferred-test backfill are real-but-mechanical and don't merit their own capabilities — they live in implementation tasks.

## Goals / Non-Goals

**Goals:**

- Player input drives a visible Iden walk in 4 cardinal directions, with EarthBound-style snap (no acceleration, most-recently-pressed axis wins on diagonal key combinations).
- Iden looks like a real RPG character — TimeFantasy sprite cropped from a sheet, replacing M1's procedural placeholder.
- Test framework + CTest preset land once, and every subsequent milestone adds tests via the same one-liner helper.
- The M1 deferred test items (texture-cache hit, invalid-handle noop) ship in this change so the deferred list resets to zero.
- 60 FPS at idle. Frame budget ≤ 5 ms in Debug.

**Non-Goals:**

- No animation (idle pose only — the middle frame of the down-facing walk cycle).
- No diagonal movement, no acceleration, no inertia.
- No JSON action-map config, no rebinding UI, no controller support.
- No collision, no map bounds — Iden walks off-screen freely.
- No coverage tooling, no CI test runs. Local `ctest --preset <preset>` is the bar.
- No Lua exposure.

## Decisions

### D1: `InputState` ingests events, doesn't poll keyboard state

**Choice:** `Engine::Input::InputState::OnEvent(const SDL_Event&)` consumes `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` events that the runtime drains each frame. The runtime calls `BeginFrame()` once per frame before draining, which resets edge state.

**Why:** Event-driven is the SDL3 idiom; polling `SDL_GetKeyboardState` is allowed but mixes the dispatch model. Event-driven also handles focus-loss correctly (the OS sends KEY_UP for held keys when focus is lost, so held-state self-corrects).

**Alternative considered:** Poll `SDL_GetKeyboardState` once per frame. Simpler in isolation but means we maintain two parallel state models (events for window close, polling for keys), and we lose the "edge events for free" property. Rejected.

### D2: Edge-triggered `Pressed` / `Released`, frame-coherent

**Choice:** `IsPressed(action)` returns true for exactly one frame — the frame on which a key bound to that action transitioned from up to down. Same shape for `IsReleased`. `IsHeld` is the steady-state query.

**Why:** Game code wants "this key just went down" semantics for menu confirms, dialogue advances, attack inputs. Without edge-triggering, every game callsite has to maintain its own "was it down last frame?" bookkeeping — death by a thousand booleans.

**Implementation:** Internal state per action is `{ heldNow: bool, heldPrev: bool }` updated in `BeginFrame` (snapshot prev = now), then `OnEvent` mutates `heldNow` as key events arrive. `IsPressed = heldNow && !heldPrev`, `IsReleased = !heldNow && heldPrev`, `IsHeld = heldNow`.

### D3: Action-name keying via `std::string_view` lookups, `std::string` storage

**Choice:** Bindings are stored in `std::unordered_map<std::string, std::vector<SDL_Scancode>>`. Queries accept `std::string_view` and use heterogeneous lookup (C++20 transparent comparators) so callers don't allocate temporary strings per call.

**Why:** Hot path is `if (input.IsHeld("MoveUp"))` once per direction per frame. Heterogeneous lookup avoids the small-string allocation for query-side. Storage cost (the string in the map) is paid once at bind time, not per query.

**Alternative considered:** Compile-time action IDs (enum or string-hash). Faster but loses runtime configurability (JSON action maps later). String-with-heterogeneous-lookup is the sweet spot — fast enough, retains flexibility.

### D4: `onUpdate(dt, input)` signature — no Renderer or AssetLoader

**Choice:** `std::function<void(float dt, Engine::Input::InputState&)> onUpdate`. The asset loader is reached via `onStart`'s closure, the renderer via `onRender`'s closure — `onUpdate` is for state mutation only.

**Why:** Separating draw-time inputs from update-time inputs makes a tiny ECS-shaped point clear: data flows update → state → render. Keeping the renderer out of `onUpdate` also discourages the "draw something during update" anti-pattern that breaks layering (and would conflict with SDL_Renderer's frame-begin/end contract).

**Alternative considered:** Pass everything to every callback for max flexibility. Rejected — friction in the signature shows up later as misuse opportunities.

### D5: `dt` measured via `SDL_GetPerformanceCounter`

**Choice:** Runtime captures `SDL_GetPerformanceCounter()` at the top of each frame; `dt = (now - prev) / SDL_GetPerformanceFrequency()` as a float. First-frame `dt` is 0 (no previous timestamp), which is a defensible default since the first frame's update is essentially "place Iden at his starting position."

**Why:** `SDL_GetPerformanceCounter` is monotonic, high-resolution, and consistent across SDL platforms — same code path on Windows and Linux.

**Alternative considered:** Fixed-timestep update loop (DesignDoc §7.9 sketches this). Premature for M2 — no physics/collision yet to demand determinism. When M3 lands tile collision, revisit then.

### D6: Most-recently-pressed axis wins on diagonal key combination

**Choice:** If `MoveUp` and `MoveRight` are both held, the game's `onUpdate` checks the press order (or holds an internal "current direction" state mutated on each new press) and picks one axis.

**Implementation lives in `games/my_rpg`, not the engine.** The engine just reports "this action is held"; deciding what to do with two simultaneously-held movement actions is a gameplay decision.

**Why:** Engine stays generic — same input system supports a future game that wants 8-directional movement, or a menu that wants both arrows held to mean something. Putting the 4-dir resolution in game code keeps the engine agnostic.

**Alternative considered:** Make the engine resolve. Rejected — too opinionated, foreclosing future games.

### D7: TimeFantasy sprite via per-frame PNG (sheet pivot, see below)

**Original choice:** Use `sheets/chara2.png` with a `DrawSprite` source-rect to crop Iden's idle frame.

**Apply-time pivot:** The "sheets" in this pack are the artist's reference sheets — frames are 17×31 with non-uniform padding, not a clean programmatic grid. The `frames/` subtree ships each pose as its own standalone PNG (e.g. `frames/chara/chara2_1/down_stand.png`). Use those directly. No source-rect; the file IS the frame.

**Net effect:** Iden loads from `assets/timefantasy_characters/frames/chara/chara2_1/down_stand.png` (chara2 slot 1, down-facing, idle pose — a specific RPG character). M1's procedural placeholder remains the fallback when the paid pack isn't present.

**Why pivot:** Cleaner than reverse-engineering the artist's sheet layout. The `DrawSprite` source-rect parameter still lives in the engine API (already in M1's spec), but M2's game code doesn't need to exercise it — tilemap rendering at M3 will be the first real customer.

### D8: Movement speed = `60.0f` logical px/sec, defined in `games/my_rpg/src/main.cpp`

**Choice:** Hardcoded constant in `main.cpp`, named `kIdenWalkSpeed`. Update each frame as `position += direction * speed * dt`.

**Why:** 60 logical px/sec × 4× scale = 240 screen px/sec. At 320×180 logical, Iden crosses the screen in ~5 seconds — deliberate stroll pace, not run pace. Tunable by editing one constant.

**Alternative considered:** Put it in `AppConfig`. Rejected — gameplay tuning doesn't belong in the engine's config struct.

### D9: Catch2 v3 via FetchContent, static, `Catch2WithMain`

**Choice:** `external/CMakeLists.txt` adds Catch2 v3.x via the same FetchContent + static pattern as SDL3 / SDL3_image. Tests link `Catch2::Catch2WithMain` so we don't write a `main()` per test target.

**Why:** Catch2 v3 is the maintained line, comparable to GoogleTest in ergonomics, lighter to integrate (header-only-like build), and the assertion macros (`REQUIRE`, `CHECK`, `SECTION`) read well in test source.

**Alternative considered:** GoogleTest. Comparable on capability; Catch2 is smaller and less Google-coupled. Either would work; pick one and move on.

### D10: `starfall_add_test(NAME ... SOURCES ... DEPENDS ...)` helper

**Choice:** New `cmake/StarfallTests.cmake` exposes `starfall_add_test` that:
1. Creates an executable.
2. Links `Catch2::Catch2WithMain` + the listed `DEPENDS`.
3. Calls `catch_discover_tests(<target>)` so individual `TEST_CASE`s show up as separate CTest entries.
4. Sets the same warnings policy as `starfall_add_module`.

**Why:** Mirrors `starfall_add_module`'s shape so adding tests feels familiar.

### D11: Per-module test targets under `tests/<module>/`

**Choice:** `tests/input/CMakeLists.txt` declares `engine_input_tests`, `tests/assets/` declares `engine_assets_tests`, etc. Single root `tests/CMakeLists.txt` `add_subdirectory`s the ones that exist.

**Why:** Isolation — a test failure in `engine_assets_tests` doesn't block `engine_input_tests` from running. Failure attribution is clearer. Per-module build dependencies (e.g. `engine_assets_tests` needs a real `SDL_Renderer` for texture loading; `engine_input_tests` doesn't) stay scoped.

**Alternative considered:** Single `engine_tests` umbrella target. Simpler in CMake, worse for isolation. If we end up with 50 tiny targets it's a re-evaluate; for the M2 starting set (3 modules, ~10 tests total) per-module is fine.

### D12: Backfill the M1 deferred tests in M2

**Choice:** This change adds:
- `tests/input/action_map_tests.cpp` — bind/query, default action set
- `tests/input/input_state_tests.cpp` — press/hold/release edges, frame coherence
- `tests/assets/asset_loader_tests.cpp` — cache hit on second load, invalid path returns Err
- `tests/render/renderer_tests.cpp` — DrawSprite with null handle is a no-op (rate-limited warning)

**Why:** "Adopt a test framework but don't write tests with it" is the worst-of-both-worlds outcome. The M1 deferred items are concrete, small, and prove the framework works end-to-end before any later milestone leans on it.

**Constraint:** `engine_assets` tests need an `SDL_Renderer*`. We create a headless/offscreen renderer in test setup (`SDL_INIT_VIDEO` + a hidden window). The test target links `SDL3::SDL3` for this.

## Risks / Trade-offs

- **[Risk] `std::function` callbacks (now three: onStart, onUpdate, onRender) add a small heap allocation each at Configure time.** → Negligible (3 allocations once). If profiling ever flags it, switch to `function_ref` or fixed-size callable.
- **[Risk] Frame `dt` from `SDL_GetPerformanceCounter` can spike during alt-tab / window resize.** → Acceptable at M2 (no physics); when fixed-timestep arrives at M3+, this becomes a real consideration.
- **[Risk] TimeFantasy assets are paid + license-restricted, so the placeholder Iden is what ships in the public repo.** → `games/my_rpg/assets/timefantasy_characters/` is gitignored already. The runtime falls back to the placeholder if the TimeFantasy sheet isn't present, so contributors without the pack still get a buildable runtime that draws *something*. Listed as a task — graceful fallback in `onStart`.
- **[Risk] Catch2 v3 fetch + build adds ~30s to a clean configure.** → One-time cost per developer, paid in parallel with SDL3 fetches. Acceptable.
- **[Risk] Per-module test targets risk fragmenting setup code (each test target reinitializing SDL, etc).** → Add a tiny `tests/common/sdl_fixture.hpp` if duplication becomes real. For M2's 3 test targets, don't preemptively share.
- **[Risk] Diagonal-key resolution lives in game code, so two future games might both reinvent it.** → Acceptable. When the second game appears (this is the kind of thing the engine learns from a second use case, not a first one), promote the resolver into engine_input.
- **[Risk] Tests that need a renderer hit a real GPU.** → On WSL2 / headless CI later, this may need SDL's software renderer fallback or `SDL_HINT_RENDER_DRIVER=software`. Document; address when CI lands.
