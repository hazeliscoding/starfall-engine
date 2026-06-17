#pragma once

#include "engine/math/vec2.hpp"

namespace Engine::Render {

struct Camera2D {
    Engine::Math::Vec2 position{};
    float              zoom = 1.0f;
};

}  // namespace Engine::Render
