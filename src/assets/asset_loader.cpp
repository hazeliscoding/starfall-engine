#include "engine/assets/asset_loader.hpp"

#include "engine/core/logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <utility>

namespace Engine::Assets {

namespace {

// SDL_GetBasePath returns a heap string the caller owns. Snapshot it once.
std::string CaptureBasePath() {
    const char* base = SDL_GetBasePath();
    if (base == nullptr) {
        // Fall back to "" — relative paths will resolve from CWD.
        SF_LOG_WARN("Assets", "SDL_GetBasePath returned null: %s", SDL_GetError());
        return {};
    }
    return std::string(base);
}

}  // namespace

AssetLoader::AssetLoader(SDL_Renderer* renderer)
    : renderer_(renderer), basePath_(CaptureBasePath()) {}

AssetLoader::~AssetLoader() = default;

Engine::Core::Result<TextureHandle>
AssetLoader::LoadTexture(std::string_view relativePath) {
    std::string key(relativePath);

    // Cache hit (still alive)?
    if (auto it = cache_.find(key); it != cache_.end()) {
        if (auto shared = it->second.lock()) {
            return Engine::Core::Result<TextureHandle>::Ok(std::move(shared));
        }
        // Expired weak ref — drop it and re-load below.
        cache_.erase(it);
    }

    std::string fullPath = basePath_ + key;

    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (surface == nullptr) {
        std::string err = "IMG_Load failed for '" + fullPath + "': " + SDL_GetError();
        SF_LOG_ERROR("Assets", "%s", err.c_str());
        return Engine::Core::Result<TextureHandle>::Err(std::move(err));
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surface);
    const int w = surface->w;
    const int h = surface->h;
    SDL_DestroySurface(surface);

    if (tex == nullptr) {
        std::string err = "SDL_CreateTextureFromSurface failed for '" + fullPath + "': " + SDL_GetError();
        SF_LOG_ERROR("Assets", "%s", err.c_str());
        return Engine::Core::Result<TextureHandle>::Err(std::move(err));
    }

    // Nearest-neighbor sampling — pixel-art friendly. Without this, the
    // logical-resolution upscale would blur sprites.
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

    auto handle = std::make_shared<Texture>(tex, w, h);
    cache_[key] = handle;
    SF_LOG_INFO("Assets", "Loaded texture '%s' (%dx%d)", fullPath.c_str(), w, h);
    return Engine::Core::Result<TextureHandle>::Ok(std::move(handle));
}

}  // namespace Engine::Assets
