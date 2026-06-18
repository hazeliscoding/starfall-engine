#include "engine/assets/asset_loader.hpp"
#include "engine/render/renderer.hpp"
#include "engine/runtime/application.hpp"

int main(int /*argc*/, char** /*argv*/) {
    Engine::Runtime::Application app;

    // Captured by reference into the lambdas: onStart loads the texture
    // once, onRender draws it centered each frame.
    Engine::Assets::TextureHandle iden;

    app.Configure({
        .title        = "Starfall",
        .windowWidth  = 1280,
        .windowHeight = 720,
        .onStart = [&iden](Engine::Render::Renderer& /*renderer*/,
                           Engine::Assets::AssetLoader& loader) {
            auto result = loader.LoadTexture("assets/characters/iden_placeholder.png");
            if (result.IsOk()) {
                iden = result.Value();
            }
            // On failure, iden stays null and DrawSprite logs a single warning.
        },
        .onRender = [&iden](Engine::Render::Renderer& renderer) {
            if (!iden) return;
            // Center on the 320x180 logical surface.
            const float w = static_cast<float>(iden->Width());
            const float h = static_cast<float>(iden->Height());
            renderer.DrawSprite(iden, {160.0f - w * 0.5f, 90.0f - h * 0.5f});
        },
    });

    return app.Run();
}
