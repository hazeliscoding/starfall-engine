#include "engine/runtime/application.hpp"

#include "engine/assets/asset_loader.hpp"
#include "engine/core/logger.hpp"
#include "engine/input/action_map.hpp"
#include "engine/input/input_state.hpp"
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

    auto input = std::make_unique<Engine::Input::InputState>(
        Engine::Input::ActionMap{});

    SF_LOG_INFO("Runtime", "Window opened: \"%s\" (%dx%d, logical %dx%d)",
                config_.title.c_str(),
                config_.windowWidth, config_.windowHeight,
                config_.logicalWidth, config_.logicalHeight);

    // One-time setup phase (design D11). onStart exception → clean teardown,
    // no frame loop.
    if (config_.onStart) {
        try {
            config_.onStart(*renderer, *assetLoader);
        } catch (const std::exception& e) {
            SF_LOG_ERROR("Runtime", "onStart threw: %s", e.what());
            input.reset();
            assetLoader.reset();
            renderer.reset();
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 5;
        }
    }

    bool running = true;
    Uint64 prevTicks = 0;  // 0 = first-frame sentinel
    const Uint64 perfFreq = SDL_GetPerformanceFrequency();

    while (running) {
        // ---------------- Frame begin ----------------
        const Uint64 nowTicks = SDL_GetPerformanceCounter();
        const float dt = (prevTicks == 0)
            ? 0.0f
            : static_cast<float>(static_cast<double>(nowTicks - prevTicks) /
                                 static_cast<double>(perfFreq));
        prevTicks = nowTicks;

        input->BeginFrame();

        // Drain OS events: route input to input_state, watch for close.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // Pass every event through input — it filters to keyboard events
            // internally and ignores the rest.
            input->OnEvent(event);
        }
        if (!running) break;

        // ---------------- Update ----------------
        bool updateThrew = false;
        if (config_.onUpdate) {
            try {
                config_.onUpdate(dt, *input);
            } catch (const std::exception& e) {
                SF_LOG_ERROR("Runtime", "onUpdate threw: %s", e.what());
                updateThrew = true;
            }
        }

        // ---------------- Render ----------------
        renderer->Clear(config_.clearColor);

        if (!updateThrew && config_.onRender) {
            try {
                config_.onRender(*renderer);
            } catch (const std::exception& e) {
                SF_LOG_ERROR("Runtime", "onRender threw: %s", e.what());
            }
        }

        renderer->Present();
    }

    // Reverse-construction-order teardown.
    input.reset();
    assetLoader.reset();
    renderer.reset();
    SDL_DestroyWindow(window);
    SDL_Quit();
    SF_LOG_INFO("Runtime", "Clean shutdown.");
    return 0;
}

}  // namespace Engine::Runtime
