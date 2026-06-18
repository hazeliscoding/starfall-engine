#pragma once

#include "engine/math/color.hpp"

#include <functional>
#include <string>

namespace Engine::Assets { class AssetLoader; }
namespace Engine::Input  { class InputState; }
namespace Engine::Render { class Renderer; }

namespace Engine::Runtime {

// Plain aggregate. Use C++20 designated initializers from the game entry
// point:
//   app.Configure({.title = "Starfall", .onStart = ..., .onRender = ...});
struct AppConfig {
    // Window
    std::string title         = "Starfall";
    int         windowWidth   = 1280;
    int         windowHeight  = 720;

    // Renderer logical resolution. 320x180 default scales 4x to 1280x720
    // pixel-perfect (design D6).
    int         logicalWidth  = 320;
    int         logicalHeight = 180;

    // Per-frame clear color. Default = pre-dawn deep blue (design D7).
    Engine::Math::Color clearColor{0x1A, 0x24, 0x40, 0xFF};

    // Called once after subsystem init, before the first frame. Use for
    // texture loads and other one-time setup. Optional (design D11).
    std::function<void(Engine::Render::Renderer&,
                       Engine::Assets::AssetLoader&)> onStart;

    // Called every frame before Clear. Use for game-state mutation driven
    // by input. `dt` is wall-clock seconds since the previous frame's start
    // (0 on the first frame). Optional.
    std::function<void(float dt,
                       Engine::Input::InputState&)>   onUpdate;

    // Called every frame between Clear and Present. Optional.
    std::function<void(Engine::Render::Renderer&)>    onRender;

    // Reserved for milestones M4+ (scene loading). Unused at M1.
    std::string initialScene;
};

}  // namespace Engine::Runtime
