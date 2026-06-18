#include "engine/input/input_state.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

namespace Engine::Input {

InputState::InputState(ActionMap map) : map_(std::move(map)) {
    // Pre-populate state_ with one Edge row per known action so OnEvent
    // doesn't have to insert during the hot path and queries against
    // never-pressed actions return false uniformly (rather than missing
    // entirely).
    for (const auto& [action, _bindings] : map_.Entries()) {
        state_.emplace(action, Edge{});
    }
}

void InputState::BeginFrame() {
    for (auto& [_, edge] : state_) {
        edge.prev = edge.now;
    }
}

void InputState::OnEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
        return;
    }
    // OS auto-repeat would re-fire IsPressed every frame the key is held.
    // We already have steady-state Held; skip repeat events.
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat != 0) {
        return;
    }

    const SDL_Scancode scancode = event.key.scancode;
    const bool isDown = (event.type == SDL_EVENT_KEY_DOWN);

    // Walk every known action and flip the Edge for any whose bindings
    // include this scancode. The same physical key may belong to multiple
    // actions (e.g. W could bind to both MoveUp and a future "Up" menu nav).
    for (const auto& [action, bindings] : map_.Entries()) {
        if (std::find(bindings.begin(), bindings.end(), scancode) != bindings.end()) {
            state_[action].now = isDown;
        }
    }
}

bool InputState::IsHeld(std::string_view action) const {
    // libstdc++ 10 lacks heterogeneous find — materialize. See action_map.cpp.
    auto it = state_.find(std::string{action});
    return it != state_.end() && it->second.now;
}

bool InputState::IsPressed(std::string_view action) const {
    auto it = state_.find(std::string{action});
    return it != state_.end() && it->second.now && !it->second.prev;
}

bool InputState::IsReleased(std::string_view action) const {
    auto it = state_.find(std::string{action});
    return it != state_.end() && !it->second.now && it->second.prev;
}

void InputState::EnsureRow(std::string_view action) {
    std::string key{action};
    if (state_.find(key) == state_.end()) {
        state_.emplace(std::move(key), Edge{});
    }
}

}  // namespace Engine::Input
