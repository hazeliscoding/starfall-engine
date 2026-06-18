## ADDED Requirements

### Requirement: Action Map

`engine_input` SHALL provide an `Engine::Input::ActionMap` that maps an action name (`std::string` or `std::string_view`) to a list of bound `SDL_Scancode` keys. The same physical key MAY be bound to multiple actions, and an action MAY have zero or more bindings.

#### Scenario: Bind and query

- **WHEN** game code constructs an `ActionMap`, binds `"MoveUp"` to `SDL_SCANCODE_W` and `SDL_SCANCODE_UP`, then asks the map for `"MoveUp"`'s bindings
- **THEN** the returned list contains exactly those two scancodes

#### Scenario: Unknown action

- **WHEN** game code queries bindings for an action it never bound
- **THEN** the returned list is empty (no exception, no crash)

#### Scenario: Default game action set

- **WHEN** an `ActionMap` is default-constructed by the game (`games/my_rpg`)
- **THEN** it MUST contain at minimum: `MoveUp` (W + Up), `MoveDown` (S + Down), `MoveLeft` (A + Left), `MoveRight` (D + Right), `Confirm` (Enter + Space), `Cancel` (Escape)

### Requirement: Frame-Coherent Input State

`Engine::Input::InputState` SHALL expose per-action queries `IsHeld(action)`, `IsPressed(action)`, `IsReleased(action)`. `IsHeld` is true on every frame the key is down; `IsPressed` and `IsReleased` are edge-triggered and true for exactly one frame each. Within a single frame, every call to these methods MUST return the same value (frame-coherent).

#### Scenario: Held query while key down

- **WHEN** the player holds the `W` key for 10 frames AND `ActionMap` binds `MoveUp` to `W`
- **THEN** `IsHeld("MoveUp")` returns true on each of those 10 frames

#### Scenario: Press edge fires once

- **WHEN** the player presses `W` (it transitions from up to down) at frame N
- **THEN** `IsPressed("MoveUp")` returns true on frame N only; on frame N+1 and beyond (while still held), `IsPressed` returns false but `IsHeld` returns true

#### Scenario: Release edge fires once

- **WHEN** the player releases `W` (it transitions from down to up) at frame N
- **THEN** `IsReleased("MoveUp")` returns true on frame N only; on frame N+1 `IsReleased` returns false and `IsHeld` returns false

#### Scenario: Multiple queries within a frame agree

- **WHEN** game code calls `IsPressed("Confirm")` three times during a single `onUpdate` invocation
- **THEN** all three calls return the same value

#### Scenario: Unbound action queries return false

- **WHEN** game code queries any state method for an action with no bindings
- **THEN** the result is false; no warning is logged (silent allows reserved-but-unbound actions)

### Requirement: Event Dispatch Contract

`engine_runtime` SHALL drain all pending `SDL_Event`s each frame and call `InputState::OnEvent(event)` for each event before invoking the game's `onUpdate` callback. The runtime SHALL call `InputState::BeginFrame()` exactly once per frame before draining events, which resets edge-triggered (`Pressed`/`Released`) state.

#### Scenario: Events flow into state before onUpdate

- **WHEN** the OS dispatches a `SDL_EVENT_KEY_DOWN` for `W` during frame N
- **THEN** by the time the runtime calls `onUpdate` on frame N, `IsPressed("MoveUp")` returns true

#### Scenario: BeginFrame resets edge state

- **WHEN** `IsPressed("MoveUp")` was true on frame N (from a key-down event) and no new key-down event arrives on frame N+1
- **THEN** after `BeginFrame()` runs on frame N+1, `IsPressed("MoveUp")` returns false (`IsHeld` may still be true)

#### Scenario: Events outside the action map are ignored, not errors

- **WHEN** an `SDL_EVENT_KEY_DOWN` arrives for a scancode bound to no action
- **THEN** `OnEvent` accepts and consumes it without logging; the state simply records nothing
