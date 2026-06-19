## ADDED Requirements

### Requirement: TileLayer Data Shape

`engine_scene` SHALL provide an `Engine::Scene::TileLayer` aggregate with: `std::string name`, `int width`, `int height` (both in tiles), `std::vector<int> tileIds` (row-major, length `width * height`; `0` reserved as "empty"), `bool visible` (default `true`), and `int sortOrder` (lower draws first). Zero-sized layers are legal but render nothing.

#### Scenario: Construct a 2-layer map

- **WHEN** game code constructs a ground layer (`sortOrder = 0`) and an overhead layer (`sortOrder = 100`), both 40×30 tiles
- **THEN** both layers are well-formed; `tileIds.size() == 40 * 30`; querying tile (x,y) yields `tileIds[y * width + x]`

#### Scenario: Empty tile sentinel

- **WHEN** a layer's `tileIds[i]` is `0`
- **THEN** that cell is treated as empty for both rendering (skipped) and collision (treated as not-solid regardless of any `MarkSolid(0)` call)

### Requirement: TileSet Resolves Tile IDs To Source Rects

`Engine::Scene::TileSet` SHALL hold one `Engine::Assets::TextureHandle` plus tile dimensions (`tileWidth`, `tileHeight`) and atlas dimensions (`columns` — tiles per row). It SHALL resolve `tileId → Engine::Math::Rect sourceRect` via row-major division: tile ID `N` (1-indexed; 0 is empty) maps to source rect `(((N-1) % columns) * tileWidth, ((N-1) / columns) * tileHeight, tileWidth, tileHeight)`.

#### Scenario: Source rect for tile 1

- **WHEN** a `TileSet` with `tileWidth=16, tileHeight=16, columns=8` is queried for `SourceRect(1)`
- **THEN** the result is `{x=0, y=0, w=16, h=16}`

#### Scenario: Source rect for tile in second row

- **WHEN** the same `TileSet` is queried for `SourceRect(9)` (first tile of the second row)
- **THEN** the result is `{x=0, y=16, w=16, h=16}`

#### Scenario: Source rect for non-zero non-first row tile

- **WHEN** queried for `SourceRect(11)` on the same TileSet (third tile of second row)
- **THEN** the result is `{x=32, y=16, w=16, h=16}`

### Requirement: TileSet Tracks Solid Tile IDs

`TileSet` SHALL expose `MarkSolid(int tileId)` to flag a tile ID as solid for collision queries, and `IsSolid(int tileId) → bool` to query the flag. `IsSolid(0)` SHALL always return `false` (the empty sentinel can never be solid). `MarkSolid` is idempotent.

#### Scenario: Default is non-solid

- **WHEN** a fresh `TileSet` is queried for `IsSolid(42)` without ever calling `MarkSolid`
- **THEN** the result is `false`

#### Scenario: Mark and query

- **WHEN** `MarkSolid(7)` then `MarkSolid(8)` are called, then `IsSolid` is queried for 7, 8, and 99
- **THEN** results are `true, true, false`

#### Scenario: Empty tile id is never solid

- **WHEN** `MarkSolid(0)` is called and then `IsSolid(0)` is queried
- **THEN** the result is `false` (the call to `MarkSolid(0)` is a silent no-op)

### Requirement: Tilemap Owns Layers + TileSet

`Engine::Scene::Tilemap` SHALL hold a `std::shared_ptr<TileSet>` and a sorted list of `TileLayer`s. Tile and world dimensions are exposed via accessors. `AddLayer(TileLayer)` SHALL insert the layer into the internal list maintaining ascending `sortOrder`; layers with equal `sortOrder` retain insertion order (stable sort). Layer iteration via `Layers()` returns layers in sort order.

#### Scenario: Layers sort on add

- **WHEN** game code adds layers in order `{overhead, sortOrder=100}`, `{ground, sortOrder=0}`
- **THEN** `Layers()` returns them in `{ground, overhead}` order

#### Scenario: World dimensions reflect largest layer

- **WHEN** a Tilemap contains one 40×30 layer and one 50×25 layer with `tileWidth=tileHeight=16`
- **THEN** `WorldWidth() == 50 * 16` (800) and `WorldHeight() == 30 * 16` (480)

#### Scenario: TileSet accessor

- **WHEN** game code calls `tilemap.TileSet()` after construction with a valid `std::shared_ptr<TileSet>`
- **THEN** the returned shared_ptr is the same one supplied

### Requirement: Visible Tile Iteration

`Tilemap::ForEachVisibleTile(viewport, visit)` SHALL invoke `visit(const TileLayer&, Math::Vec2 worldPos, int tileId)` for every non-empty tile across all visible layers (skipping layers with `visible == false`) whose world rect overlaps the supplied viewport `Math::Rect`. Layers SHALL be visited in `sortOrder` ascending. Tiles within a layer SHALL be visited in row-major order. The visitor is responsible for whatever rendering / processing happens — `Tilemap` does NOT call into `engine_render`.

#### Scenario: Viewport that covers the entire map

- **WHEN** a 4×3-tile single-layer map (all 12 tiles non-empty, tileSize 16) is queried with a viewport `{0, 0, 64, 48}`
- **THEN** `visit` is called exactly 12 times

#### Scenario: Viewport clipped to a subregion

- **WHEN** the same 4×3 map is queried with viewport `{16, 0, 32, 16}` (covering tiles at columns 1-2, row 0)
- **THEN** `visit` is called exactly 2 times — for the tiles at (1,0) and (2,0); each visit receives the tile's world position (16,0) and (32,0)

#### Scenario: Empty tiles are skipped

- **WHEN** a layer's `tileIds` contains zeros and the viewport covers them
- **THEN** `visit` is NOT called for those tiles

#### Scenario: Hidden layers are skipped

- **WHEN** a layer has `visible = false`
- **THEN** none of its tiles are visited even if the viewport covers them

#### Scenario: Layer iteration order

- **WHEN** game code captures the visit order across a ground layer (sortOrder=0) and an overhead layer (sortOrder=100), with overlapping tile positions
- **THEN** every ground-layer tile is visited before any overhead-layer tile

### Requirement: AABB Collision Against Solid Tiles

`Tilemap::CollidesAABB(const Math::Rect& worldRect) → bool` SHALL return `true` if any solid tile (per the configured `TileSet::IsSolid`) on ANY visible layer overlaps `worldRect`. Hidden layers SHALL be ignored. The query MUST be O(tiles touched by worldRect) — bounded constant for entity-sized AABBs.

#### Scenario: AABB clear of all solid tiles

- **WHEN** an entity AABB sits entirely on non-solid tiles
- **THEN** `CollidesAABB` returns `false`

#### Scenario: AABB overlaps one solid tile

- **WHEN** an entity AABB overlaps even one tile flagged solid via `TileSet::MarkSolid`
- **THEN** `CollidesAABB` returns `true`

#### Scenario: AABB straddling solid + empty

- **WHEN** an entity AABB partially overlaps a solid tile AND a non-solid tile
- **THEN** `CollidesAABB` returns `true` (any-overlap-counts)

#### Scenario: Solid tile on hidden layer ignored

- **WHEN** a tile flagged solid lives on a layer with `visible = false`
- **THEN** `CollidesAABB` returns `false` for an AABB overlapping that tile

### Requirement: SweepMove Per-Axis Slide

`Engine::Scene::SweepMove(currentPos, displacement, aabbSize, tilemap) → Math::Vec2` SHALL return the displacement actually applied (which MAY be less than the requested `displacement`), computed by attempting the X-axis move and Y-axis move independently and using `Tilemap::CollidesAABB` to reject the move on each axis if it would penetrate a solid tile. This produces the standard "slide along walls" feel — entity moving NE into a wall on the N side still moves E.

#### Scenario: No obstacles

- **WHEN** SweepMove is called with a displacement entirely in open space
- **THEN** the returned displacement equals the input

#### Scenario: Wall blocks one axis, other axis free

- **WHEN** an entity at (10, 10) with size 12×8 requests displacement (5, 5), and a solid wall sits to the right blocking the X move but not the Y move
- **THEN** the returned displacement is (0, 5) — Y applied, X rejected

#### Scenario: Corner pin

- **WHEN** an entity is pinned in a corner and requests displacement (5, 5) where both axes hit solid tiles
- **THEN** the returned displacement is (0, 0) — entity does not move

#### Scenario: Slide along a wall

- **WHEN** an entity is hugging a wall on its north and requests displacement (5, -3) (E and N)
- **THEN** X applies (entity moves east along the wall), Y is rejected (would penetrate the wall) — returned displacement is (5, 0)
