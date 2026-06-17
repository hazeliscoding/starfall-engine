## ADDED Requirements

### Requirement: Renderer Lifecycle

`engine_render` SHALL provide an `Engine::Render::Renderer` type whose lifetime is managed by `engine_runtime`. It SHALL initialize on top of an existing SDL window, expose drawing primitives, and shut down cleanly when destroyed.

#### Scenario: Renderer initializes against a window

- **WHEN** `engine_runtime` constructs a `Renderer` passing the just-created `SDL_Window*` and a logical resolution
- **THEN** the renderer creates an `SDL_Renderer` with a hardware-accelerated backend if available, configures logical-resolution presentation, and reports ready

#### Scenario: Renderer init failure surfaces cleanly

- **WHEN** SDL fails to create a renderer (e.g. no compatible backend)
- **THEN** the constructor MUST report the failure via `Result` or exception (consistent with engine convention), log an `[Render]` error with `SDL_GetError()`, and not leak the partially-initialized SDL renderer

#### Scenario: Renderer destruction is idempotent and ordered

- **WHEN** the `Renderer` is destroyed
- **THEN** all GPU-side textures it owns are released BEFORE its `SDL_Renderer` is destroyed, and destruction is safe to call exactly once with no double-free

### Requirement: Frame Begin/End Contract

The renderer SHALL expose `Clear(Color)` to fill the back buffer with a color and `Present()` to flip the back buffer to the window. Calls between `Clear` and `Present` are drawing calls; the order is per-frame: `Clear → DrawSprite × N → Present`.

#### Scenario: Clear sets the background

- **WHEN** game code calls `renderer.Clear({0x1A, 0x24, 0x40, 0xFF})` followed by `renderer.Present()` with no draws in between
- **THEN** the window displays a solid deep-blue fill matching the supplied color (within 1 unit per channel for gamma/precision)

#### Scenario: Draw between Clear and Present appears

- **WHEN** game code clears, calls `DrawSprite(iden, {160, 90})`, then calls `Present()` with a 320×180 logical resolution
- **THEN** the next displayed frame contains the iden sprite at the center of the window

### Requirement: Sprite Drawing

`Renderer` SHALL expose `DrawSprite(TextureHandle, Vec2 worldPos, optional<Rect> sourceRect)`. World positions are translated to screen space by the active camera. An omitted source rect draws the full texture; a present source rect draws a sub-region.

#### Scenario: Full-texture draw at world origin

- **WHEN** `DrawSprite(handle, {0, 0}, nullopt)` is called with a default camera (position={0,0}, zoom=1.0) and a logical resolution of 320×180
- **THEN** the texture's top-left appears at the logical-pixel (0, 0) of the window (after logical-resolution scaling to the actual window pixels)

#### Scenario: Camera offset translates draws

- **WHEN** the camera position is set to `{50, 30}` and `DrawSprite(handle, {100, 60})` is called
- **THEN** the sprite is drawn at screen position `{50, 30}` (world − camera)

#### Scenario: Source rect samples a sub-region

- **WHEN** `DrawSprite(handle, {0, 0}, Rect{16, 0, 16, 16})` is called on a 32×16 sprite sheet
- **THEN** only the right half of the sheet is drawn

#### Scenario: Invalid handle does not crash

- **WHEN** `DrawSprite` is called with a handle returned from a failed `LoadTexture`
- **THEN** the call MUST be a noop (or a visible placeholder draw such as a magenta square) and MUST NOT crash

### Requirement: Logical Resolution Presentation

The renderer SHALL present at a fixed logical resolution that is scaled to fit the actual window, preserving aspect ratio with integer scaling when possible (for pixel-perfect output) and letterboxing the difference.

#### Scenario: Window resized scales letterboxed

- **WHEN** the window is resized from 1280×720 to 1600×900 with a 320×180 logical resolution
- **THEN** the rendered logical surface is scaled up uniformly (5× in this case) and centered; letterbox bars appear on the sides if the aspect ratio differs

#### Scenario: Pixel-perfect at integer scales

- **WHEN** the window dimensions are an exact integer multiple of the logical resolution
- **THEN** sprites render with no filtering / no fractional sampling (pixel-art-friendly)

### Requirement: Camera2D

The renderer SHALL provide `Engine::Render::Camera2D` with a 2D position and a uniform zoom factor (default 1.0). The currently active camera is accessible via `renderer.Camera()` for game code to mutate.

#### Scenario: Default camera draws world origin at logical origin

- **WHEN** the camera has not been touched after renderer construction
- **THEN** `position == {0, 0}` and `zoom == 1.0`, and `DrawSprite(h, {0, 0})` puts the sprite at logical (0, 0)

#### Scenario: Camera mutation is read by subsequent draws

- **WHEN** game code sets `renderer.Camera().position = {10, 20}` and then calls `DrawSprite(h, {10, 20})`
- **THEN** the sprite appears at logical (0, 0) (world − camera)
