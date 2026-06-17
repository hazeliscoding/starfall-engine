## Context

The repo has DesignDoc.md and GameDesign.md but no source. DesignDoc §29 names the next step ("scaffold CMakeLists, presets, /src/core, /src/runtime, /src/editor, /games/my_rpg, open a window from game_my_rpg via engine_runtime"). GameDesign §9 row M0 promises: window opens, titled "Starfall". This change is that step.

Two specs frame the work: `build-system` (the target graph, presets, third-party acquisition, dependency-rule enforcement) and `runtime-window` (the `Application` lifecycle and the thin game entry point). The design decisions below resolve the choices those specs deliberately leave open.

Constraints:
- C++20, cross-platform Windows + Linux, Steam-friendly distribution (DesignDoc §1, §18).
- Hard module-dependency rules from DesignDoc §6.3 / CLAUDE.md must be enforceable, not just documented.
- Every milestone produces a visible game improvement (DesignDoc §30) — M0's deliverable is the window with title "Starfall."
- Soft rule from CLAUDE.md: prefer simple systems before abstractions. No render-backend abstraction yet; no SDL-version abstraction; no test framework until something needs testing.

## Goals / Non-Goals

**Goals:**

- A working CMake configure + build on Windows (MSVC) and Linux (gcc/clang), via presets, on a clean checkout.
- `game_my_rpg` runs and opens a window titled "Starfall" on both platforms; close button exits cleanly with code 0.
- DesignDoc §6.3 dependency rules are enforced at configure time, not by code review.
- The shape of the repo matches DesignDoc §5: every module that will eventually exist has its target + folder reserved so the graph is complete on day one.
- Third-party dependency acquisition is reproducible and version-pinned.

**Non-Goals:**

- No rendering, input, audio, scripting, scenes, or assets — just an empty cleared window.
- No editor functionality — `engine_editor` is a stub executable.
- No CI pipeline configuration; local-host builds on both OSes are the bar.
- No macOS support.
- No test framework setup; first milestone that needs tests picks Catch2 vs GoogleTest at that point.
- No SDL2/SDL3 portability shim — commit to one version.

## Decisions

### D1: SDL3, not SDL2

**Choice:** Use SDL3 (3.2.x or later stable line).

**Why:** SDL3 went stable in late 2024 and is the active development line. Greenfield projects starting in 2026 should not pick the legacy version. SDL3's API is cleaner (typed events, better init/quit lifecycle, GPU API for later when render needs it), and its license (zlib) and packaging story are identical to SDL2's.

**Alternative considered:** SDL2 — more existing tutorials/examples, but locks the engine to a deprecated API on day one. Rejected.

**Implication for the spec:** `runtime-window`'s "via SDL" requirement resolves to SDL3 specifically. Engine code includes `<SDL3/SDL.h>`.

### D2: SDL acquired via FetchContent, pinned by git tag

**Choice:** `external/CMakeLists.txt` uses `FetchContent_Declare(SDL3 GIT_REPOSITORY … GIT_TAG <pinned-release>)` with `FetchContent_MakeAvailable`. The pinned tag lives in a single variable so version bumps are one-line.

**Why:**
- No submodule sharp edges (clones-with-submodules, detached HEAD, etc.).
- Reproducible — pinned to a tag SHA, not a moving branch.
- Builds SDL from source against our compiler, sidestepping ABI mismatches with system SDL on Linux.
- One mechanism for the whole project — when Lua, ImGui, sol2 land in later milestones they'll use the same pattern, satisfying the `build-system` spec's "single mechanism" requirement.

**Alternatives considered:**
- **Git submodule** — extra ceremony for contributors, and `git clone` without `--recursive` is a common foot-gun. Rejected.
- **System SDL via `find_package`** — fast on Linux but fragile on Windows and produces version drift. The `build-system` spec explicitly forbids silent system substitution, so this is out.
- **Vendored source dump** — bloats the repo and makes upstream patches invisible. Rejected.

### D3: SDL lives at engine_runtime, not engine_core

**Choice:** `engine_runtime` is the only module that links SDL. `engine_core` has no third-party deps.

**Why:** DesignDoc §7.1 says `engine_core` "should remain boring and extremely stable" and lists windowing as a non-responsibility. Putting SDL in core would taint future modules (editor, asset_packer) that link core but have no business pulling in window/event subsystems. Runtime is the right owner because runtime is what *opens* the window. This is the answer to the punt in the proposal's Impact section.

**Alternative considered:** Put SDL in a new `engine_platform` module. Premature — we'd be inventing an abstraction layer before there's a second platform backend, violating CLAUDE.md soft rule "no render backend abstraction until a second backend is actually needed." If a non-SDL backend ever appears, extract `engine_platform` then.

### D4: `starfall_add_module` helper enforces §6.3 at configure time

**Choice:** Add `cmake/StarfallModule.cmake` exposing one function:

```cmake
starfall_add_module(NAME engine_runtime
                    DEPENDS engine_core SDL3::SDL3)
```

The function:
1. Validates `NAME` matches `engine_*`, `game_*`, or `editor_*`.
2. Checks every name in `DEPENDS` against a hardcoded forbidden-edges table mirroring DesignDoc §6.3 (e.g. `engine_core` cannot depend on render/input/audio/scene/scripting; engine_* cannot depend on game_* or editor_*).
3. Verifies a matching `src/<suffix>/` directory exists for engine modules.
4. Calls `add_library` / `add_executable` and `target_link_libraries`.
5. Sets C++20, warnings-as-errors in Debug presets, position-independent code.

**Why:** Pre-merge `check-deps` script catches violations after the fact. Configure-time fails them at the source of the change, on every developer's machine, with the exact rule cited in the error message. That's the right place for hard rules.

**Alternative considered:** Pure documentation + the existing `check-deps` skill. Documentation rots; helpers enforce. Configure-time enforcement is cheap (one function call per target) and proportional to the value (these rules are foundational).

### D5: Empty module targets are real targets, not placeholders

**Choice:** Every engine module from DesignDoc §6.1 gets its `src/<name>/CMakeLists.txt`, `src/<name>/.gitkeep` (or an empty `<name>.cpp` placeholder if CMake refuses zero-source libraries), and `starfall_add_module(...)` call. The module is built; it just exports nothing useful yet.

**Why:** Wiring the full dependency graph in one change means every subsequent milestone only has to *add* code to its module, not also add the target, the CMake plumbing, the include dirs, and the dependency declarations. It also makes the `starfall_add_module` enforcement immediately load-bearing for every module, not just the two with code.

**Alternative considered:** Add modules lazily as milestones need them. Cheaper now, more churn over the next 10 milestones, and the dependency graph is invisible until M10. Rejected — small upfront cost, big payoff.

### D6: Logger as a freestanding header-only utility in engine_core

**Choice:** `Engine::Core::Logger` is a thin wrapper over `printf`-style formatted output with category tags (`[Core]`, `[Runtime]`, etc.) as DesignDoc §20 specifies. No spdlog, no fmt dependency yet.

**Why:** A real logging library is overkill for a single-window M0. The interface (`SF_LOG_INFO("Runtime", "Window opened")`) can be swapped to spdlog later without callers changing — that's the whole point of having an interface. CLAUDE.md soft rule: prefer simple before abstract.

**Alternative considered:** Pull `spdlog` via FetchContent now. Defer until logging volume justifies it (likely M3 or later, when there are real subsystems producing output).

### D7: `AppConfig` is a plain aggregate, no builder pattern

**Choice:**

```cpp
namespace Engine::Runtime {
struct AppConfig {
    std::string title = "Starfall";
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string initialScene; // reserved, unused in M0
};
}
```

Used as `app.Configure({.title = "Starfall", .windowWidth = 1280, ...})` (C++20 designated initializers).

**Why:** Spec requires documented defaults and a thin game entry point; C++20 designated initializers give both for free. A builder pattern would be ceremony with no payoff at this size.

### D8: Run loop uses event-wait with a frame-tick fallback (M0 only)

**Choice:** Since M0 renders nothing and has no per-frame work, the loop uses `SDL_WaitEventTimeout(16ms)`. When M1 lands real rendering, the loop switches to poll + render-every-frame.

**Why:** Satisfies the "no busy spin on idle window" scenario from the `runtime-window` spec without inventing a frame scheduler M0 doesn't need. The change to a polling loop in M1 is one function call.

**Alternative considered:** Poll + `SDL_Delay(16)`. Equivalent CPU behavior; `WaitEventTimeout` reads more clearly as "we have nothing to do but wait for input."

### D9: Editor stub links nothing engine-side yet

**Choice:** `engine_editor`'s `main.cpp` is a 3-line program that logs "editor stub — not implemented" and returns 0. Its CMake target declares no engine dependencies in this change.

**Why:** Reserves the target name and the binary, lets the `build-system` spec's "editor optional at runtime" scenario be checked (because the binary exists and is independent), and avoids prematurely binding the editor to modules whose APIs aren't designed yet. Editor work starts at M5.

### D10: Repo layout matches DesignDoc §5 literally

**Choice:** Directory tree:

```
/cmake/StarfallModule.cmake
/external/CMakeLists.txt           # SDL3 FetchContent
/include/engine/core/              # public headers for engine_core
/include/engine/runtime/           # public headers for engine_runtime
/include/engine/{math,assets,render,input,audio,scene,scripting}/  # empty trees
/src/core/                         # logger, Result<T>
/src/runtime/                      # Application
/src/{math,assets,render,input,audio,scene,scripting}/  # empty + CMakeLists
/src/editor/                       # stub main
/games/my_rpg/{src,assets,scripts,scenes}/  + CMakeLists.txt
/tools/                            # empty for now
/tests/                            # empty for now
CMakeLists.txt
CMakePresets.json
```

No reshuffling later. Every milestone fills in folders that already exist.

## Risks / Trade-offs

- **[Risk] FetchContent of SDL3 lengthens first-time configure to several minutes on Windows.** → Mitigation: pin a known-buildable SDL3 tag; document `--parallel` use in the README; trust developers to configure once.
- **[Risk] `starfall_add_module`'s forbidden-edges table drifts from DesignDoc §6.3 if the rules change.** → Mitigation: the table is the single source of truth at build time; if §6.3 changes, both update in the same commit (per the same-commit-update rule in DesignDoc §23.3 / CLAUDE.md).
- **[Risk] Empty module targets clutter the build with libraries that export nothing.** → Mitigation: they compile in <1s each as essentially no-op TUs; payoff (dependency graph completeness from day one — D5) outweighs the noise.
- **[Risk] Choosing SDL3 over SDL2 narrows the available pool of tutorials/examples.** → Mitigation: SDL3's docs are good and the API surface used in M0 (init/window/event-wait/quit) is small and well-covered. Acceptable.
- **[Risk] No CI means a contributor on the wrong platform breaks the other.** → Mitigation: this is a single-dev project right now (CLAUDE.md, GameDesign.md context). When a second contributor lands or the first dev gets sloppy, add a tiny GitHub Actions matrix job; not justified yet.
- **[Risk] `engine_editor` as a stub means the "editor not required at runtime" rule isn't truly tested until the editor has code.** → Mitigation: spec scenario explicitly tests by copying only `game_my_rpg` + SDL to a clean dir — that test works today against the stub and keeps working as the editor grows.
