#include "engine/input/action_map.hpp"

#include <utility>

namespace Engine::Input {

const std::vector<SDL_Scancode> ActionMap::kEmpty{};

ActionMap::ActionMap() {
    // M2 game action set.
    Bind("MoveUp",    SDL_SCANCODE_W);
    Bind("MoveUp",    SDL_SCANCODE_UP);
    Bind("MoveDown",  SDL_SCANCODE_S);
    Bind("MoveDown",  SDL_SCANCODE_DOWN);
    Bind("MoveLeft",  SDL_SCANCODE_A);
    Bind("MoveLeft",  SDL_SCANCODE_LEFT);
    Bind("MoveRight", SDL_SCANCODE_D);
    Bind("MoveRight", SDL_SCANCODE_RIGHT);
    Bind("Confirm",   SDL_SCANCODE_RETURN);
    Bind("Confirm",   SDL_SCANCODE_SPACE);
    Bind("Cancel",    SDL_SCANCODE_ESCAPE);
}

void ActionMap::Bind(std::string action, SDL_Scancode key) {
    bindings_[std::move(action)].push_back(key);
}

const std::vector<SDL_Scancode>& ActionMap::Bindings(std::string_view action) const {
    // Heterogeneous unordered_map::find requires libstdc++ 11+ (P0919, GCC
    // 11+). Materialize a std::string for compatibility with libstdc++ 10
    // (Ubuntu 20.04's g++-10 ships with it). MSVC's STL supports the
    // heterogeneous overload natively, so this is a one-allocation per
    // query everywhere; revisit if profiling flags it.
    auto it = bindings_.find(std::string{action});
    if (it == bindings_.end()) {
        return kEmpty;
    }
    return it->second;
}

}  // namespace Engine::Input
