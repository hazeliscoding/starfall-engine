#pragma once

#include "engine/assets/texture.hpp"
#include "engine/render/animation_clip.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Engine::Render {

// Heterogeneous string-key hashing — mirrors engine_input's StringHash so
// queries don't allocate on platforms with full C++20 P0919 support.
// (libstdc++ < 11 doesn't have the heterogeneous find overload yet; the
// implementation materializes std::string at .find() sites as a
// workaround. See animated_sprite.cpp.)
struct AnimatedSpriteStringHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
    std::size_t operator()(const std::string& s) const noexcept { return std::hash<std::string_view>{}(s); }
    std::size_t operator()(const char* s)        const noexcept { return std::hash<std::string_view>{}(std::string_view{s}); }
};

// Named-clip state machine. Owned by-value in game state (one per
// animated entity). Game ticks Update(dt) each frame; renderer draws
// CurrentFrame().
//
// Semantics:
//   - Play(name) on the currently-playing clip is a no-op (preserves
//     cycle continuity — game calls Play("walk_down") every frame while
//     the player walks left, that shouldn't freeze the cycle at frame 0).
//   - Play(name) on a different clip resets frame index + timer to 0
//     and implicitly Resumes.
//   - Update advances the timer; when it crosses frameDuration the
//     frame index steps. Looping clips wrap mod frame count; non-looping
//     clips clamp at the last frame and enter the paused state.
class AnimatedSprite {
public:
    AnimatedSprite();

    // Register a clip under the given name. Re-adding under an existing
    // name replaces the previous clip.
    void AddClip(std::string name, AnimationClip clip);

    // Select a clip. See class doc for no-op / reset semantics.
    void Play(std::string_view name);

    void Pause()  noexcept { paused_ = true;  }
    void Resume() noexcept { paused_ = false; }

    // Advance the internal timer. No-op while paused.
    void Update(float dt);

    // The texture to draw this frame. Null if no clip is playing or the
    // current clip has zero frames.
    Engine::Assets::TextureHandle CurrentFrame() const;

    const std::string& CurrentClip() const noexcept { return current_; }
    bool               IsPlaying()   const noexcept { return !paused_ && !current_.empty(); }

private:
    template <typename V>
    using ClipMap = std::unordered_map<std::string, V, AnimatedSpriteStringHash, std::equal_to<>>;

    ClipMap<AnimationClip> clips_;
    std::string            current_;
    float                  timer_      = 0.0f;
    std::size_t            frameIndex_ = 0;
    bool                   paused_     = false;
};

}  // namespace Engine::Render
