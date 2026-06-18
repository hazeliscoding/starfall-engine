# Starfall Engine

A modular C++20 2D RPG game engine inspired by EarthBound, RPG Maker, and classic SNES/GBA RPGs. Built alongside a real game (`games/my_rpg/`) using vertical-slice development.

Two design docs, two owners:

- **`docs/DesignDoc.md`** — the engine architecture (this is what the engineers read).
- **`docs/GameDesign.md`** — the specific game being built (owned by the Game Director agent — theme, story, characters, world, v1 scope).

The two are bound by the milestone contract in `DesignDoc.md` §30: every engine milestone produces a visible game improvement. If the game falls behind, engine work pauses.

## Tech Stack

- **Language**: C++20, Lua (gameplay scripting)
- **Build**: CMake + CMakePresets.json (Ninja preferred)
- **Platform**: SDL3 (window, input, audio) — pinned in `external/CMakeLists.txt`
- **Editor UI**: Dear ImGui
- **Serialization**: JSON (development); binary/packed later
- **Testing**: Catch2 or GoogleTest
- **Targets**: Windows + Linux, Steam-friendly distribution

## Architecture

The engine is a set of static libraries linked by thin executables. Module dependency rules are strict — see `docs/DesignDoc.md` §6.3.

```
engine_core ← engine_math, engine_assets, engine_render, engine_input,
              engine_audio, engine_scene, engine_scripting
engine_runtime ← all of the above
game_my_rpg ← engine_runtime, engine_scene, engine_scripting
engine_editor ← engine modules (NOT runtime, NOT game)
```

**Three-way separation:**
- Engine provides **capability** (C++)
- Game provides **behavior** (Lua + thin C++ entry point)
- Editor provides **tooling** (separate executable, not required at runtime)

## Hard Rules (do not break)

1. Engine code must NOT depend on game code or editor code.
2. `engine_core` must NOT depend on render/input/audio/scene/scripting.
3. Editor must NOT be required for the shipped runtime.
4. Lua must NOT mutate engine internals directly — only through curated bindings (`Entity`, `Scene`, `Input`, `Audio`, `Dialogue`, `Time`, `Math`).
5. Scene data must be serializable (JSON, versioned).
6. Game logic belongs in Lua unless there's a strong perf/platform reason.
7. Every serialized asset must include a `version` field.

## Soft Rules

- Prefer simple systems before abstract ones — don't introduce a render backend abstraction until a second backend is actually needed.
- Build vertical slices: every milestone improves both the engine AND the game.
- Avoid premature optimization, but establish performance budgets up front.
- Prefer readable data formats (JSON) early; pack/optimize later.

## Repository Layout

```
/cmake          /external       /docs
/include        /src            /tools
/games/my_rpg   /tests
CMakeLists.txt  CMakePresets.json
```

`/src` is split into per-module subdirectories matching the CMake targets: `core`, `math`, `assets`, `render`, `input`, `audio`, `scene`, `scripting`, `runtime`, `editor`.

## Spec-Driven Development

This project uses OpenSpec (`/openspec`) for change proposals and specs. When proposing non-trivial engine features, write a proposal in `openspec/changes/` before implementation. Config lives in `openspec/config.yaml`.

## Current Status

**M2 Input & Movement: complete on Windows and Linux (2026-06-18).** Input stack landed: `Engine::Input::ActionMap` (string-keyed bindings, default M2 game action set on WASD+arrows) + `Engine::Input::InputState` (edge-triggered Pressed/Released, steady-state Held, frame-coherent, OS-repeat-filtered). `Application` now drains events through the input state and exposes an `onUpdate(dt, input)` callback for game-state mutation. Iden walks 4-directionally with most-recent-axis-wins on diagonal key combinations at 60 logical px/sec. Player sprite swapped to TimeFantasy `chara2_1/down_stand.png` with graceful fallback to the M1 procedural placeholder if the paid pack isn't present.

**Test framework adopted**: Catch2 v3.7.1 via FetchContent + CTest integration via `cmake/StarfallTests.cmake` (`starfall_add_test` helper). Per-module tests under `tests/<module>/`. M1's three deferred test items (texture cache hit, missing-path Err, null-handle DrawSprite noop) backfilled. 14 tests passing on both platforms. Run with `ctest --preset debug-<platform>`.

**Next milestone target: M3 Tilemap & Collision** (per `docs/GameDesign.md` §9). Goal: Embercoast traversable on a hand-coded map. Cliff path blocked.

## Custom Agents

- `Game Engine Programmer` — core C++ engine, rendering, SDL.
- `Lua Gameplay Programmer` — `engine_scripting` + the actual Lua game behavior.
- `Build Systems Engineer` — CMake, dependencies, packaging.
- `Editor Tools Programmer` — Dear ImGui editor (`engine_editor`).
- `RPG Systems Designer` — transferable JRPG mechanic patterns and feel.
- `Game Director` — the specific game's vision (theme, characters, world, v1 scope). Owns `docs/GameDesign.md`.

Engineering agents read `docs/DesignDoc.md`; design agents read `docs/GameDesign.md`; the Game Director coordinates between them.
