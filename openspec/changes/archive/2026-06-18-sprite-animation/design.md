## Context

M2 left a real visual bug: Iden's sprite never changes regardless of movement state or direction. Walking up uses the same down-facing idle frame as standing still. The TimeFantasy pack ships everything we need to fix this (12 PNGs per character covering idle + walk × 4 cardinal directions), and DesignDoc §7.7 already names `AnimatedSpriteComponent` as a planned component.

This change inserts an **M2.5: Sprite Animation** slot between M2 and M3 (no renumbering of M3-M10), introduces a standalone `Engine::Render::AnimatedSprite` primitive in `engine_render`, and wires Iden up to use it. The future `AnimatedSpriteComponent` (when scenes land in M3-M4) wraps the same primitive.

One spec frames the work: the new `sprite-animation` capability. No existing capability is modified — `2d-renderer` and `runtime-window` already provide everything needed (DrawSprite takes any TextureHandle, onUpdate already exposes dt to the game).

## Goals / Non-Goals

**Goals:**

- Iden faces the direction of travel and her sprite cycles through walk frames when moving.
- A reusable engine primitive (`AnimatedSprite`) the future scene system + NPCs can consume unchanged.
- Walk-cycle pacing that reads as classic JRPG (~8 fps, ~2 footsteps/sec).
- Frame budget unchanged at idle. One sprite's per-frame Update is sub-microsecond.

**Non-Goals:**

- Animation state machines / transition graphs (DesignDoc §2.2 explicit exclusion).
- Per-frame events (footstep SFX, etc).
- Sprite blending or motion interpolation — frame swap only, pixel-art appropriate.
- Animation-driven movement. Animation reflects state; movement is still driven by input.
- NPC animation in this change. Iden only.
- Scene/component system. `AnimatedSprite` is standalone, used directly from game code.
- Data-driven clip definitions (JSON). Clips are constructed in code at M2.5. The scene system (M3-M4) is where data-driven everything starts.

## Decisions

### D1: `AnimatedSprite` is a standalone type in `engine_render`, not a component yet

**Choice:** Add `Engine::Render::AnimatedSprite` as a plain class (PIMPL not needed — internal data is just clips + index + timer). Game code holds one by value inside `PlayerState`. No scene, no component, no ECS registration.

**Why:** The scene/component system arrives at M3-M4. Introducing a half-built component framework now to host this one type would be putting the cart before the horse. Standalone usage from game code is the simplest thing that works AND establishes the data shape the future `AnimatedSpriteComponent` will wrap.

**Trade-off:** When the scene system lands, `AnimatedSpriteComponent` adapts `AnimatedSprite` (one wraps the other). That's a minor refactor — the data model doesn't change, only ownership and update-tick wiring.

**Alternative considered:** Introduce a minimal component skeleton now to "future-proof." Rejected per CLAUDE.md soft rule "prefer simple systems before abstract systems."

### D2: Named clips via `std::unordered_map<std::string, AnimationClip>`

**Choice:** `AnimatedSprite::AddClip(std::string name, AnimationClip clip)` stores in a string-keyed map. `Play(std::string_view name)` looks up the name. Same `StringHash` heterogeneous-comparator infrastructure as `engine_input` (with the same libstdc++ 10 workaround at the `.find()` site).

**Why:** Game code wants to say `Play("walk_down")` based on derived state. Numeric clip IDs would force the game to maintain a parallel name → ID table. Strings are the natural API.

**Cost:** Hash lookup per `Play` call. `Play` is once per frame typically, so trivial overhead.

### D3: `Play(current_clip)` is a no-op; `Play(different_clip)` resets

**Choice:** If `Play` is called with the same name as the current clip AND the sprite is playing, do nothing. If a different name is passed, reset frame index to 0 and timer to 0.

**Why:** The expected game loop is "call `Play(desired)` every frame based on current state." If the desired matches what's already playing, restarting the walk cycle to frame 0 every frame would freeze visible animation at frame 0 forever. The no-op preserves cycle continuity. Switching directions (walk_down → walk_left) is a discrete event that DOES want a reset — feels right for the foot to "start over" when direction changes.

**Alternative considered:** Always reset on `Play`. Forces the game to track "did the clip change since last frame?" manually — boilerplate every game would re-invent.

### D4: 8 fps default (frameDuration = 0.125f)

**Choice:** `AnimationClip::frameDuration` defaults to `0.125f` (8 fps). With the standard 4-frame walk cycle `[walk1, stand, walk2, stand]`, a full cycle takes 0.5 seconds → about 2 footsteps per second.

**Why:** Matches the EarthBound / classic JRPG feel — visible footsteps, neither slideshow-slow nor frantic. Tunable per-clip; future fast/slow animations override at clip construction.

### D5: Walk cycle order = `[walk1, stand, walk2, stand]`

**Choice:** A walk cycle is 4 frames: foot-down, recover (stand pose), other-foot-down, recover (stand pose), loop.

**Why:** Standard 2D RPG animation convention. The "stand" between footfalls reads as a brief recovery beat and matches how the player's eyes track motion. The frames themselves are TimeFantasy's `_walk1.png`, `_stand.png`, `_walk2.png`.

**Alternative considered:** `[stand, walk1, stand, walk2]` — start on a recovery pose. Either works; the difference is whether the cycle begins on a foot-down or a stand. Going with foot-down-first because `Play` resets to frame 0 and "starting a new direction lands on a footstep" reads as more energetic than "first show a recovery frame."

### D6: `isMoving` derived game-side from "did position change this frame OR is any move key held"

**Choice:** The game's `onUpdate` computes:

```cpp
const bool wasMoving = (player.position != prevPosition) ||
                       input.IsHeld("MoveUp") || input.IsHeld("MoveDown") ||
                       input.IsHeld("MoveLeft") || input.IsHeld("MoveRight");
```

Then picks `walk_<dir>` vs `idle_<dir>` based on that.

**Why:** "Did position change" alone is fragile — at boundaries (e.g. a future collision wall) the player can be holding a key but not moving; we still want the walk animation to play (Iden "pushing against the wall"). "Is any move key held" alone fails for one-frame programmatic motions (cutscenes later). The OR covers both.

For M2.5 with no collision, position always changes when a key is held, so the two terms agree. The pattern just survives the M3 collision change.

**Alternative considered:** Let engine track and report movement state. Premature — game already knows.

### D7: Fallback path: placeholder is a single-frame "idle_down" clip

**Choice:** When the TimeFantasy pack isn't licensed (paid asset missing), `onStart` builds 8 clips like normal but every clip's frames vector contains exactly one entry — the procedural placeholder. The animation system runs identically; the visible result is "Iden looks the same regardless of state," which is honest about the missing assets.

**Why:** Keeps the engine API surface uniform across "has paid assets" and "doesn't." No conditional in game code beyond the load itself. Public contributors still get a buildable, runnable game that just looks placeholder-y.

### D8: Tests live alongside engine_render tests; SDL fixture re-used

**Choice:** New `tests/render/animated_sprite_tests.cpp` (alongside the existing `renderer_tests.cpp`). Uses the existing `tests/common/sdl_fixture.hpp`'s `SdlWindowFixture` only if a test needs real TextureHandles; pure timing/index tests use trivial null-frame clips and don't need SDL at all.

**Why:** `AnimatedSprite` is mostly a state machine; most behavior (index advance, looping, pause, play-same-noop) is testable with empty TextureHandles. Two or three tests need real handles to verify CurrentFrame returns the right one — those use the fixture.

## Risks / Trade-offs

- **[Risk] `AnimationClip` stores `TextureHandle`s by value (shared_ptr).** → Multiple clips sharing the same underlying texture share the shared_ptr — no double-load, refcounting handles cleanup. Fine.
- **[Risk] String-keyed clip lookup allocates per `Play` call (libstdc++ 10 workaround).** → One small allocation per frame. Sub-microsecond. Revisit only if profiling flags it.
- **[Risk] Game has to load 12 PNGs at startup (4 dirs × 3 frames).** → All small, all cached by AssetLoader. One-time cost at startup, negligible.
- **[Risk] `AnimatedSprite` lives in `engine_render` but doesn't actually call any rendering APIs — it produces a TextureHandle that the game then passes to `Renderer::DrawSprite`.** → Acceptable. Putting it in a future `engine_anim` module would be premature; render is the most natural home for now (it's adjacent to TextureHandle and Camera2D, both already in render).
- **[Risk] When the scene system lands, "every entity ticks its own AnimatedSprite::Update from its component" will not scale to thousands of sprites.** → True; at that point the scene system batches sprite update before render. M2.5 is fine — one sprite, one Update.
- **[Risk] Inserting M2.5 between M2 and M3 in the docs means later archived changes reference "M3" inconsistently.** → All current archives reference M0, M1, M2 only. M3+ are all unstarted. No retroactive churn.
- **[Risk] If Game Director later wants the idle to "breathe" (2-3 frames of subtle motion), idle clips need more than one frame.** → AnimationClip already supports any frame count. One-line change: idle clips become multi-frame at low fps.
