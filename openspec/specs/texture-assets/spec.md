# texture-assets Specification

## Purpose

Defines `engine_assets`'s texture-loading surface: a path-keyed loader that decodes PNG files from disk into GPU-side textures, returns opaque handles, and caches by path so repeated requests are cheap. Also defines the build-time mirroring contract that ensures source assets under `games/my_rpg/assets/` are available next to the running executable at runtime, so the same relative path strings work in dev builds and packaged distributions.

## Requirements

### Requirement: Load PNG Textures From Disk

`engine_assets` SHALL provide a function that loads a PNG file from disk into a GPU-side texture and returns an opaque `Engine::Assets::TextureHandle`. The same path requested twice SHALL return the same handle without re-reading the file.

#### Scenario: First load reads from disk

- **WHEN** game code calls `LoadTexture("assets/characters/iden_placeholder.png")` and that path has not been loaded before
- **THEN** the file is read, decoded as PNG, uploaded to the renderer, and a valid handle is returned

#### Scenario: Second load returns the cached handle

- **WHEN** game code calls `LoadTexture` with the same path twice
- **THEN** both calls MUST return handles that resolve to the same underlying GPU texture; the file MUST NOT be read or decoded a second time

#### Scenario: Missing file produces a usable error

- **WHEN** game code calls `LoadTexture` with a path that does not exist
- **THEN** the call MUST return a sentinel "invalid" handle (or `Result<TextureHandle>::Err`), log an `[Assets]` error including the requested path, and MUST NOT crash; rendering an invalid handle MUST be a visible-but-non-fatal noop (e.g. a magenta rectangle or nothing drawn)

### Requirement: Asset Paths Resolve Relative To The Executable

The asset loader SHALL resolve relative paths against the directory containing the running executable. This works for both `cmake --build` runs and packaged distributions (M9), so the same path string works in dev and in production.

#### Scenario: Game runs from build output

- **WHEN** the developer runs `bin/game_my_rpg` and game code calls `LoadTexture("assets/characters/iden_placeholder.png")`
- **THEN** the loader reads `<exe-dir>/assets/characters/iden_placeholder.png` (which the build pipeline copied there) and returns a valid handle

#### Scenario: Game runs from a copied directory

- **WHEN** the executable + its sibling `assets/` directory are copied to any other location and run from there
- **THEN** the same calls succeed using the new exe-relative paths; no environment variable or working-directory assumption is required

### Requirement: Build Pipeline Mirrors Game Assets To The Executable Directory

The CMake build for `game_my_rpg` SHALL copy the contents of `games/my_rpg/assets/` to `<runtime-output-dir>/assets/` as a post-build step. Editing a source asset and rebuilding MUST update the mirror.

#### Scenario: Editing a source asset propagates

- **WHEN** a developer edits `games/my_rpg/assets/characters/iden_placeholder.png` and rebuilds the `game_my_rpg` target
- **THEN** `<build>/<preset>/bin/assets/characters/iden_placeholder.png` MUST match the edited source after the build completes

#### Scenario: Removing a source asset does not leave a stale copy

- **WHEN** a developer deletes a source asset and rebuilds
- **THEN** the mirror directory MUST also no longer contain that asset (full sync, not additive)
