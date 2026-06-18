#include "engine/render/animated_sprite.hpp"

#include "engine/core/logger.hpp"

#include <utility>

namespace Engine::Render {

AnimatedSprite::AnimatedSprite() = default;

void AnimatedSprite::AddClip(std::string name, AnimationClip clip) {
    clips_[std::move(name)] = std::move(clip);
}

void AnimatedSprite::Play(std::string_view name) {
    // libstdc++ 10 lacks heterogeneous unordered_map::find — materialize
    // std::string at the lookup site. Same compatibility workaround as
    // engine_input. Tiny per-call alloc.
    std::string key{name};

    if (clips_.find(key) == clips_.end()) {
        static bool warned_once = false;
        if (!warned_once) {
            SF_LOG_WARN("Render",
                "AnimatedSprite::Play(\"%s\") — no clip registered under that name "
                "(further occurrences suppressed)",
                key.c_str());
            warned_once = true;
        }
        current_.clear();
        return;
    }

    // No-op when re-playing the current clip (design D3). This preserves
    // walk-cycle continuity when the game calls Play("walk_down") every
    // frame the player walks left.
    if (key == current_ && !paused_) {
        return;
    }

    current_    = std::move(key);
    timer_      = 0.0f;
    frameIndex_ = 0;
    paused_     = false;
}

void AnimatedSprite::Update(float dt) {
    if (paused_ || current_.empty()) return;

    auto it = clips_.find(current_);  // current_ is a std::string, no heterogeneous find needed
    if (it == clips_.end()) return;   // shouldn't happen — Play guards
    const AnimationClip& clip = it->second;
    if (clip.frames.empty() || clip.frameDuration <= 0.0f) return;

    timer_ += dt;
    while (timer_ >= clip.frameDuration) {
        timer_ -= clip.frameDuration;
        const std::size_t nextIndex = frameIndex_ + 1;
        if (nextIndex >= clip.frames.size()) {
            if (clip.looping) {
                frameIndex_ = nextIndex % clip.frames.size();
            } else {
                frameIndex_ = clip.frames.size() - 1;
                paused_ = true;
                break;
            }
        } else {
            frameIndex_ = nextIndex;
        }
    }
}

Engine::Assets::TextureHandle AnimatedSprite::CurrentFrame() const {
    if (current_.empty()) return {};
    auto it = clips_.find(current_);
    if (it == clips_.end()) return {};
    const auto& frames = it->second.frames;
    if (frames.empty()) return {};
    return frames[frameIndex_];
}

}  // namespace Engine::Render
