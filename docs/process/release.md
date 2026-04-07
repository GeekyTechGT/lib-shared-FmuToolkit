# Release Process

## Versioned artifacts

A scaffold release is defined by:

- `project(... VERSION ...)` in `CMakeLists.txt`
- generated `yourlib/version.hpp`
- installable CMake package metadata
- `CHANGELOG.md`
- the published private-registry `vcpkg` port metadata

## Versioning Rules

Keep these concepts separate:

- library version: the `VERSION` in `CMakeLists.txt` and `version-string` in `vcpkg.json`
- vcpkg port revision: `port-version` when packaging changes without a source release
- toolchain variant: selected through the consumer triplet, not through a new package name
