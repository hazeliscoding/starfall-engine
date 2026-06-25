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

**M3 Tilemap & Collision: complete on Windows and Linux (2026-06-19).** `engine_scene` graduates from placeholder to the multi-layer tilemap module. `Engine::Scene::TileLayer` (data), `Engine::Scene::TileSet` (texture + tile dims + columns + MarkSolid/IsSolid), `Engine::Scene::Tilemap` (stable-sorted layers + `ForEachVisibleTile` visitor + `CollidesAABB`) all land. `Engine::Scene::SweepMove` does per-axis-independent AABB sweeps for the "slide along walls" feel. Engine_render stays tilemap-agnostic per §6.3 — the game's `onRender` does a three-pass draw (ground → Y-sorted mid-layer entities → overhead) using `ForEachVisibleTile`'s sortOrder-window overload. Game side: 20×12 Embercoast map authored as ASCII-art in `main.cpp` (cliff path blocked north, building with awning south-of-which Iden walks under, sea south). Iden's collision is a 12×8 feet AABB (head clears doorways — JRPG convention). game_my_rpg gains a direct `→ engine_scene` link (allowed per §6.3 game_*→engine_*).

**Roadmap audit ride-along**: scope expanded for M3 (multi-layer + Y-sort folded in; tile-trigger zones folded into M7 instead). New **M3.25 Tileset Animation** stub inserted between M3 and M3.5 in DesignDoc §22 + GameDesign §9 + README.

**Tileset deviation**: M3 ships with the procedural placeholder tileset (5 colored solid tiles) for deterministic visuals. The licensed TimeFantasy `outside.png` (52×24 tiles) is available locally — swap is a Game-Director art pass that picks specific tile IDs.

**Test count: 58** (22 new scene tests on top of M2.75's 36). Run with `ctest --preset debug-<platform>`.

**Previously complete**:
- **M2.75 Audio (2026-06-18)** — `AudioSystem` + `Music`/`Sound` handles via SDL3_mixer 3.2.4; licensed HydroGene "Peaceful Village" theme + procedural-WAV fallback; PlayMusic-on-first-movement + footstep-every-16px.
- **M2.5 Sprite Animation (2026-06-18)** — `AnimatedSprite` named-clip state machine; 4-directional walk cycle.
- **M2 Input & Movement (2026-06-18)** — `ActionMap` + `InputState` + `onUpdate` callback; 4-dir movement; Catch2/CTest adopted.
- **M1 Sprite Rendering (2026-06-17)** — `texture-assets` + `2d-renderer` capabilities; Iden visible on pre-dawn blue.
- **M0 Bootstrap (2026-06-17)** — CMake graph + module dependency enforcement; SDL3 window.

**Next milestone target: M3.25 Tileset Animation** (per `docs/GameDesign.md` §9). Goal: ocean tiles lap, lanterns flicker at Embercoast — per-tile animation clips re-using the M2.5 `AnimationClip` shape.

**Roadmap shape**: M0–M10 ships the v1 slice (Embercoast playable). **Phase 2** (M11–M16: UI Framework, Pause + Settings, Map Transitions, Asset Packer, Accessibility, Particles) covers ship-polish work every shipping game needs. **Phase 3** (M17–M22: Inventory, Combat shell + AI, Party, Quests, Cutscenes) covers Acts 2–3 systems that any future 2D RPG on the engine will reuse. Full breakdown in `docs/DesignDoc.md` §22.A.

## Custom Agents

- `Game Engine Programmer` — core C++ engine, rendering, SDL.
- `Lua Gameplay Programmer` — `engine_scripting` + the actual Lua game behavior.
- `Build Systems Engineer` — CMake, dependencies, packaging.
- `Editor Tools Programmer` — Dear ImGui editor (`engine_editor`).
- `RPG Systems Designer` — transferable JRPG mechanic patterns and feel.
- `Game Director` — the specific game's vision (theme, characters, world, v1 scope). Owns `docs/GameDesign.md`.

Engineering agents read `docs/DesignDoc.md`; design agents read `docs/GameDesign.md`; the Game Director coordinates between them.
