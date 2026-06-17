#pragma once

namespace Engine::Math {

struct IVec2 {
    int x = 0;
    int y = 0;

    constexpr IVec2 operator+(IVec2 o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr IVec2 operator-(IVec2 o) const noexcept { return {x - o.x, y - o.y}; }
};

}  // namespace Engine::Math
