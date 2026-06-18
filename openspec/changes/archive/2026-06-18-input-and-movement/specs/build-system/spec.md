## ADDED Requirements

### Requirement: Test Framework Integration

The build system SHALL integrate Catch2 (acquired via `external/` per the existing third-party rule) and SHALL expose CTest as the canonical test runner for the project. A helper `starfall_add_test(NAME <test-target> SOURCES ... DEPENDS ...)` SHALL register the target with CTest and link Catch2's main entry point so each test module's `main` function is provided by the framework.

#### Scenario: Catch2 acquired via external/

- **WHEN** a developer configures the project on a host with no system Catch2 installed
- **THEN** configure MUST succeed and `Catch2::Catch2WithMain` MUST be made available through `external/CMakeLists.txt`

#### Scenario: Test target registered with CTest

- **WHEN** a developer adds a test via `starfall_add_test(NAME engine_input_tests SOURCES ... DEPENDS engine_input)`
- **THEN** `ctest --preset debug-windows` (or `debug-linux`) lists `engine_input_tests` and runs it as part of the test suite

#### Scenario: Tests link only the module under test

- **WHEN** `starfall_add_test` is invoked with `DEPENDS engine_input`
- **THEN** the test target links `engine_input` and `Catch2::Catch2WithMain` only — it MUST NOT pull in `engine_runtime`, `engine_render`, or any other module not explicitly named in `DEPENDS`

#### Scenario: ctest preset runs the suite

- **WHEN** a developer runs `ctest --preset debug-windows` after a successful build
- **THEN** every registered Catch2 test target runs, output is captured per-test, and a non-zero exit code is returned if any test fails

### Requirement: Test Sources Mirror Module Layout

Tests SHALL live under `tests/<module>/` mirroring the engine module layout (e.g. `tests/input/` for `engine_input`'s tests). Each module's tests are an independent CMake target named `<module>_tests` (e.g. `engine_input_tests`).

#### Scenario: New module's tests appear under tests/

- **WHEN** a developer adds tests for a hypothetical `engine_foo` module
- **THEN** the tests live in `tests/foo/` with `tests/foo/CMakeLists.txt` declaring `starfall_add_test(NAME engine_foo_tests ...)`

## MODIFIED Requirements

### Requirement: Cross-Platform Presets

The project SHALL ship `CMakePresets.json` with presets `debug-windows`, `release-windows`, `debug-linux`, and `release-linux` that select Ninja when available and configure into per-preset build directories. Each configure preset SHALL have a matching `buildPresets` entry AND a matching `testPresets` entry that invokes `ctest` against the preset's build directory.

#### Scenario: Build via preset on Windows

- **WHEN** a developer runs `cmake --preset debug-windows` followed by `cmake --build --preset debug-windows` on a Windows host with MSVC and Ninja installed
- **THEN** the build succeeds and produces `game_my_rpg.exe` and `engine_editor.exe` in the preset's build directory

#### Scenario: Build via preset on Linux

- **WHEN** a developer runs `cmake --preset debug-linux` followed by `cmake --build --preset debug-linux` on a Linux host with a C++20 toolchain and Ninja installed
- **THEN** the build succeeds and produces `game_my_rpg` and `engine_editor` executables in the preset's build directory

#### Scenario: Build directories are isolated per preset

- **WHEN** a developer configures `debug-linux` and then `release-linux`
- **THEN** each preset uses a distinct build directory under `build/` and switching presets does not require a clean

#### Scenario: Tests run via ctest preset

- **WHEN** a developer runs `ctest --preset debug-windows` (or `debug-linux`) after a successful build of that preset
- **THEN** every registered test target executes and returns a non-zero exit code on any failure
