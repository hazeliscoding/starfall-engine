## Why

M1 puts a static sprite on screen. The next user-visible step per `docs/GameDesign.md` §9 (M2) is: **"Iden walks. Movement speed feels like Embercoast, not WASD demo."** Per `docs/DesignDoc.md` §22 M2 also requires input action mapping, player movement, and frame-coherent delta time — the input plumbing every later gameplay milestone depends on.

This change builds the input stack, adds a per-frame `onUpdate(dt, input)` hook to the engine→game contract, swaps Iden's procedural placeholder for a real TimeFantasy character sprite, and finally — because all three open M1 test items have been waiting on it — adopts Catch2 + CTest as the project's test framework. Backfilling those M1 tests in this change keeps the deferred list from growing.

## What Changes

- Add **`Engine::Input::ActionMap`** (in `engine_input`): a map of action-name → list of bound `SDL_Scancode` values. Default constructor populates the game's M2 action set (`MoveUp`/`Down`/`Left`/`Right` bound to WASD + arrow keys, `Confirm`/`Cancel` reserved but unbound for later milestones).
- Add **`Engine::Input::InputState`** (in `engine_input`): owns the action map; ingests `SDL_Event` instances via `OnEvent`; exposes per-frame queries `IsHeld(action)`, `IsPressed(action)`, `IsReleased(action)`. `Pressed`/`Released` are edge-triggered for exactly one frame; the runtime calls `BeginFrame()` once per frame before draining events so edges reset cleanly.
- Grow **`Engine::Runtime::Application`**: owns an `InputState` instance, dispatches all OS events through it, calls a new `onUpdate(float dt, Input::InputState&)` callback once per frame before `onRender`. Per-frame `dt` measured via `SDL_GetPerformanceCounter` deltas.
- Grow **`Engine::Runtime::AppConfig`** with `std::function<void(float dt, Engine::Input::InputState&)> onUpdate`. Optional — the default behavior is no-op (clear + render every frame, no game logic).
- Grow **`games/my_rpg`**: declares a `PlayerState { Math::Vec2 position; }` captured by the lambdas. `onUpdate` moves the position by `speed * dt` in the most-recently-pressed cardinal direction (4-directional, EarthBound-style snap — no acceleration, no diagonals). `onRender` draws the Iden frame at `position`, replacing the procedural placeholder with a sub-region of a real TimeFantasy character sheet.
- Add **`Engine::Render::Renderer::DrawSprite` source-rect path verification** — already part of M1's spec; M2 is the first caller that actually exercises it.
- Add **Catch2** (v3.x via FetchContent) and wire it through CTest. Add `cmake/StarfallTests.cmake` helper `starfall_add_test(<module>_tests ...)` that creates a CTest target per module so `ctest --preset debug-windows` runs them. Add tests for the new `input_actions` capability AND backfill `texture-assets` (cache-hit, invalid-path) and `2d-renderer` (invalid handle is noop) tests per M1's deferred items.
- Update **`docs/GameDesign.md`** §9 M2 row to ✅ when the slice runs on both platforms with Iden walking on the real sprite.

### Non-Goals

- No animation. Sprite frame is static — the middle (idle) frame of the down-facing walk cycle. Per-step animation lands in its own change (likely bundled with M3's tilemap movement).
- No diagonal movement. 4-directional only. Most-recently-pressed axis wins on diagonal key combinations.
- No JSON action-map config. Hardcoded default. JSON config arrives when the editor or a settings menu needs it.
- No controller / gamepad support. Keyboard only at M2. Controller is M10 per GameDesign §9.
- No input rebinding UI. Bindings are code-defined; runtime swap deferred.
- No physics, no collision, no map bounds. Iden can walk off-screen. M3 owns tilemaps + collision.
- No `onUpdate` callback receives the asset loader — `onStart` already loaded everything Iden needs. If a later change needs runtime asset loading from `onUpdate`, the callback signature grows then.
- No editor work. M5 owns editor.
- No `Time` Lua binding. M6 owns Lua.
- No coverage tooling, no CI test run. Local `ctest` is the M2 bar.

## Capabilities

### New Capabilities

- `input-actions`: `engine_input`'s capability for the action map, edge-triggered + held input queries, and the SDL_Event dispatch contract.

### Modified Capabilities

- `runtime-window`: `AppConfig` grows `onUpdate(dt, InputState&)`. `Application` owns the `InputState`, dispatches events through it, and calls `onUpdate` once per frame before `onRender`. Frame loop becomes `BeginFrame → drain events → onUpdate(dt, input) → Clear → onRender → Present`.
- `build-system`: adds Catch2 (vendored via `external/`) and a `starfall_add_test` helper that registers tests with CTest. Adds a test-execution preset/contract so `ctest --preset <preset>` is the canonical "run all tests" command.

## Impact

- **Affected modules**: `engine_input` gains real code (was placeholder). `engine_runtime` grows to own and dispatch input. `games/my_rpg` gains player state + movement + real-sprite rendering. Three modules (`engine_assets`, `engine_render`, the new `engine_input`) gain test coverage. `engine_core`, `engine_math`, `engine_audio`, `engine_scene`, `engine_scripting` unchanged.
- **Dependency-rule compliance**: `engine_input → engine_core + SDL3` (no new forbidden edges). `engine_runtime → engine_core + engine_render + engine_assets + engine_input + SDL3` (adds `engine_input` — within §6.3). Tests link only their target module + Catch2; no test target links across module boundaries except for fixture data.
- **New external dependencies**: Catch2 v3.x via FetchContent (static, similar to SDL3/SDL3_image setup).
- **New serialized formats**: none. (No JSON action-map config at M2 — deferred. When it lands it will include the standard `version` field.)
- **New Lua bindings**: none.
- **Performance budget**: input polling + 4-direction movement is sub-microsecond. Continue to target ≤ 5 ms per frame in Debug; M2 doesn't move that needle.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M2): launching `game_my_rpg`, pressing WASD/arrow keys makes Iden walk in 4 cardinal directions. Iden looks like a real RPG character (TimeFantasy sprite). Tick M2 row in `docs/GameDesign.md` §9 in the same change.
- **Docs**: GameDesign §9 M2 row ticked. README "Editor / clangd support" section unchanged. CLAUDE.md "Current Status" updated to M2 done + M3 next.
- **Test infrastructure once-and-done**: the M2 PR establishes Catch2 + CTest + the `starfall_add_test` helper. Every subsequent milestone (M3 tilemap, M5 editor, M6 Lua) adds tests via the same pattern — no per-milestone setup cost.
- **Open questions (validate at apply time):**
  - **Which TimeFantasy character sheet + slot for Iden?** Default plan: `chara2.png` slot 0, down-facing middle frame (sheet layout: 4 directions × 3 frames per direction per character, top-left character of the sheet). Game Director's final pick — different character / different sheet is a one-config-line change.
  - **Frame size:** TimeFantasy's hero sprites are typically 16×16 — confirm at apply time by inspecting `chara2.png` dimensions. If 32×32, the source-rect math scales; the only consequence is a chunkier on-screen Iden (still 4× scaled by logical presentation, so 32×32 logical → 128×128 displayed at 1280×720).
  - **Movement speed (logical px/sec):** Default 60 — at 4× scale that's 240 screen px/sec, which reads as "deliberate stroll" rather than "WASD demo dash." Game Director / RPG Systems Designer can tune; one constant in `games/my_rpg/src/main.cpp`.
  - **Test layout:** Per-module test targets (`engine_core_tests`, `engine_input_tests`, etc.) co-located in `tests/<module>/`. Alternative would be a single `engine_tests` umbrella. Per-module wins on isolation; per-module is the default. Reconsider if the count explodes.
