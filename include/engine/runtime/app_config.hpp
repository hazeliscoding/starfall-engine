#pragma once

#include <string>

namespace Engine::Runtime {

// Plain aggregate per design D7. Use C++20 designated initializers from the
// game entry point:
//   app.Configure({.title = "Starfall", .windowWidth = 1280, .windowHeight = 720});
struct AppConfig {
    std::string title         = "Starfall";
    int         windowWidth   = 1280;
    int         windowHeight  = 720;

    // Reserved for milestones M4+ (scene loading). Unused at M0.
    std::string initialScene;
};

}  // namespace Engine::Runtime
