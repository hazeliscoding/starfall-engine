#pragma once

#include "engine/assets/texture.hpp"
#include "engine/math/color.hpp"
#include "engine/math/rect.hpp"
#include "engine/math/vec2.hpp"
#include "engine/render/camera_2d.hpp"

#include <optional>

struct SDL_Renderer;
struct SDL_Window;

namespace Engine::Render {

// 2D renderer wrapping SDL_Renderer + logical-resolution presentation.
// Owned by engine_runtime; lifetime spans between window creation and
// destruction. Per-frame contract: Clear -> DrawSprite* -> Present.
class Renderer {
public:
    Renderer(SDL_Window* window, int logicalWidth, int logicalHeight);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&)                 = delete;
    Renderer& operator=(Renderer&&)      = delete;

    // True if SDL_Renderer creation succeeded. Check before use.
    bool IsValid() const noexcept { return renderer_ != nullptr; }

    // Raw handle used by engine_assets to upload textures. Game code never
    // touches this — DrawSprite is the public surface.
    SDL_Renderer* RawHandle() const noexcept { return renderer_; }

    void Clear(Engine::Math::Color color);
    void Present();

    // Draws the texture (or a sub-region of it) at the given world position.
    // World coords are translated to screen by the active camera before
    // drawing. A null handle is a logged noop.
    void DrawSprite(
        const Engine::Assets::TextureHandle&  texture,
        Engine::Math::Vec2                    worldPos,
        std::optional<Engine::Math::Rect>     sourceRect = std::nullopt);

    Camera2D&       Camera()       noexcept { return camera_; }
    const Camera2D& Camera() const noexcept { return camera_; }

private:
    SDL_Renderer* renderer_      = nullptr;
    int           logicalWidth_  = 0;
    int           logicalHeight_ = 0;
    Camera2D      camera_{};
};

}  // namespace Engine::Render
