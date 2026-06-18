#include "engine/input/action_map.hpp"
#include "engine/input/input_state.hpp"

#include <SDL3/SDL.h>

#include <catch2/catch_test_macros.hpp>

using Engine::Input::ActionMap;
using Engine::Input::InputState;

namespace {

// Synthesize an SDL key event for tests without needing a real window.
SDL_Event MakeKeyEvent(Uint32 type, SDL_Scancode scancode, bool repeat = false) {
    SDL_Event e{};
    e.type = type;
    e.key.scancode = scancode;
    e.key.repeat = repeat ? 1 : 0;
    return e;
}

}  // namespace

TEST_CASE("InputState: held while key down across frames", "[input][input_state]") {
    InputState input{ActionMap{}};  // default action set: MoveUp on W/Up

    // Frame 1: key down arrives.
    input.BeginFrame();
    auto down = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W);
    input.OnEvent(down);

    CHECK(input.IsHeld("MoveUp"));
    CHECK(input.IsPressed("MoveUp"));    // edge fires this frame
    CHECK_FALSE(input.IsReleased("MoveUp"));

    // Frame 2: no new events, still held.
    input.BeginFrame();
    CHECK(input.IsHeld("MoveUp"));
    CHECK_FALSE(input.IsPressed("MoveUp"));   // edge already consumed
    CHECK_FALSE(input.IsReleased("MoveUp"));

    // Frame 3: key up arrives.
    input.BeginFrame();
    auto up = MakeKeyEvent(SDL_EVENT_KEY_UP, SDL_SCANCODE_W);
    input.OnEvent(up);

    CHECK_FALSE(input.IsHeld("MoveUp"));
    CHECK_FALSE(input.IsPressed("MoveUp"));
    CHECK(input.IsReleased("MoveUp"));    // release edge fires

    // Frame 4: edge consumed.
    input.BeginFrame();
    CHECK_FALSE(input.IsHeld("MoveUp"));
    CHECK_FALSE(input.IsPressed("MoveUp"));
    CHECK_FALSE(input.IsReleased("MoveUp"));
}

TEST_CASE("InputState: OS auto-repeat does not re-fire Pressed", "[input][input_state]") {
    InputState input{ActionMap{}};

    input.BeginFrame();
    auto first = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, /*repeat*/ false);
    input.OnEvent(first);
    CHECK(input.IsPressed("MoveUp"));

    // Same frame, repeat event arrives — should be ignored.
    auto repeat = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, /*repeat*/ true);
    input.OnEvent(repeat);
    CHECK(input.IsPressed("MoveUp"));  // still pressed (not double-edge-trip)
    CHECK(input.IsHeld("MoveUp"));

    // Next frame, more repeats arrive: still no new Pressed edge.
    input.BeginFrame();
    auto repeat2 = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, /*repeat*/ true);
    input.OnEvent(repeat2);
    CHECK_FALSE(input.IsPressed("MoveUp"));
    CHECK(input.IsHeld("MoveUp"));
}

TEST_CASE("InputState: multiple queries within a frame agree", "[input][input_state]") {
    InputState input{ActionMap{}};

    input.BeginFrame();
    auto down = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_RETURN);
    input.OnEvent(down);

    // Three calls in a row must agree.
    CHECK(input.IsPressed("Confirm"));
    CHECK(input.IsPressed("Confirm"));
    CHECK(input.IsPressed("Confirm"));
}

TEST_CASE("InputState: queries for unbound action return false silently",
          "[input][input_state]") {
    InputState input{ActionMap{}};
    CHECK_FALSE(input.IsHeld("NoSuchAction"));
    CHECK_FALSE(input.IsPressed("NoSuchAction"));
    CHECK_FALSE(input.IsReleased("NoSuchAction"));
}

TEST_CASE("InputState: events outside the action map are ignored",
          "[input][input_state]") {
    // Build a map that ONLY binds Jump to Space — F1 is unbound entirely.
    auto map = ActionMap::Empty();
    map.Bind("Jump", SDL_SCANCODE_SPACE);
    InputState input{std::move(map)};

    input.BeginFrame();
    auto f1down = MakeKeyEvent(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_F1);
    input.OnEvent(f1down);  // must not crash, must not affect any state

    CHECK_FALSE(input.IsHeld("Jump"));
    CHECK_FALSE(input.IsPressed("Jump"));
}
