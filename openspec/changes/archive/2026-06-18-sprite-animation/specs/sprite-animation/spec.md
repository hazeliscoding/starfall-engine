## ADDED Requirements

### Requirement: Animation Clip Data Shape

`engine_render` SHALL provide an `Engine::Render::AnimationClip` aggregate carrying a `std::vector<Engine::Assets::TextureHandle> frames`, a `float frameDuration` (seconds per frame, default 0.125), and a `bool looping` (default true). Constructing a clip with zero frames is legal but a clip with zero frames produces a null `CurrentFrame()`.

#### Scenario: Construct a 4-frame walking clip

- **WHEN** game code creates a clip with 4 TextureHandles, `frameDuration = 0.125f`, `looping = true`
- **THEN** the clip can be passed to `AnimatedSprite::AddClip` and queried as the current clip after `Play`

#### Scenario: Zero-frame clip is constructible but produces null current frame

- **WHEN** game code constructs a clip with an empty frames vector
- **THEN** the clip is valid to add; an `AnimatedSprite` playing it returns a null handle from `CurrentFrame()` and does not crash

### Requirement: AnimatedSprite Owns Named Clips

`Engine::Render::AnimatedSprite` SHALL store an arbitrary number of named `AnimationClip`s and SHALL expose `AddClip(std::string name, AnimationClip clip)` to register them. Re-adding a clip under an existing name SHALL replace the previous clip.

#### Scenario: Add and switch between two clips

- **WHEN** game code adds `idle_down` and `walk_down` clips, calls `Play("idle_down")`, then `Play("walk_down")`
- **THEN** `CurrentClip()` returns `"walk_down"` after the second `Play`, and `CurrentFrame()` returns a frame from the walk clip

#### Scenario: Re-adding a clip under an existing name replaces it

- **WHEN** game code calls `AddClip("walk_down", oldClip)` and later `AddClip("walk_down", newClip)`
- **THEN** subsequent `Play("walk_down")` uses the new clip's frames

### Requirement: Play and Frame Advance

`AnimatedSprite::Play(name)` SHALL select a clip as current and (re)set the frame index to 0 AND reset the internal timer to 0, UNLESS the named clip is already the current clip AND playing — in that case `Play` SHALL be a no-op to preserve walk-cycle continuity when the game calls `Play("walk_down")` every frame the player moves left to left.

`AnimatedSprite::Update(float dt)` SHALL advance the internal timer by `dt`. Each time the timer crosses `frameDuration`, the frame index advances by one (subtracting `frameDuration` from the timer per step). For a looping clip, the frame index wraps modulo the frame count. For a non-looping clip, the frame index clamps at the last frame and the sprite enters a `paused_` state at the end.

#### Scenario: Initial Play starts at frame 0

- **WHEN** `AddClip("walk_down", clipWith3Frames)` is followed by `Play("walk_down")`
- **THEN** `CurrentFrame()` returns the first of the three frames before any `Update` is called

#### Scenario: Update advances at the configured rate

- **WHEN** a 4-frame looping clip with `frameDuration = 0.1f` is playing and game code calls `Update(0.1f)`
- **THEN** the frame index transitions from 0 to 1; calling `Update(0.1f)` again moves it to 2

#### Scenario: Looping wraps

- **WHEN** the same 4-frame looping clip's frame index is at 3 and `Update(0.1f)` is called
- **THEN** the frame index wraps to 0

#### Scenario: Non-looping clamps and pauses

- **WHEN** a 3-frame `looping = false` clip's frame index is at 2 and `Update(0.5f)` is called
- **THEN** the frame index stays at 2 and `IsPlaying()` returns false

#### Scenario: Replay of currently-playing clip is a no-op

- **WHEN** a clip is mid-play (frame index 2 of 4) and game code calls `Play("walk_down")` again
- **THEN** the frame index stays at 2, the timer is unchanged, and the cycle continues from where it was

#### Scenario: Switching to a different clip resets timer and frame index

- **WHEN** game code calls `Play("walk_down")` then `Play("idle_down")`
- **THEN** the new clip starts at frame 0 with timer 0 regardless of the previous clip's state

### Requirement: Pause and Resume

`AnimatedSprite::Pause()` SHALL freeze the current frame so subsequent `Update` calls do not advance it. `Resume()` SHALL un-freeze. `IsPlaying()` returns false while paused. `Play(name)` on a new clip SHALL implicitly resume.

#### Scenario: Pause freezes the current frame

- **WHEN** game code calls `Pause()` and then `Update(1.0f)` many times
- **THEN** the frame index stays at whatever it was when Pause was called

#### Scenario: Resume continues from where Pause left off

- **WHEN** game code Pauses at frame 2, then Resumes, then calls `Update(frameDuration)`
- **THEN** the frame index advances to 3

#### Scenario: Play on a different clip implicitly resumes

- **WHEN** the sprite is paused on clip A and game code calls `Play("B")`
- **THEN** `IsPlaying()` returns true and Update advances B's frames

### Requirement: CurrentFrame Reflects The Current Index

`AnimatedSprite::CurrentFrame()` SHALL return the `TextureHandle` at the current frame index of the current clip. If no clip is currently selected (no `Play` ever called, or the named clip doesn't exist), `CurrentFrame()` SHALL return a null `TextureHandle`.

#### Scenario: No clip selected returns null

- **WHEN** `CurrentFrame()` is called on a freshly-constructed `AnimatedSprite` with no `Play` invocation
- **THEN** the returned handle is null (`shared_ptr` evaluates to false)

#### Scenario: Play with unknown name returns null current frame

- **WHEN** game code calls `Play("does_not_exist")` without having added a clip by that name
- **THEN** the call is a no-op (logged once with an `[Render]` warning) and `CurrentFrame()` returns null until a valid Play is issued

#### Scenario: Frame index moves the returned handle

- **WHEN** a clip with three distinct TextureHandles is playing and Update advances the index from 0 to 1
- **THEN** `CurrentFrame()` returns the second handle on the second call (the one at index 1)
