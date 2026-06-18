#pragma once

#include <SDL3/SDL_scancode.h>

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Engine::Input {

// Heterogeneous string hashing so IsHeld("MoveUp") doesn't allocate.
// std::equal_to<> is already transparent (C++14+).
struct StringHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
    std::size_t operator()(const std::string& s) const noexcept { return std::hash<std::string_view>{}(s); }
    std::size_t operator()(const char* s)        const noexcept { return std::hash<std::string_view>{}(std::string_view{s}); }
};

template <typename V>
using StringKeyedMap = std::unordered_map<std::string, V, StringHash, std::equal_to<>>;

// Maps named actions ("MoveUp", "Confirm", ...) to a list of SDL scancodes.
// The same scancode may be bound to multiple actions; an action may have
// zero bindings (reserved-but-unused is legal).
class ActionMap {
public:
    // Default ctor populates the M2 game's action set per the input-actions
    // spec: MoveUp/Down/Left/Right on WASD + arrows; Confirm on Enter+Space;
    // Cancel on Escape.
    ActionMap();

    // Empty map (no bindings) — useful in tests.
    static ActionMap Empty() noexcept { return ActionMap{Tag::EmptyTag}; }

    void Bind(std::string action, SDL_Scancode key);

    // Returns the empty vector if the action has no bindings.
    const std::vector<SDL_Scancode>& Bindings(std::string_view action) const;

    // Iteration surface for engine_runtime / InputState: gives them every
    // action name and its bindings without exposing the storage type.
    const StringKeyedMap<std::vector<SDL_Scancode>>& Entries() const noexcept { return bindings_; }

private:
    enum class Tag { EmptyTag };
    explicit ActionMap(Tag) noexcept {}

    StringKeyedMap<std::vector<SDL_Scancode>> bindings_;

    static const std::vector<SDL_Scancode> kEmpty;
};

}  // namespace Engine::Input
