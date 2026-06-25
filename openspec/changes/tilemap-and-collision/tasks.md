## 1. Roadmap Doc Updates (Land Up Front)

- [x] 1.1 DesignDoc §22 Milestone 3 expanded: added multi-layer rendering and Y-sort to Goals; appended a "scope expanded on 2026-06-19" note explaining why both folded into M3. Deliverable line updated to mention the visible outcome (walls block cleanly, sliding works, cliff path blocked).
- [x] 1.2 DesignDoc §22 gained new "Milestone 3.25 — Tileset Animation" subsection between M3 and M3.5. Marked explicitly as a stub. Goals reference re-using the M2.5 AnimationClip primitive shape.
- [x] 1.3 GameDesign §9 row inserted: `| M3.25 Tileset Animation | Ocean tiles lap and lanterns flicker at Embercoast. |`.
- [x] 1.4 README status table row inserted with `⏳`.

## 2. engine_scene — Real Code

- [x] 2.1 Added `include/engine/scene/tile_layer.hpp`: header-only aggregate per spec.
- [x] 2.2 Added `tile_set.hpp` + `tile_set.cpp`: row-major SourceRect lookup, MarkSolid/IsSolid with IsSolid(0)=false always, MarkSolid(0)=silent no-op.
- [x] 2.3 Added `tilemap.hpp` + `tilemap.cpp`: stable-sort AddLayer; Layers() returns span; WorldWidth/Height from max-layer-dims; ForEachVisibleTile with viewport-rect culling + 4-arg overload that filters by sortOrder window (lets game render ground / Y-sort / overhead in three passes with one API). CollidesAABB iterates only tiles touched by the AABB. Used elaborated `class TileSet` in member-function return types to disambiguate from the same-named accessor.
- [x] 2.4 Added `sweep.hpp` + `sweep.cpp`: per-axis-independent SweepMove function.
- [x] 2.5 `src/scene/CMakeLists.txt` updated; placeholder.cpp dropped.

## 3. Tileset Asset Pipeline

- [x] 3.1 Inspected `TimeFantasy_TILES_6.24.17/TILESETS/`: candidates are `outside.png` (52×24 tiles — primary), `terrain.png` (39×38), `water.png` (51×55), `world.png` (39×26). All clean 16×16 grids. **Deviation**: M3 ships with the procedural placeholder tileset (deterministic visuals); swapping to specific TimeFantasy tile IDs becomes a Game Director art pass. Documented inline in `main.cpp` near the tileset load.
- [x] 3.2 Added `tools/gen_placeholder_tileset.py`: pure-stdlib PNG generator, 16×80 (1 col × 5 rows). IDs 1=grass, 2=path, 3=stone, 4=wall, 5=water. Subtle per-pixel noise so tiles read as pixel-art not flat fill.
- [x] 3.3 Ran the script. Output: 1182 bytes (slightly over the <1KB target — noise inflates the deflate; acceptable). Committed script + PNG.

## 4. Embercoast Map Data + Game Wire-Up

- [x] 4.1 Added `MakeLayerFromAscii` helper in `main.cpp`; charset W/G/./S/~/A/space.
- [x] 4.2 Authored Embercoast as 20×12 tiles (smaller than the 40×30 default — sized for one logical-screen-wide M3 demo without needing the M3.5 camera follow first). Ground layer: north wall (cliff path blocked), grass borders, one stone building (4×3 with doorway visual), south sea band. Overhead layer: a 4-tile awning extending south of the building so Iden visibly walks under it.
- [x] 4.3 onStart loads `assets/tiles/placeholder_tileset.png` (the licensed-tileset swap is the Game Director's polish pass); constructs TileSet with stone/wall/water marked solid; constructs Tilemap; adds ground + overhead layers; spawns Iden on the walkable strip south of the building. PlayerState gained `std::unique_ptr<Tilemap> tilemap;`.
- [x] 4.4 onUpdate runs movement through `Engine::Scene::SweepMove` against the feet-AABB; falls back to free movement when no tilemap is present (defensive — doesn't happen in practice).
- [x] 4.5 onRender does the three-pass draw: ground (sortOrder < 50) → Y-sorted mid-layer drawables → overhead (sortOrder ≥ 50). Added the 4-arg `ForEachVisibleTile` overload to `engine_scene` for this.
- [x] 4.6 `kPlayerCollisionSize = {12.0f, 8.0f}` constant added.

## 5. game_my_rpg — Link engine_scene

- [x] 5.1 `engine_scene` added to game_my_rpg DEPENDS.

## 6. Tests

- [x] 6.1 `tests/scene/CMakeLists.txt` added (registers `engine_scene_tests`; no SDL_image dep needed since tests don't load real PNGs).
- [x] 6.2 `tests/CMakeLists.txt` foreach extended with `scene`.
- [x] 6.3 `tests/scene/tilemap_tests.cpp` — 22 TEST_CASEs covering the full spec: TileSet SourceRect math (3 cases), MarkSolid/IsSolid (3 cases), Tilemap stable-sort layers, WorldWidth/Height, ForEachVisibleTile full-map / clipped / empty-skip / hidden-layer / order / sortOrder window (6 cases), CollidesAABB four scenarios, SweepMove four scenarios. All tests run without a real renderer using null `TextureHandle{}`.

## 7. Verification (Both Platforms)

- [x] 7.1 Windows (VS Dev Shell): build + ctest → **58/58 tests pass** in 4.57s (22 new scene tests + the existing 36). Two test fixes during apply: corner-pin test had entity coords that didn't actually reach the solid walls (fixed coords); slide-along-wall test had an em-dash in the name that broke ctest's name-match regex (renamed to use `-`).
- [x] 7.2 Windows runtime smoke: `game_my_rpg.exe` launches, logs `Embercoast loaded: 20x12 tiles (world 320x192)`, all assets (12 frames + theme + footstep + tileset) load clean. WM_CLOSE → exit 0. Audible/visible movement-vs-wall checks need attended manual test — same caveat as M2/M2.5/M2.75.
- [x] 7.3 Linux (WSL2 g++-10): build + ctest 58/58 in 23.71s; runtime smoke clean.
- [x] 7.4 Linux runtime smoke: same log lines as Windows; clean SIGTERM → exit 0.
- [x] 7.5 check-deps: engine_scene's deps unchanged (engine_core + engine_math + engine_assets). game_my_rpg gained a direct → engine_scene edge per design (allowed per §6.3). 11 targets, ~27 edges, 0 violations.
- [ ] 7.6 Visual eye-check for overhead-tile pass-over. **Deferred to user attended-test.** Awning is placed in the map at row 6 cols 7-10 (south of the building) so walking horizontally through that strip should show the awning drawing over Iden's upper body.
- [ ] 7.7 Visual eye-check for collision footprint feel. **Deferred to user attended-test.** `kPlayerCollisionSize = {12, 8}` is the tuning knob if it feels off.

## 8. Docs Wrap-Up

- [x] 8.1 GameDesign §9 M3 row marked ✅ with date + summary.
- [x] 8.2 CLAUDE.md "Current Status" rewritten for M3 done; M2.75 → Previously complete; Next target M3.25 Tileset Animation.
- [x] 8.3 README status table: M3 ✅; M3.25 🎯 Next.

## 9. Per-Layer TileSet + TimeFantasy Art Swap (in-PR follow-on)

Scope added 2026-06-19 after the initial placeholder-tileset implementation landed: extend the engine so a single Tilemap can mix multiple TileSets (one per layer), then re-author Embercoast against the licensed TimeFantasy `terrain.png` + `water.png` + `house.png` packs with a procedural-tileset fallback for public clones.

- [x] 9.1 `TileLayer` gains optional `std::shared_ptr<TileSet> tileset` (null = use the Tilemap's shared one).
- [x] 9.2 `Tilemap::ResolveTileSet(const TileLayer&)` returns the layer's own or the shared, in that order.
- [x] 9.3 `CollidesAABB` uses `ResolveTileSet(layer)` for per-layer solidity. Backward compatible: callers that only set `Tilemap::SetTileSet` still work.
- [x] 9.4 2 new tests: ResolveTileSet preference + CollidesAABB per-layer solidity (60/60 pass on both platforms).
- [x] 9.5 Spec delta extended: new `Per-Layer TileSet Resolution` requirement + 3 scenarios; CollidesAABB requirement reworded to reference `ResolveTileSet`.
- [x] 9.6 `main.cpp` rewritten to load 3 PNGs (terrain.png 39 cols, water.png 51 cols, house.png 69 cols) with licensed-tileset-or-fallback logic mirroring the M2.75 audio approach. 5 thin masks (grass, cliff, building, sea, awning) authored as ASCII; each layer pinned to its TileSet.
- [x] 9.7 Tile-ID picks centralised in `TerrainTile{Grass=44, Path=161, Cliff=550}`, `WaterTile{Sea=207}`, `HouseTile{Wall=278, Awning=299}` namespaces — iteration is a one-line change.
- [x] 9.8 Windows + Linux verified: 60/60 tests, runtime smoke logs `Embercoast loaded: 20x12 tiles (world 320x192), TimeFantasy art` on both.
- [ ] 9.9 Visual eye-check of TimeFantasy art picks. **Deferred to user attended-test.** First-pass IDs were picked from probe + crop renders; cliff cap may tile imperfectly horizontally and the awning is a slate-roof-with-beam tile rather than a pure overhang. Adjust the constants in `main.cpp` per Game Director taste.
