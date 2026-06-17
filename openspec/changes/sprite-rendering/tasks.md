## 1. Third-Party — SDL3_image

- [ ] 1.1 Extend `external/CMakeLists.txt` to pin SDL3_image 3.2.x via `FetchContent_Declare(SDL3_image GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git GIT_TAG <pinned-release-tag>)`. Disable tests/examples/install in the same style as SDL3. Static linking only. Normalize alias to `SDL3_image::SDL3_image` post-fetch. Touches: external dep.
- [ ] 1.2 Verify configure succeeds on Windows (MSVC + Ninja) — SDL3_image builds, `SDL3_image::SDL3_image` resolves. Touches: external smoke test (Windows).
- [ ] 1.3 Verify configure succeeds on Linux (WSL2 g++-10 + Ninja). Touches: external smoke test (Linux).

## 2. engine_math (Real Code)

- [ ] 2.1 Replace `src/math/placeholder.cpp` with real headers under `include/engine/math/`: `vec2.hpp` (`Engine::Math::Vec2 {float x,y}` + `Vec2 operator+/-`, `Vec2 operator*(Vec2, float)`), `ivec2.hpp`, `rect.hpp` (`{float x,y,w,h}`), `color.hpp` (`{uint8_t r,g,b,a}`). All header-only aggregates. Touches: `engine_math`.
- [ ] 2.2 Update `src/math/CMakeLists.txt`: keep `starfall_add_module(NAME engine_math DEPENDS engine_core)`; remove the placeholder.cpp source (header-only target). Touches: `engine_math`.

## 3. engine_assets (Real Code)

- [ ] 3.1 Add `include/engine/assets/texture.hpp` defining an opaque `Engine::Assets::Texture` (PIMPL: holds an `SDL_Texture*` + a width/height pair) and a `TextureHandle = std::shared_ptr<Texture>` alias. Texture's destructor calls `SDL_DestroyTexture`. Touches: `engine_assets`.
- [ ] 3.2 Add `include/engine/assets/asset_loader.hpp` + `src/assets/asset_loader.cpp`: `Engine::Assets::AssetLoader` constructed with a renderer reference, exposing `Result<TextureHandle> LoadTexture(std::string_view path)`. Uses `SDL_GetBasePath()` once at construction for exe-relative resolution, caches by canonical string path with `std::unordered_map<std::string, std::weak_ptr<Texture>>`. Touches: `engine_assets`.
- [ ] 3.3 Update `src/assets/CMakeLists.txt`: `starfall_add_module(NAME engine_assets DEPENDS engine_core SDL3::SDL3 SDL3_image::SDL3_image)`. Drop placeholder.cpp. Touches: `engine_assets`.

## 4. engine_render (Real Code)

- [ ] 4.1 Add `include/engine/render/camera_2d.hpp`: `Engine::Render::Camera2D {Math::Vec2 position{}; float zoom=1.0f;}`. Aggregate. Touches: `engine_render`.
- [ ] 4.2 Add `include/engine/render/renderer.hpp` + `src/render/renderer.cpp`: `Engine::Render::Renderer` constructed with `SDL_Window*` + logical width/height. Owns `SDL_Renderer*`. Calls `SDL_SetRenderLogicalPresentation(..., SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)` in ctor. Exposes `Clear(Math::Color)`, `DrawSprite(TextureHandle, Math::Vec2 worldPos, std::optional<Math::Rect> sourceRect = std::nullopt)`, `Present()`, `Camera2D& Camera()`. Camera2D mutation read by next DrawSprite. Touches: `engine_render`.
- [ ] 4.3 Implement invalid-handle path in `DrawSprite`: if handle is null or its inner pointer is null, log once-per-handle (rate-limited) and return without drawing. Touches: `engine_render`.
- [ ] 4.4 Update `src/render/CMakeLists.txt`: `starfall_add_module(NAME engine_render DEPENDS engine_core engine_math engine_assets SDL3::SDL3)`. Drop placeholder.cpp. Touches: `engine_render`.

## 5. engine_runtime (Grow Application)

- [ ] 5.1 Extend `include/engine/runtime/app_config.hpp`: add `int logicalWidth = 320`, `int logicalHeight = 180`, `Engine::Math::Color clearColor{0x1A, 0x24, 0x40, 0xFF}`, `std::function<void(Engine::Render::Renderer&, Engine::Assets::AssetLoader&)> onStart;`, and `std::function<void(Engine::Render::Renderer&)> onRender;` (see design D11 for the onStart/onRender split). Touches: `engine_runtime`.
- [ ] 5.2 Update `src/runtime/application.cpp` `Run()`: after creating the window, construct an `Engine::Render::Renderer` and an `Engine::Assets::AssetLoader` (both as `std::unique_ptr` members). If `config_.onStart` is set, call it once in a `try`/`catch`; an `onStart` exception logs `[Runtime]` and aborts `Run()` with a non-zero exit code (no frame loop). Then enter the frame loop: poll events → `renderer->Clear(config_.clearColor)` → if `config_.onRender` set, `try { config_.onRender(*renderer); } catch (std::exception& e) { SF_LOG_ERROR("Runtime", "onRender threw: %s", e.what()); }` → `renderer->Present()`. Tear down in reverse: asset loader → renderer → window. Touches: `engine_runtime`.
- [ ] 5.3 Update `src/runtime/CMakeLists.txt`: `starfall_add_module(NAME engine_runtime DEPENDS engine_core engine_render SDL3::SDL3)`. Touches: `engine_runtime`.
- [ ] 5.4 Confirm the `runtime-window` MODIFIED spec scenarios pass at code level: default logical resolution / clear color, onRender optional, onRender called once per frame, onRender exception doesn't crash loop. Touches: spec compliance.

## 6. Placeholder Iden Sprite Asset

- [ ] 6.1 Create `tools/gen_iden_placeholder.py` (Python; no runtime dep needed): a 30-line script that emits a 16×16 PNG of a deliberately-generic pixel-art figure (head + body silhouette, 3-4 colors) to `games/my_rpg/assets/characters/iden_placeholder.png`. Reproducible — checked-in script + checked-in output. Touches: tooling + game asset.
- [ ] 6.2 Run the script once to produce the PNG. Verify it loads in any image viewer and is < 1 KB. Commit both the script and the PNG. Touches: asset.

## 7. CMake Asset Sync (Build → Runtime Output)

- [ ] 7.1 Add a CMake helper in `cmake/StarfallAssets.cmake` (or inline in `games/my_rpg/CMakeLists.txt` for M1 simplicity): `add_custom_command(TARGET game_my_rpg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:game_my_rpg>/assets")`. Touches: build pipeline for `game_my_rpg`.
- [ ] 7.2 Verify after build: `build/<preset>/bin/assets/characters/iden_placeholder.png` exists and matches the source. Touches: asset-sync smoke test.

## 8. Game Side — Wire Up The onRender Callback

- [ ] 8.1 Update `games/my_rpg/src/main.cpp`: declare a `TextureHandle iden;` in `main` scope. Configure with `onStart` that loads the texture once (`iden = loader.LoadTexture("assets/characters/iden_placeholder.png").Value();`) and captures by reference, and `onRender` that draws it at logical center (`renderer.DrawSprite(iden, {160.0f - spriteW/2.0f, 90.0f - spriteH/2.0f});`). No platform conditionals, no SDL includes. Touches: `game_my_rpg`.
- [x] 8.2 Resolved at the proposal/spec/design layer before implementation: `onStart` (one-time setup) is split from `onRender` (per-frame draw) per design D11. Spec scenarios for both callbacks now live in the runtime-window delta; AppConfig and Application::Run task entries (5.1 / 5.2) carry the implementation requirements. Touches: spec resolved up-front.

## 9. Verification (Both Platforms)

- [ ] 9.1 On Windows (VS Dev Shell): `cmake --preset debug-windows && cmake --build --preset debug-windows`. Launch `bin\game_my_rpg.exe`. Confirm: window opens titled "Starfall", background is the deep-blue clear color, a small pixel-art figure is visible at logical center (~screen center after 4× scale). Close button → exit 0. Touches: end-to-end Windows.
- [ ] 9.2 On Linux (WSL2): same flow with `debug-linux`. Confirm window opens via WSLg with the same visual. SIGTERM → exit 0. Touches: end-to-end Linux.
- [ ] 9.3 Window-resize test: resize the window to a non-multiple of 320×180 (e.g. 1100×700). Confirm letterbox bars appear, sprite stays pixel-sharp, no blurry sampling. Touches: logical-presentation spec compliance.
- [ ] 9.4 Texture-cache test: temporarily extend onRender to load the same path twice, log a counter. Confirm the second call hits the cache (no second decode). Revert when verified. Touches: texture-assets spec compliance.
- [ ] 9.5 Invalid-handle test: temporarily call `LoadTexture("does-not-exist.png")` and pass the resulting handle to DrawSprite. Confirm: error logged, no crash, frame still presents. Revert. Touches: error-path spec compliance.
- [ ] 9.6 Frame-budget check: time the average per-frame work in Debug. Goal ≤ 5 ms with one sprite. If significantly over, profile before declaring M1 done. Touches: performance budget.

## 10. Docs Update (Milestone Contract)

- [ ] 10.1 In `docs/GameDesign.md` §9, mark M1 row complete: `✅ M1 Sprite Rendering (YYYY-MM-DD)` + brief annotation confirming both platforms verified. Touches: GameDesign §9.
- [ ] 10.2 Update `CLAUDE.md` "Current Status": M1 done, next target M2 (Input & Movement). Touches: project root CLAUDE.md.
- [ ] 10.3 Cross-check `check-deps` skill against the new `engine_runtime → engine_render` edge — confirm clean. Touches: rule audit.
