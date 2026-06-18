#include "engine/assets/asset_loader.hpp"
#include "engine/core/logger.hpp"
#include "engine/input/input_state.hpp"
#include "engine/math/vec2.hpp"
#include "engine/render/renderer.hpp"
#include "engine/runtime/application.hpp"

// M2 game state: a single player. Captured by reference into the lambdas
// below so onStart loads + initializes it and onUpdate / onRender drive it.
struct PlayerState {
    Engine::Assets::TextureHandle sprite;
    Engine::Math::Vec2            position{160.0f, 90.0f};  // logical center
    // Cardinal direction the last movement key pressed selected.
    // Used to resolve diagonal key holds (most-recent-axis wins).
    enum class Direction { None, Up, Down, Left, Right } facing = Direction::Down;
};

// Logical pixels per second. 60 px/s * 4x scale -> 240 screen px/s ->
// ~5 seconds to cross the screen — a deliberate stroll, not a dash.
// Tune here (design D8).
constexpr float kIdenWalkSpeed = 60.0f;

int main(int /*argc*/, char** /*argv*/) {
    Engine::Runtime::Application app;
    PlayerState player;

    app.Configure({
        .title        = "Starfall",
        .windowWidth  = 1280,
        .windowHeight = 720,

        .onStart = [&player](Engine::Render::Renderer& /*renderer*/,
                             Engine::Assets::AssetLoader& loader) {
            // Try the TimeFantasy sprite first (paid pack, may be absent).
            auto result = loader.LoadTexture(
                "assets/timefantasy_characters/frames/chara/chara2_1/down_stand.png");
            if (result.IsOk()) {
                player.sprite = result.Value();
            } else {
                SF_LOG_WARN("Game",
                    "TimeFantasy sprite not available, falling back to placeholder. "
                    "(IGNORE this warning if you haven't licensed the pack.)");
                auto fallback = loader.LoadTexture("assets/characters/iden_placeholder.png");
                if (fallback.IsOk()) {
                    player.sprite = fallback.Value();
                }
            }
            if (player.sprite) {
                // Re-center on the actual sprite size (placeholder is 16x16,
                // TimeFantasy is 17x31 — both end up centered on the 320x180
                // logical surface).
                const float w = static_cast<float>(player.sprite->Width());
                const float h = static_cast<float>(player.sprite->Height());
                player.position = {160.0f - w * 0.5f, 90.0f - h * 0.5f};
            }
        },

        .onUpdate = [&player](float dt, Engine::Input::InputState& input) {
            // 4-directional, most-recently-pressed-axis wins on diagonals
            // (design D6). Treat Pressed (edge) as "switch to this axis,"
            // then Held maintains it. If the active-axis key isn't held
            // anymore, fall back to any other held movement.
            using Dir = PlayerState::Direction;

            // 1) Edge-pressed key wins immediately.
            if      (input.IsPressed("MoveUp"))    player.facing = Dir::Up;
            else if (input.IsPressed("MoveDown"))  player.facing = Dir::Down;
            else if (input.IsPressed("MoveLeft"))  player.facing = Dir::Left;
            else if (input.IsPressed("MoveRight")) player.facing = Dir::Right;

            // 2) If the currently-facing direction's key is no longer held,
            // pick any other held direction (preserves the snap on release).
            auto facingHeld = [&]() {
                switch (player.facing) {
                    case Dir::Up:    return input.IsHeld("MoveUp");
                    case Dir::Down:  return input.IsHeld("MoveDown");
                    case Dir::Left:  return input.IsHeld("MoveLeft");
                    case Dir::Right: return input.IsHeld("MoveRight");
                    case Dir::None:  return false;
                }
                return false;
            };
            if (!facingHeld()) {
                if      (input.IsHeld("MoveUp"))    player.facing = Dir::Up;
                else if (input.IsHeld("MoveDown"))  player.facing = Dir::Down;
                else if (input.IsHeld("MoveLeft"))  player.facing = Dir::Left;
                else if (input.IsHeld("MoveRight")) player.facing = Dir::Right;
                else                                player.facing = Dir::None;
            }

            // 3) Apply movement in the facing direction.
            const float step = kIdenWalkSpeed * dt;
            switch (player.facing) {
                case Dir::Up:    player.position.y -= step; break;
                case Dir::Down:  player.position.y += step; break;
                case Dir::Left:  player.position.x -= step; break;
                case Dir::Right: player.position.x += step; break;
                case Dir::None:  break;
            }
        },

        .onRender = [&player](Engine::Render::Renderer& renderer) {
            if (!player.sprite) return;
            renderer.DrawSprite(player.sprite, player.position);
        },
    });

    return app.Run();
}
