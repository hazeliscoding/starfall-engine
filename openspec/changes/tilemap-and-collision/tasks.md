## 1. Roadmap Doc Updates (Land Up Front)

- [ ] 1.1 Expand DesignDoc §22 "Milestone 3 — Tilemap and Collision" Goals to add: "Multiple tile layers (ground + overhead)" and "Y-sort entities against tilemap depth". Deliverable line stays. Touches: `docs/DesignDoc.md`.
- [ ] 1.2 Insert a new "Milestone 3.25 — Tileset Animation" subsection in DesignDoc §22 between M3 and M3.5. Goals: per-tile animation clips (re-use AnimationClip primitive shape), water + lantern flicker on Embercoast. Deliverable: visible tile motion in the M3 map. Mark explicitly as "stub, real spec lands in its own change." Touches: `docs/DesignDoc.md`.
- [ ] 1.3 Insert M3.25 row in GameDesign §9 between M3 and M3.5: `| M3.25 Tileset Animation | Ocean tiles lap and lanterns flicker at Embercoast. |`. Touches: `docs/GameDesign.md`.
- [ ] 1.4 Insert M3.25 row in README.md status table between M3 and M3.5, with `⏳` state. Touches: `README.md`.

## 2. engine_scene — Real Code

- [ ] 2.1 Add `include/engine/scene/tile_layer.hpp`: `Engine::Scene::TileLayer` aggregate (name + width + height + tileIds vector + visible + sortOrder). Header-only. Touches: `engine_scene`.
- [ ] 2.2 Add `include/engine/scene/tile_set.hpp` + `src/scene/tile_set.cpp`: `Engine::Scene::TileSet` holds `Engine::Assets::TextureHandle` + tile dims + columns; `SourceRect(tileId)` row-major lookup; `MarkSolid` / `IsSolid` via `std::unordered_set<int> solidTileIds_`; `IsSolid(0)` always returns false; `MarkSolid(0)` is silent no-op. Touches: `engine_scene`.
- [ ] 2.3 Add `include/engine/scene/tilemap.hpp` + `src/scene/tilemap.cpp`: `Engine::Scene::Tilemap` with constructor taking tile dims; `SetTileSet(std::shared_ptr<TileSet>)`; `AddLayer(TileLayer)` stable-sorts by sortOrder; `Layers()` returns `std::span<const TileLayer>`; `WorldWidth()` / `WorldHeight()` computed from max-layer-dims × tile dims; `ForEachVisibleTile(viewport, visit)` per spec (visit signature: `void(const TileLayer&, Math::Vec2, int)`); `CollidesAABB(rect)` iterates only the tiles the AABB overlaps (O(≤6) per query for entity-sized AABBs). Touches: `engine_scene`.
- [ ] 2.4 Add `include/engine/scene/sweep.hpp` + `src/scene/sweep.cpp`: free function `Engine::Scene::SweepMove(currentPos, displacement, aabbSize, tilemap) → Math::Vec2`. Per-axis independent test (X then Y, both rejected independently if they'd collide). Touches: `engine_scene`.
- [ ] 2.5 Update `src/scene/CMakeLists.txt`: SOURCES `tile_set.cpp + tilemap.cpp + sweep.cpp`; drop placeholder.cpp. DEPENDS unchanged (`engine_core + engine_math + engine_assets`). Touches: `engine_scene`.

## 3. Tileset Asset Pipeline

- [ ] 3.1 Inspect `games/my_rpg/assets/TimeFantasy_TILES_6.24.17/TILESETS/` at apply time. Pick the closest match for coastal/village/weathered Embercoast feel. Document the chosen file + 2-3 alternates in `main.cpp` as the licensed tileset. Touches: investigation.
- [ ] 3.2 Add `tools/gen_placeholder_tileset.py`: pure-stdlib PNG generator (re-using the inline PNG-encoder pattern from `tools/gen_iden_placeholder.py`). Emits a 16×80 (1 col × 5 rows) tileset to `games/my_rpg/assets/tiles/placeholder_tileset.png`: row 0 transparent (empty), row 1 grass (green), row 2 path/sand (tan), row 3 stone (gray, solid), row 4 wall (dark gray, solid), row 5 water (blue, solid). Document the IDs (1=grass, 2=path, 3=stone, 4=wall, 5=water) in a comment matching the constants used in main.cpp's map data. Touches: tooling.
- [ ] 3.3 Run the script. Verify the PNG opens in an image viewer and is < 1 KB. Commit script + output. Touches: assets.

## 4. Embercoast Map Data + Game Wire-Up

- [ ] 4.1 In `games/my_rpg/src/main.cpp`, define an ASCII-art helper:
  ```cpp
  // 'G' = grass, '.' = path/sand, 'S' = stone, 'W' = wall, '~' = water, ' ' = empty
  ```
  …and a `MakeLayerFromAscii(width, height, str, charMap)` helper that converts a multi-line raw-string-literal ASCII map to a `TileLayer`. Touches: `game_my_rpg`.
- [ ] 4.2 Author the Embercoast map (~40×30 tiles) as two ASCII layers (ground + overhead) in `main.cpp`. Per-tile choices should leave a clear "cliff path" to the north that's bordered by walls (per GameDesign §4 — the road north is gated at M3, opens at M7's Halor interaction). For M3 the cliff path is solid all the way; M7 will toggle it. Touches: `game_my_rpg`.
- [ ] 4.3 In `onStart`: load the TimeFantasy tileset (with placeholder fallback per design D10); construct TileSet (mark stone/wall/water as solid); construct Tilemap; AddLayer for ground + overhead; assign to PlayerState.tilemap (PlayerState gains a `std::optional<Tilemap> tilemap;` field — or `std::unique_ptr<Tilemap>` so onStart can move-construct it). Touches: `game_my_rpg`.
- [ ] 4.4 Update `onUpdate`'s movement block: instead of unconditionally applying `position += direction * step`, build the desired `Math::Vec2 displacement`, call `Engine::Scene::SweepMove(player.position, displacement, kPlayerCollisionSize, *player.tilemap)`, and apply the returned (clamped) displacement. The "Iden walks off the screen" no-collision M2.75 behavior becomes "Iden bumps into walls cleanly." Touches: `game_my_rpg`.
- [ ] 4.5 Update `onRender` to use the tilemap. Order:
  1. `tilemap.ForEachVisibleTile(viewport, ...)` — but ONLY for layers with `sortOrder < 50` (ground).
  2. Build the Y-sort drawables vector (player + future entities); sort by `position.y + spriteHeight`; draw.
  3. `tilemap.ForEachVisibleTile(viewport, ...)` again — but ONLY for layers with `sortOrder >= 50` (overhead).
  Refactor: pull the layer-range parameter into `ForEachVisibleTile` (overload with `int minSortOrder, int maxSortOrder` filter) so the same call works for both halves without two viewport-rect intersections. Touches: `game_my_rpg` + minor `engine_scene` API addition.
- [ ] 4.6 Add `kPlayerCollisionSize = {12.0f, 8.0f}` constant near the other game-tuning constants in `main.cpp`. Touches: `game_my_rpg`.

## 5. game_my_rpg — Link engine_scene

- [ ] 5.1 Add `engine_scene` to `games/my_rpg/CMakeLists.txt` DEPENDS (allowed per §6.3 "game_* → engine_*"). Touches: `game_my_rpg`.

## 6. Tests

- [ ] 6.1 Add `tests/scene/CMakeLists.txt` with `starfall_add_test(NAME engine_scene_tests SOURCES tilemap_tests.cpp DEPENDS engine_scene engine_assets SDL3::SDL3 SDL3_image::SDL3_image)`. Include `tests/common/` for the SDL fixture. Touches: tests.
- [ ] 6.2 Update `tests/CMakeLists.txt` foreach to add `scene`. Touches: tests root.
- [ ] 6.3 Add `tests/scene/tilemap_tests.cpp`. Use `SdlWindowAndRendererFixture` + `AssetLoader` for any test that needs a real TextureHandle for the TileSet; the rest can use a dummy `TextureHandle{}`. Coverage:
  - TileSet: SourceRect math for tile 1, tile in second column, tile in second row (3 cases per spec).
  - TileSet: default IsSolid returns false; MarkSolid + IsSolid round-trip; IsSolid(0) always false; MarkSolid(0) is no-op.
  - Tilemap: AddLayer sorts by sortOrder (insert overhead first, then ground; Layers() returns ground first).
  - Tilemap: WorldWidth/Height reflect largest layer.
  - Tilemap: ForEachVisibleTile — full-map viewport visits every non-empty tile; clipped viewport visits only overlapping cells; empty tiles skipped; hidden layers skipped.
  - Tilemap: CollidesAABB — true for AABB on solid tile, false for AABB in open space, true for AABB straddling, false when the solid tile is on a hidden layer.
  - SweepMove: no-obstacle returns full displacement; wall-on-X returns (0, dy); corner pin returns (0,0); slide-along-wall returns (dx, 0).
  Touches: tests.

## 7. Verification (Both Platforms)

- [ ] 7.1 Windows (VS Dev Shell): full configure + build + `ctest --preset debug-windows`. All previous 36 + new scene tests pass. Touches: end-to-end Windows.
- [ ] 7.2 Windows runtime smoke: launch `game_my_rpg.exe`. Embercoast ground tiles render. Iden visible at her start position. Walking into a wall stops her cleanly; diagonal into a wall slides along it. Cliff path is visibly blocked. Music still triggers on first movement. Close window → exit 0. Touches: end-to-end Windows runtime.
- [ ] 7.3 Linux (WSL2 g++-10): same flow with `debug-linux`. Touches: end-to-end Linux.
- [ ] 7.4 Linux runtime smoke via WSLg: same as 7.2. Touches: end-to-end Linux runtime.
- [ ] 7.5 Rule audit (`check-deps`): no new module-level edges (engine_scene's deps unchanged; game_my_rpg gains a `→ engine_scene` edge which is `game_* → engine_*` and allowed). Confirm zero §6.3 violations. Touches: rule audit.
- [ ] 7.6 Visual eye-check: verify multi-layer rendering — confirm an overhead tile (e.g. an awning) renders OVER Iden when she walks under it. (Requires the Embercoast map to have at least one overhead tile positioned where Iden walks under it. Author the map accordingly.) Touches: visual verification.
- [ ] 7.7 Visual eye-check: collision footprint. Verify Iden's head clears low overhangs (a 1-tile-tall passage works), and her feet collide cleanly with the walls. If collision feels off (too generous or too tight), tune `kPlayerCollisionSize` and re-verify. Touches: feel tuning.

## 8. Docs Wrap-Up

- [ ] 8.1 Mark GameDesign §9 M3 row complete: `✅ M3 Tilemap & Collision (YYYY-MM-DD)` with brief summary (multi-layer Embercoast map; per-axis sweep collision; Y-sort entities). M3.25 row stays unchecked. Touches: GameDesign §9.
- [ ] 8.2 Update CLAUDE.md "Current Status": M3 done; tilemap + collision + Y-sort summary; next target M3.25 Tileset Animation. Move M2.75 into "Previously complete" list. Touches: project root CLAUDE.md.
- [ ] 8.3 README.md status table: M3 → ✅; bump 🎯 Next to M3.25. Touches: README.
