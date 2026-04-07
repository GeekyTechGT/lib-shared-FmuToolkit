# Library Scaffold Specification

## Functional Requirements

- the scaffold must configure, build, test, install, and package without modification
- users must be able to add implementation `.cpp` files under `src/` without editing the top-level `CMakeLists.txt`
- users must be able to add test `.cpp` files under `tests/` without editing `tests/CMakeLists.txt`
- the scaffold must export an installable CMake package target
- the scaffold must provide a working example consumer and a working test suite out of the box

## Non-Functional Requirements

- the infrastructure must stay generic rather than domain-specific
- builds must remain under `build/<preset>`
- the scaffold must support multiple compiler families through presets and `vcpkg` triplets
