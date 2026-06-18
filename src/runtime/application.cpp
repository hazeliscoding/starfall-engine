#include "engine/runtime/application.hpp"

#include "engine/assets/asset_loader.hpp"
#include "engine/core/logger.hpp"
#include "engine/render/renderer.hpp"

#include <SDL3/SDL.h>

#include <exception>
#include <memory>
#include <utility>

namespace Engine::Runtime {

Application::Application()  = default;
Application::~Application() = default;

void Application::Configure(AppConfig config) {
    config_     = std::move(config);
    configured_ = true;
}

int Application::Run() {
    if (!configured_) {
        SF_LOG_ERROR("Core",
            "Application::Run called before Configure. Refusing to open a window.");
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SF_LOG_ERROR("Runtime", "SDL_Init(VIDEO) failed: %s", SDL_GetError());
        return 2;
    }

    SDL_Window* window = SDL_CreateWindow(
        config_.title.c_str(),
        config_.windowWidth,
        config_.windowHeight,
        SDL_WINDOW_RESIZABLE);

    if (window == nullptr) {
        SF_LOG_ERROR("Runtime", "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    auto renderer = std::make_unique<Engine::Render::Renderer>(
        window, config_.logicalWidth, config_.logicalHeight);
    if (!renderer->IsValid()) {
        SF_LOG_ERROR("Runtime", "Renderer init failed; aborting Run()");
        renderer.reset();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 4;
    }

    auto assetLoader = std::make_unique<Engine::Assets::AssetLoader>(
        renderer->RawHandle());

    SF_LOG_INFO("Runtime", "Window opened: \"%s\" (%dx%d, logical %dx%d)",
                config_.title.c_str(),
                config_.windowWidth, config_.windowHeight,
                config_.logicalWidth, config_.logicalHeight);

    // One-time setup phase (design D11). If onStart throws, abort cleanly
    // without entering the frame loop.
    if (config_.onStart) {
        try {
            config_.onStart(*renderer, *assetLoader);
        } catch (const std::exception& e) {
            SF_LOG_ERROR("Runtime", "onStart threw: %s", e.what());
            assetLoader.reset();
            renderer.reset();
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 5;
        }
    }

    bool running = true;
    while (running) {
        // Drain all pending events (close, resize, etc.). Frame pacing is
        // implicit via vsync inside SDL_RenderPresent.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        if (!running) break;

        renderer->Clear(config_.clearColor);

        if (config_.onRender) {
            try {
                config_.onRender(*renderer);
            } catch (const std::exception& e) {
                SF_LOG_ERROR("Runtime", "onRender threw: %s", e.what());
                // Frame continues; user can see the log and close normally.
            }
        }

        renderer->Present();
    }

    // Reverse-construction-order teardown.
    assetLoader.reset();
    renderer.reset();
    SDL_DestroyWindow(window);
    SDL_Quit();
    SF_LOG_INFO("Runtime", "Clean shutdown.");
    return 0;
}

}  // namespace Engine::Runtime
