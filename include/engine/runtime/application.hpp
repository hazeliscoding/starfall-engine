#pragma once

#include "engine/runtime/app_config.hpp"

namespace Engine::Runtime {

// Application owns the engine lifecycle: configure -> initialize -> run loop
// -> shutdown. Game code drives the engine ONLY through this surface
// (runtime-window spec).
class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    // Stores a copy of the config to use during Run(). Calling this more than
    // once before Run() is allowed; only the last call wins.
    void Configure(AppConfig config);

    // Initializes subsystems, enters the main loop, returns an exit code.
    // Returns 0 on clean shutdown via window-close. Returns non-zero on any
    // error (including "Configure was never called").
    int Run();

private:
    AppConfig config_;
    bool      configured_ = false;
};

}  // namespace Engine::Runtime
