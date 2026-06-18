// tests/common/sdl_fixture.hpp
//
// Header-only RAII fixtures for tests that need SDL initialized. Two
// flavors:
//
//   SdlWindowFixture          - window only. Use when the test itself
//                               constructs an Engine::Render::Renderer
//                               (which creates the SDL_Renderer inside).
//   SdlWindowAndRendererFixture - window + raw SDL_Renderer. Use when the
//                               test needs an SDL_Renderer* directly (e.g.
//                               to construct an AssetLoader).
//
// SDL_Init/SDL_Quit are refcounted, so multiple fixtures across tests are
// safe.

#pragma once

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Engine::Tests {

class SdlWindowFixture {
public:
    SdlWindowFixture() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        window_ = SDL_CreateWindow("starfall-test", 1, 1, SDL_WINDOW_HIDDEN);
        if (window_ == nullptr) {
            SDL_Quit();
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        }
    }

    ~SdlWindowFixture() {
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    SdlWindowFixture(const SdlWindowFixture&)            = delete;
    SdlWindowFixture& operator=(const SdlWindowFixture&) = delete;

    SDL_Window* Window() const noexcept { return window_; }

private:
    SDL_Window* window_ = nullptr;
};

class SdlWindowAndRendererFixture {
public:
    SdlWindowAndRendererFixture() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        window_ = SDL_CreateWindow("starfall-test", 1, 1, SDL_WINDOW_HIDDEN);
        if (window_ == nullptr) {
            SDL_Quit();
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        }
        renderer_ = SDL_CreateRenderer(window_, nullptr);
        if (renderer_ == nullptr) {
            SDL_DestroyWindow(window_);
            SDL_Quit();
            throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        }
    }

    ~SdlWindowAndRendererFixture() {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_)   SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    SdlWindowAndRendererFixture(const SdlWindowAndRendererFixture&)            = delete;
    SdlWindowAndRendererFixture& operator=(const SdlWindowAndRendererFixture&) = delete;

    SDL_Renderer* Renderer() const noexcept { return renderer_; }
    SDL_Window*   Window()   const noexcept { return window_;   }

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace Engine::Tests
