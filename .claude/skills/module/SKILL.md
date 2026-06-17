---
name: module
description: Scaffold a new Starfall Engine module — creates /src/<name>/, /include/engine/<name>/, a module CMakeLists.txt, the Engine::<Name> namespace skeleton, and registers the target in the root CMakeLists.txt. Validates the requested dependencies against the project's hard rules before writing anything. Use when adding a new `engine_*` static library target.
---

# module

Scaffold a new engine module that conforms to Starfall Engine's architecture (see `CLAUDE.md` and `docs/DesignDoc.md` §6, §7).

## Inputs to gather

Ask the user (only if not already provided):

1. **Module name** — short, lowercase, no `engine_` prefix in the user-facing name (e.g. `render`, not `engine_render`). The CMake target becomes `engine_<name>`. The C++ namespace becomes `Engine::<PascalName>`.
2. **One-line purpose** — for the module header doc comment and the CMakeLists comment.
3. **Dependencies** — which other `engine_*` modules this one links against. **Validate against the rules below before writing files.**

## Dependency validation (do this BEFORE writing files)

Reject the request and surface the violation if the requested dependencies break any rule:

- A new module always implicitly depends on `engine_core` and `engine_math` (these are universal).
- A new module **may NOT depend on** any `game_*` target or on `engine_editor`.
- If the new module is `engine_core` itself: **no engine dependencies allowed**.
- If the new module is `engine_runtime`: it may depend on any other `engine_*`.
- If the user adds `engine_scripting`, `engine_render`, `engine_input`, `engine_audio`, or `engine_scene` as a dep of something that is supposed to be a low-tier module (core, math, assets), surface the concern and confirm before proceeding.

When in doubt, re-read `docs/DesignDoc.md` §6.2 (target dependency graph) — it is the source of truth.

## Files to create

For a module named `<name>`:

1. **`/src/<name>/CMakeLists.txt`** — declares `add_library(engine_<name> STATIC ...)`, lists the `.cpp` files, calls `target_link_libraries(engine_<name> PUBLIC engine_core engine_math <validated deps>)`, sets `target_include_directories(engine_<name> PUBLIC ${CMAKE_SOURCE_DIR}/include)`.

2. **`/src/<name>/<name>.cpp`** — empty stub with the namespace opened:
   ```cpp
   #include "engine/<name>/<name>.h"

   namespace Engine::<PascalName>
   {
       // Implementation goes here.
   }
   ```

3. **`/include/engine/<name>/<name>.h`** — public header stub:
   ```cpp
   #pragma once

   // <one-line purpose from the user>

   namespace Engine::<PascalName>
   {
       // Public API goes here.
   }
   ```

4. **Register in the root `CMakeLists.txt`** — add `add_subdirectory(src/<name>)` in the appropriate location, alphabetized with siblings.

## Style rules

- Use `#pragma once`, not include guards.
- One namespace block per file: `namespace Engine::<PascalName> { ... }`.
- C++20 throughout — no C++11/14 idioms.
- Do NOT scaffold a `Result<T>` or `Logger` inside the new module — those live in `engine_core`.
- Do NOT add tests in this skill. Tests get their own scaffold (or a follow-up skill).

## After scaffolding

Tell the user:
1. The module is created at `/src/<name>/` and `/include/engine/<name>/`.
2. CMake will pick it up after the next configure step.
3. Suggest running the `check-deps` skill to verify the link edges are clean.
4. Remind them to update `docs/DesignDoc.md` if this module wasn't already listed there (per the project's docs-in-same-commit rule).

## What this skill does NOT do

- It does not create executables (game or editor targets) — those have a different scaffold pattern.
- It does not create test targets.
- It does not generate Lua bindings — that's `engine_scripting`'s job.
- It does not commit the files. Use the `commit` skill after review.
