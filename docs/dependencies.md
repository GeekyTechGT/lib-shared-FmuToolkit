# Dependencies

## Runtime dependencies

The example library has no mandatory third-party runtime dependencies beyond the C++ standard library.

## Build-time dependencies

Required:

- CMake 3.25+
- a C++23-capable compiler
- Ninja for the preset-driven workflows in this scaffold

Optional:

- `vcpkg` when using `vcpkg-*` presets

## Test dependencies

- GoogleTest

The test build first tries `find_package(GTest CONFIG QUIET)`. If that does not succeed, `tests/CMakeLists.txt` falls back to `FetchContent` and downloads GoogleTest 1.14.0.

## Developer tooling dependencies

- `clang-format` for `scripts/pm_format.bat`
- `clang-tidy` for `scripts/pm_lint.bat`
- `vcpkg` for `scripts/pm_vcpkg_publish_registry.bat` and registry version database updates
- Git for custom registry publication and version-tree tracking
- PowerShell for JSON parsing inside the Windows registry helper scripts
- Docker for containerized Linux builds
- GitHub Actions runners for CI and release automation

## Manifest and automation files

- `vcpkg.json` declares `gtest` for manifest-mode dependency resolution.
- `Dockerfile` installs the compiler and analysis toolchain used by the documented Linux workflow.
- `.github/workflows/ci.yml` installs `ninja-build`, `g++`, and `cmake` on Ubuntu before building.

## Why this dependency set is small

The scaffold is intentionally conservative:

- the public library example depends only on the standard library
- only testing introduces a third-party C++ dependency
- developer tooling is optional and isolated from the public API

This keeps the scaffold easy to clone and adapt to a concrete library.
