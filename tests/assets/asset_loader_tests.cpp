// tests/assets/asset_loader_tests.cpp
//
// Backfills two M1-deferred items: AssetLoader caches by path (second
// LoadTexture call returns the same handle without re-decoding), and a
// missing path returns Err rather than crashing.

#include "common/sdl_fixture.hpp"
#include "engine/assets/asset_loader.hpp"

#include <catch2/catch_test_macros.hpp>

using Engine::Assets::AssetLoader;
using Engine::Tests::SdlWindowAndRendererFixture;

TEST_CASE("AssetLoader: second load returns the cached handle",
          "[assets][asset_loader]") {
    SdlWindowAndRendererFixture sdl;
    AssetLoader loader{sdl.Renderer()};

    auto first = loader.LoadTexture("assets/characters/iden_placeholder.png");
    REQUIRE(first.IsOk());
    auto firstHandle = first.Value();
    REQUIRE(firstHandle);

    auto second = loader.LoadTexture("assets/characters/iden_placeholder.png");
    REQUIRE(second.IsOk());
    auto secondHandle = second.Value();

    // Same underlying Texture instance (cached weak_ptr resolved live).
    CHECK(firstHandle.get() == secondHandle.get());
    // Same SDL_Texture pointer too — no re-decode.
    CHECK(firstHandle->RawHandle() == secondHandle->RawHandle());
}

TEST_CASE("AssetLoader: missing path returns Err with the path",
          "[assets][asset_loader]") {
    SdlWindowAndRendererFixture sdl;
    AssetLoader loader{sdl.Renderer()};

    auto result = loader.LoadTexture("assets/does/not/exist.png");
    REQUIRE(result.IsErr());
    // The error message should mention the missing path so a developer
    // can trace it.
    const std::string& msg = result.Error();
    CHECK(msg.find("does/not/exist.png") != std::string::npos);
}

TEST_CASE("AssetLoader: handle drops free the texture, re-load succeeds",
          "[assets][asset_loader]") {
    SdlWindowAndRendererFixture sdl;
    AssetLoader loader{sdl.Renderer()};

    // Earlier draft of this test asserted that the SDL_Texture* after a
    // re-load differed from the one before the handle drop. That's
    // brittle — SDL's allocator can reuse the same memory address for a
    // freshly-created texture (verified on SDL3 3.4.10 / Linux). The
    // contract is "after all handles drop, the cache entry expires and
    // the next load re-decodes" — best verified by the two REQUIRE
    // calls succeeding on either side of the scope.

    std::weak_ptr<Engine::Assets::Texture> weak;

    {
        auto handle = loader.LoadTexture("assets/characters/iden_placeholder.png");
        REQUIRE(handle.IsOk());
        weak = handle.Value();
        // shared_ptr is alive in `weak` via the handle.
        CHECK_FALSE(weak.expired());
    }  // handle dropped — last strong ref gone

    // Cache held a weak_ptr, so the weak reference must now be expired.
    CHECK(weak.expired());

    // Re-load succeeds (proves the cache cleanup path didn't leave a
    // broken entry around).
    auto reloaded = loader.LoadTexture("assets/characters/iden_placeholder.png");
    REQUIRE(reloaded.IsOk());
}
