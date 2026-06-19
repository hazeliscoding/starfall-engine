## Why

M2.75 left Iden walking on a static blue background. The next user-visible step per GameDesign §9 (M3) is: **"Embercoast traversable on a hand-coded map. Cliff path blocked."** A tilemap is also the prerequisite for everything downstream — M3.25 (animated tiles), M3.5 (camera follow with map bounds), M4 (JSON scene loading), M5 (editor painting), M7 (interaction zones).

The roadmap audit between M3 and M4 surfaced two scope expansions that belong in M3 itself (not separate milestones):
- **Multiple tile layers** (ground + overhead): without an overhead layer, buildings can't have awnings/eaves/signs that pass over Iden's head when she walks under them. Single-layer tilemap looks visibly wrong for any town scene.
- **Y-sort entities against tile-layer depth**: Iden walking past Mina (M6) needs the front/back order to track each entity's Y position. Without this, NPCs always render either in front of or behind the player — both wrong.

This change also inserts an **M3.25 Tileset Animation** stub into the roadmap (DesignDoc §22 + GameDesign §9) so the next-milestone landing pad exists. M3.25 implementation comes in its own proposal.

## What Changes

- Add **`Engine::Scene::TileLayer`** aggregate: name, width/height in tiles, flat `std::vector<int>` of tile IDs (row-major, 0 = empty), visibility flag, sort order (0 = ground, 100 = overhead).
- Add **`Engine::Scene::TileSet`**: holds a single tileset `Engine::Assets::TextureHandle`, tile width/height, columns per row; resolves `tileId → Engine::Math::Rect sourceRect` via simple division. Exposes `MarkSolid(tileId)` / `IsSolid(tileId)` for per-tile-id solid flagging (configured once at startup).
- Add **`Engine::Scene::Tilemap`**: owns a `std::shared_ptr<TileSet>`, a list of `TileLayer`s, and world dimensions derived from layers. Provides:
  - `AddLayer(TileLayer)` — append a layer; auto-sorts by `sortOrder`.
  - `Layers() → std::span<const TileLayer>` — iteration in sort order (ground → overhead).
  - `WorldWidth()` / `WorldHeight()` — bounds in world units (for M3.5 camera clamp).
  - `ForEachVisibleTile(viewport, visit)` — iterates each layer in sort order, calls `visit(layer, worldPos, tileId)` for tiles whose world rect overlaps the viewport. Game uses this to drive `Renderer::DrawSprite` calls — keeps engine_scene unaware of engine_render (per §6.3 module rules).
  - `CollidesAABB(worldRect) → bool` — true if any solid tile overlaps the AABB. Uses the configured TileSet's `IsSolid` predicate.
- Add **`Engine::Scene::SweepMove`** helper: given a current position, desired displacement, AABB size, and tilemap, returns the largest allowable displacement that doesn't penetrate solid tiles. Per-axis sweep so player slides along walls instead of getting stuck on corners.
- **Y-sort game-side**: game's `onRender` builds a small `std::vector<Drawable>` of mid-layer things (player + future NPCs), sorts by feet-Y (`position.y + spriteHeight`) ascending, then draws between the ground and overhead tile layers. No new engine capability — engine provides the primitives, game does the wiring (per CLAUDE.md soft rule "prefer simple before abstract").
- **Game side `games/my_rpg`**: hardcoded Embercoast map data in `main.cpp` (Tilemap built programmatically — M4 JSON-izes this). Player collision uses a footprint AABB (smaller than the full sprite — see design D5) so Iden's head doesn't catch on doorframes. Player movement runs through `SweepMove`.
- **Tileset choice**: a tileset from the licensed `TimeFantasy_TILES_6.24.17` pack (specific file picked at apply time after inspecting the pack — coastal/village vibe). Fallback for public contributors: a procedurally-generated 16×16 tileset PNG (sky / grass / stone / wall / water — five tiles) via a new `tools/gen_placeholder_tileset.py`.
- **`engine_runtime` does NOT grow** — `onStart` / `onUpdate` / `onRender` signatures unchanged. Game owns the Tilemap, builds it in `onStart`, uses it in `onUpdate` (collision) and `onRender` (visible-tile iteration).
- **Roadmap doc updates** (land first per the established workflow):
  - DesignDoc §22 "Milestone 3" Goals expand to include multiple-layer rendering + entity Y-sort against tilemap depth.
  - DesignDoc §22 inserts a new "Milestone 3.25 — Tileset Animation" subsection between M3 and M3.5 (placeholder spec; real implementation in its own change).
  - GameDesign §9 inserts an M3.25 row between M3 and M3.5.
  - README status table inserts M3.25 row.

### Non-Goals

- **No JSON scene loading.** The Embercoast map is hardcoded in C++ at M3. JSON loading is M4 — the data shape proposed here (TileLayer / TileSet / Tilemap) is JSON-friendly so M4 is a serializer addition, not a re-design.
- **No animated tiles.** GameDesign §5.1's "amber lantern light" + coastal water need animation, but that's M3.25 — separate per-tile timer/clip concern. Tiles render as static frames at M3.
- **No tile-trigger / interaction zones** (stand-on-tile fires event). Folds into M7 Dialogue & Interaction since both use the same "approach + Confirm" pattern.
- **No editor tile painting.** M5 owns the editor; M3 ships with a hardcoded map. The data shape supports painting later.
- **No auto-tiling / smart corner-tile selection.** Hand-coded map at M3; the user picks tile IDs by hand. Auto-tiling is a v2 polish concern.
- **No multi-tile entities or sub-tile movement granularity issues.** Standard 16×16 tiles, sub-pixel-precise player movement via float Vec2.
- **No physics beyond AABB-vs-solid-tile.** No slopes, no diagonal one-way walls, no breakable tiles. Each is a future change if a milestone needs it.
- **No spatial query primitive** ("find nearest entity within R") — folds into M7.
- **No tilemap-level lighting.** GameDesign §5.1 talks about lantern light atmospherically; actual lighting is post-v1.

## Capabilities

### New Capabilities

- `tilemap`: `engine_scene`'s capability for the multi-layer tilemap data shape (TileLayer / TileSet / Tilemap), visible-tile iteration that lets the renderer stay tilemap-agnostic, and AABB-vs-solid-tile collision queries (plus the per-axis SweepMove helper).

### Modified Capabilities

None. The engine_render / runtime-window / input-actions / audio-system / sprite-animation / texture-assets capabilities are all unchanged — M3 is engine_scene + game-side work that consumes existing primitives.

## Impact

- **Affected modules**: `engine_scene` graduates from placeholder to a working module. `games/my_rpg` grows (hardcoded Embercoast map, Y-sort, collision-aware movement). `engine_render`, `engine_runtime`, `engine_assets`, `engine_input`, `engine_audio`, `engine_math` unchanged.
- **Dependency-rule compliance**: `engine_scene → engine_core + engine_math + engine_assets` (the existing deps — no new edges). Game still depends only on `engine_runtime`. `engine_runtime` does NOT need to grow to include `engine_scene` — game accesses scene types directly through its `engine_runtime` dep (which carries the include path).

  Wait — that's wrong. `game_my_rpg` depends on `engine_runtime` only. To use `Engine::Scene::*`, we either add `engine_scene` to game's DEPENDS list (clean), OR make `engine_runtime` re-export `engine_scene` (less clean). Going with adding `engine_scene` to `game_my_rpg`'s DEPENDS — same shape as already-allowed `game_*` → `engine_*` edges per §6.3 "Allowed: game → engine".
- **New external dependencies**: none.
- **New serialized formats**: none yet — M3's map is C++. M4 owns the JSON shape (with `"version": 1` per CLAUDE.md hard rule #7).
- **New Lua bindings**: none (M6 owns scripting).
- **Performance budget**: 320×180 logical at 16px tiles = 20×11 = 220 visible tiles per layer. Two layers = 440 DrawSprite calls/frame. SDL_Renderer batches these internally; well within the ≤5ms frame budget. AABB-vs-grid collision is O(tiles touched by AABB) = ≤6 tiles per query. Negligible.
- **Game-visible improvement** (per DesignDoc §30 / GameDesign §9 M3): launching `game_my_rpg`, Iden stands in Embercoast — ground tiles below, overhead tiles (eaves/awnings) above. Walking into a wall blocks her. The cliff path is visibly blocked. Tick M3 row in GameDesign §9.
- **Docs**: GameDesign §9 + DesignDoc §22 + README + CLAUDE.md all updated. M3.25 row/section inserted as stub.
- **Tests**: new `tests/scene/` target; covers TileSet source-rect math, Tilemap.AddLayer sort order, ForEachVisibleTile culling (visit count for a known viewport), CollidesAABB on solid + empty tiles, SweepMove along a wall (stops cleanly), SweepMove into corner (slides along one axis).
- **Open questions (validate at apply time):**
  - **Which tileset file** from `TimeFantasy_TILES_6.24.17/TILESETS/`? Embercoast is coastal, weathered, pop. 47 — village-with-sea vibe. The pack's specific TILESETS subfolder will dictate; I'll inspect at apply time and pick the closest match (and document the alternates).
  - **Embercoast map dimensions**: defaulting to ~40×30 tiles (640×480 world units, ~3.3 logical screens wide). Big enough to feel like a village without being a maze; small enough to fit on the painter's "one screen and change" mental model. Tunable.
  - **Iden collision footprint**: defaulting to a 12×8 AABB at the sprite's feet (sprite is 17×31 but we want her to walk past tall things her head clears). The "feet-only" collision is what every classic JRPG uses; the exact dimensions are tunable.
  - **Solid-tile palette**: defaulting to "stone walls and water = solid; grass/sand/path/floor = walkable." Sea tiles solid prevents Iden from walking into the ocean. Tunable per scene later.
  - **Y-sort key**: defaulting to `position.y + spriteHeight` ("feet Y"). Standard JRPG convention. Alternative is sprite center — "feet Y" reads more natural for top-down.
