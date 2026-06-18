// tests/render/renderer_tests.cpp
//
// Backfill of M1's deferred null-handle DrawSprite test: passing a null
// TextureHandle must be a logged noop, not a crash.

#include "common/sdl_fixture.hpp"
#include "engine/assets/texture.hpp"
#include "engine/render/renderer.hpp"

#include <catch2/catch_test_macros.hpp>

using Engine::Render::Renderer;
using Engine::Tests::SdlWindowFixture;

TEST_CASE("Renderer::DrawSprite with null handle does not crash",
          "[render][drawsprite]") {
    SdlWindowFixture sdl;
    Renderer renderer{sdl.Window(), 320, 180};
    REQUIRE(renderer.IsValid());

    // Null TextureHandle — the spec says this is a logged noop.
    Engine::Assets::TextureHandle null_handle;  // default-constructed shared_ptr == nullptr

    // Issue the draw twice — the second call exercises the rate-limited
    // warning path. Neither call should crash or throw.
    REQUIRE_NOTHROW(renderer.DrawSprite(null_handle, {0.0f, 0.0f}));
    REQUIRE_NOTHROW(renderer.DrawSprite(null_handle, {10.0f, 10.0f}));
}

TEST_CASE("Renderer::Camera default state", "[render][camera]") {
    SdlWindowFixture sdl;
    Renderer renderer{sdl.Window(), 320, 180};
    REQUIRE(renderer.IsValid());

    const auto& cam = renderer.Camera();
    CHECK(cam.position.x == 0.0f);
    CHECK(cam.position.y == 0.0f);
    CHECK(cam.zoom == 1.0f);
}

TEST_CASE("Renderer::Camera mutation persists", "[render][camera]") {
    SdlWindowFixture sdl;
    Renderer renderer{sdl.Window(), 320, 180};
    REQUIRE(renderer.IsValid());

    renderer.Camera().position = {25.0f, -10.0f};
    renderer.Camera().zoom = 2.0f;

    CHECK(renderer.Camera().position.x == 25.0f);
    CHECK(renderer.Camera().position.y == -10.0f);
    CHECK(renderer.Camera().zoom == 2.0f);
}
