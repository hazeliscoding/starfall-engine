#pragma once

#include "engine/input/action_map.hpp"

#include <string_view>
#include <utility>

union SDL_Event;

namespace Engine::Input {

// Frame-coherent input state. Edge-triggered IsPressed/IsReleased fire for
// exactly one frame around the up→down and down→up transitions; IsHeld is
// the steady-state query.
//
// Per-frame contract (driven by the runtime):
//   1. BeginFrame()                 // snapshot prev = now
//   2. for each SDL_Event: OnEvent(e)
//   3. game's onUpdate uses IsHeld/IsPressed/IsReleased
//
// Within a single frame every query returns the same value.
class InputState {
public:
    // Stores the ActionMap by value so the caller can drop their copy.
    explicit InputState(ActionMap map);

    void BeginFrame();
    void OnEvent(const SDL_Event& event);

    bool IsHeld(std::string_view action)     const;
    bool IsPressed(std::string_view action)  const;
    bool IsReleased(std::string_view action) const;

    const ActionMap& Map() const noexcept { return map_; }

private:
    // Per-action {now, prev} states. now mutates as events arrive; prev
    // snapshots once per frame from BeginFrame.
    struct Edge { bool now = false; bool prev = false; };

    void EnsureRow(std::string_view action);

    ActionMap                        map_;
    mutable StringKeyedMap<Edge>     state_;  // mutable: lazy row creation on query
};

}  // namespace Engine::Input
