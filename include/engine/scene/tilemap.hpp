#pragma once

#include "engine/math/rect.hpp"
#include "engine/math/vec2.hpp"
#include "engine/scene/tile_layer.hpp"
#include "engine/scene/tile_set.hpp"

#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace Engine::Scene {

// Multi-layer 2D tilemap. Owns N layers (each a flat array of tile IDs)
// and a shared TileSet that resolves IDs to atlas source rects + solid-
// or-not flags.
//
// Per design D1: this class does NOT depend on engine_render. Rendering
// is driven by the caller via ForEachVisibleTile + a visitor lambda;
// the game's onRender turns each visited tile into a Renderer.DrawSprite
// call.
class Tilemap {
public:
    Tilemap(int tileWidth, int tileHeight) noexcept;

    void SetTileSet(std::shared_ptr<class TileSet> tileset) noexcept;
    const std::shared_ptr<class TileSet>& TileSet() const noexcept { return tileset_; }

    // Stable-insert a layer in ascending sortOrder. Later AddLayer calls
    // with the same sortOrder retain insertion order (stable).
    void AddLayer(TileLayer layer);

    std::span<const TileLayer> Layers() const noexcept {
        return std::span<const TileLayer>(layers_.data(), layers_.size());
    }

    int TileWidth()  const noexcept { return tileWidth_;  }
    int TileHeight() const noexcept { return tileHeight_; }

    // World dimensions = (max-layer-width × tileWidth, max-layer-height
    // × tileHeight). For camera bounds + viewport clamp.
    int WorldWidth()  const noexcept;
    int WorldHeight() const noexcept;

    // Iterate every non-empty tile of every visible layer whose world
    // rect overlaps `viewport`. Visitor receives the layer reference,
    // the tile's world position, and the tile ID.
    //
    // Layers visited in sortOrder ascending. Tiles within a layer
    // visited in row-major order.
    using Visitor = std::function<void(const TileLayer&,
                                       Engine::Math::Vec2 worldPos,
                                       int tileId)>;
    void ForEachVisibleTile(const Engine::Math::Rect& viewport,
                            const Visitor& visit) const;

    // Overload: restrict to layers whose sortOrder is in
    // [minSortOrder, maxSortOrder] (inclusive). Lets the game render
    // ground-layer tiles, then Y-sort entities, then overhead-layer
    // tiles, in one ForEachVisibleTile call per pass.
    void ForEachVisibleTile(const Engine::Math::Rect& viewport,
                            int minSortOrder, int maxSortOrder,
                            const Visitor& visit) const;

    // True if any solid tile on any visible layer overlaps `worldRect`.
    // Hidden layers are ignored. Bounded to O(tiles overlapping
    // worldRect) per layer — constant for entity-sized AABBs.
    bool CollidesAABB(const Engine::Math::Rect& worldRect) const;

private:
    int                            tileWidth_;
    int                            tileHeight_;
    std::vector<TileLayer>         layers_;     // kept sorted ascending by sortOrder
    std::shared_ptr<class TileSet> tileset_;
};

}  // namespace Engine::Scene
