#include "engine/scene/sweep.hpp"

#include "engine/math/rect.hpp"
#include "engine/scene/tilemap.hpp"

namespace Engine::Scene {

Engine::Math::Vec2 SweepMove(Engine::Math::Vec2 currentPos,
                             Engine::Math::Vec2 displacement,
                             Engine::Math::Vec2 aabbSize,
                             const Tilemap&     tilemap) {
    Engine::Math::Vec2 applied{0.0f, 0.0f};

    // ---- X axis ----
    if (displacement.x != 0.0f) {
        const Engine::Math::Rect testX{
            currentPos.x + displacement.x,
            currentPos.y,
            aabbSize.x,
            aabbSize.y,
        };
        if (!tilemap.CollidesAABB(testX)) {
            applied.x = displacement.x;
        }
    }

    // ---- Y axis (against the X-adjusted position) ----
    if (displacement.y != 0.0f) {
        const Engine::Math::Rect testY{
            currentPos.x + applied.x,
            currentPos.y + displacement.y,
            aabbSize.x,
            aabbSize.y,
        };
        if (!tilemap.CollidesAABB(testY)) {
            applied.y = displacement.y;
        }
    }

    return applied;
}

}  // namespace Engine::Scene
