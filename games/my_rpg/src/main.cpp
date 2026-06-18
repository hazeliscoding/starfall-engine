#include "engine/assets/asset_loader.hpp"
#include "engine/core/logger.hpp"
#include "engine/input/input_state.hpp"
#include "engine/math/vec2.hpp"
#include "engine/render/animated_sprite.hpp"
#include "engine/render/animation_clip.hpp"
#include "engine/render/renderer.hpp"
#include "engine/runtime/application.hpp"

#include <array>
#include <string>
#include <string_view>

// M2.5 game state: a single player with an AnimatedSprite. Captured by
// reference into the lambdas below so onStart loads + initializes it,
// onUpdate drives the animation, and onRender draws the current frame.
struct PlayerState {
    Engine::Render::AnimatedSprite sprite;
    Engine::Math::Vec2             position{160.0f, 90.0f};  // logical center
    enum class Direction { Up, Down, Left, Right } facing = Direction::Down;
};

// Logical pixels per second. 60 px/s * 4x scale = 240 screen px/s.
// Tunable here (design D8 of input-and-movement).
constexpr float kIdenWalkSpeed = 60.0f;

namespace {

const char* DirectionName(PlayerState::Direction d) noexcept {
    switch (d) {
        case PlayerState::Direction::Up:    return "up";
        case PlayerState::Direction::Down:  return "down";
        case PlayerState::Direction::Left:  return "left";
        case PlayerState::Direction::Right: return "right";
    }
    return "down";
}

// Build the 8 clips needed for 4-direction idle + walk animation from a
// pre-loaded grid of TimeFantasy frames. Frame order in `frames`:
//   [0..2]   down  : stand, walk1, walk2
//   [3..5]   up    : stand, walk1, walk2
//   [6..8]   left  : stand, walk1, walk2
//   [9..11]  right : stand, walk1, walk2
//
// Walk cycle uses [walk1, stand, walk2, stand] per design D5.
void BuildClips(Engine::Render::AnimatedSprite& sprite,
                const std::array<Engine::Assets::TextureHandle, 12>& f) {
    using Engine::Render::AnimationClip;

    auto addPair = [&](std::string_view dir, std::size_t base) {
        // Idle clip = single stand frame.
        sprite.AddClip(std::string{"idle_"} + std::string{dir},
                       AnimationClip{ {f[base]}, 0.125f, true });
        // Walk clip = [walk1, stand, walk2, stand].
        sprite.AddClip(std::string{"walk_"} + std::string{dir},
                       AnimationClip{ {f[base + 1], f[base], f[base + 2], f[base]}, 0.125f, true });
    };

    addPair("down",  0);
    addPair("up",    3);
    addPair("left",  6);
    addPair("right", 9);
}

// Single-frame fallback clips when the paid TimeFantasy pack isn't
// available — every clip plays the same placeholder texture (design D7).
void BuildFallbackClips(Engine::Render::AnimatedSprite& sprite,
                        const Engine::Assets::TextureHandle& placeholder) {
    using Engine::Render::AnimationClip;
    AnimationClip oneFrame{ {placeholder}, 0.125f, true };
    for (std::string_view dir : {"down", "up", "left", "right"}) {
        sprite.AddClip(std::string{"idle_"} + std::string{dir}, oneFrame);
        sprite.AddClip(std::string{"walk_"} + std::string{dir}, oneFrame);
    }
}

}  // namespace

int main(int /*argc*/, char** /*argv*/) {
    Engine::Runtime::Application app;
    PlayerState player;

    app.Configure({
        .title        = "Starfall",
        .windowWidth  = 1280,
        .windowHeight = 720,

        .onStart = [&player](Engine::Render::Renderer& /*renderer*/,
                             Engine::Assets::AssetLoader& loader) {
            // Try loading all 12 TimeFantasy frames for chara2_1.
            // Order matches BuildClips above.
            static constexpr std::array<const char*, 12> kFramePaths = {
                "assets/timefantasy_characters/frames/chara/chara2_1/down_stand.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/down_walk1.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/down_walk2.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/up_stand.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/up_walk1.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/up_walk2.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/left_stand.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/left_walk1.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/left_walk2.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/right_stand.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/right_walk1.png",
                "assets/timefantasy_characters/frames/chara/chara2_1/right_walk2.png",
            };

            std::array<Engine::Assets::TextureHandle, 12> frames{};
            bool allLoaded = true;
            for (std::size_t i = 0; i < kFramePaths.size(); ++i) {
                auto r = loader.LoadTexture(kFramePaths[i]);
                if (r.IsErr()) { allLoaded = false; break; }
                frames[i] = r.Value();
            }

            if (allLoaded) {
                BuildClips(player.sprite, frames);
            } else {
                SF_LOG_WARN("Game",
                    "TimeFantasy frames not available, falling back to single-frame "
                    "placeholder for all directions. (IGNORE this warning if you "
                    "haven't licensed the pack.)");
                auto fallback = loader.LoadTexture("assets/characters/iden_placeholder.png");
                if (fallback.IsOk()) {
                    BuildFallbackClips(player.sprite, fallback.Value());
                }
            }

            // Initial state: idle facing down.
            player.sprite.Play("idle_down");

            // Center the player on the actual sprite size.
            if (auto frame = player.sprite.CurrentFrame()) {
                const float w = static_cast<float>(frame->Width());
                const float h = static_cast<float>(frame->Height());
                player.position = {160.0f - w * 0.5f, 90.0f - h * 0.5f};
            }
        },

        .onUpdate = [&player](float dt, Engine::Input::InputState& input) {
            using Dir = PlayerState::Direction;

            // 4-directional, most-recently-pressed wins (same as M2).
            if      (input.IsPressed("MoveUp"))    player.facing = Dir::Up;
            else if (input.IsPressed("MoveDown"))  player.facing = Dir::Down;
            else if (input.IsPressed("MoveLeft"))  player.facing = Dir::Left;
            else if (input.IsPressed("MoveRight")) player.facing = Dir::Right;

            auto facingHeld = [&]() {
                switch (player.facing) {
                    case Dir::Up:    return input.IsHeld("MoveUp");
                    case Dir::Down:  return input.IsHeld("MoveDown");
                    case Dir::Left:  return input.IsHeld("MoveLeft");
                    case Dir::Right: return input.IsHeld("MoveRight");
                }
                return false;
            };

            const bool anyMoveHeld =
                input.IsHeld("MoveUp")    || input.IsHeld("MoveDown") ||
                input.IsHeld("MoveLeft")  || input.IsHeld("MoveRight");

            if (!facingHeld() && anyMoveHeld) {
                if      (input.IsHeld("MoveUp"))    player.facing = Dir::Up;
                else if (input.IsHeld("MoveDown"))  player.facing = Dir::Down;
                else if (input.IsHeld("MoveLeft"))  player.facing = Dir::Left;
                else if (input.IsHeld("MoveRight")) player.facing = Dir::Right;
            }

            // Apply movement (only when actually moving).
            const float step = kIdenWalkSpeed * dt;
            const bool isMoving = anyMoveHeld;  // design D6: in M2.5 the two
                                                // terms agree (no collision yet)
            if (isMoving) {
                switch (player.facing) {
                    case Dir::Up:    player.position.y -= step; break;
                    case Dir::Down:  player.position.y += step; break;
                    case Dir::Left:  player.position.x -= step; break;
                    case Dir::Right: player.position.x += step; break;
                }
            }

            // Drive the animation: pick a clip from (isMoving, facing) and
            // tick its timer. Play() on the currently-playing clip is a
            // no-op (design D3) — cycle continuity is preserved across
            // frames the player keeps walking left.
            const std::string clip =
                std::string{isMoving ? "walk_" : "idle_"} + DirectionName(player.facing);
            player.sprite.Play(clip);
            player.sprite.Update(dt);
        },

        .onRender = [&player](Engine::Render::Renderer& renderer) {
            auto frame = player.sprite.CurrentFrame();
            if (!frame) return;
            renderer.DrawSprite(frame, player.position);
        },
    });

    return app.Run();
}
