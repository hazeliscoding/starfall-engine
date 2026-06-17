## 1. Repo Skeleton

- [x] 1.1 Create the directory tree from design D10: `/cmake`, `/external`, `/include/engine/{core,math,assets,render,input,audio,scene,scripting,runtime}`, `/src/{core,math,assets,render,input,audio,scene,scripting,runtime,editor}`, `/games/my_rpg/{src,assets,scripts,scenes}`, `/tools`, `/tests`. Add `.gitkeep` in each empty leaf so git tracks the shape. Touches: no CMake targets yet.
- [x] 1.2 Add a top-level `.gitignore` covering `build/`, IDE folders (`.vs/`, `.vscode/`, `.idea/`), and the FetchContent cache (`_deps/`). Touches: none.

## 2. CMake Helper And Module Enforcement (cmake/)

- [x] 2.1 Write `cmake/StarfallModule.cmake` exposing `starfall_add_module(NAME ... DEPENDS ...)` per design D4. Hardcode the §6.3 forbidden-edges table (engine_core ↛ render/input/audio/scene/scripting; engine_* ↛ game_*/editor_*; etc.). Touches: cmake helper only — no targets yet.
- [x] 2.2 Write `cmake/StarfallWarnings.cmake` with a single `starfall_set_warnings(<target>)` function (`-Wall -Wextra -Wpedantic -Werror` for gcc/clang; `/W4 /WX` for MSVC; disable a small named-and-justified set). Called from `starfall_add_module`. Touches: cmake helper only.
- [x] 2.3 Add a configure-time self-test: `cmake/tests/StarfallModuleTest.cmake` that loads StarfallModule.cmake and calls `_starfall_check_edge` directly on a known-forbidden + known-allowed matrix, wired via `cmake -P` behind a `STARFALL_RUN_CMAKE_SELFTESTS` option (default ON in Debug). Implementation deviation from "try_compile-style": script-mode invocation is simpler and tests the rule table directly without needing real targets. Touches: cmake helper validation.

## 3. Third-Party (external/SDL3)

- [x] 3.1 Write `external/CMakeLists.txt` that pins SDL3 via `FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.2.16)` and calls `FetchContent_MakeAvailable`. SDL_TEST/SDL_TESTS/SDL_EXAMPLES disabled; SDL_STATIC=ON, SDL_SHARED=OFF (Steam-friendly single binary). Alias `SDL3::SDL3` defined post-fetch since static-only builds don't auto-create it. Touches: SDL3 dependency only.
- [x] 3.2 Verify configure succeeds and `SDL3::SDL3` target is importable on a clean Windows checkout (MSVC + Ninja). Verified 2026-06-16 with VS 2026 / cmake 3.x / Ninja — configure clean in 42s, SDL3 3.2.16 fetched and built static. Touches: external integration smoke test.
- [x] 3.3 Verify configure succeeds on a clean Linux checkout (gcc or clang + Ninja). Verified 2026-06-17 in WSL2 Ubuntu 20.04 with cmake 4.3.3 / ninja 1.10.0 / g++-10 (CC=gcc-10 CXX=g++-10). SDL3 3.2.16 fetched and configured static. Touches: external integration smoke test.

## 4. Root CMakeLists And Presets

- [x] 4.1 Write root `CMakeLists.txt`: cmake 3.24, C++20 (no extensions), include `StarfallModule`, run the selftest via `execute_process(cmake -P ...)`, `add_subdirectory(external)`, then every `src/<module>/` subdir, then `src/editor/` and `games/my_rpg/`. Touches: root build graph.
- [x] 4.2 Write `CMakePresets.json` with four presets: `debug-windows`, `release-windows`, `debug-linux`, `release-linux`, each pinned to Ninja, build dir `build/${presetName}`, matching `CMAKE_BUILD_TYPE`, host-OS condition, and a corresponding `buildPresets` entry per configure preset. Touches: presets only.

## 5. engine_core (Real Code)

- [x] 5.1 Create `src/core/CMakeLists.txt` invoking `starfall_add_module(NAME engine_core DEPENDS)` (no deps). Add source files listed in 5.2–5.3. Touches: `engine_core` target.
- [x] 5.2 Implement `include/engine/core/logger.hpp` + `src/core/logger.cpp` per design D6: `SF_LOG_INFO`, `SF_LOG_WARN`, `SF_LOG_ERROR` macros that take a category string + printf-style format, writing to stdout/stderr with `[Category]` prefix. No external deps. Touches: `engine_core`.
- [x] 5.3 Implement `include/engine/core/result.hpp`: a minimal `Engine::Core::Result<T>` (success-or-error-string variant) as DesignDoc §7.1 + §20.3 specify. Header-only. Touches: `engine_core`.

## 6. Empty Engine Modules (Targets Only)

- [x] 6.1 For each of `engine_math`, `engine_assets`, `engine_render`, `engine_input`, `engine_audio`, `engine_scene`, `engine_scripting`: write `src/<name>/CMakeLists.txt` calling `starfall_add_module` with the correct §6.2 deps (render→core+math+assets; audio→core+assets; scene→core+math+assets; scripting→core+scene; math/assets/input→core). Each module gets `placeholder.cpp` with `namespace Engine::<Name> {}` so the static lib is non-empty. Touches: seven empty engine targets.
- [x] 6.2 Add the matching empty header dir markers under `include/engine/<name>/.gitkeep`. Touches: header tree shape.

## 7. engine_runtime (Real Code)

- [x] 7.1 Create `src/runtime/CMakeLists.txt` invoking `starfall_add_module(NAME engine_runtime DEPENDS engine_core SDL3::SDL3)`. Touches: `engine_runtime` target.
- [x] 7.2 Implement `include/engine/runtime/app_config.hpp` per design D7: `Engine::Runtime::AppConfig` aggregate with `title="Starfall"`, `windowWidth=1280`, `windowHeight=720`, `initialScene` (reserved, unused). Touches: `engine_runtime`.
- [x] 7.3 Implement `include/engine/runtime/application.hpp` + `src/runtime/application.cpp`: class `Engine::Runtime::Application` with `void Configure(AppConfig)`, `int Run()`. Run() validates that Configure was called (else log Core error, return non-zero per `runtime-window` spec). Touches: `engine_runtime`.
- [x] 7.4 Implement SDL3 init/window-create/event-loop/shutdown inside `Run()`: `SDL_Init(SDL_INIT_VIDEO)`; create window with title/size from config; loop on `SDL_WaitEventTimeout(16)` (design D8) until `SDL_EVENT_QUIT`; on any init failure, tear down partially-init subsystems, log `[Runtime]` error with `SDL_GetError()`, return non-zero. Touches: `engine_runtime`.

## 8. game_my_rpg (Thin Entry Point)

- [x] 8.1 Write `games/my_rpg/CMakeLists.txt`: `starfall_add_module(NAME game_my_rpg TYPE executable DEPENDS engine_runtime)`. Touches: `game_my_rpg` target.
- [x] 8.2 Write `games/my_rpg/src/main.cpp`: thin entry point with designated initializers, no SDL includes, no platform `#ifdef`s. Touches: `game_my_rpg`.

## 9. engine_editor (Stub Executable)

- [x] 9.1 Write `src/editor/CMakeLists.txt` declaring `engine_editor` as an executable with NO engine deps (per design D9). Touches: `engine_editor` target.
- [x] 9.2 Write `src/editor/main.cpp`: 3-line program that prints `[Editor] stub - not implemented` and returns 0. Touches: `engine_editor`.

## 10. Verification On Both Platforms

- [x] 10.1 On Windows: configure (42s) + build (279 targets) via debug-windows preset. `game_my_rpg.exe` opens window in 0.84s with title "Starfall", WM_CLOSE → exit 0 in 0.09s. `engine_editor.exe` prints stub message, exits 0. Touches: end-to-end Windows validation.
- [x] 10.2 On Linux: same flow with `debug-linux` preset. Verified 2026-06-17 in WSL2 Ubuntu 20.04 + WSLg (DISPLAY=:0, WAYLAND_DISPLAY=wayland-0 — SDL3 picked Wayland). `engine_editor` stub exits 0. `game_my_rpg` logs "Window opened: \"Starfall\" (1280x720)"; SIGTERM (SDL3 translates to SDL_EVENT_QUIT) → "Clean shutdown." log → exit 0 in 104ms (well under the 1s spec target). Required loosening `cmake/StarfallWarnings.cmake` for gcc: dropped `-Wold-style-cast` (fires inside SDL3 macro expansions) and added `-Wno-missing-field-initializers` (allow C++20 designated initializers with defaulted aggregate members). MSVC unaffected. Touches: end-to-end Linux validation + warnings policy.
- [x] 10.3 Sanity-check dependency rules: temporarily added `engine_render` to `engine_core` DEPENDS; configure failed at `cmake/StarfallModule.cmake:122` with `"forbidden dependency on 'engine_render'. Rule: engine_core MUST NOT depend on render/input/audio/scene/scripting/runtime ... See docs/DesignDoc.md §6.3."`; reverted, reconfigure clean in 1.9s. Used `debug-windows` instead of `debug-linux` (Linux unavailable; rule check is platform-neutral). Touches: enforcement smoke test.
- [x] 10.4 Sanity-check "editor optional at runtime": copied `game_my_rpg.exe` alone (SDL3 statically linked, no DLLs needed) to a clean temp dir with no editor binary; window opened with title "Starfall", WM_CLOSE → exit 0. Touches: runtime-isolation smoke test.
- [x] 10.5 Ran `check-deps` skill: 11 targets, 18 link edges audited; zero violations against §6.3. Touches: cross-check the cmake-time enforcement against the project-wide audit.

## 11. Docs Update (Milestone Contract)

- [x] 11.1 In `docs/GameDesign.md` §9, marked M0 row with ✅ + completion date (2026-06-16) and a "Linux verification pending" note. Touches: GameDesign §9.
- [x] 11.2 Updated `CLAUDE.md`'s "Current Status" section: M0 done on Windows (with Linux pending caveat), next target M1. Also updated Tech Stack to commit to SDL3 (no longer "SDL2 or SDL3"). Touches: project root CLAUDE.md.
