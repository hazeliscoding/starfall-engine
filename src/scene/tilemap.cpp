#include "engine/scene/tilemap.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace Engine::Scene {

Tilemap::Tilemap(int tileWidth, int tileHeight) noexcept
    : tileWidth_(tileWidth), tileHeight_(tileHeight) {}

void Tilemap::SetTileSet(std::shared_ptr<class TileSet> tileset) noexcept {
    tileset_ = std::move(tileset);
}

const std::shared_ptr<class TileSet>&
Tilemap::ResolveTileSet(const TileLayer& layer) const noexcept {
    return layer.tileset ? layer.tileset : tileset_;
}

void Tilemap::AddLayer(TileLayer layer) {
    // Stable insert: walk to the first existing layer with strictly
    // greater sortOrder; insert before it.
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [&](const TileLayer& existing) {
            return existing.sortOrder > layer.sortOrder;
        });
    layers_.insert(it, std::move(layer));
}

int Tilemap::WorldWidth() const noexcept {
    int maxCols = 0;
    for (const auto& layer : layers_) {
        if (layer.width > maxCols) maxCols = layer.width;
    }
    return maxCols * tileWidth_;
}

int Tilemap::WorldHeight() const noexcept {
    int maxRows = 0;
    for (const auto& layer : layers_) {
        if (layer.height > maxRows) maxRows = layer.height;
    }
    return maxRows * tileHeight_;
}

namespace {

// Clamp [start, end) tile-index range to a non-negative window inside
// [0, dim). Returns true if the resulting range is non-empty.
bool ClampTileRange(int viewportStartPx, int viewportEndPx,
                    int tileSizePx, int dimTiles,
                    int& outStart, int& outEndExclusive) noexcept {
    if (tileSizePx <= 0 || dimTiles <= 0) {
        outStart = 0; outEndExclusive = 0; return false;
    }
    int start = viewportStartPx / tileSizePx;
    int endExclusive = (viewportEndPx + tileSizePx - 1) / tileSizePx;
    if (start < 0) start = 0;
    if (endExclusive > dimTiles) endExclusive = dimTiles;
    outStart = start;
    outEndExclusive = endExclusive;
    return endExclusive > start;
}

}  // namespace

void Tilemap::ForEachVisibleTile(const Engine::Math::Rect& viewport,
                                 const Visitor& visit) const {
    // Pass through to the overload with widest possible sort-order
    // window — every layer qualifies.
    ForEachVisibleTile(viewport,
                       std::numeric_limits<int>::min(),
                       std::numeric_limits<int>::max(),
                       visit);
}

void Tilemap::ForEachVisibleTile(const Engine::Math::Rect& viewport,
                                 int minSortOrder, int maxSortOrder,
                                 const Visitor& visit) const {
    const int viewLeftPx   = static_cast<int>(viewport.x);
    const int viewTopPx    = static_cast<int>(viewport.y);
    const int viewRightPx  = static_cast<int>(viewport.x + viewport.w);
    const int viewBottomPx = static_cast<int>(viewport.y + viewport.h);

    for (const auto& layer : layers_) {
        if (!layer.visible) continue;
        if (layer.sortOrder < minSortOrder || layer.sortOrder > maxSortOrder) continue;

        int colStart, colEnd, rowStart, rowEnd;
        if (!ClampTileRange(viewLeftPx, viewRightPx,  tileWidth_,  layer.width,  colStart, colEnd)) continue;
        if (!ClampTileRange(viewTopPx,  viewBottomPx, tileHeight_, layer.height, rowStart, rowEnd)) continue;

        for (int row = rowStart; row < rowEnd; ++row) {
            for (int col = colStart; col < colEnd; ++col) {
                const int tileId = layer.tileIds[row * layer.width + col];
                if (tileId == 0) continue;
                const Engine::Math::Vec2 worldPos{
                    static_cast<float>(col * tileWidth_),
                    static_cast<float>(row * tileHeight_),
                };
                visit(layer, worldPos, tileId);
            }
        }
    }
}

bool Tilemap::CollidesAABB(const Engine::Math::Rect& worldRect) const {
    for (const auto& layer : layers_) {
        if (!layer.visible) continue;
        const auto& ts = ResolveTileSet(layer);
        if (!ts) continue;

        int colStart, colEnd, rowStart, rowEnd;
        const int leftPx   = static_cast<int>(worldRect.x);
        const int topPx    = static_cast<int>(worldRect.y);
        const int rightPx  = static_cast<int>(worldRect.x + worldRect.w);
        const int bottomPx = static_cast<int>(worldRect.y + worldRect.h);
        if (!ClampTileRange(leftPx, rightPx,  tileWidth_,  layer.width,  colStart, colEnd)) continue;
        if (!ClampTileRange(topPx,  bottomPx, tileHeight_, layer.height, rowStart, rowEnd)) continue;

        for (int row = rowStart; row < rowEnd; ++row) {
            for (int col = colStart; col < colEnd; ++col) {
                const int tileId = layer.tileIds[row * layer.width + col];
                if (ts->IsSolid(tileId)) return true;
            }
        }
    }
    return false;
}

}  // namespace Engine::Scene
