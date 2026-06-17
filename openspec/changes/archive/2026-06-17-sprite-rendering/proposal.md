## Why

M0 puts a window on screen but draws nothing. The next user-visible step per `docs/GameDesign.md` §9 (M1) is: **"Iden's sprite visible. Pre-dawn blue tint applied to the scene."** Per `docs/DesignDoc.md` §22 M1 also requires texture loading, sprite drawing, a camera, and pixel-perfect scaling — the minimum render stack a 2D RPG needs.

This change implements that stack and uses it to show a single placeholder Iden sprite on a deep-blue cleared scene. Every later milestone (tilemap rendering, dialogue UI, editor preview) builds on the primitives this change introduces.

## What Changes

- Add **PNG texture loading** via SDL3_image (fetched through `external/`), surfaced as `Engine::Assets::TextureHandle`. In-memory cache by path so a texture is loaded once.
- Add **`Engine::Render::Renderer`** wrapping `SDL_Renderer`. Owns the GPU-side texture cache, the camera, and the logical-resolution presentation. Surface: `Clear(Color)`, `DrawSprite(TextureHandle, Vec2 worldPos, Rect? sourceRect = nullopt)`, `Present()`, `Camera2D& Camera()`.
- Add **`Engine::Render::Camera2D`** — `Vec2 position` + `float zoom` (default 1.0). Sprite world positions are translated by `-cameraPosition` and scaled by `zoom` during draw.
- Add **`Engine::Math` primitives** (just what M1 uses): `Vec2`, `IVec2`, `Rect`, `Color` (8-bit RGBA). Aggregate types, no methods beyond `operator+/-/*` on Vec2.
- Grow **`Engine::Runtime::Application`**: initialize/teardown the renderer alongside the window; per-frame loop becomes `poll events → clear(clearColor) → onRender(renderer) → present`.
- Grow **`Engine::Runtime::AppConfig`** with `int logicalWidth = 320`, `int logicalHeight = 180`, `Color clearColor = {0x1A, 0x24, 0x40, 0xFF}` (pre-dawn deep blue), and `std::function<void(Render::Renderer&)> onRender`. Defaults render a fully-black-but-correctly-presented window if no callback is set — useful for tests.
- Grow **`game_my_rpg`** to ship a placeholder `assets/characters/iden_placeholder.png` (small 16×16 pixel-art figure), load it once at startup, and draw it centered each frame via the `onRender` callback.
- Add **CMake post-build asset sync**: `games/my_rpg/assets/` is copied to `<build>/<preset>/bin/assets/` after every game build. The runtime resolves asset paths relative to the executable directory.
- Pin **`SDL3_image`** in `external/CMakeLists.txt` alongside SDL3, same FetchContent + static pattern, normalized to `SDL3_image::SDL3_image`.
- Update **`docs/GameDesign.md`** §9 M1 row to ✅ when the slice runs on both platforms with the Iden sprite visible.

### Non-Goals

- No animation. Sprites are static at M1 — `AnimatedSpriteComponent` arrives later when the scene system needs it.
- No tilemap rendering. M3 owns that.
- No sprite batching or render-command queue. `DrawSprite` issues one `SDL_RenderTexture` call per invocation; one sprite per frame doesn't justify batching machinery (DesignDoc soft rule).
- No multiple cameras, no render layers, no z-sort. One camera, painter's algorithm by call order.
- No per-sprite tint/color modulation. The "blue tint" at M1 is the clear color only; per-sprite tinting becomes interesting once tilemaps + portraits land.
- No Lua bindings for any of this. M6 owns scripting.
- No asset hot-reload. Editor and file-watching come later.
- No render backend abstraction. SDL_Renderer is the only backend; per CLAUDE.md soft rule, no abstraction layer until a second backend is real.
- No scene file format yet. The game draws via a hardcoded `onRender` callback. M3-M4 replace that with a real scene.

## Capabilities

### New Capabilities

- `texture-assets`: `engine_assets`'s capability for PNG texture loading, the `TextureHandle` opaque type, and the in-memory by-path cache. Owns the asset-path-relative-to-executable convention.
- `2d-renderer`: `engine_render`'s capability for the `Renderer` lifecycle, sprite drawing, `Camera2D`, logical-resolution presentation (320×180 scaled to window), and the clear-color contract.

### Modified Capabilities

- `runtime-window`: `Application` now owns a renderer in addition to a window. `AppConfig` grows `logicalWidth/Height`, `clearColor`, and the `onRender` callback. Run loop's per-frame body changes from "wait for event" to "clear + onRender + present + poll events with a frame budget."

## Impact

- **Affected modules**: `engine_assets`, `engine_render`, `engine_math` gain real implementations (no longer empty placeholders). `engine_runtime` and `game_my_rpg` grow. `engine_core` and the other empty modules unchanged.
- **Dependency-rule compliance**: `engine_assets → engine_core`, `engine_render → engine_core + engine_math + engine_assets + SDL3::SDL3`, `engine_runtime → engine_core + engine_render + SDL3::SDL3` (runtime now needs render). All within `docs/DesignDoc.md` §6.3.
- **New external dependencies**: `SDL3_image` 3.2.x (PNG/JPEG support, official SDL companion library, static, fetched via `external/CMakeLists.txt`).
- **New serialized format**: none. PNG files are inputs; no new file formats. (No `version` field needed.)
- **New Lua bindings**: none.
- **Performance budget**: M1 hits 60 FPS with one sprite trivially. Establish that the runtime's per-frame loop now does real work (clear + draw + present) and budget ≤ 5 ms/frame at M1 idle so M2-M4 have headroom.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M1): launching `game_my_rpg` opens a window with a deep-blue background and a small placeholder Iden sprite centered. Tick M1 row in `docs/GameDesign.md` §9 in the same change.
- **Docs**: GameDesign §9 M1 row ticked. No DesignDoc edits required — this implements what §7.3/§7.4 and §22 already specify.
- **Open questions (validate at apply time, not blockers for the proposal):**
  - **Logical resolution**: 320×180 chosen for crisp 4× pixel scale to 1280×720 and classic JRPG feel. Alternative: 480×270 (3× scale, more detail) or 640×360 (2× scale, more modern). RPG Systems Designer / Game Director should weigh in before code lands.
  - **Pre-dawn blue clear color**: `#1A2440` chosen as a starting point. GameDesign §6 calls for "muted dusk — soft purples, deep blues, amber lantern." Final color is a Game Director call.
  - **Iden placeholder shape**: 16×16 pixel-art figure, generated at apply time (procedural PNG via a tiny build-tool script, or hand-drawn placeholder). Per GameDesign §6.7, Iden's portrait is intentionally absent for v1 to keep the character undefined — the placeholder sprite should be deliberately generic for the same reason.
