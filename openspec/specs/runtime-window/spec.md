# runtime-window Specification

## Purpose

Defines `engine_runtime`'s application-lifecycle surface and cross-platform windowing behavior. This capability gives the game executable a single, thin entry point (`Engine::Runtime::Application`) that owns SDL initialization, opens a top-level window on Windows and Linux, pumps OS events, and shuts down cleanly — without exposing SDL or platform conditionals to game code.

## Requirements

### Requirement: Application Lifecycle

`engine_runtime` SHALL expose an `Engine::Runtime::Application` type that owns the engine lifecycle: configure → initialize → run loop → shutdown. The game executable SHALL drive the engine ONLY through this surface.

#### Scenario: Configure before run

- **WHEN** game code constructs an `Application`, calls `Configure(AppConfig{...})`, then calls `Run()`
- **THEN** the application initializes its subsystems, enters the main loop, and returns an integer exit code

#### Scenario: Configure required before run

- **WHEN** game code calls `Run()` without first calling `Configure`
- **THEN** `Run()` MUST return a non-zero exit code and log a `Core` category error explaining the missing configuration, without attempting to open a window

#### Scenario: Game entry point stays thin

- **WHEN** a reviewer reads `games/my_rpg/src/main.cpp`
- **THEN** `main` MUST contain only construction of `Application`, a `Configure` call with literal config values, and a `Run` call whose result is returned — no engine loop logic, no SDL calls, no manual subsystem init

### Requirement: Application Configuration Surface

`AppConfig` SHALL be a plain aggregate that carries at minimum a window title, window width, window height, and (reserved for later milestones) an initial-scene path. Every field MUST have a documented default so omitting it does not crash.

#### Scenario: Title appears on the window

- **WHEN** `Configure` is called with `title = "Starfall"`
- **THEN** the OS window opened by `Run` MUST display "Starfall" in its title bar on both Windows and Linux

#### Scenario: Default window size

- **WHEN** `Configure` is called without specifying width/height
- **THEN** the window MUST open at a sane default no smaller than 640×480 and no larger than 1920×1080

### Requirement: Cross-Platform Window Via SDL

`engine_runtime` SHALL open the application window through SDL (the version chosen in design.md), and `game_my_rpg` SHALL run the same source on Windows and Linux without per-platform conditionals in game code.

#### Scenario: Window opens on Windows

- **WHEN** `game_my_rpg.exe` is launched on Windows 10 or later
- **THEN** a top-level OS window appears titled "Starfall" within 2 seconds and remains responsive (does not freeze the message pump)

#### Scenario: Window opens on Linux

- **WHEN** `game_my_rpg` is launched on a Linux desktop session (X11 or Wayland — whichever SDL selects)
- **THEN** a top-level window appears titled "Starfall" within 2 seconds and remains responsive

#### Scenario: Same game source on both platforms

- **WHEN** a reviewer reads `games/my_rpg/src/main.cpp`
- **THEN** there MUST be no `#ifdef _WIN32` / `#ifdef __linux__` branches; platform handling lives inside `engine_runtime`

### Requirement: Run Loop Pumps Events And Honors Close

The application's main loop SHALL pump OS events every frame and SHALL terminate cleanly when the OS requests window close, returning exit code 0.

#### Scenario: Window close button exits cleanly

- **WHEN** the user clicks the window's close button (or sends `SDL_QUIT`)
- **THEN** the run loop exits, SDL is shut down, `Run()` returns 0, and the process exits within 1 second of the close event

#### Scenario: Loop does not busy-spin

- **WHEN** the application is running with an idle window
- **THEN** the loop MUST yield to the OS each frame (event-wait or frame-pacing) such that a single idle window does not pin a CPU core at 100%

### Requirement: Clean Shutdown On Errors

If subsystem initialization fails (e.g. SDL cannot create a window), `Run()` SHALL tear down any partially-initialized subsystems, log a `Runtime` category error describing the failure, and return a non-zero exit code.

#### Scenario: SDL init failure

- **WHEN** SDL initialization fails inside `Run()`
- **THEN** any SDL subsystems that were initialized MUST be shut down before return, an error MUST be logged with the SDL error message, and `Run()` MUST return non-zero

### Requirement: Runtime Owns Only Allowed Engine Modules

`engine_runtime` SHALL link only `engine_core` in this change. Other engine modules (`render`, `input`, `audio`, `scene`, `scripting`) will be added by the milestones that introduce real implementations.

#### Scenario: Runtime dependencies audited

- **WHEN** a reviewer inspects `engine_runtime`'s CMake `DEPENDS` list
- **THEN** it lists `engine_core` (and the SDL target it needs for windowing) and nothing else; no link to `engine_render`, `engine_input`, `engine_audio`, `engine_scene`, `engine_scripting`, `engine_editor`, or `game_my_rpg`
