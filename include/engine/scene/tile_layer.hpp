#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Engine::Scene {

class TileSet;

// One layer of a Tilemap. Tile IDs are 1-indexed; 0 means "empty" and
// is treated as not-rendered + not-solid regardless of any TileSet
// MarkSolid configuration.
//
// Convention: sortOrder = 0 is ground, 100 is overhead. Mid-layer
// entities (player + NPCs) draw between layers in a game-side Y-sort
// (see games/my_rpg/src/main.cpp).
//
// `tileset` is optional: when null, Tilemap::ResolveTileSet falls back
// to the shared TileSet on the parent Tilemap. Setting it lets a single
// Tilemap mix layers backed by different atlases (e.g. terrain.png for
// the ground, water.png for the sea band, house.png for the building).
struct TileLayer {
    std::string              name;
    int                      width  = 0;
    int                      height = 0;
    std::vector<int>         tileIds;       // row-major, length = width * height
    bool                     visible   = true;
    int                      sortOrder = 0;
    std::shared_ptr<TileSet> tileset;       // optional; null = use Tilemap's shared
};

}  // namespace Engine::Scene
