#pragma once

#include "engine/assets/texture.hpp"
#include "engine/core/result.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

struct SDL_Renderer;

namespace Engine::Assets {

// Loads assets from disk into GPU resources, caching by canonical path.
// All paths are resolved relative to the directory containing the running
// executable (SDL_GetBasePath()).
class AssetLoader {
public:
    explicit AssetLoader(SDL_Renderer* renderer);
    ~AssetLoader();

    AssetLoader(const AssetLoader&)            = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;
    AssetLoader(AssetLoader&&)                 = delete;
    AssetLoader& operator=(AssetLoader&&)      = delete;

    // Returns a shared handle to the texture. The same path returned twice
    // resolves to the same GPU resource. On failure, the result holds an
    // error message and the handle is null.
    Engine::Core::Result<TextureHandle> LoadTexture(std::string_view relativePath);

    // For Renderer's "is this a known-bad path?" path. Game code shouldn't
    // need this — it gets a null handle from a failed LoadTexture call.
    const std::string& BasePath() const noexcept { return basePath_; }

private:
    SDL_Renderer*                                            renderer_;
    std::string                                              basePath_;
    std::unordered_map<std::string, std::weak_ptr<Texture>>  cache_;
};

}  // namespace Engine::Assets
