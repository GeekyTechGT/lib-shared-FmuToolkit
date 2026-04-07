# Documentation

This directory contains the long-lived technical documentation for the scaffold.

## Reading order

1. `architecture/overview.md`
2. `architecture/design.md`
3. `reference/api-reference.md`
4. `specs/library-spec.md`
5. `process/build.md`
6. `process/dependencies.md`
7. `process/customer-vcpkg-consumption.md`
8. `process/release.md`
9. `adr/0001-use-pimpl.md`

## Document map

- `architecture/overview.md`: repository structure, zero-touch reuse expectations, and public/private boundaries
- `architecture/design.md`: rationale for the placeholder API, PIMPL, install surface, and package model
- `reference/api-reference.md`: API reference for the placeholder scaffold surface
- `process/build.md`: CMake presets, build layout, install flow, test flow, Docker, and Windows scripts
- `process/dependencies.md`: compiler, toolchain, test, and packaging dependencies
- `process/customer-vcpkg-consumption.md`: customer-app integration flow through the private vcpkg registry
- `process/release.md`: versioning, changelog, install validation, and private distribution notes
- `specs/library-spec.md`: scaffold functional and non-functional expectations
- `adr/`: architecture decisions
