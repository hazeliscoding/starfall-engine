#include "engine/assets/asset_loader.hpp"
#include "engine/audio/audio_system.hpp"
#include "engine/core/logger.hpp"
#include "engine/input/input_state.hpp"
#include "engine/math/rect.hpp"
#include "engine/math/vec2.hpp"
#include "engine/render/animated_sprite.hpp"
#include "engine/render/animation_clip.hpp"
#include "engine/render/renderer.hpp"
#include "engine/runtime/application.hpp"
#include "engine/scene/sweep.hpp"
#include "engine/scene/tile_layer.hpp"
#include "engine/scene/tile_set.hpp"
#include "engine/scene/tilemap.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// ============================================================================
// Tile IDs — matched to the placeholder tileset row order (see
// tools/gen_placeholder_tileset.py for the source-of-truth palette).
// ============================================================================
namespace TileId {
    constexpr int Empty = 0;
    constexpr int Grass = 1;
    constexpr int Path  = 2;
    constexpr int Stone = 3;  // solid
    constexpr int Wall  = 4;  // solid
    constexpr int Water = 5;  // solid
}

// ============================================================================
// Embercoast ground layer (20×12 — one logical-screen-wide, slightly taller).
// Charset: W=wall, G=grass, .=path, S=stone (building), ~=water, space=empty.
//
// Scene reads (north = top): cliff-path wall to the north; one small stone
// building in the middle; sea to the south. Grass borders.
// ============================================================================
constexpr std::string_view kEmbercoastGround =
    "WWWWWWWWWWWWWWWWWWWW"   // r0  cliff path blocked
    "WGGGGGGGGGGGGGGGGGGW"   // r1
    "WG................GW"   // r2
    "WG......SSSS......GW"   // r3  building north wall
    "WG......S..S......GW"   // r4  building (doorway visual)
    "WG......SSSS......GW"   // r5  building south wall
    "WG................GW"   // r6  walkable strip — Iden walks under the awning here
    "WG................GW"   // r7
    "WG................GW"   // r8
    "WG~~~~~~~~~~~~~~~~GW"   // r9  sea
    "WG~~~~~~~~~~~~~~~~GW"   // r10
    "WWWWWWWWWWWWWWWWWWWW";  // r11

// Overhead layer — same dimensions. 'A' = awning (rendered as stone-tile
// for placeholder; real awning art comes with the TimeFantasy swap).
constexpr std::string_view kEmbercoastOverhead =
    "                    "   // r0
    "                    "   // r1
    "                    "   // r2
    "                    "   // r3
    "                    "   // r4
    "                    "   // r5
    "       AAAA         "   // r6  awning extends south of building
    "                    "   // r7
    "                    "   // r8
    "                    "   // r9
    "                    "   // r10
    "                    "; // r11

constexpr int kEmbercoastWidth  = 20;
constexpr int kEmbercoastHeight = 12;
constexpr int kTileSize         = 16;

// Convert a multi-row char string (no separators) to a TileLayer.
Engine::Scene::TileLayer MakeLayerFromAscii(std::string_view name,
                                            int width, int height,
                                            std::string_view ascii,
                                            int sortOrder = 0) {
    Engine::Scene::TileLayer layer;
    layer.name      = std::string{name};
    layer.width     = width;
    layer.height    = height;
    layer.sortOrder = sortOrder;
    layer.tileIds.reserve(static_cast<std::size_t>(width * height));
    for (char c : ascii) {
        switch (c) {
            case '.': layer.tileIds.push_back(TileId::Path);  break;
            case 'G': layer.tileIds.push_back(TileId::Grass); break;
            case 'S': layer.tileIds.push_back(TileId::Stone); break;
            case 'W': layer.tileIds.push_back(TileId::Wall);  break;
            case '~': layer.tileIds.push_back(TileId::Water); break;
            case 'A': layer.tileIds.push_back(TileId::Stone); break;  // awning placeholder
            case ' ': layer.tileIds.push_back(TileId::Empty); break;
            default:  layer.tileIds.push_back(TileId::Empty); break;
        }
    }
    return layer;
}

// ============================================================================
// Player state
// ============================================================================
struct PlayerState {
    Engine::Render::AnimatedSprite sprite;
    Engine::Math::Vec2             position{160.0f - 16.0f, 90.0f};  // re-centered in onStart
    enum class Direction { Up, Down, Left, Right } facing = Direction::Down;

    Engine::Audio::AudioSystem*   audio = nullptr;
    Engine::Audio::MusicHandle    theme;
    Engine::Audio::SoundHandle    footstep;
    bool                          musicTriggered      = false;
    float                         footstepDistanceAcc = 0.0f;

    // M3: tilemap + collision.
    std::unique_ptr<Engine::Scene::Tilemap> tilemap;
};

constexpr float                  kIdenWalkSpeed     = 60.0f;
constexpr float                  kFootstepDistance  = 16.0f;
constexpr Engine::Math::Vec2     kPlayerCollisionSize{12.0f, 8.0f};   // feet AABB (design D5)

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

// Bottom-center feet AABB at a sprite position. Sprite is drawn from
// position (its top-left); the AABB sits at the bottom-center.
Engine::Math::Rect FeetAABB(Engine::Math::Vec2 position, int spriteW, int spriteH) {
    return Engine::Math::Rect{
        position.x + (static_cast<float>(spriteW) - kPlayerCollisionSize.x) * 0.5f,
        position.y + static_cast<float>(spriteH) - kPlayerCollisionSize.y,
        kPlayerCollisionSize.x,
        kPlayerCollisionSize.y,
    };
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
                    "TimeFantasy frames not available, falling back to placeholder.");
                auto fallback = loader.LoadTexture("assets/characters/iden_placeholder.png");
                if (fallback.IsOk()) BuildFallbackClips(player.sprite, fallback.Value());
            }
            player.sprite.Play("idle_down");

            // ---- Audio (M2.75) ----
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
            }
            if (auto s = audio.LoadSound("assets/audio/footstep.wav"); s.IsOk()) {
                player.footstep = s.Value();
            }

            // ---- Tilemap (M3) ----
            // M3 ships with the procedural placeholder tileset to keep
            // visuals deterministic. The TimeFantasy `outside.png` pack
            // is available locally for a Game-Director art pass; swapping
            // is a one-line change to the texture path + tile-ID picks.
            auto tilesetTex = loader.LoadTexture("assets/tiles/placeholder_tileset.png");
            if (tilesetTex.IsOk()) {
                auto tileset = std::make_shared<Engine::Scene::TileSet>(
                    tilesetTex.Value(),
                    /*tileWidth*/ kTileSize, /*tileHeight*/ kTileSize,
                    /*columns*/ 1);
                tileset->MarkSolid(TileId::Stone);
                tileset->MarkSolid(TileId::Wall);
                tileset->MarkSolid(TileId::Water);

                player.tilemap = std::make_unique<Engine::Scene::Tilemap>(kTileSize, kTileSize);
                player.tilemap->SetTileSet(tileset);
                player.tilemap->AddLayer(MakeLayerFromAscii(
                    "ground", kEmbercoastWidth, kEmbercoastHeight,
                    kEmbercoastGround, /*sortOrder*/ 0));
                player.tilemap->AddLayer(MakeLayerFromAscii(
                    "overhead", kEmbercoastWidth, kEmbercoastHeight,
                    kEmbercoastOverhead, /*sortOrder*/ 100));

                // Spawn Iden on the walkable strip south of the building.
                // Target: feet ~ row 7 col 9 (world tile (9, 7)). Sprite
                // top-left = (col*16 - (w-16)/2, row*16 - (h-16)) so
                // feet land on the target tile.
                if (auto frame = player.sprite.CurrentFrame()) {
                    const float w = static_cast<float>(frame->Width());
                    const float h = static_cast<float>(frame->Height());
                    player.position = Engine::Math::Vec2{
                        9.0f * kTileSize - (w - kTileSize) * 0.5f,
                        7.0f * kTileSize - (h - kTileSize),
                    };
                }
                SF_LOG_INFO("Game",
                    "Embercoast loaded: %dx%d tiles (world %dx%d)",
                    kEmbercoastWidth, kEmbercoastHeight,
                    player.tilemap->WorldWidth(), player.tilemap->WorldHeight());
            } else {
                SF_LOG_ERROR("Game",
                    "Tileset failed to load — Iden will walk on a blank scene.");
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

            // Desired displacement.
            const float step = kIdenWalkSpeed * dt;
            Engine::Math::Vec2 displacement{0.0f, 0.0f};
            const bool isMoving = anyMoveHeld;
            if (isMoving) {
                switch (player.facing) {
                    case Dir::Up:    displacement.y = -step; break;
                    case Dir::Down:  displacement.y =  step; break;
                    case Dir::Left:  displacement.x = -step; break;
                    case Dir::Right: displacement.x =  step; break;
                }
            }

            // M3: route movement through the per-axis tilemap sweep.
            if (player.tilemap && (displacement.x != 0.0f || displacement.y != 0.0f)) {
                auto frame = player.sprite.CurrentFrame();
                const int  spriteW = frame ? frame->Width()  : 16;
                const int  spriteH = frame ? frame->Height() : 31;
                const auto feet    = FeetAABB(player.position, spriteW, spriteH);
                const auto applied = Engine::Scene::SweepMove(
                    Engine::Math::Vec2{feet.x, feet.y},
                    displacement,
                    kPlayerCollisionSize,
                    *player.tilemap);
                player.position.x += applied.x;
                player.position.y += applied.y;
            } else if (displacement.x != 0.0f || displacement.y != 0.0f) {
                // No tilemap — fall back to free movement (M2 behavior).
                player.position.x += displacement.x;
                player.position.y += displacement.y;
            }

            // Animation clip + Update.
            const std::string clip =
                std::string{isMoving ? "walk_" : "idle_"} + DirectionName(player.facing);
            player.sprite.Play(clip);
            player.sprite.Update(dt);

            // Audio triggers (M2.75).
            if (player.audio != nullptr) {
                if (!player.musicTriggered && isMoving && player.theme) {
                    player.audio->PlayMusic(player.theme, true, 2.0f, 0.0f);
                    player.musicTriggered = true;
                }
                if (isMoving && player.footstep) {
                    player.footstepDistanceAcc += step;
                    while (player.footstepDistanceAcc >= kFootstepDistance) {
                        player.footstepDistanceAcc -= kFootstepDistance;
                        player.audio->PlaySound(player.footstep);
                    }
                } else {
                    player.footstepDistanceAcc = 0.0f;
                }
            }
        },

        .onRender = [&player](Engine::Render::Renderer& renderer) {
            // M3 viewport = whole logical surface, anchored at (0, 0).
            // M3.5 moves the viewport to follow the player.
            const Engine::Math::Rect viewport{0.0f, 0.0f, 320.0f, 180.0f};

            auto drawTile = [&](const Engine::Scene::TileLayer& /*layer*/,
                                Engine::Math::Vec2 worldPos, int tileId) {
                if (!player.tilemap) return;
                const auto& tileset = player.tilemap->TileSet();
                if (!tileset) return;
                renderer.DrawSprite(tileset->Texture(), worldPos,
                                    tileset->SourceRect(tileId));
            };

            // Pass 1: ground layers.
            if (player.tilemap) {
                player.tilemap->ForEachVisibleTile(viewport, /*minSort*/ -10'000, /*maxSort*/ 49, drawTile);
            }

            // Pass 2: mid-layer entities, Y-sorted (design D4).
            struct Drawable {
                float                 sortKey;  // feet Y
                std::function<void()> draw;
            };
            std::vector<Drawable> mid;
            if (auto frame = player.sprite.CurrentFrame()) {
                const float h = static_cast<float>(frame->Height());
                mid.push_back({
                    player.position.y + h,
                    [&renderer, &player, frame]() {
                        renderer.DrawSprite(frame, player.position);
                    },
                });
            }
            std::sort(mid.begin(), mid.end(),
                      [](const Drawable& a, const Drawable& b) { return a.sortKey < b.sortKey; });
            for (auto& d : mid) d.draw();

            // Pass 3: overhead layers.
            if (player.tilemap) {
                player.tilemap->ForEachVisibleTile(viewport, /*minSort*/ 50, /*maxSort*/ 10'000, drawTile);
            }
        },
    });

    return app.Run();
}
