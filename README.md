# Starfall Engine 🌠

A modular **C++20 2D RPG game engine** inspired by *EarthBound*, *RPG Maker*, and classic SNES/GBA RPGs — built in lockstep with a real game (`games/my_rpg/` working title **Starfall**).

> *The engine provides capability. The game provides behavior. The editor provides tooling.*

Two design docs, two owners:

- 📘 [`docs/DesignDoc.md`](docs/DesignDoc.md) — engine architecture.
- 📗 [`docs/GameDesign.md`](docs/GameDesign.md) — the specific game (theme, characters, world, v1 scope).

The two are bound by the **milestone contract** in `DesignDoc.md` §30: every engine milestone produces a visible game improvement. If the game falls behind, engine work pauses.

---

## Status

### v1 Slice (Embercoast playable)

| Milestone | Game Deliverable | State |
|-----------|------------------|-------|
| M0  Bootstrap              | Window opens. Title bar reads "Starfall." | ✅ Done 2026-06-17 |
| M1  Sprite Rendering       | Iden visible on pre-dawn blue.              | ✅ Done 2026-06-17 |
| M2  Input & Movement       | Iden walks. Feels like Embercoast.          | ✅ Done 2026-06-18 |
| M2.5 Sprite Animation      | Directional sprite + walk cycle.            | ✅ Done 2026-06-18 |
| M2.75 Audio                | Music + SFX. Opening silence, then theme.   | ✅ Done 2026-06-18 |
| M3  Tilemap & Collision    | Embercoast traversable. Cliff path blocked. | 🎯 Next |
| M3.5 Camera Follow         | Camera tracks Iden, clamps to map bounds.   | ⏳ |
| M4  Scene File Format      | `embercoast.scene.json` loadable.           | ⏳ |
| M5  Editor v1              | Room rearrangeable in editor without code.  | ⏳ |
| M6  Lua Scripting          | Mina exists as a `ScriptComponent`.         | ⏳ |
| M7  Dialogue               | All three NPCs playable. End-to-end slice.  | ⏳ |
| M8  Save System            | Salt Light cairn save point works.          | ⏳ |
| M9  Packaging              | A friend downloads and plays.               | ⏳ |
| M10 Steam Prep             | Controller support. Store-page draft ready. | ⏳ |

### Beyond v1 (ship-quality + Acts 2-3)

Planned milestones to grow from "v1 slice ships" to "high-quality indie title shippable for any 2D RPG built on the engine." Grouped roughly by purpose; not strictly sequential.

- **Phase 2 — Ship Polish:** UI Framework (M11), Pause + Settings Menu (M12), Map Transitions (M13), Asset Packer + Build Polish (M14), Accessibility (M15), Particles + Juice (M16).
- **Phase 3 — Game Expansion (Acts 2-3):** Inventory + Items (M17), Combat Shell (M18), Combat AI + Balance Hooks (M19), Party / Companions (M20), Quest / Event Flags (M21), Cutscene / Scripted Event System (M22).

Full milestone breakdown: [`docs/DesignDoc.md` §22](docs/DesignDoc.md) (engineering view) and [`docs/GameDesign.md` §9](docs/GameDesign.md) (game-deliverable view).

---

## Tech Stack 🧱

- **Language:** C++20 (engine, editor, tools) + Lua (gameplay scripting, M6+)
- **Build:** CMake ≥ 3.24 + `CMakePresets.json` (Ninja preferred)
- **Window/Input:** [SDL3](https://github.com/libsdl-org/SDL) 3.4.10 (static, fetched via `external/`)
- **Audio:** [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer) 3.2.4 — Ogg + WAV (static, fetched via `external/`)
- **Image loading:** [SDL3_image](https://github.com/libsdl-org/SDL_image) 3.4.4 — PNG (static, fetched via `external/`)
- **Editor UI:** Dear ImGui (lands at M5)
- **Serialization:** JSON during development; binary/packed later (M14)
- **Testing:** [Catch2](https://github.com/catchorg/Catch2) v3.7.1 + CTest (adopted at M2)
- **Platforms:** Windows + Linux 🐧 🪟 (Steam-friendly distribution)

---

## Quick Start 🛠️

### Prerequisites

- **Windows:** Visual Studio 2022 or 2026 with the **"Desktop development with C++"** workload (provides CMake + Ninja + MSVC + Windows SDK).
- **Linux:** cmake ≥ 3.24, ninja, g++-10 (or newer), plus SDL3 build deps:
  ```bash
  sudo apt install cmake ninja-build g++-10 \
      libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
      libxinerama-dev libxss-dev libxxf86vm-dev libwayland-dev \
      libxkbcommon-dev libegl1-mesa-dev libgl1-mesa-dev \
      libasound2-dev libpulse-dev libdrm-dev libgbm-dev libudev-dev \
      libdbus-1-dev libibus-1.0-dev pkg-config
  ```

### ⚠️ Windows: load the VS developer environment first

The MSVC toolchain needs `INCLUDE` / `LIB` / `PATH` set up by `VsDevCmd.bat`. Without it, the linker can't find `kernel32.lib` and CMake fails with `LNK1104`. Pick one:

```powershell
# Option A — open the Start menu, launch "Developer PowerShell for VS 2026" (or 2022),
#            then `cd` into this repo. Done.

# Option B — from any PowerShell, use the helper:
.\scripts\dev-shell.ps1
# (drops you into a dev-enabled prompt at the repo root)

# Option C — one-shot build via the helper:
.\scripts\dev-shell.ps1 -Run "cmake --preset debug-windows; cmake --build --preset debug-windows"
```

If you forget, `cmake --preset debug-windows` will refuse with a clear message instead of an opaque linker error — see `CMakeLists.txt` line ~10.

### Build & Run

```powershell
# Windows (inside a VS dev shell, see above)
cmake --preset debug-windows
cmake --build --preset debug-windows
.\build\debug-windows\bin\game_my_rpg.exe
```

```bash
# Linux
cmake --preset debug-linux
cmake --build --preset debug-linux
./build/debug-linux/bin/game_my_rpg
```

First configure takes a few minutes — SDL3, SDL3_image, and Catch2 are fetched and built from source. Subsequent builds are incremental.

A window titled **Starfall** should appear with the player sprite visible at logical-center. Press **WASD** or arrow keys to walk; close the window (or send `SIGTERM`) and the process exits cleanly with code 0.

### Run the tests

```powershell
# Windows (inside a VS dev shell)
ctest --preset debug-windows
```

```bash
# Linux
ctest --preset debug-linux
```

### Available presets

- `debug-windows`, `release-windows`
- `debug-linux`, `release-linux`

### Editor / clangd support ✨

After any configure, the root `CMakeLists.txt` mirrors `compile_commands.json` from the active build dir to the repo root (symlink, with copy fallback on Windows without developer mode). Zed, VS Code with the clangd extension, Neovim with `nvim-lspconfig`, and any other clangd-based editor pick it up automatically — no per-editor config needed. Reload the LSP if you change presets.

---

## Repository Layout 📁

```
starfall-engine/
├── cmake/                  # CMake helpers (StarfallModule.cmake, warnings)
├── external/               # Third-party fetch (SDL3 today; Lua/ImGui later)
├── include/engine/         # Public headers, one subdir per module
│   ├── core/  math/  assets/  render/  input/
│   ├── audio/ scene/ scripting/ runtime/
├── src/                    # Engine module implementations (1:1 with include/engine/)
│   ├── core/    runtime/   editor/        # only modules with real code at M0
│   └── …                                  # others are reserved targets
├── games/
│   └── my_rpg/             # The game being built alongside the engine
│       ├── src/main.cpp    # Thin entry point — Configure + Run
│       ├── assets/  scripts/  scenes/
├── docs/
│   ├── DesignDoc.md        # Engine architecture
│   └── GameDesign.md       # Game vision (theme, characters, world)
├── openspec/               # Change proposals + capability specs
│   ├── specs/              # Synced capability specs (source of truth)
│   └── changes/archive/    # Completed change proposals
├── tools/  tests/          # Reserved
├── CMakeLists.txt
└── CMakePresets.json
```

---

## Architecture 📐

The engine is a set of static libraries linked by thin executables:

```
engine_core   ← engine_math, engine_assets,
                engine_render, engine_input,
                engine_audio, engine_scene, engine_scripting
engine_runtime ← engine_core + SDL3
game_my_rpg    ← engine_runtime
engine_editor  ← engine modules (NOT runtime, NOT game)
```

**Three-way separation:**

- 🛠️ **Engine** provides capability (C++)
- 🎮 **Game** provides behavior (Lua + thin C++ entry point)
- ✏️ **Editor** provides tooling (separate executable, not required at runtime)

### Hard rules (enforced — see `cmake/StarfallModule.cmake`) 🚨

1. Engine code **must not** depend on game code or editor code.
2. `engine_core` **must not** depend on render / input / audio / scene / scripting / runtime.
3. Editor **must not** be required for the shipped runtime.
4. Lua **must not** mutate engine internals directly — only via curated bindings.
5. Scene data **must** be serializable (versioned JSON).
6. Game logic belongs in Lua unless there's a strong perf/platform reason.
7. Every serialized asset **must** include a `version` field.

Violations of rules 1–3 fail CMake configure with a clear message and a `§6.3` citation. Try it: add `engine_render` to `engine_core`'s `DEPENDS` and watch the build refuse.

---

## Spec-Driven Development 📋

Non-trivial changes follow [OpenSpec](https://github.com/Fission-AI/OpenSpec) — proposal, design, capability specs, and tasks live under `openspec/changes/` before any code is written. **Every change ships on its own branch, lands via PR. `main` is updated only through merge.**

- **Active changes:** `openspec/changes/<change-name>/`
- **Capability specs:** `openspec/specs/<capability>/spec.md` (synced from changes when archived)
- **Archive:** `openspec/changes/archive/YYYY-MM-DD-<name>/`

The full loop (Claude Code skills in **bold**):

```text
1. /opsx:propose <name>   →  branches from latest origin/main as <name>,
                              writes proposal + design + specs + tasks
2. /opsx:apply <name>     →  implements tasks, marking each [x] as it lands
3. **commit**             →  Conventional Commits format, no AI co-author
4. /opsx:archive <name>   →  syncs delta specs into openspec/specs/, moves
                              the change to openspec/changes/archive/
5. **push**               →  pushes the branch (sets upstream on first push)
6. **pr**                 →  opens a PR via gh CLI, body derived from the
                              proposal + commits on the branch
```

Both `openspec` (CLI) and the `/opsx:*` slash commands work the same way — pick whichever feels natural.

---

## Documentation Map 📚

| File | What's in it |
|------|--------------|
| [`README.md`](README.md) (this) | Project overview, build, layout |
| [`CLAUDE.md`](CLAUDE.md) | Project rules and current status (for AI tools and humans alike) |
| [`docs/DesignDoc.md`](docs/DesignDoc.md) | Engine architecture, module design, milestones |
| [`docs/GameDesign.md`](docs/GameDesign.md) | The game's theme, characters, world, v1 scope |
| `openspec/specs/build-system/spec.md` | Build-system capability requirements |
| `openspec/specs/runtime-window/spec.md` | Runtime + window capability requirements |

---

## Custom Agents 🤖

The repo defines specialized AI agents under `.claude/agents/`:

| Agent | Owns |
|-------|------|
| Game Engine Programmer    | Core C++ engine, rendering, SDL |
| Lua Gameplay Programmer   | `engine_scripting` + the actual Lua game behavior |
| Build Systems Engineer    | CMake, dependencies, packaging |
| Editor Tools Programmer   | Dear ImGui editor (`engine_editor`) |
| RPG Systems Designer      | JRPG mechanic patterns and feel |
| Game Director             | The specific game's vision (theme, characters, world) |

Engineering agents read `docs/DesignDoc.md`; design agents read `docs/GameDesign.md`; the Game Director coordinates between them.
