#pragma once

#include <memory>

struct SDL_Texture;

namespace Engine::Assets {

// Opaque GPU-side texture. The destructor releases the SDL_Texture.
class Texture {
public:
    Texture(SDL_Texture* tex, int w, int h) noexcept;
    ~Texture();

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&)                 = delete;
    Texture& operator=(Texture&&)      = delete;

    // Renderer needs the raw handle to issue draw calls. Game code never does.
    SDL_Texture* RawHandle() const noexcept { return tex_; }

    int Width()  const noexcept { return width_; }
    int Height() const noexcept { return height_; }

private:
    SDL_Texture* tex_    = nullptr;
    int          width_  = 0;
    int          height_ = 0;
};

// Shared handle for game code. nullptr is a valid sentinel for "failed
// load" / "invalid handle"; rendering it is a noop (Renderer handles this).
using TextureHandle = std::shared_ptr<Texture>;

}  // namespace Engine::Assets
