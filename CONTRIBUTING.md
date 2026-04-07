# Contributing

## Scope

This repository is a reusable C++23 library scaffold.
Changes should preserve zero-touch reuse of the infrastructure: users should be able to add library sources under `src/` and tests under `tests/` without editing the build-system files.

## Contribution Baseline

- keep the distributable header surface intentionally small
- keep infrastructure generic rather than domain-specific
- preserve package export, install, test, preset, and vcpkg flows
- keep examples aligned with the installed consumer story
- update docs when API, behavior, packaging, presets, or outputs change
- update tests when fixing scaffold behavior or placeholder example behavior

## Common Commands

### Generic local debug flow

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

### Generic local release flow

```bash
cmake --preset dev-release
cmake --build --preset dev-release
ctest --preset dev-release
```

### Windows MinGW

```bash
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug
```

### Windows MSVC

```bash
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

### Interactive Windows helper

```bat
project_manager.bat
```

## Build And Packaging Rules

- all configured builds must stay under `build/<preset>`
- do not introduce ad-hoc root build directories
- preserve `your_library::your_library` as the default consumer target unless compatibility requires a deliberate change
- keep `your_library` as the library/package identity and `your-library` as the vcpkg port name
- keep the scaffold usable by adding source files only, without editing target file lists
