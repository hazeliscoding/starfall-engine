// tests/scene/tilemap_tests.cpp
//
// Covers the tilemap capability per the spec: TileSet source-rect math,
// MarkSolid/IsSolid semantics, Tilemap stable-sort layers, world
// dimensions, ForEachVisibleTile culling + visit ordering, CollidesAABB
// against solid + hidden layers, and SweepMove per-axis behavior.
//
// Most tests don't need real TextureHandles — a default-constructed
// (null) TextureHandle is fine for everything except texture-bound
// integration tests (none here).

#include "engine/assets/texture.hpp"
#include "engine/math/rect.hpp"
#include "engine/scene/sweep.hpp"
#include "engine/scene/tile_layer.hpp"
#include "engine/scene/tile_set.hpp"
#include "engine/scene/tilemap.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

using Engine::Assets::TextureHandle;
using Engine::Math::Rect;
using Engine::Math::Vec2;
using Engine::Scene::SweepMove;
using Engine::Scene::TileLayer;
using Engine::Scene::TileSet;
using Engine::Scene::Tilemap;

namespace {

// Helper: build a TileLayer from a flat list of IDs.
TileLayer MakeLayer(std::string name, int width, int height,
                    std::vector<int> ids, int sortOrder = 0) {
    TileLayer l;
    l.name      = std::move(name);
    l.width     = width;
    l.height    = height;
    l.tileIds   = std::move(ids);
    l.sortOrder = sortOrder;
    return l;
}

}  // namespace

// ---------- TileSet: source rect math ----------

TEST_CASE("TileSet: SourceRect for tile 1 returns origin cell",
          "[scene][tileset]") {
    TileSet ts{TextureHandle{}, /*tileW*/ 16, /*tileH*/ 16, /*columns*/ 8};
    const auto r = ts.SourceRect(1);
    CHECK(r.x == 0.0f); CHECK(r.y == 0.0f);
    CHECK(r.w == 16.0f); CHECK(r.h == 16.0f);
}

TEST_CASE("TileSet: SourceRect for first cell of second row",
          "[scene][tileset]") {
    TileSet ts{TextureHandle{}, 16, 16, 8};
    const auto r = ts.SourceRect(9);  // (9-1) / 8 = row 1, col 0
    CHECK(r.x == 0.0f); CHECK(r.y == 16.0f);
}

TEST_CASE("TileSet: SourceRect for third cell of second row",
          "[scene][tileset]") {
    TileSet ts{TextureHandle{}, 16, 16, 8};
    const auto r = ts.SourceRect(11);  // (11-1) / 8 = row 1, col 2
    CHECK(r.x == 32.0f); CHECK(r.y == 16.0f);
}

// ---------- TileSet: solid flag ----------

TEST_CASE("TileSet: default IsSolid returns false",
          "[scene][tileset][solid]") {
    TileSet ts{TextureHandle{}, 16, 16, 1};
    CHECK_FALSE(ts.IsSolid(1));
    CHECK_FALSE(ts.IsSolid(42));
}

TEST_CASE("TileSet: MarkSolid then IsSolid round-trip",
          "[scene][tileset][solid]") {
    TileSet ts{TextureHandle{}, 16, 16, 1};
    ts.MarkSolid(7);
    ts.MarkSolid(8);
    CHECK(ts.IsSolid(7));
    CHECK(ts.IsSolid(8));
    CHECK_FALSE(ts.IsSolid(99));
}

TEST_CASE("TileSet: tile 0 is never solid even after MarkSolid(0)",
          "[scene][tileset][solid]") {
    TileSet ts{TextureHandle{}, 16, 16, 1};
    ts.MarkSolid(0);
    CHECK_FALSE(ts.IsSolid(0));
}

// ---------- Tilemap: layer sort + world dimensions ----------

TEST_CASE("Tilemap: AddLayer stable-sorts by sortOrder ascending",
          "[scene][tilemap][layers]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("overhead", 1, 1, {1}, /*sortOrder*/ 100));
    tm.AddLayer(MakeLayer("ground",   1, 1, {1}, /*sortOrder*/ 0));
    auto layers = tm.Layers();
    REQUIRE(layers.size() == 2);
    CHECK(layers[0].name == "ground");
    CHECK(layers[1].name == "overhead");
}

TEST_CASE("Tilemap: WorldWidth/Height reflect largest layer",
          "[scene][tilemap][dimensions]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("small", 40, 30, std::vector<int>(40 * 30, 0)));
    tm.AddLayer(MakeLayer("big",   50, 25, std::vector<int>(50 * 25, 0)));
    CHECK(tm.WorldWidth()  == 50 * 16);
    CHECK(tm.WorldHeight() == 30 * 16);
}

// ---------- Tilemap: ForEachVisibleTile ----------

TEST_CASE("Tilemap: ForEachVisibleTile covers all non-empty tiles for full viewport",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("g", 4, 3,
        {1, 1, 1, 1,
         1, 1, 1, 1,
         1, 1, 1, 1}));
    int count = 0;
    tm.ForEachVisibleTile(Rect{0, 0, 64, 48}, [&](auto, auto, int) { ++count; });
    CHECK(count == 12);
}

TEST_CASE("Tilemap: ForEachVisibleTile culls to clipped viewport",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("g", 4, 3,
        {1, 2, 3, 4,
         5, 6, 7, 8,
         9, 10, 11, 12}));
    std::vector<Vec2> positions;
    tm.ForEachVisibleTile(Rect{16, 0, 32, 16},
        [&](auto, Vec2 pos, int) { positions.push_back(pos); });
    REQUIRE(positions.size() == 2);
    CHECK(positions[0].x == 16.0f); CHECK(positions[0].y == 0.0f);
    CHECK(positions[1].x == 32.0f); CHECK(positions[1].y == 0.0f);
}

TEST_CASE("Tilemap: ForEachVisibleTile skips empty (id=0) tiles",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("g", 2, 2, {0, 1, 0, 1}));
    int count = 0;
    tm.ForEachVisibleTile(Rect{0, 0, 32, 32}, [&](auto, auto, int) { ++count; });
    CHECK(count == 2);
}

TEST_CASE("Tilemap: ForEachVisibleTile skips hidden layers",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    auto layer = MakeLayer("hidden", 2, 2, {1, 1, 1, 1});
    layer.visible = false;
    tm.AddLayer(std::move(layer));
    int count = 0;
    tm.ForEachVisibleTile(Rect{0, 0, 32, 32}, [&](auto, auto, int) { ++count; });
    CHECK(count == 0);
}

TEST_CASE("Tilemap: ForEachVisibleTile visits ground before overhead",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("ground",   1, 1, {1}, /*sortOrder*/ 0));
    tm.AddLayer(MakeLayer("overhead", 1, 1, {2}, /*sortOrder*/ 100));
    std::vector<int> visitOrder;
    tm.ForEachVisibleTile(Rect{0, 0, 16, 16},
        [&](auto, auto, int id) { visitOrder.push_back(id); });
    REQUIRE(visitOrder.size() == 2);
    CHECK(visitOrder[0] == 1);
    CHECK(visitOrder[1] == 2);
}

TEST_CASE("Tilemap: ForEachVisibleTile sortOrder window filters",
          "[scene][tilemap][visible]") {
    Tilemap tm{16, 16};
    tm.AddLayer(MakeLayer("ground",   1, 1, {1}, /*sortOrder*/ 0));
    tm.AddLayer(MakeLayer("overhead", 1, 1, {2}, /*sortOrder*/ 100));
    // Only ground (sortOrder < 50).
    std::vector<int> ids;
    tm.ForEachVisibleTile(Rect{0, 0, 16, 16}, -10'000, 49,
        [&](auto, auto, int id) { ids.push_back(id); });
    REQUIRE(ids.size() == 1);
    CHECK(ids[0] == 1);
    // Only overhead (sortOrder >= 50).
    ids.clear();
    tm.ForEachVisibleTile(Rect{0, 0, 16, 16}, 50, 10'000,
        [&](auto, auto, int id) { ids.push_back(id); });
    REQUIRE(ids.size() == 1);
    CHECK(ids[0] == 2);
}

// ---------- Tilemap: CollidesAABB ----------

TEST_CASE("Tilemap: CollidesAABB false in open space",
          "[scene][tilemap][collision]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    tm.AddLayer(MakeLayer("g", 3, 3,
        {1, 1, 1,
         1, 1, 1,
         1, 1, 1}));
    CHECK_FALSE(tm.CollidesAABB(Rect{8, 8, 4, 4}));
}

TEST_CASE("Tilemap: CollidesAABB true on solid tile",
          "[scene][tilemap][collision]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    tm.AddLayer(MakeLayer("g", 3, 3,
        {1, 1, 1,
         1, 3, 1,
         1, 1, 1}));
    // Center tile (1,1) is solid (tileId=3); AABB at (20,20) sits on it.
    CHECK(tm.CollidesAABB(Rect{20, 20, 4, 4}));
}

TEST_CASE("Tilemap: CollidesAABB true when straddling solid + empty",
          "[scene][tilemap][collision]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    tm.AddLayer(MakeLayer("g", 2, 1, {1, 3}));  // walkable then solid
    // AABB straddles the boundary at x=16.
    CHECK(tm.CollidesAABB(Rect{14, 4, 4, 4}));
}

TEST_CASE("Tilemap: CollidesAABB ignores solid tiles on hidden layers",
          "[scene][tilemap][collision]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    auto layer = MakeLayer("hidden_solid", 1, 1, {3});
    layer.visible = false;
    tm.AddLayer(std::move(layer));
    CHECK_FALSE(tm.CollidesAABB(Rect{4, 4, 4, 4}));
}

// ---------- SweepMove ----------

TEST_CASE("SweepMove: no obstacle returns full displacement",
          "[scene][sweep]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    tm.AddLayer(MakeLayer("g", 4, 4, std::vector<int>(16, 1)));  // all walkable
    const auto applied = SweepMove(Vec2{8, 8}, Vec2{4, 3}, Vec2{4, 4}, tm);
    CHECK(applied.x == 4.0f); CHECK(applied.y == 3.0f);
}

TEST_CASE("SweepMove: wall on X axis only, Y still applies",
          "[scene][sweep]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    // 3 cols x 3 rows. Solid column on the right (col 2).
    tm.AddLayer(MakeLayer("g", 3, 3,
        {1, 1, 3,
         1, 1, 3,
         1, 1, 3}));
    // Entity is just left of the wall — moving 10 right would push into it,
    // moving down stays in walkable space.
    const auto applied = SweepMove(Vec2{20, 8}, Vec2{10, 5}, Vec2{4, 4}, tm);
    CHECK(applied.x == 0.0f);    // X blocked
    CHECK(applied.y == 5.0f);    // Y applied
}

TEST_CASE("SweepMove: corner pin returns zero displacement",
          "[scene][sweep]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    // Solid wall on the east (col 2) AND south (row 2).
    tm.AddLayer(MakeLayer("g", 3, 3,
        {1, 1, 3,
         1, 1, 3,
         1, 3, 3}));
    // Entity AABB at (24, 24) size 4 sits inside cell (1, 1). Move SE
    // by (8, 8) puts X into col 2 (solid east wall) AND Y into row 2
    // (solid south wall) — both axes must reject.
    const auto applied = SweepMove(Vec2{24, 24}, Vec2{8, 8}, Vec2{4, 4}, tm);
    CHECK(applied.x == 0.0f);
    CHECK(applied.y == 0.0f);
}

TEST_CASE("SweepMove: slide along a wall - X applies Y rejected",
          "[scene][sweep]") {
    auto ts = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    ts->MarkSolid(3);
    Tilemap tm{16, 16};
    tm.SetTileSet(ts);
    // Wall on the north (row 0).
    tm.AddLayer(MakeLayer("g", 3, 3,
        {3, 3, 3,
         1, 1, 1,
         1, 1, 1}));
    // Entity at (4, 18) hugging the wall on its north. Move NE: (5, -3).
    // X should apply (still walkable below the wall row). Y should not
    // (would push the AABB up into the wall row).
    const auto applied = SweepMove(Vec2{4, 18}, Vec2{5, -3}, Vec2{4, 4}, tm);
    CHECK(applied.x == 5.0f);
    CHECK(applied.y == 0.0f);
}

// ---------- Per-layer TileSet ----------

TEST_CASE("Tilemap: ResolveTileSet prefers per-layer over shared",
          "[scene][tilemap][per-layer-tileset]") {
    auto shared    = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    auto perLayer  = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 4);
    Tilemap tm{16, 16};
    tm.SetTileSet(shared);

    auto layerA = MakeLayer("uses_shared", 1, 1, {1});
    auto layerB = MakeLayer("uses_own",    1, 1, {1}, /*sortOrder*/ 10);
    layerB.tileset = perLayer;
    tm.AddLayer(std::move(layerA));
    tm.AddLayer(std::move(layerB));

    const auto& a = tm.Layers()[0];
    const auto& b = tm.Layers()[1];
    CHECK(tm.ResolveTileSet(a).get() == shared.get());
    CHECK(tm.ResolveTileSet(b).get() == perLayer.get());
}

TEST_CASE("Tilemap: CollidesAABB uses per-layer solidity flags",
          "[scene][tilemap][collision][per-layer-tileset]") {
    // Two tilesets with different solid IDs: shared marks tileId=3,
    // per-layer marks tileId=7. The same numeric tile (3) should NOT
    // be solid on the per-layer-backed layer.
    auto shared   = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    shared->MarkSolid(3);
    auto perLayer = std::make_shared<TileSet>(TextureHandle{}, 16, 16, 1);
    perLayer->MarkSolid(7);

    Tilemap tm{16, 16};
    tm.SetTileSet(shared);

    auto floor = MakeLayer("floor_shared", 2, 1, {3, 1});  // (0,0)=solid via shared
    auto over  = MakeLayer("decals_own",   2, 1, {3, 7}, /*sortOrder*/ 50);
    over.tileset = perLayer;  // (0,0)=tile 3 NOT solid here, (1,0)=tile 7 IS solid
    tm.AddLayer(std::move(floor));
    tm.AddLayer(std::move(over));

    CHECK(tm.CollidesAABB(Rect{2, 2, 4, 4}));    // floor tile 3 at col 0
    CHECK(tm.CollidesAABB(Rect{18, 2, 4, 4}));   // over tile 7 at col 1
    // A layer-only world where the per-layer tileset doesn't mark 3 as
    // solid: drop the floor + walk over col 0 of the decals layer.
    Tilemap tm2{16, 16};
    auto decalsOnly = MakeLayer("decals_only", 2, 1, {3, 7});
    decalsOnly.tileset = perLayer;
    tm2.AddLayer(std::move(decalsOnly));
    CHECK_FALSE(tm2.CollidesAABB(Rect{2, 2, 4, 4}));  // tile 3 not solid in perLayer
    CHECK(tm2.CollidesAABB(Rect{18, 2, 4, 4}));        // tile 7 is solid
}
