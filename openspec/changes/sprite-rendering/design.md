## Context

M0 produced an empty window. The next vertical slice (M1 per DesignDoc §22 and GameDesign §9) needs the minimum render stack to put a single sprite on a tinted background. Three specs frame the work:

- New `texture-assets` — PNG loading + caching + exe-relative resolution.
- New `2d-renderer` — `Renderer`, `Camera2D`, sprite drawing, logical-resolution presentation.
- Modified `runtime-window` — `Application` owns a renderer, `AppConfig` grows the render-side fields and an `onRender` callback, run loop becomes clear→callback→present each frame.

The pre-dawn blue mood, the placeholder Iden sprite, and the asset pipeline are M1-only stopgaps that later milestones (M3 tilemap, M4 scene format, art-direction pass) replace.

## Goals / Non-Goals

**Goals:**

- One sprite visible on a deep-blue background on Windows + Linux from `game_my_rpg`.
- A primitive render API that scales to tilemaps, multiple sprites, and dialogue UI without redesign.
- A texture-cache shape that prevents accidental re-decode of the same PNG.
- An asset pipeline that works identically in dev and packaged builds.
- Logical-resolution presentation (pixel-perfect upscale) — the single most defining feel choice for a JRPG.
- 60 FPS at idle. Frame budget ≤ 5 ms so future milestones have headroom.

**Non-Goals:**

- No animation, tilemap, multiple cameras, render layers, z-sort, batching, color modulation, post-processing, GPU shaders, hot reload, or Lua exposure. Each gets its own milestone.
- No render-backend abstraction. SDL_Renderer is the only backend.
- No asset GUIDs (DesignDoc §7.3 mentions them but defers; M1 uses raw paths).
- No file watching, no editor preview.

## Decisions

### D1: SDL_Renderer, not SDL_GPU

**Choice:** Use SDL3's 2D-accelerated `SDL_Renderer` API.

**Why:** SDL_GPU (introduced in SDL3) is a lower-level API requiring shader management and explicit command buffers — overkill for 2D sprite drawing today. `SDL_Renderer` automatically picks the best backend (D3D12/D3D11 on Windows, Vulkan/OpenGL on Linux), batches under the hood, and has built-in logical-resolution presentation. CLAUDE.md soft rule: "Prefer simple systems before abstractions; no render backend abstraction until a second backend is actually needed." If we ever need shader effects we don't have today (custom lighting, post-process tint), we can either layer on SDL_GPU for those passes or switch the renderer wholesale at that point.

**Alternative considered:** SDL_GPU now. Rejected — speculative complexity for a feature (custom shaders) that no milestone through M10 requires.

### D2: SDL3_image for PNG, fetched same way as SDL3

**Choice:** Pin `SDL3_image` 3.2.x via FetchContent in `external/CMakeLists.txt`, static, normalized to `SDL3_image::SDL3_image`.

**Why:** Official SDL companion library, maintained in lockstep with SDL3, builds cleanly with the same toolchain we already invoke. Produces `SDL_Surface*` that hands directly to `SDL_CreateTextureFromSurface` — no manual decode pipeline to maintain.

**Alternatives considered:**
- **stb_image** (single-header) — fewer dependencies but produces a raw byte buffer; we'd hand-roll the SDL_Surface from it. Tiny code, but we lose SDL_image's broader format support (JPG, WebP, AVIF, animated GIF) that later milestones (UI mockups, dialogue portraits) may want.
- **SDL3 built-in `SDL_LoadBMP`** — zero dependencies but BMP only. Standard pixel-art tooling (Aseprite, Photoshop) exports PNG; forcing BMP throughout the project is hostile to artists for negligible build-time savings. Rejected.

### D3: Game-to-engine render handoff via `std::function` callback

**Choice:** `AppConfig` grows `std::function<void(Render::Renderer&)> onRender`. The runtime calls it each frame between `Clear` and `Present`.

**Why:** Simplest C++20-idiomatic way to hand a "draw routine" from game to engine without inheritance. The game owns its draw logic; the engine owns the frame loop. When M3-M4 land the scene system, this callback becomes `[&scene](Renderer& r) { scene.Render(r); }` — same shape, swap the body.

**Alternatives considered:**
- **`Application` subclass with a virtual `OnRender(Renderer&)`** — clean inheritance pattern but introduces ceremony (header for the subclass, separate translation unit). For a thin `main.cpp` entry point, designated-initializer config is friendlier.
- **C-style global function** (game implements `extern "C" void GameRender(Renderer*)`) — ugly, error-prone, no closure capture.
- **Engine-owned "render queue" the game pushes commands to** — DesignDoc §7.4 sketches this for the future. Premature for M1; one sprite per frame doesn't need a command queue.

### D4: Texture cache keyed by canonical path, returns refcounted handle

**Choice:** `Engine::Assets::TextureHandle` is a `std::shared_ptr<Texture>` (or a thin wrapper around one). The cache is a `std::unordered_map<std::string, std::weak_ptr<Texture>>` keyed by canonical path. When all handles to a texture drop, the cache entry's weak_ptr expires and the GPU resource is freed; a subsequent load re-reads.

**Why:** Shared ownership matches how game code thinks ("I have a texture; the renderer also draws it"). Weak-ptr cache prevents holding GPU memory forever for never-re-used textures. Path keying is fine for M1 — when M-something introduces asset GUIDs (DesignDoc §7.3), the cache key changes but the API doesn't.

**Alternative considered:** Strong-ref cache that never evicts. Simpler but leaks GPU memory in long sessions and after scene transitions. Rejected.

### D5: Asset pipeline = CMake post-build copy + exe-relative resolution

**Choice:** A `add_custom_command(POST_BUILD ...)` on the `game_my_rpg` target syncs `games/my_rpg/assets/` → `<runtime-output-dir>/assets/` after every build. The runtime resolves relative asset paths against `argv[0]`'s directory (or SDL's `SDL_GetBasePath()`).

**Why:** Single source of truth (the source tree), single mechanism that works for both `cmake --build` and packaging (M9 just copies the bin dir as-is). `SDL_GetBasePath()` is the canonical "where is my executable" call on every SDL platform and handles macOS bundle structure too if we ever ship there.

**Sync strategy:** Use `cmake -E copy_directory_if_different` for incremental updates. For "removed source asset → remove from mirror," use a small custom script (cmake function) that diffs and prunes; OR accept that stale copies persist until clean build (M1 acceptable, can tighten later).

**Alternatives considered:**
- **Symlink** instead of copy — fastest, but Windows symlink permission issues again, and broken once packaged.
- **Run from source tree (CWD-relative paths)** — works in dev only, breaks the M9 packaging promise. Rejected.

### D6: Logical resolution = 320×180 default

**Choice:** Default `AppConfig::logicalWidth = 320`, `logicalHeight = 180`. SDL3's `SDL_SetRenderLogicalPresentation(renderer, 320, 180, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)` scales pixel-perfect to any window size, letterboxing as needed.

**Why:** Classic 16:9 pixel-art resolution. 4× scale to 1280×720 (the default window size) is integer-clean, no blurry sampling. EarthBound-feel pixel density (EarthBound was ~256×224 — same neighborhood).

**Trade-off:** 320×180 is small. UI text needs a pixel font. Dialogue boxes are constrained. If GameDesign / Game Director wants more pixel real estate (480×270 or 640×360), this is a one-config-line change with no engine impact. Listed as Open Question in the proposal.

### D7: Pre-dawn blue clear = `#1A2440`

**Choice:** Default `AppConfig::clearColor = {0x1A, 0x24, 0x40, 0xFF}` — a deep blue with a faint warm bias.

**Why:** GameDesign §6 calls for "muted dusk — soft purples, deep blues, amber lantern" and §6.6 says "the game starts in blue-hour light." A near-neutral blue with hint of warmth is the starting point; Game Director can tune. Pure cyan/teal would feel cold; pure royal blue would feel digital. `#1A2440` reads as "5 AM, still dark out, a hint of sun coming."

**Alternative considered:** Black (`#000000`). Functionally fine but misses the M1 game-deliverable requirement explicitly: "pre-dawn blue tint applied to the scene." Rejected — the mood is the M1 deliverable.

### D8: Camera2D owned by Renderer; mutable via `renderer.Camera()`

**Choice:** The renderer owns a single `Camera2D` (position + zoom). Game code reads/writes it through `renderer.Camera()`. No camera stack, no per-draw camera arg.

**Why:** One camera covers M1 (no UI overlay yet), M2 (player movement updates it), and most of M3-M4. When multiple cameras become real (mini-map, editor preview overlay), we add stack push/pop primitives. For now, one mutable camera is the simplest thing that matches every M0-M4 usage.

### D11: Split `onStart` (one-time init) from `onRender` (per-frame draw)

**Choice:** `AppConfig` exposes BOTH `std::function<void(Render::Renderer&, Assets::AssetLoader&)> onStart` (called exactly once after subsystem init, before the first frame) AND `std::function<void(Render::Renderer&)> onRender` (called every frame between Clear and Present). `onStart`'s second argument is the asset loader so game code can build texture handles up-front.

**Why:** The `AssetLoader` needs a live renderer to upload GPU textures, but the renderer doesn't exist until `Run()` constructs it. If the game tried to load textures in `main()` before calling `Run()`, there'd be no renderer; if it loaded them lazily inside `onRender`, the first frame would stutter and we'd have to write rate-limiting around "did we already load this?" logic in game code. A dedicated init phase is the clean answer — game code's `onStart` says "load these once and remember the handles," then `onRender` draws using captured handles.

**Alternative considered:** Lazy load inside `onRender` with a once-flag pattern. Works but pushes "is this the first frame?" boilerplate into every game. Rejected.

**Trade-off:** Two callbacks instead of one. Acceptable — they map to two distinct moments (init vs steady-state) that real games will always want separately.

### D9: Exceptions from `onRender` are caught and logged, loop continues

**Choice:** The runtime wraps `onRender(renderer)` in `try { ... } catch (std::exception& e) { SF_LOG_ERROR("Runtime", "onRender threw: %s", e.what()); }`.

**Why:** A typo in game code shouldn't crash the engine — the developer should see the log and have a chance to close the window normally. Re-throwing or aborting wastes their iteration time. For shipping builds we revisit (might want fail-fast there).

### D10: `Engine::Math` is aggregate-only, no methods beyond operators

**Choice:** `Vec2`, `IVec2`, `Rect`, `Color` are plain aggregates. Vec2 gets `operator+/-/*` (with another Vec2 and with float scalars). No `.Normalize()`, no `.Length()` — add when a milestone needs them.

**Why:** YAGNI. M1 needs add/subtract for camera-to-world translation; that's it. CLAUDE.md soft rule: prefer simple before abstract.

## Risks / Trade-offs

- **[Risk] `std::function` for `onRender` allocates on every Configure call.** → Mitigation: it's called once at setup, not per frame. Negligible overhead. If profiling later flags it (it won't), use `function_ref` or a fixed-size callable.
- **[Risk] Logical-resolution presentation with integer-scale produces letterbox bars at non-multiple window sizes.** → That's the trade-off for pixel-perfect output. Document; users can switch to `SDL_LOGICAL_PRESENTATION_LETTERBOX` (non-integer) if they don't care about pixel-perfect — but the M1 default is integer-scale because RPG.
- **[Risk] SDL3_image static linking adds ~500 KB-1 MB to the binary.** → Acceptable for a Steam-distributed indie game; M9 packaging will swallow this.
- **[Risk] Texture cache evicts when all handles drop, causing re-decode on next request.** → For M1 with one sprite, this is irrelevant. For later, scene-level "this scene's textures stay loaded" is a job for the scene manager (M3-M4), not the asset cache.
- **[Risk] `SDL_GetBasePath` returns a heap-allocated string the caller must free.** → Wrap in a one-shot init that captures it as a `std::string` member of the asset loader. Minor.
- **[Risk] The placeholder Iden sprite doesn't exist yet; we need to create one for the M1 deliverable.** → Either: (a) hand-draw a 16×16 placeholder in a paint program, (b) generate one procedurally at apply time (small script writes a recognizable pixel-art shape into a PNG). Option (b) is reproducible, doesn't require artist time, and reinforces the "deliberate placeholder" feel — picked unless the Game Director wants to draw one.
- **[Risk] We're building a callback-based render handoff that the scene system will replace.** → The callback's body becomes `scene.Render(renderer)` at M3-M4. Zero engine change required.
