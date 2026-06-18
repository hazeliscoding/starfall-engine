## MODIFIED Requirements

### Requirement: Application Configuration Surface

`AppConfig` SHALL be a plain aggregate carrying the window title, window dimensions, the renderer's logical resolution, the per-frame clear color, an optional `onStart` callback invoked once after subsystem initialization (for one-time setup like loading textures and music), an optional `onUpdate` callback invoked once per frame for game-state mutation, an optional `onRender` callback the game uses to draw each frame, and (reserved for later milestones) an initial-scene path. Every field MUST have a documented default so omitting it does not crash; in particular, omitting `onStart`, `onUpdate`, or `onRender` is valid and produces a window that clears each frame but draws nothing.

The `onStart` callback signature is `void(Engine::Render::Renderer&, Engine::Assets::AssetLoader&, Engine::Audio::AudioSystem&)`. All three references remain valid for the lifetime of `Run()` and are the same references passed to subsequent `onUpdate` / `onRender` calls (via their own captured copies, since `onUpdate` and `onRender` don't take audio/asset arguments directly).

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
- **THEN** `onStart(renderer, assetLoader, audioSystem)` MUST be called exactly once after SDL + renderer + asset loader + input state + audio system are all initialized, and BEFORE the first `onUpdate` and `onRender` invocations

#### Scenario: onStart failure aborts cleanly

- **WHEN** `onStart` throws
- **THEN** the exception MUST be logged as `[Runtime]` error including its text; SDL + renderer + asset loader + input state + audio system MUST be torn down in reverse-construction order; `Run()` MUST return non-zero without entering the frame loop

#### Scenario: onUpdate callback is optional

- **WHEN** `Configure` is called without specifying `onUpdate`
- **THEN** `Run()` MUST still produce a valid frame loop; game state simply doesn't advance per-frame, the window remains responsive, and onRender (if set) runs as normal

### Requirement: Runtime Owns Only Allowed Engine Modules

`engine_runtime` SHALL link `engine_core`, `engine_render`, `engine_assets`, `engine_input`, and `engine_audio` (plus the SDL targets they need). Other engine modules (`scene`, `scripting`) will be added by the milestones that introduce real implementations.

#### Scenario: Runtime dependencies audited

- **WHEN** a reviewer inspects `engine_runtime`'s CMake `DEPENDS` list
- **THEN** it lists `engine_core`, `engine_render`, `engine_assets`, `engine_input`, `engine_audio`, and the SDL targets, and nothing else; no link to `engine_scene`, `engine_scripting`, `engine_editor`, or `game_my_rpg`
