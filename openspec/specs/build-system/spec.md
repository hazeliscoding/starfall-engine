# build-system Specification

## Purpose

Defines the CMake-based build system for Starfall Engine: project structure, module dependency enforcement, cross-platform presets, and third-party dependency acquisition. This capability ensures the engine, game, editor, and tools build reproducibly on Windows and Linux while structurally enforcing the module-dependency rules from DesignDoc §6.3.

## Requirements

### Requirement: Root CMake Project

The project SHALL declare a single CMake project at the repository root that requires C++20, defines all engine module targets named in DesignDoc §6.1, and produces the `game_my_rpg` and `engine_editor` executables.

#### Scenario: Configure on a clean checkout

- **WHEN** a developer runs `cmake --preset debug-linux` (or `debug-windows`) on a fresh clone
- **THEN** configuration succeeds with no warnings classified as errors and reports targets `engine_core`, `engine_math`, `engine_assets`, `engine_render`, `engine_input`, `engine_audio`, `engine_scene`, `engine_scripting`, `engine_runtime`, `engine_editor`, and `game_my_rpg`

#### Scenario: C++20 enforced project-wide

- **WHEN** any engine or game target is compiled
- **THEN** the compile command MUST request C++20 (or later) with extensions disabled, and any target that downgrades the standard MUST fail configure

### Requirement: Module Dependency Rules Enforced At Configure Time

The build system SHALL provide a helper (`starfall_add_module` or equivalent) that every engine module uses to declare its dependencies, and that helper MUST fail CMake configuration when a declared dependency violates DesignDoc §6.3.

#### Scenario: Forbidden engine_core dependency

- **WHEN** a developer adds `engine_render` to the `DEPENDS` list of `engine_core`
- **THEN** CMake configure MUST fail with a message naming the violated rule ("engine_core MUST NOT depend on render/input/audio/scene/scripting")

#### Scenario: Forbidden game dependency from engine

- **WHEN** any `engine_*` target declares a dependency on `game_my_rpg` or `engine_editor`
- **THEN** CMake configure MUST fail with a message naming the violated rule

#### Scenario: Allowed dependency

- **WHEN** `engine_runtime` declares a dependency on `engine_core`
- **THEN** CMake configure MUST succeed

### Requirement: Cross-Platform Presets

The project SHALL ship `CMakePresets.json` with presets `debug-windows`, `release-windows`, `debug-linux`, and `release-linux` that select Ninja when available and configure into per-preset build directories.

#### Scenario: Build via preset on Windows

- **WHEN** a developer runs `cmake --preset debug-windows` followed by `cmake --build --preset debug-windows` on a Windows host with MSVC and Ninja installed
- **THEN** the build succeeds and produces `game_my_rpg.exe` and `engine_editor.exe` in the preset's build directory

#### Scenario: Build via preset on Linux

- **WHEN** a developer runs `cmake --preset debug-linux` followed by `cmake --build --preset debug-linux` on a Linux host with a C++20 toolchain and Ninja installed
- **THEN** the build succeeds and produces `game_my_rpg` and `engine_editor` executables in the preset's build directory

#### Scenario: Build directories are isolated per preset

- **WHEN** a developer configures `debug-linux` and then `release-linux`
- **THEN** each preset uses a distinct build directory under `build/` and switching presets does not require a clean

### Requirement: Module Targets Map 1:1 To Source Directories

Each `engine_*` static-library target SHALL own exactly one subdirectory under `src/` of the same suffix (e.g. `engine_core` ↔ `src/core/`) and exactly one header subtree under `include/engine/` (e.g. `include/engine/core/`).

#### Scenario: Module without source directory fails

- **WHEN** a CMake target declares itself as an engine module but no matching `src/<name>/` directory exists
- **THEN** configure MUST fail rather than silently produce an empty library

### Requirement: Third-Party Dependencies Vendored Through external/

All third-party C/C++ dependencies SHALL be acquired through a single mechanism rooted in `external/` (either git submodule, `FetchContent`, or vendored source), and no engine module SHALL reach outside that mechanism to consume a system-installed dependency.

#### Scenario: SDL is acquired via external/

- **WHEN** a developer configures the project on a host with no system-installed SDL
- **THEN** configure MUST succeed and the SDL target MUST be made available through `external/`

#### Scenario: System SDL is not silently substituted

- **WHEN** a developer has a system-installed SDL of a different version than the pinned one
- **THEN** the build MUST link the pinned `external/` SDL, not the system one

### Requirement: Editor And Tools Are Optional At Runtime

The `engine_editor` executable and any `tools/` executables SHALL NOT be linked into `game_my_rpg`, and the `game_my_rpg` executable SHALL run on a machine where the editor binary is absent.

#### Scenario: Game runs without editor binary present

- **WHEN** only `game_my_rpg` (plus its required shared libraries) is copied to a clean machine
- **THEN** the game launches and opens its window without referencing the editor binary
