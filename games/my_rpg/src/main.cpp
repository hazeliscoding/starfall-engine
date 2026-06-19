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
// Tile-ID picks per TimeFantasy tileset. To iterate, change a single number
// and rebuild; tools/preview_tilesets.py renders each pick for visual check.
// ============================================================================
namespace TerrainTile {                              // terrain.png, 39 cols
    constexpr int Grass = 1 * 39 +  4 + 1;           // (4,1)   = 44   plain green
    constexpr int Path  = 4 * 39 +  4 + 1;           // (4,4)   = 161  brown dirt
    constexpr int Cliff = 14 * 39 +  3 + 1;          // (3,14)  = 550  cliff w/ grass cap
}
namespace WaterTile {                                // water.png, 51 cols
    constexpr int Sea   = 4 * 51 +  2 + 1;           // (2,4)   = 207  plain water frame 0
}
namespace HouseTile {                                // house.png, 69 cols
    constexpr int Wall   = 4 * 69 +  1 + 1;          // (1,4)   = 278  wood-log wall
    constexpr int Awning = 4 * 69 + 22 + 1;          // (22,4)  = 299  slate roof + beam
}

// Placeholder fallback IDs (the procedural 5-tile PNG, 1-column layout).
namespace PlaceholderTile {
    constexpr int Empty = 0;
    constexpr int Grass = 1;
    constexpr int Path  = 2;
    constexpr int Stone = 3;
    constexpr int Wall  = 4;
    constexpr int Water = 5;
}

// ============================================================================
// Embercoast — 20x12 hand-authored. Each '.' in a layer means "empty here";
// the layer only carries its own art (grass layer has grass, water layer has
// water, etc.). Layers stack with the Y-sort visitor in onRender.
//
// Reads (north = top): cliff wall (cliff layer); grass band; building
// (building layer); awning overhead (awning layer, drawn over player);
// sea band (water layer).
// ============================================================================
constexpr int kEmbercoastWidth  = 20;
constexpr int kEmbercoastHeight = 12;
constexpr int kTileSize         = 16;

// Layer 0 — grass everywhere except cliff row + water rows.
constexpr std::string_view kGrassMask =
    "...................."   // r0  cliff above (no grass)
    "GGGGGGGGGGGGGGGGGGGG"   // r1
    "GGGGGGGGGGGGGGGGGGGG"   // r2
    "GGGGGGGGGGGGGGGGGGGG"   // r3  grass beneath building footprint
    "GGGGGGGGGGGGGGGGGGGG"   // r4
    "GGGGGGGGGGGGGGGGGGGG"   // r5
    "GGGGGGGGGGGGGGGGGGGG"   // r6
    "GGGGGGGGGGGGGGGGGGGG"   // r7
    "GGGGGGGGGGGGGGGGGGGG"   // r8
    "...................."   // r9  sea
    "...................."   // r10
    "...................."; // r11

// Layer 1 — cliff wall north edge (solid).
constexpr std::string_view kCliffMask =
    "CCCCCCCCCCCCCCCCCCCC"   // r0
    "...................."   // r1..r11 empty
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "....................";

// Layer 2 — building walls (solid).
constexpr std::string_view kBuildingMask =
    "...................."   // r0
    "...................."   // r1
    "...................."   // r2
    "........BBBB........"   // r3  building north wall
    "........B..B........"   // r4  building (interior strip; B walls are solid)
    "........BBBB........"   // r5  building south wall
    "...................."   // r6  (awning lives on layer 4)
    "...................."   // r7
    "...................."   // r8
    "...................."   // r9
    "...................."   // r10
    "...................."; // r11

// Layer 3 — sea band (solid).
constexpr std::string_view kSeaMask =
    "...................."   // r0..r8 empty
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "~~~~~~~~~~~~~~~~~~~~"   // r9
    "~~~~~~~~~~~~~~~~~~~~"   // r10
    "~~~~~~~~~~~~~~~~~~~~"; // r11

// Layer 4 — overhead awning (drawn after player; player walks under).
constexpr std::string_view kAwningMask =
    "...................."   // r0..r5
    "...................."
    "...................."
    "...................."
    "...................."
    "...................."
    "........AAAA........"   // r6  awning extends south of building
    "...................."   // r7..r11
    "...................."
    "...................."
    "...................."
    "....................";

// Build a TileLayer from one mask + a charset-to-tileId map.
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

// ============================================================================
// Player state
// ============================================================================
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

// Build a 5-layer Embercoast backed by 3 TimeFantasy tilesets (one tileset
// per ground/water/house layer). Each call sets up MarkSolid on its own
// tileset before the layer is added.
void BuildLicensedEmbercoast(Engine::Scene::Tilemap& tm,
                             std::shared_ptr<Engine::Scene::TileSet> terrainTs,
                             std::shared_ptr<Engine::Scene::TileSet> waterTs,
                             std::shared_ptr<Engine::Scene::TileSet> houseTs) {
    terrainTs->MarkSolid(TerrainTile::Cliff);
    waterTs  ->MarkSolid(WaterTile::Sea);
    houseTs  ->MarkSolid(HouseTile::Wall);

    auto grass = MakeLayer("grass",    0,  kGrassMask,    {{'G', TerrainTile::Grass}});
    auto cliff = MakeLayer("cliff",    1,  kCliffMask,    {{'C', TerrainTile::Cliff}});
    auto bldg  = MakeLayer("building", 2,  kBuildingMask, {{'B', HouseTile::Wall}});
    auto sea   = MakeLayer("sea",      3,  kSeaMask,      {{'~', WaterTile::Sea}});
    auto awn   = MakeLayer("awning", 100,  kAwningMask,   {{'A', HouseTile::Awning}});
    grass.tileset = terrainTs;
    cliff.tileset = terrainTs;
    bldg .tileset = houseTs;
    sea  .tileset = waterTs;
    awn  .tileset = houseTs;
    tm.AddLayer(std::move(grass));
    tm.AddLayer(std::move(cliff));
    tm.AddLayer(std::move(bldg));
    tm.AddLayer(std::move(sea));
    tm.AddLayer(std::move(awn));
}

// Public-contributor fallback: a 2-layer Embercoast on the procedural
// placeholder tileset (5 IDs in a single column). Same map shape, plainer
// art. Tilemap's shared TileSet handles solidity for both layers.
void BuildPlaceholderEmbercoast(Engine::Scene::Tilemap& tm,
                                std::shared_ptr<Engine::Scene::TileSet> placeholder) {
    placeholder->MarkSolid(PlaceholderTile::Stone);
    placeholder->MarkSolid(PlaceholderTile::Wall);
    placeholder->MarkSolid(PlaceholderTile::Water);
    tm.SetTileSet(placeholder);

    // Combine all features into 2 layers (ground + overhead) using the
    // single placeholder tileset.
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
    tm.AddLayer(MakeLayer("ground",   0, kGround,
        {{'W', PlaceholderTile::Wall},  {'G', PlaceholderTile::Grass},
         {'.', PlaceholderTile::Path},  {'S', PlaceholderTile::Stone},
         {'~', PlaceholderTile::Water}}));
    tm.AddLayer(MakeLayer("overhead", 100, kOver,
        {{'A', PlaceholderTile::Stone}}));
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
            // Prefer the licensed TimeFantasy tilesets (3 PNGs: terrain.png
            // for ground+cliff, water.png for the sea, house.png for the
            // building + awning). Fall back to the procedural placeholder
            // tileset (a single PNG, committed) if any are missing — public
            // clones still build and run.
            auto terrainRes = loader.LoadTexture(
                "assets/TimeFantasy_TILES_6.24.17/TILESETS/terrain.png");
            auto waterRes   = loader.LoadTexture(
                "assets/TimeFantasy_TILES_6.24.17/TILESETS/water.png");
            auto houseRes   = loader.LoadTexture(
                "assets/TimeFantasy_TILES_6.24.17/TILESETS/house.png");
            const bool licensedReady =
                terrainRes.IsOk() && waterRes.IsOk() && houseRes.IsOk();

            player.tilemap = std::make_unique<Engine::Scene::Tilemap>(kTileSize, kTileSize);

            if (licensedReady) {
                auto terrainTs = std::make_shared<Engine::Scene::TileSet>(
                    terrainRes.Value(), kTileSize, kTileSize, /*columns*/ 39);
                auto waterTs = std::make_shared<Engine::Scene::TileSet>(
                    waterRes.Value(),   kTileSize, kTileSize, /*columns*/ 51);
                auto houseTs = std::make_shared<Engine::Scene::TileSet>(
                    houseRes.Value(),   kTileSize, kTileSize, /*columns*/ 69);
                BuildLicensedEmbercoast(*player.tilemap, terrainTs, waterTs, houseTs);
                SF_LOG_INFO("Game",
                    "Embercoast loaded: %dx%d tiles (world %dx%d), TimeFantasy art",
                    kEmbercoastWidth, kEmbercoastHeight,
                    player.tilemap->WorldWidth(), player.tilemap->WorldHeight());
            } else {
                SF_LOG_WARN("Game",
                    "TimeFantasy tilesets not available, falling back to placeholder. "
                    "(IGNORE this warning if you haven't licensed the TimeFantasy pack.)");
                auto placeholderTex = loader.LoadTexture("assets/tiles/placeholder_tileset.png");
                if (placeholderTex.IsErr()) {
                    SF_LOG_ERROR("Game",
                        "Placeholder tileset failed too — Iden will walk on a blank scene.");
                    player.tilemap.reset();
                } else {
                    auto placeholder = std::make_shared<Engine::Scene::TileSet>(
                        placeholderTex.Value(), kTileSize, kTileSize, /*columns*/ 1);
                    BuildPlaceholderEmbercoast(*player.tilemap, placeholder);
                    SF_LOG_INFO("Game",
                        "Embercoast loaded: %dx%d tiles (world %dx%d), placeholder art",
                        kEmbercoastWidth, kEmbercoastHeight,
                        player.tilemap->WorldWidth(), player.tilemap->WorldHeight());
                }
            }

            // Spawn Iden on the walkable strip south of the building (row 7).
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
