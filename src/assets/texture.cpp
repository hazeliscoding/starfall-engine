#include "engine/assets/texture.hpp"

#include <SDL3/SDL.h>

namespace Engine::Assets {

Texture::Texture(SDL_Texture* tex, int w, int h) noexcept
    : tex_(tex), width_(w), height_(h) {}

Texture::~Texture() {
    if (tex_ != nullptr) {
        SDL_DestroyTexture(tex_);
    }
}

}  // namespace Engine::Assets
