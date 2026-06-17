#include "engine/render/renderer.hpp"

#include "engine/core/logger.hpp"

#include <SDL3/SDL.h>

namespace Engine::Render {

Renderer::Renderer(SDL_Window* window, int logicalWidth, int logicalHeight)
    : logicalWidth_(logicalWidth), logicalHeight_(logicalHeight) {
    renderer_ = SDL_CreateRenderer(window, /*name*/ nullptr);
    if (renderer_ == nullptr) {
        SF_LOG_ERROR("Render", "SDL_CreateRenderer failed: %s", SDL_GetError());
        return;
    }

    if (!SDL_SetRenderLogicalPresentation(
            renderer_, logicalWidth, logicalHeight,
            SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        SF_LOG_WARN("Render",
            "SDL_SetRenderLogicalPresentation failed (%dx%d INTEGER_SCALE): %s. "
            "Falling back to LETTERBOX (non-integer scale).",
            logicalWidth, logicalHeight, SDL_GetError());
        // Letterbox keeps aspect ratio but allows fractional scale.
        SDL_SetRenderLogicalPresentation(
            renderer_, logicalWidth, logicalHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }

    SF_LOG_INFO("Render", "Renderer initialized (logical %dx%d)",
                logicalWidth, logicalHeight);
}

Renderer::~Renderer() {
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}

void Renderer::Clear(Engine::Math::Color color) {
    if (renderer_ == nullptr) return;
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

void Renderer::Present() {
    if (renderer_ == nullptr) return;
    SDL_RenderPresent(renderer_);
}

void Renderer::DrawSprite(
    const Engine::Assets::TextureHandle&  texture,
    Engine::Math::Vec2                    worldPos,
    std::optional<Engine::Math::Rect>     sourceRect) {
    if (renderer_ == nullptr) return;
    if (!texture || texture->RawHandle() == nullptr) {
        // Rate-limit this so a misconfigured game doesn't spam the log.
        static bool logged = false;
        if (!logged) {
            SF_LOG_WARN("Render", "DrawSprite called with null texture handle "
                                   "(further occurrences suppressed)");
            logged = true;
        }
        return;
    }

    // World -> screen via camera. Z-axis is implicit (painter's algorithm).
    const Engine::Math::Vec2 screen = (worldPos - camera_.position) * camera_.zoom;

    SDL_FRect dst{};
    dst.x = screen.x;
    dst.y = screen.y;
    if (sourceRect.has_value()) {
        dst.w = sourceRect->w * camera_.zoom;
        dst.h = sourceRect->h * camera_.zoom;
        SDL_FRect src{sourceRect->x, sourceRect->y, sourceRect->w, sourceRect->h};
        SDL_RenderTexture(renderer_, texture->RawHandle(), &src, &dst);
    } else {
        dst.w = static_cast<float>(texture->Width())  * camera_.zoom;
        dst.h = static_cast<float>(texture->Height()) * camera_.zoom;
        SDL_RenderTexture(renderer_, texture->RawHandle(), /*src*/ nullptr, &dst);
    }
}

}  // namespace Engine::Render
