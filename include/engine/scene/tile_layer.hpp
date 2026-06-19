#pragma once

#include <string>
#include <vector>

namespace Engine::Scene {

// One layer of a Tilemap. Tile IDs are 1-indexed; 0 means "empty" and
// is treated as not-rendered + not-solid regardless of any TileSet
// MarkSolid configuration.
//
// Convention: sortOrder = 0 is ground, 100 is overhead. Mid-layer
// entities (player + NPCs) draw between layers in a game-side Y-sort
// (see games/my_rpg/src/main.cpp).
struct TileLayer {
    std::string      name;
    int              width  = 0;
    int              height = 0;
    std::vector<int> tileIds;       // row-major, length = width * height
    bool             visible   = true;
    int              sortOrder = 0;
};

}  // namespace Engine::Scene
