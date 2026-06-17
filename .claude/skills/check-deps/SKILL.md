---
name: check-deps
description: Audit the project's CMake target dependencies against Starfall Engine's hard module-dependency rules (engine cannot depend on game/editor, engine_core cannot depend on render/input/audio/scene/scripting, etc.). Use before merging structural changes, after adding a new module, or when a new `target_link_libraries` call is introduced.
---

# check-deps

Verify that no CMake target violates the hard module-dependency rules defined in `CLAUDE.md` and `docs/DesignDoc.md` §6.3.

## The rules being checked

**Forbidden edges (any of these is a violation):**

| From | To | Reason |
|------|-----|--------|
| any `engine_*` | `game_*` | Engine must not depend on game |
| any `engine_*` | `engine_editor` | Engine runtime must not depend on editor |
| `engine_core` | `engine_render`, `engine_input`, `engine_audio`, `engine_scene`, `engine_scripting` | Core must stay minimal and stable |
| `engine_editor` | `game_*` | Editor must not contain game-specific logic |
| `game_*` runtime path | `engine_editor` | Shipped game must not require editor |

**Allowed edges** (for context — these are fine):

- `game_*` → `engine_runtime`, `engine_scene`, `engine_scripting`
- `engine_editor` → any `engine_*` module
- `engine_runtime` → any other `engine_*` module
- Any `engine_*` → `engine_core`, `engine_math`

## Procedure

1. **Find every `target_link_libraries(...)` call** in the repository. Use Grep across `**/CMakeLists.txt` and `**/*.cmake`.
2. For each call, identify the consuming target (first arg) and the dependencies it links.
3. Compare each `(consumer, dependency)` pair against the forbidden edges above.
4. Also check **public include directories**: an `engine_*` target that does `target_include_directories(... PUBLIC games/...)` is a violation even if no link is declared.
5. Report findings in this format:

   ```
   VIOLATIONS
     <file>:<line>  engine_core → engine_render  (engine_core must stay minimal)
     <file>:<line>  engine_audio → games/my_rpg  (engine cannot depend on game)

   CLEAN
     <N> targets, <M> link edges, no violations.
   ```

6. If violations are found, **do not auto-fix them.** Surface them to the user and propose a fix (usually: move the dependency, invert it via events, or refactor the offending code out of the lower-tier module).

## What this skill does NOT do

- It does not run CMake or build anything — it's a static analysis of the build files.
- It does not check transitive runtime dependencies (e.g. dlopen, plugin loading). Those need a different audit.
- It does not enforce *soft* rules (preferring simple systems, avoiding premature abstraction) — those are judgment calls, not mechanical checks.

## Pre-implementation note

Until the CMake scaffold exists (Milestone 0), this skill will report "no CMakeLists.txt found." That is the correct behavior — there is nothing to audit yet. Re-run after M0 lands.
