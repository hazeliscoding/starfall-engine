#include "engine/runtime/application.hpp"

int main(int /*argc*/, char** /*argv*/) {
    Engine::Runtime::Application app;
    app.Configure({
        .title        = "Starfall",
        .windowWidth  = 1280,
        .windowHeight = 720,
    });
    return app.Run();
}
