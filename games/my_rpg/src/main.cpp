#include "engine/assets/asset_loader.hpp"
#include "engine/audio/audio_system.hpp"
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

// M2.5 + M2.75 game state: a single player + audio handles. Captured by
// reference into the lambdas below.
struct PlayerState {
    Engine::Render::AnimatedSprite sprite;
    Engine::Math::Vec2             position{160.0f, 90.0f};  // logical center
    enum class Direction { Up, Down, Left, Right } facing = Direction::Down;

    // M2.75 audio.
    Engine::Audio::AudioSystem*   audio = nullptr;  // borrowed; non-owning
    Engine::Audio::MusicHandle    theme;
    Engine::Audio::SoundHandle    footstep;
    bool                          musicTriggered      = false;
    float                         footstepDistanceAcc = 0.0f;
};

constexpr float kIdenWalkSpeed   = 60.0f;  // M2 default (design D8 of input-and-movement)
constexpr float kFootstepDistance = 16.0f;  // ~one tile per footstep, ~2/sec at 60 px/s

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

void BuildClips(Engine::Render::AnimatedSprite& sprite,
                const std::array<Engine::Assets::TextureHandle, 12>& f) {
    using Engine::Render::AnimationClip;

    auto addPair = [&](std::string_view dir, std::size_t base) {
        sprite.AddClip(std::string{"idle_"} + std::string{dir},
                       AnimationClip{ {f[base]}, 0.125f, true });
        sprite.AddClip(std::string{"walk_"} + std::string{dir},
                       AnimationClip{ {f[base + 1], f[base], f[base + 2], f[base]}, 0.125f, true });
    };

    addPair("down",  0);
    addPair("up",    3);
    addPair("left",  6);
    addPair("right", 9);
}

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
                             Engine::Assets::AssetLoader&  loader,
                             Engine::Audio::AudioSystem&   audio) {
            // Borrow audio for use from onUpdate (PlayerState holds a non-
            // owning pointer; AudioSystem outlives onUpdate per the runtime
            // teardown order).
            player.audio = &audio;

            // ---- Sprite (M2.5) ----
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
            player.sprite.Play("idle_down");
            if (auto frame = player.sprite.CurrentFrame()) {
                const float w = static_cast<float>(frame->Width());
                const float h = static_cast<float>(frame->Height());
                player.position = {160.0f - w * 0.5f, 90.0f - h * 0.5f};
            }

            // ---- Audio (M2.75) ----
            // Per GameDesign §5.2, the opening of the slice has NO music
            // for the first 30-60 seconds. The first music cue triggers
            // when the player moves (onUpdate handles the trigger).
            //
            // Theme: try the licensed HydroGene "Peaceful Village" track
            // first (matches Embercoast's quiet coastal morning vibe). If
            // the pack isn't present (public contributor), fall back to
            // the procedural placeholder WAV.
            //
            // To swap tracks: replace the path below. Other on-genre
            // candidates from the same pack:
            //   - "08. Wood Forest Town.ogg"
            //   - "17. Unknown Island.ogg" (coastal mystery)
            //   - "18. The Old Magician.ogg" (weathered/mysterious)
            constexpr const char* kThemeLicensed =
                "assets/28 High Quality 16-bit RPG Music/ogg/04. Peaceful Village.ogg";
            constexpr const char* kThemePlaceholder = "assets/audio/embercoast_morning.wav";

            if (auto m = audio.LoadMusic(kThemeLicensed); m.IsOk()) {
                player.theme = m.Value();
            } else if (auto m2 = audio.LoadMusic(kThemePlaceholder); m2.IsOk()) {
                SF_LOG_WARN("Game",
                    "Licensed music pack not available, using procedural placeholder. "
                    "(IGNORE this warning if you haven't licensed the HydroGene pack.)");
                player.theme = m2.Value();
            } else {
                SF_LOG_WARN("Game", "No music available; the game will run silent for music.");
            }
            if (auto s = audio.LoadSound("assets/audio/footstep.wav"); s.IsOk()) {
                player.footstep = s.Value();
            } else {
                SF_LOG_WARN("Game", "Placeholder footstep SFX not available; movement will be silent.");
            }
        },

        .onUpdate = [&player](float dt, Engine::Input::InputState& input) {
            using Dir = PlayerState::Direction;

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

            const float step = kIdenWalkSpeed * dt;
            const bool isMoving = anyMoveHeld;
            if (isMoving) {
                switch (player.facing) {
                    case Dir::Up:    player.position.y -= step; break;
                    case Dir::Down:  player.position.y += step; break;
                    case Dir::Left:  player.position.x -= step; break;
                    case Dir::Right: player.position.x += step; break;
                }
            }

            const std::string clip =
                std::string{isMoving ? "walk_" : "idle_"} + DirectionName(player.facing);
            player.sprite.Play(clip);
            player.sprite.Update(dt);

            // ---- M2.75: audio triggers ----
            if (player.audio != nullptr) {
                // First movement triggers the music fade-in. Proxy for
                // "Iden walks outside her door" until M3 lands real
                // regions and we can ask "did she cross the threshold?"
                if (!player.musicTriggered && isMoving && player.theme) {
                    player.audio->PlayMusic(player.theme,
                                            /*loop*/         true,
                                            /*fadeIn*/       2.0f,
                                            /*loopPoint*/    0.0f);
                    player.musicTriggered = true;
                }

                // Footstep SFX every kFootstepDistance logical px of travel.
                if (isMoving && player.footstep) {
                    player.footstepDistanceAcc += step;
                    while (player.footstepDistanceAcc >= kFootstepDistance) {
                        player.footstepDistanceAcc -= kFootstepDistance;
                        player.audio->PlaySound(player.footstep);
                    }
                } else {
                    // Reset accumulator when stopped — first step after a
                    // pause shouldn't fire instantly.
                    player.footstepDistanceAcc = 0.0f;
                }
            }
        },

        .onRender = [&player](Engine::Render::Renderer& renderer) {
            auto frame = player.sprite.CurrentFrame();
            if (!frame) return;
            renderer.DrawSprite(frame, player.position);
        },
    });

    return app.Run();
}
