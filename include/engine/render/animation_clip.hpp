#pragma once

#include "engine/assets/texture.hpp"

#include <vector>

namespace Engine::Render {

// Plain aggregate. A clip is N frames played at frameDuration seconds
// each, looping or one-shot. Zero-frame clips are legal but produce a
// null CurrentFrame() on an AnimatedSprite playing them.
struct AnimationClip {
    std::vector<Engine::Assets::TextureHandle> frames;
    float frameDuration = 0.125f;  // 8 fps default (design D4)
    bool  looping       = true;
};

}  // namespace Engine::Render
