## Why

The repository currently has design docs and zero source. Without a build graph and a runnable executable, every subsequent milestone (sprite rendering, input, tilemaps, Lua) has nowhere to land. DesignDoc §29 names this as the immediate next step, and GameDesign §9 ties it to a concrete game deliverable: "Window opens. Title bar reads 'Starfall.'"

This change bootstraps the CMake build graph that DesignDoc §6 describes and stands up the thinnest possible vertical slice — `game_my_rpg` calls `engine_runtime`, which opens a cross-platform SDL window. Everything else gets built on top of this scaffolding.

## What Changes

- Add root `CMakeLists.txt` declaring C++20, the project, and all module targets named in DesignDoc §6.1 (initially as empty/skeleton libraries — only the ones needed for the window slice carry real code).
- Add `CMakePresets.json` with `debug-windows`, `release-windows`, `debug-linux`, `release-linux` presets (Ninja-preferred).
- Add `external/` dependency management for SDL (vendored or fetched via `FetchContent` — decision in design.md).
- Scaffold module directories that compile in this change: `src/core/`, `src/runtime/`, plus header trees under `include/engine/core/` and `include/engine/runtime/`. Other engine modules (`math`, `assets`, `render`, `input`, `audio`, `scene`, `scripting`, `editor`) get target declarations and empty source folders so the dependency graph is wired up, but ship no implementation in this change.
- Implement a minimal `Engine::Core` surface needed by runtime: logger, `Result<T>`, a tiny app-config struct.
- Implement `Engine::Runtime::Application` — SDL init, window creation, an event-pump loop that exits on close, SDL shutdown.
- Scaffold `games/my_rpg/` with `CMakeLists.txt`, `src/main.cpp` that configures `Application` with title `"Starfall"` and runs it, and an empty `assets/`, `scripts/`, `scenes/` so the layout is real.
- Add `engine_editor` as an empty executable target (linking only engine modules) so the dependency rules are enforceable from day one. It compiles but does nothing yet.
- Add `cmake/` helper module that enforces module dependency rules (a single helper function `starfall_add_module(NAME … DEPENDS …)` that fails configure if a module declares a forbidden dependency from DesignDoc §6.3).
- Mark M0 done in `docs/GameDesign.md` §9 once the slice runs on both platforms.

### Non-Goals

- No rendering of anything inside the window — the window is cleared each frame but no sprites, no ImGui, no text. M1 owns first pixels.
- No input handling beyond the OS close event. M2 owns input mapping.
- No Lua, no scenes, no assets loaded, no audio. The respective module folders exist but ship no code yet.
- No editor functionality — `engine_editor` is an empty `main()` that exits 0. M5 owns the editor.
- No test framework yet. Catch2/GoogleTest selection deferred to whichever module first needs tests (likely M1 or M3).
- No CI configuration in this change — local `cmake --preset … && cmake --build …` on Windows + Linux is the bar.
- No SDL2-vs-SDL3 abstraction layer. Pick one, depend on it directly.

## Capabilities

### New Capabilities

- `build-system`: The CMake target graph, presets, dependency-rule enforcement, and third-party (SDL) acquisition strategy that the whole engine builds on.
- `runtime-window`: The `engine_runtime::Application` lifecycle (init → loop → shutdown) and the `game_my_rpg` thin entry point that drives it. Owns the cross-platform window contract.

### Modified Capabilities

None — no specs exist yet.

## Impact

- **Affected modules**: introduces `engine_core` and `engine_runtime` as the only modules with real code. Declares (empty) targets for `engine_math`, `engine_assets`, `engine_render`, `engine_input`, `engine_audio`, `engine_scene`, `engine_scripting`, `engine_editor`, and `game_my_rpg`.
- **Dependency-rule compliance**: `engine_runtime → engine_core` (plus SDL via the runtime — see design.md for why SDL lives at the runtime layer, not in `engine_core`). `game_my_rpg → engine_runtime`. `engine_editor` links no engine modules yet (will link non-runtime modules as they grow). No engine target depends on game or editor code. `engine_core` keeps zero third-party deps so it remains the boring stable base DesignDoc §7.1 calls for.
- **New external dependencies**: SDL (version pinned in design.md). No Lua, no ImGui, no JSON library yet.
- **Platforms**: Windows + Linux. macOS deliberately out of scope per DesignDoc §1.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M0): launching `game_my_rpg` opens a window titled "Starfall" on Windows and Linux. This is the M0 row of GameDesign §9's milestone table and must be checked off in the same change.
- **Docs**: `docs/GameDesign.md` §9 M0 row gets ticked. No DesignDoc edits required — this change implements what §6 and §29 already specify.
- **Serialized formats**: none introduced.
- **Lua bindings**: none introduced.
