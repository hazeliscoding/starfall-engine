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
// Procedural-art Embercoast.
//
// M3 ships on the 5-tile procedural placeholder tileset
// (tools/gen_placeholder_tileset.py).  Licensed TimeFantasy tile art lives
// on disk but is intentionally not loaded — visual iteration in C++ ASCII
// masks is the wrong tool for the job.  Real tile art lands after M5
// (Editor v1) when scenes load from JSON and can be edited visually.
// ============================================================================
namespace Tile {
    constexpr int Empty = 0;
    constexpr int Grass = 1;
    constexpr int Path  = 2;
    constexpr int Stone = 3;
    constexpr int Wall  = 4;
    constexpr int Water = 5;
}

constexpr int kEmbercoastWidth  = 20;
constexpr int kEmbercoastHeight = 12;
constexpr int kTileSize         = 16;

struct PlayerState {
    Engine::Render::AnimatedSprite sprite;
    Engine::Math::Vec2             position{160.0f - 16.0f, 90.0f};
    enum class Direction { Up, Down, Left, Right } facing = Direction::Down;

    Engine::Audio::AudioSystem*   audio = nullptr;
    Engine::Audio::MusicHandle    theme;
    Engine::Audio::SoundHandle    footstep;
    bool                          musicTriggered      = false;
    float                         footstepDistanceAcc = 0.0f;

    std::unique_ptr<Engine::Scene::Tilemap> tilemap;
};

constexpr float                  kIdenWalkSpeed     = 60.0f;
constexpr float                  kFootstepDistance  = 16.0f;
constexpr Engine::Math::Vec2     kPlayerCollisionSize{12.0f, 8.0f};

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

Engine::Math::Rect FeetAABB(Engine::Math::Vec2 position, int spriteW, int spriteH) {
    return Engine::Math::Rect{
        position.x + (static_cast<float>(spriteW) - kPlayerCollisionSize.x) * 0.5f,
        position.y + static_cast<float>(spriteH) - kPlayerCollisionSize.y,
        kPlayerCollisionSize.x,
        kPlayerCollisionSize.y,
    };
}

struct CharToId { char c; int id; };
Engine::Scene::TileLayer MakeLayer(std::string_view name, int sortOrder,
                                   std::string_view ascii,
                                   std::initializer_list<CharToId> map) {
    Engine::Scene::TileLayer layer;
    layer.name      = std::string{name};
    layer.width     = kEmbercoastWidth;
    layer.height    = kEmbercoastHeight;
    layer.sortOrder = sortOrder;
    layer.tileIds.reserve(static_cast<std::size_t>(kEmbercoastWidth * kEmbercoastHeight));
    for (char c : ascii) {
        int id = 0;
        for (const auto& [k, v] : map) { if (k == c) { id = v; break; } }
        layer.tileIds.push_back(id);
    }
    return layer;
}

// Build the Embercoast scene on the procedural placeholder tileset.
// Two layers (ground + overhead).  Solid tiles: Stone, Wall, Water.
// M5 (Editor v1) will replace this with editor-authored JSON scenes.
void BuildEmbercoast(Engine::Scene::Tilemap& tm,
                     std::shared_ptr<Engine::Scene::TileSet> tileset) {
    tileset->MarkSolid(Tile::Stone);
    tileset->MarkSolid(Tile::Wall);
    tileset->MarkSolid(Tile::Water);
    tm.SetTileSet(tileset);

    constexpr std::string_view kGround =
        "WWWWWWWWWWWWWWWWWWWW"
        "WGGGGGGGGGGGGGGGGGGW"
        "WG................GW"
        "WG......SSSS......GW"
        "WG......S..S......GW"
        "WG......SSSS......GW"
        "WG................GW"
        "WG................GW"
        "WG................GW"
        "WG~~~~~~~~~~~~~~~~GW"
        "WG~~~~~~~~~~~~~~~~GW"
        "WWWWWWWWWWWWWWWWWWWW";
    constexpr std::string_view kOver =
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "       AAAA         "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    ";
    tm.AddLayer(MakeLayer("ground", 0, kGround,
        {{'W', Tile::Wall},  {'G', Tile::Grass},
         {'.', Tile::Path},  {'S', Tile::Stone},
         {'~', Tile::Water}}));
    tm.AddLayer(MakeLayer("overhead", 100, kOver,
        {{'A', Tile::Stone}}));
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

            // ---- Sprite (M2.5) — TimeFantasy chara2_1 with placeholder
            // fallback for public clones without the licensed pack. ----
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

            // ---- Audio (M2.75) — licensed HydroGene with procedural fallback. ----
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

            // ---- Tilemap (M3) — procedural placeholder only.  Licensed
            // tile art deferred until M5 (Editor v1). ----
            player.tilemap = std::make_unique<Engine::Scene::Tilemap>(kTileSize, kTileSize);
            auto placeholderTex = loader.LoadTexture("assets/tiles/placeholder_tileset.png");
            if (placeholderTex.IsErr()) {
                SF_LOG_ERROR("Game",
                    "Placeholder tileset failed to load — Iden will walk on a blank scene.");
                player.tilemap.reset();
            } else {
                auto tileset = std::make_shared<Engine::Scene::TileSet>(
                    placeholderTex.Value(), kTileSize, kTileSize, /*columns*/ 1);
                BuildEmbercoast(*player.tilemap, tileset);
                SF_LOG_INFO("Game",
                    "Embercoast loaded: %dx%d tiles (world %dx%d), procedural art",
                    kEmbercoastWidth, kEmbercoastHeight,
                    player.tilemap->WorldWidth(), player.tilemap->WorldHeight());
            }

            // Spawn Iden on the grass south of the inn block (rows 3-5
            // cols 8-11), in the open area between the building and the
            // sea band.
            if (player.tilemap) {
                if (auto frame = player.sprite.CurrentFrame()) {
                    const float w = static_cast<float>(frame->Width());
                    const float h = static_cast<float>(frame->Height());
                    player.position = Engine::Math::Vec2{
                        9.0f * kTileSize - (w - kTileSize) * 0.5f,
                        7.0f * kTileSize - (h - kTileSize),
                    };
                }
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
                player.position.x += displacement.x;
                player.position.y += displacement.y;
            }

            const std::string clip =
                std::string{isMoving ? "walk_" : "idle_"} + DirectionName(player.facing);
            player.sprite.Play(clip);
            player.sprite.Update(dt);

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
            const Engine::Math::Rect viewport{0.0f, 0.0f, 320.0f, 180.0f};

            // Visitor resolves the per-layer tileset (falls back to the
            // shared one when a layer doesn't carry its own).
            auto drawTile = [&](const Engine::Scene::TileLayer& layer,
                                Engine::Math::Vec2 worldPos, int tileId) {
                if (!player.tilemap) return;
                const auto& ts = player.tilemap->ResolveTileSet(layer);
                if (!ts) return;
                renderer.DrawSprite(ts->Texture(), worldPos, ts->SourceRect(tileId));
            };

            // Pass 1: ground + mid layers below the player.
            if (player.tilemap) {
                player.tilemap->ForEachVisibleTile(viewport, /*minSort*/ -10'000, /*maxSort*/ 49, drawTile);
            }

            // Pass 2: mid-layer entities Y-sorted by feet.
            struct Drawable {
                float                 sortKey;
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

            // Pass 3: overhead (awning) layers.
            if (player.tilemap) {
                player.tilemap->ForEachVisibleTile(viewport, /*minSort*/ 50, /*maxSort*/ 10'000, drawTile);
            }
        },
    });

    return app.Run();
}
