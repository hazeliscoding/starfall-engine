#pragma once

#include "engine/assets/texture.hpp"
#include "engine/math/rect.hpp"

#include <unordered_set>

namespace Engine::Scene {

// Tiles are arranged on a single texture in a row-major grid:
//   tileId 1 = top-left cell, tileId 2 = next column, etc.
//   tileId 0 is reserved as the "empty" sentinel.
//
// Solid tile IDs are configured via MarkSolid; Tilemap::CollidesAABB
// uses IsSolid to detect blocked movement.
class TileSet {
public:
    TileSet(Engine::Assets::TextureHandle texture,
            int tileWidth, int tileHeight, int columns) noexcept;

    const Engine::Assets::TextureHandle& Texture() const noexcept { return texture_; }
    int TileWidth()  const noexcept { return tileWidth_;  }
    int TileHeight() const noexcept { return tileHeight_; }
    int Columns()    const noexcept { return columns_;    }

    // Source rect for the given tile ID. Caller is responsible for not
    // passing 0 (empty) — the result for 0 is well-defined (returns the
    // last-row past-the-end position) but semantically meaningless.
    Engine::Math::Rect SourceRect(int tileId) const noexcept;

    // Flag a tile ID as solid for collision queries. Calling with 0 is
    // a silent no-op (0 is the empty sentinel; can never be solid).
    void MarkSolid(int tileId);

    // True if MarkSolid(tileId) was called. IsSolid(0) is always false.
    bool IsSolid(int tileId) const noexcept;

private:
    Engine::Assets::TextureHandle texture_;
    int                            tileWidth_;
    int                            tileHeight_;
    int                            columns_;
    std::unordered_set<int>        solidTileIds_;
};

}  // namespace Engine::Scene
