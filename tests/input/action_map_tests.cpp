#include "engine/input/action_map.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using Engine::Input::ActionMap;

TEST_CASE("ActionMap: bind and query a scancode", "[input][action_map]") {
    auto map = ActionMap::Empty();
    map.Bind("Jump", SDL_SCANCODE_SPACE);
    map.Bind("Jump", SDL_SCANCODE_RETURN);

    const auto& bindings = map.Bindings("Jump");
    REQUIRE(bindings.size() == 2);
    CHECK(std::find(bindings.begin(), bindings.end(), SDL_SCANCODE_SPACE) != bindings.end());
    CHECK(std::find(bindings.begin(), bindings.end(), SDL_SCANCODE_RETURN) != bindings.end());
}

TEST_CASE("ActionMap: unknown action returns empty bindings", "[input][action_map]") {
    auto map = ActionMap::Empty();
    CHECK(map.Bindings("DoesNotExist").empty());
}

TEST_CASE("ActionMap: default ctor binds the M2 game action set", "[input][action_map]") {
    ActionMap map;  // default ctor — should be populated

    SECTION("MoveUp bound to W and Up arrow") {
        const auto& b = map.Bindings("MoveUp");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_W)  != b.end());
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_UP) != b.end());
    }
    SECTION("MoveDown bound to S and Down arrow") {
        const auto& b = map.Bindings("MoveDown");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_S)    != b.end());
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_DOWN) != b.end());
    }
    SECTION("MoveLeft bound to A and Left arrow") {
        const auto& b = map.Bindings("MoveLeft");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_A)    != b.end());
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_LEFT) != b.end());
    }
    SECTION("MoveRight bound to D and Right arrow") {
        const auto& b = map.Bindings("MoveRight");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_D)     != b.end());
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_RIGHT) != b.end());
    }
    SECTION("Confirm bound to Return and Space") {
        const auto& b = map.Bindings("Confirm");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_RETURN) != b.end());
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_SPACE)  != b.end());
    }
    SECTION("Cancel bound to Escape") {
        const auto& b = map.Bindings("Cancel");
        CHECK(std::find(b.begin(), b.end(), SDL_SCANCODE_ESCAPE) != b.end());
    }
}
