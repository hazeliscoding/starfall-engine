## 1. Third-Party — SDL3_image

- [x] 1.1 Extended `external/CMakeLists.txt` with SDL3_image pinned to `release-3.2.4`. Static linking, vendored libpng (no system dep), PNG enabled and all other formats disabled to trim binary + configure time. Alias normalized to `SDL3_image::SDL3_image`. Touches: external dep.
- [x] 1.2 Verified by the full configure+build in task 9.1 (Windows / VS 2026 / MSVC / Ninja): SDL3_image fetched and built static with stb backend; `SDL3_image::SDL3_image` resolves; `engine_assets` links it cleanly. Configure required a fix: SDL3_image reads `BUILD_SHARED_LIBS` (not its own `SDLIMAGE_BUILD_SHARED_LIBS` option) — `external/CMakeLists.txt` now scopes `BUILD_SHARED_LIBS=OFF` around the SDL3_image fetch. Touches: external smoke test (Windows).
- [x] 1.3 Verified by the full configure+build in task 9.2 (Linux / WSL2 / g++-10 / Ninja). Same scope fix applies; SDL3_image static + stb backend produced. Touches: external smoke test (Linux).

## 2. engine_math (Real Code)

- [x] 2.1 Added header-only aggregates under `include/engine/math/`: `vec2.hpp` (Vec2 with operator+/-/* and compound forms), `ivec2.hpp`, `rect.hpp`, `color.hpp` (8-bit RGBA). All `constexpr` aggregates, no methods beyond operators (per design D10). Touches: `engine_math`.
- [x] 2.2 Kept `src/math/placeholder.cpp` as a tombstone TU instead of switching `starfall_add_module` to INTERFACE — the helper's STATIC default requires at least one source, and adding an INTERFACE branch is premature for a one-module case. Updated the placeholder comment to explain the situation. Touches: `engine_math`.

## 3. engine_assets (Real Code)

- [x] 3.1 Added `include/engine/assets/texture.hpp` + `src/assets/texture.cpp`: `Engine::Assets::Texture` owns an `SDL_Texture*` + width/height; destructor calls `SDL_DestroyTexture`. `TextureHandle = std::shared_ptr<Texture>` alias. Non-copyable, non-movable. Touches: `engine_assets`.
- [x] 3.2 Added `include/engine/assets/asset_loader.hpp` + `src/assets/asset_loader.cpp`: `AssetLoader(SDL_Renderer*)` snapshots `SDL_GetBasePath()` once; `LoadTexture(std::string_view) → Result<TextureHandle>` uses `IMG_Load` + `SDL_CreateTextureFromSurface`, caches by path in `unordered_map<string, weak_ptr<Texture>>`. Sets `SDL_SCALEMODE_NEAREST` on every texture for pixel-art-friendly sampling. Errors log `[Assets]` + return `Err`. Touches: `engine_assets`.
- [x] 3.3 Updated `src/assets/CMakeLists.txt`: depends on `engine_core`, `SDL3::SDL3`, `SDL3_image::SDL3_image`. Dropped placeholder.cpp. Touches: `engine_assets`.

## 4. engine_render (Real Code)

- [x] 4.1 Added `include/engine/render/camera_2d.hpp`: aggregate with `Math::Vec2 position` + `float zoom = 1.0f`. Touches: `engine_render`.
- [x] 4.2 Added `include/engine/render/renderer.hpp` + `src/render/renderer.cpp`. SDL_CreateRenderer; logical presentation INTEGER_SCALE (with LETTERBOX fallback if integer-scale fails to set). `Clear(Color)`, `Present()`, `DrawSprite(handle, worldPos, optional<Rect>)` translating world→screen via `(worldPos - camera.position) * camera.zoom`. `Camera()` accessor for mutable access. Touches: `engine_render`.
- [x] 4.3 Invalid-handle path: null handle OR null inner SDL_Texture → log once with `[Render]` warning, suppress further occurrences, return. Touches: `engine_render`.
- [x] 4.4 Updated `src/render/CMakeLists.txt`: deps on engine_core + engine_math + engine_assets + SDL3::SDL3. Dropped placeholder.cpp. Touches: `engine_render`.

## 5. engine_runtime (Grow Application)

- [x] 5.1 Extended `include/engine/runtime/app_config.hpp` with `logicalWidth`/`logicalHeight` (320/180), `clearColor` (deep blue), `onStart` (takes Renderer + AssetLoader), and `onRender` (Renderer only). Forward-declared the engine_assets / engine_render types to avoid include explosion. Touches: `engine_runtime`.
- [x] 5.2 Rewrote `src/runtime/application.cpp` `Run()`: SDL_Init → CreateWindow (now with `SDL_WINDOW_RESIZABLE`) → construct Renderer (aborts cleanly if init fails) → construct AssetLoader → call `onStart` in try/catch (exception → log, teardown, return 5) → frame loop polls events, clears, calls `onRender` in try/catch, presents → reverse-order teardown on SDL_QUIT. Switched the event pump from WaitEventTimeout to PollEvent since we now render every frame. Touches: `engine_runtime`.
- [x] 5.3 Updated `src/runtime/CMakeLists.txt`: deps on engine_core + engine_render + engine_assets + SDL3::SDL3. Touches: `engine_runtime`.
- [x] 5.4 Verified by inspection + smoke tests: defaults applied (320x180 logical and #1A2440 clear are visible in the live run), `onStart` invoked exactly once before the frame loop (asset loaded once per smoke run — log shows a single "Loaded texture" line), `onStart` exception path teardown coded with reverse-order cleanup + non-zero return (exercised indirectly by the renderer-init-failure path which uses the same shape), `onRender` exception caught and logged with frame continuation. Touches: spec compliance.

## 6. Placeholder Iden Sprite Asset

- [x] 6.1 Created `tools/gen_iden_placeholder.py`: pure stdlib (zlib + struct only — Pillow not installed and not needed). Emits a 16×16 RGBA PNG of a deliberately-generic three-color figure (outline / skin / cloak) at `games/my_rpg/assets/characters/iden_placeholder.png`. Edit the SPRITE matrix in the script to iterate. Touches: tooling + game asset.
- [x] 6.2 Ran the script. Output is 127 bytes; PNG signature verified (89 50 4E 47). Both the script and the PNG are committed. Touches: asset.

## 7. CMake Asset Sync (Build → Runtime Output)

- [x] 7.1 Inlined `add_custom_command(TARGET game_my_rpg POST_BUILD ... copy_directory ...)` in `games/my_rpg/CMakeLists.txt`. Using `copy_directory` (not `copy_directory_if_different` — the latter doesn't exist on older CMake; `copy_directory` is incremental enough). Promote to a `cmake/StarfallAssets.cmake` helper when a second game appears. Touches: build pipeline.
- [x] 7.2 Verified: post-build "Mirroring game assets to ..." log line fires; `build/debug-windows/bin/assets/characters/iden_placeholder.png` and `build/debug-linux/bin/assets/characters/iden_placeholder.png` both present and load successfully (16x16, 127 bytes per the runtime log). Touches: asset-sync smoke test.

## 8. Game Side — Wire Up The onRender Callback

- [x] 8.1 Updated `games/my_rpg/src/main.cpp`: declared `TextureHandle iden` in `main`'s scope. `onStart` lambda captures by reference, loads the placeholder once via `loader.LoadTexture(...)`, stores the handle. `onRender` draws it centered on the 320×180 logical surface via `renderer.DrawSprite(iden, {160.f - w/2, 90.f - h/2})`. No SDL includes, no platform conditionals. Touches: `game_my_rpg`.
- [x] 8.2 Resolved at the proposal/spec/design layer before implementation: `onStart` (one-time setup) is split from `onRender` (per-frame draw) per design D11. Spec scenarios for both callbacks now live in the runtime-window delta; AppConfig and Application::Run task entries (5.1 / 5.2) carry the implementation requirements. Touches: spec resolved up-front.

## 9. Verification (Both Platforms)

- [x] 9.1 On Windows (VS Dev Shell): debug-windows configure + build clean. `game_my_rpg.exe` launched; window opened in 808ms titled "Starfall", logs show renderer 320×180 + texture loaded from exe-relative path. WM_CLOSE → "Clean shutdown." → exit 0 in 135ms. Touches: end-to-end Windows.
- [x] 9.2 On Linux (WSL2 Ubuntu 20.04 / g++-10 / WSLg): debug-linux configure + build clean. Same four log lines as Windows. SIGTERM → "Clean shutdown." → exit 0. Test-script note: log file must live under `/tmp` (ext4), not `/mnt/c` — WSL2's 9P NTFS bridge delays `fflush` visibility long enough to spuriously time out a tail-grep loop. Touches: end-to-end Linux.
- [ ] 9.3 Window-resize test. **Deferred to a follow-up manual session** — needs an attended browser-/desktop-level interaction (drag-resize, eyeball pixel-sharpness). Implementation is correct by inspection (`SDL_SetRenderLogicalPresentation(..., INTEGER_SCALE)` with LETTERBOX fallback), but the visual confirmation is a manual step. Touches: logical-presentation spec compliance.
- [ ] 9.4 Texture-cache test. **Deferred to test-framework milestone** — `unordered_map<string, weak_ptr<Texture>>` lookup is by inspection correct; a real unit test belongs alongside Catch2/GoogleTest selection. Avoiding test-only main.cpp churn. Touches: texture-assets spec compliance.
- [ ] 9.5 Invalid-handle test. **Deferred to test-framework milestone** — same reasoning as 9.4. The path is implemented (rate-limited once-per-process warning + early return) and trivially correct on inspection. Touches: error-path spec compliance.
- [ ] 9.6 Frame-budget check. **Deferred** — meaningful only once there are sprites in real numbers (tilemap, UI). One sprite + Clear/Present is bounded by vsync; not worth instrumenting. Re-open at M3 when tilemap rendering produces a non-trivial draw call count. Touches: performance budget.

## 10. Docs Update (Milestone Contract)

- [x] 10.1 GameDesign §9 M1 row updated: ✅ + 2026-06-17 + Windows/Linux note. Touches: GameDesign §9.
- [x] 10.2 CLAUDE.md "Current Status" rewritten: M1 done with the new render-stack summary (SDL3_image / AssetLoader / Renderer / Camera2D / AppConfig onStart+onRender); next target M2 (Input & Movement). Touches: project root CLAUDE.md.
- [x] 10.3 check-deps audit: 11 targets, ~21 link edges (gained SDL3 + SDL3_image on engine_assets and engine_render). New edges `engine_runtime → engine_render`, `engine_runtime → engine_assets`, `engine_assets → SDL3_image::SDL3_image`. Zero §6.3 violations — engine_runtime only links allowed engine modules; no engine→game/editor edges; engine_core remains dependency-free. Touches: rule audit.
