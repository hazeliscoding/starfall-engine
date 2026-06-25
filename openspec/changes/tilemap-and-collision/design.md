## Context

M2.75 finished a "what's on screen + sounds" stack: window, renderer, input, animated sprite, audio. The next vertical slice is **the world**: a real tilemap that Iden can walk around, with collision against walls. That's the M3 contract in GameDesign §9.

The roadmap audit between M3 and M4 surfaced three concerns:
1. **Tileset animation** (water, lanterns) → its own milestone, **M3.25**, inserted but not implemented here.
2. **Multiple tile layers + Y-sort entities** → folded into M3 itself; without them the tilemap looks visibly wrong for any town scene with NPCs.
3. **Tile interaction triggers** → folded into M7 Dialogue & Interaction (same approach-and-confirm pattern).

This change ships #2 and stubs #1 in the roadmap. One spec frames the engine work — `tilemap`, a new capability in `engine_scene`.

The cross-cutting design question is **where the engine boundary sits between scene (data) and render (drawing)**. Per DesignDoc §6.3 / CLAUDE.md hard rules, `engine_scene` can't depend on `engine_render` (and vice versa). The answer: scene owns data + iteration helpers; game owns the loop that pulls visible tiles from scene and pushes them to render. The renderer stays tilemap-agnostic.

## Goals / Non-Goals

**Goals:**

- Iden walks Embercoast — a real (if small) hand-coded village map with ground + overhead tile layers.
- Walls block movement cleanly; sliding along walls works (per-axis sweep, no "stuck on corners").
- Y-sort entities so when M6/M7 introduce Mina/Halor/Wren, Iden walking past them looks correct from day one.
- World bounds exposed for M3.5 camera clamp.
- Tilemap data shape is JSON-friendly so M4 is a serializer addition, not a re-design.

**Non-Goals:**

- All non-goals from proposal.md hold: no JSON loading (M4), no animated tiles (M3.25), no triggers (M7), no editor painting (M5), no auto-tiling, no slopes / one-way walls, no breakable tiles, no spatial queries, no lighting.

## Decisions

### D1: `Tilemap` lives in `engine_scene`, renderer stays tilemap-agnostic

**Choice:** `Engine::Scene::Tilemap` / `TileLayer` / `TileSet` are in `engine_scene`. `Engine::Render::Renderer` knows nothing about tilemaps. Game code (or a future scene system) is responsible for iterating visible tiles and calling `renderer.DrawSprite(tileset.Texture(), worldPos, tileset.SourceRect(tileId))`.

**Why:** DesignDoc §6.3 forbids `engine_scene → engine_render` and `engine_render → engine_scene` (the dependency direction would mean either render knows scene types or scene knows render types — both wrong for a layered engine). The "iterate-and-push" pattern keeps both modules clean and matches how shipping JRPG engines structure this.

**Alternative considered:** Add a `TilemapRenderer` to `engine_render` that takes raw tile data (not a `Tilemap` type). Rejected — pushing the tilemap-shaped data flatten step into the game adds boilerplate at every callsite. `ForEachVisibleTile` with a visitor lambda is the clean middle ground.

### D2: `ForEachVisibleTile` uses a `std::function` visitor (not an iterator)

**Choice:** `Tilemap::ForEachVisibleTile(viewport, std::function<void(layer, pos, tileId)>)`. The renderer-bound callback is built game-side.

**Why:** A C++ iterator with appropriate skip-empty / skip-hidden-layer logic would be ~50 lines of boilerplate. A visitor lambda is 5 lines. Performance is identical (callable inlines through `std::function` in optimized builds for hot loops, and even if it didn't, ~440 tiles/frame × a vtable hop is negligible).

**Trade-off:** `std::function` has a small heap allocation overhead at *construction* (once per `onRender` call, since the lambda captures by reference). Sub-microsecond. If profiling ever flags it, switch to `std::function_ref` or templates.

### D3: Layer sort via `int sortOrder`, not enum

**Choice:** `TileLayer::sortOrder` is a plain `int`. Convention: ground = 0, overhead = 100, future "above-overhead" = 200, etc.

**Why:** Future layers (water reflections, particle masks, UI markers) get inserted at e.g. `50` without modifying an enum. Numeric range gives room without churn.

**Alternative considered:** `enum class Layer { Ground, Overhead, ... }`. Rejected — every new layer kind would touch the enum, which lives in engine code, breaking the "game-owns-its-layout" principle.

### D4: Y-sort lives in game code, not engine_scene

**Choice:** Game's `onRender` builds a `std::vector<Drawable>` (a small struct of `{float y; std::function<void()> draw;}`) for mid-layer entities (the player + future NPCs/dynamic props), sorts by `y` ascending, then draws between the ground and overhead tile layers.

**Why:** The engine doesn't yet have entities (M6+). Adding entity infrastructure now just for Y-sort would be premature. Game-side sort is 5 lines; it gracefully scales to handle 10-100 entities in a town scene (the practical cap before some kind of spatial partition matters).

**Migration path:** When the scene system lands (M3-M4), entities live there and Y-sort becomes `scene.ForEachVisibleEntity(viewport, sortBy=Y, visit)` — same pattern as `ForEachVisibleTile`. Engine API call shape doesn't change.

### D5: Player collision footprint = 12×8 AABB at feet, not full sprite

**Choice:** Iden's sprite is 17×31, but her collision AABB is 12×8 at the bottom-center of the sprite. The "head" of the sprite passes freely through anything (e.g. a doorway that's only 16 tall — Iden's head + body wouldn't fit, but her feet do, which is how every JRPG works).

**Why:** Top-down JRPG convention. Without this, Iden gets stuck on every 16-px doorway. EarthBound, Chrono Trigger, Pokemon — all do feet-collision.

**Implementation:** `kPlayerCollisionSize = {12.0f, 8.0f}` constant in `main.cpp`. AABB origin is `{position.x + (spriteWidth - 12) / 2, position.y + spriteHeight - 8}` — bottom-center of the sprite. Tunable.

### D6: `SweepMove` is per-axis independent, not a unified swept-AABB

**Choice:** SweepMove attempts X-axis displacement first (testing the AABB at `{x+dx, y, w, h}`), then Y-axis displacement (`{x_new, y+dy, w, h}`). Each axis is rejected independently if it would collide.

**Why:** Standard 2D-tile-collision approach. Produces the "slide along walls" feel for free (move NE into a north wall → X applies, Y rejected → entity slides east). Unified swept-AABB collision is more accurate for fast-moving objects but the engine targets walking speeds (60 px/s) where the difference isn't perceivable.

**Limitation:** A swept move that goes through a thin wall in a single frame (player teleporting) wouldn't be caught. Acceptable — player movement is bounded; teleports are scripted events that handle their own collision.

### D7: Tilemap built programmatically in main.cpp at M3; JSON at M4

**Choice:** The Embercoast map is constructed in C++ in `main.cpp`'s `onStart`. Roughly:

```cpp
auto tileset = std::make_shared<TileSet>(loader.LoadTexture(...).Value(), 16, 16, 16);
tileset->MarkSolid(WALL_TILE_ID); tileset->MarkSolid(WATER_TILE_ID);
Tilemap tm{16, 16};
tm.SetTileSet(tileset);
tm.AddLayer({.name="ground", .width=40, .height=30, .tileIds={ /* 1200 ints */ }, .sortOrder=0});
tm.AddLayer({.name="overhead", .width=40, .height=30, .tileIds={ /* mostly 0s */ }, .sortOrder=100});
player.tilemap = std::move(tm);
```

**Why:** M4 is "JSON scene loading" — a deliberate separation. M3 proves the data shape works end-to-end; M4 swaps the C++ literal for a deserializer without changing the data types. The 1200-tile-ID literal will be a single large `std::array` in code, swapped to JSON in M4.

**Trade-off:** A 40×30 map's 1200 tile IDs in code is ugly. Acceptable for one milestone; M4 cleans it up.

### D8: Visible-tile culling at iteration time, not pre-baked

**Choice:** `ForEachVisibleTile` computes the visible tile range each call from the viewport rect (`floor(viewport.x / tileWidth)` to `ceil((viewport.x + viewport.w) / tileWidth)` for X bounds, same for Y). No spatial-index data structure.

**Why:** Grid-aligned tilemap lookup is O(1) per cell — already optimal. A spatial index would be slower for this case.

### D9: TileSet's tileset texture is loaded by game code, not engine

**Choice:** Game code calls `AssetLoader::LoadTexture(...)` for the tileset and passes the resulting `TextureHandle` to `TileSet`'s constructor. Engine doesn't auto-load tilesets.

**Why:** Same pattern as character sprites in M2.5. The engine doesn't decide what tileset a game uses; it provides the primitive (TextureHandle) and the data shape (TileSet) that wraps it.

### D10: Placeholder tileset for public contributors = procedural 5-tile palette

**Choice:** A new `tools/gen_placeholder_tileset.py` generates a 16×16 tileset PNG with five solid-color tiles: empty (transparent), grass (green), stone (gray, solid), wall (dark gray, solid), water (blue, solid). Game falls back to this if the licensed pack isn't present.

**Why:** Same precedent as M1's iden_placeholder.png and M2.75's embercoast_morning.wav. Reproducible, no external deps (PIL/wave), replaceable when licensed assets arrive. The placeholder map at fallback time is a 10×8 "Iden in a roofless stone room" so the demo still demonstrates collision.

## Risks / Trade-offs

- **[Risk] 1200 tile IDs in a C++ literal makes the diff for the Embercoast map enormous and ugly.** → Acceptable for M3; M4 swaps for JSON. The art-design discussion of which tiles go where lives in GameDesign anyway; the C++ literal is mechanical.
- **[Risk] Game-side Y-sort uses `std::function` per drawable; allocates per-frame.** → For M3 there's ~1 entity (player). For M6/M7 with 3 NPCs, still trivial. Re-evaluate when a town has 20+ entities (probably never for v1).
- **[Risk] `CollidesAABB` checks every visible layer.** → For M3 with 2 layers, 2× cost. With a future "decoration" layer marked non-solid, we'd want to skip collision-check for that layer entirely — add a `TileLayer::collidable` flag in a future change if profiling motivates it.
- **[Risk] Per-axis sweep doesn't handle fast-moving entities tunneling through thin walls.** → Walking speed is 60 logical px/s; at 60 FPS that's 1 px/frame. No tunneling possible. Re-evaluate if M-something adds dashing/projectiles.
- **[Risk] Hardcoded `kPlayerCollisionSize = {12, 8}` is tuned for Iden's 17×31 sprite. NPCs with different sprite dims would need their own constants.** → Acceptable for M3 (no NPCs yet). M6 introduces an NPC type that owns its own collision footprint.
- **[Risk] The user has a paid `TimeFantasy_TILES_6.24.17` pack but we haven't picked which tileset file yet.** → Inspected at apply time. The pack has a `TILESETS/` subfolder; pick the most coastal/villagey one and document the alternates.
- **[Risk] Embercoast map data is 1200 ints — easy to typo.** → Mitigation: use a small ASCII-art string in the source and convert at compile time. e.g. `"GGGGWWWGGGG"` where each char maps to a tile ID. Easier to read in diffs. Captured as a task.
- **[Risk] Visible-tile iteration doesn't know about camera zoom (only position).** → Camera zoom in `engine_render` defaults to 1.0; M3 game doesn't change it. If a future change uses zoom, `ForEachVisibleTile`'s viewport rect just needs to account for it — caller's responsibility.
- **[Risk] No tests for the SweepMove "slide along a wall" interaction with real tile data.** → Covered: tests include a SweepMove against a synthetic Tilemap with a 1-tile wall. Doesn't require the real Embercoast map.
