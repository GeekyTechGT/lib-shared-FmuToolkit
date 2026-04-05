# Release Process

## Current release posture

This repository already contains the technical pieces needed for local packaging-oriented validation:

- versioned CMake project metadata in `CMakeLists.txt`
- generated version header from `include/yourlib/version.hpp.in`
- install/export package metadata
- changelog tracking in `CHANGELOG.md`
- a placeholder GitHub release workflow in `.github/workflows/release.yml`

What it does not contain yet is a concrete publishing pipeline for a real package registry. That is intentional for a scaffold.

## Suggested release checklist

1. Update the project version in `CMakeLists.txt`.
2. Update `vcpkg.json` if its package version must stay aligned.
3. Update `CHANGELOG.md`.
4. Run configure, build, test, and install validation for the target release preset.
5. Verify the consumer path using the installed package example.
6. Tag the release in source control.
7. Publish or update the port in the custom vcpkg registry using `scripts/pm_vcpkg_publish_registry.bat`.
8. Replace the placeholder release workflow with actual packaging and publishing steps for the derived library.

## Minimum local validation

A release candidate should at least pass:

```bash
cmake --preset dev-release
cmake --build --preset dev-release
ctest --preset dev-debug
cmake --install build/dev-release
```

On Windows, the equivalent flow can be driven through `project_manager.bat`, using build/test/deploy options.

## Consumer validation

The most important release property for this scaffold is that downstream CMake consumers can install and then resolve the exported package:

```cmake
find_package(your_library REQUIRED)
target_link_libraries(app PRIVATE your_library::your_library)
```

The repository keeps that path visible through `examples/minimal/`.

## Registry publication

If the derived library is distributed through a private or organization-owned vcpkg registry, the custom registry should be updated as part of the release. This scaffold now includes:

- a templated vcpkg port under `packaging/vcpkg/`
- a local config template under `scripts/vcpkg_registry_config.example.bat`
- scripts to initialize and publish to the target Git registry

The publish step requires `git` and `vcpkg` to be installed locally.
