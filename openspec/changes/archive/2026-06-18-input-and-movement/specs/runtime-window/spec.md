## MODIFIED Requirements

### Requirement: Application Configuration Surface

`AppConfig` SHALL be a plain aggregate carrying the window title, window dimensions, the renderer's logical resolution, the per-frame clear color, an optional `onStart` callback invoked once after subsystem initialization (for one-time setup like loading textures), an optional `onUpdate` callback invoked once per frame for game-state mutation, an optional `onRender` callback the game uses to draw each frame, and (reserved for later milestones) an initial-scene path. Every field MUST have a documented default so omitting it does not crash; in particular, omitting `onStart`, `onUpdate`, or `onRender` is valid and produces a window that clears each frame but draws nothing.

#### Scenario: Title appears on the window

- **WHEN** `Configure` is called with `title = "Starfall"`
- **THEN** the OS window opened by `Run` MUST display "Starfall" in its title bar on both Windows and Linux

#### Scenario: Default window size

- **WHEN** `Configure` is called without specifying width/height
- **THEN** the window MUST open at a sane default no smaller than 640×480 and no larger than 1920×1080

#### Scenario: Default logical resolution and clear color

- **WHEN** `Configure` is called without specifying `logicalWidth`/`logicalHeight`/`clearColor`
- **THEN** the renderer presents at a documented pixel-art-friendly default (currently 320×180) and clears each frame to the documented pre-dawn deep-blue default (currently `{0x1A, 0x24, 0x40, 0xFF}`)

#### Scenario: onRender callback is optional

- **WHEN** `Configure` is called without specifying `onRender`
- **THEN** `Run()` MUST still produce a valid frame loop that clears + presents each frame; the window shows the clear color and nothing else

#### Scenario: onStart runs once after init, before first frame

- **WHEN** `Configure` is called with an `onStart` callback and `Run()` is invoked
- **THEN** `onStart(renderer, assetLoader)` MUST be called exactly once after SDL + renderer + asset loader + input state are initialized, and BEFORE the first `onUpdate` and `onRender` invocations; the same `Renderer&` and `AssetLoader&` references are used for `onStart` and every subsequent `onRender`

#### Scenario: onStart failure aborts cleanly

- **WHEN** `onStart` throws
- **THEN** the exception MUST be logged as `[Runtime]` error including its text; SDL + renderer + asset loader + input state MUST be torn down in reverse-construction order; `Run()` MUST return non-zero without entering the frame loop

#### Scenario: onUpdate callback is optional

- **WHEN** `Configure` is called without specifying `onUpdate`
- **THEN** `Run()` MUST still produce a valid frame loop; game state simply doesn't advance per-frame, the window remains responsive, and onRender (if set) runs as normal

### Requirement: Run Loop Pumps Events And Honors Close

The application's main loop SHALL, per frame: call `InputState::BeginFrame()` to reset edge state, pump OS events through `InputState::OnEvent` (and check for window-close), invoke `AppConfig::onUpdate(dt, input)` if set, clear the back buffer to `AppConfig::clearColor`, invoke `AppConfig::onRender(renderer)` if set, present, and yield to the OS to maintain a reasonable frame budget. It SHALL terminate cleanly when the OS requests window close, returning exit code 0. The `dt` value passed to `onUpdate` MUST be the wall-clock seconds elapsed since the previous frame's start.

#### Scenario: Window close button exits cleanly

- **WHEN** the user clicks the window's close button (or sends `SDL_QUIT`)
- **THEN** the run loop exits, SDL is shut down, `Run()` returns 0, and the process exits within 1 second of the close event

#### Scenario: Loop does not busy-spin

- **WHEN** the application is running with an idle window (`onUpdate`/`onRender` do no work)
- **THEN** the loop MUST yield to the OS each frame such that a single idle window does not pin a CPU core at 100%

#### Scenario: onRender is called once per frame

- **WHEN** the application is running with an `onRender` callback set and an active window
- **THEN** `onRender(renderer)` is invoked exactly once per displayed frame, after `Clear` and before `Present`, with a valid `Renderer&` reference

#### Scenario: onUpdate fires before onRender each frame

- **WHEN** both `onUpdate` and `onRender` are set
- **THEN** for each rendered frame, `onUpdate(dt, input)` MUST run to completion BEFORE `onRender(renderer)` is invoked; `dt` MUST be a non-negative float (zero only on the first frame is acceptable)

#### Scenario: onRender exception does not crash the loop

- **WHEN** `onRender` throws an exception
- **THEN** the loop catches it, logs an `[Runtime]` error including the exception text, presents the (un-overlaid) cleared frame, and continues running so the user can see the error in the log and close the window normally

#### Scenario: onUpdate exception does not crash the loop

- **WHEN** `onUpdate` throws an exception
- **THEN** the loop catches it, logs an `[Runtime]` error including the exception text, skips the rest of this frame's draw (since game state may be inconsistent), presents the cleared frame, and continues running on the next frame

### Requirement: Runtime Owns Only Allowed Engine Modules

`engine_runtime` SHALL link `engine_core`, `engine_render`, `engine_assets`, and `engine_input` (plus the SDL targets they need). Other engine modules (`audio`, `scene`, `scripting`) will be added by the milestones that introduce real implementations.

#### Scenario: Runtime dependencies audited

- **WHEN** a reviewer inspects `engine_runtime`'s CMake `DEPENDS` list
- **THEN** it lists `engine_core`, `engine_render`, `engine_assets`, `engine_input`, and the SDL targets, and nothing else; no link to `engine_audio`, `engine_scene`, `engine_scripting`, `engine_editor`, or `game_my_rpg`
