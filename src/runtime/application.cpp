#include "engine/runtime/application.hpp"

#include "engine/core/logger.hpp"

#include <SDL3/SDL.h>

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
        /*flags*/ 0);

    if (window == nullptr) {
        SF_LOG_ERROR("Runtime", "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    SF_LOG_INFO("Runtime", "Window opened: \"%s\" (%dx%d)",
                config_.title.c_str(), config_.windowWidth, config_.windowHeight);

    bool running = true;
    while (running) {
        SDL_Event event;
        // WaitEventTimeout yields to the OS until input arrives or the
        // timeout elapses — keeps an idle window off a busy-spin (design D8).
        if (SDL_WaitEventTimeout(&event, /*timeoutMS*/ 16)) {
            do {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                    break;
                }
            } while (SDL_PollEvent(&event));
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    SF_LOG_INFO("Runtime", "Clean shutdown.");
    return 0;
}

}  // namespace Engine::Runtime
