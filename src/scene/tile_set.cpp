#include "engine/scene/tile_set.hpp"

#include <utility>

namespace Engine::Scene {

TileSet::TileSet(Engine::Assets::TextureHandle texture,
                 int tileWidth, int tileHeight, int columns) noexcept
    : texture_(std::move(texture)),
      tileWidth_(tileWidth),
      tileHeight_(tileHeight),
      columns_(columns) {}

Engine::Math::Rect TileSet::SourceRect(int tileId) const noexcept {
    // tileId is 1-indexed; subtract 1 to get the atlas index.
    const int idx = tileId - 1;
    const int col = (columns_ > 0) ? (idx % columns_) : 0;
    const int row = (columns_ > 0) ? (idx / columns_) : 0;
    return Engine::Math::Rect{
        static_cast<float>(col * tileWidth_),
        static_cast<float>(row * tileHeight_),
        static_cast<float>(tileWidth_),
        static_cast<float>(tileHeight_),
    };
}

void TileSet::MarkSolid(int tileId) {
    if (tileId == 0) return;  // empty sentinel can never be solid
    solidTileIds_.insert(tileId);
}

bool TileSet::IsSolid(int tileId) const noexcept {
    if (tileId == 0) return false;
    return solidTileIds_.find(tileId) != solidTileIds_.end();
}

}  // namespace Engine::Scene
