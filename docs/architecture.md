# Architecture

## Repository purpose

This repository is a reusable scaffold for a modern C++23 library. It is not only a code sample: it also demonstrates how to structure packaging, testing, documentation, CI, and local developer workflows around the library.

## Structural overview

### Public library surface

- `include/yourlib/yourlib.hpp` defines the public `Greeter` API.
- `include/yourlib/export.hpp` defines symbol export macros for shared builds on Windows.
- `include/yourlib/version.hpp.in` is configured into a generated version header during CMake configure.

### Private implementation

- `src/yourlib.cpp` implements the public API.
- `src/yourlib_impl.hpp` contains the hidden `Greeter::Impl` definition.

This split demonstrates the intended PIMPL boundary: consumers only depend on public headers, while internal representation lives in `src/`.

### Build and package layer

- `CMakeLists.txt` declares the library target, generated headers, install rules, and exported package metadata.
- `cmake/*.cmake` centralize warning and sanitizer policy so the same rules apply to multiple targets.
- `CMakePresets.json` defines repeatable developer entry points.

### Test layer

- `tests/CMakeLists.txt` builds the test executable and wires it into CTest.
- `tests/tests_greeter.cpp` validates the example behavior.

Tests depend on the public target `your_library::your_library`, which keeps the test path close to real consumer usage.

### Example / consumer layer

- `examples/minimal/` demonstrates downstream consumption through `find_package(your_library REQUIRED)`.

That is important because the scaffold is not only meant to compile locally; it is meant to install and be consumed by another CMake project.

### Automation and workflow layer

- `.github/workflows/ci.yml` exercises configure, build, and test on Linux.
- `project_manager.bat` and `scripts/pm_*.bat` provide a Windows-oriented local orchestration layer.
- `Dockerfile` provides a portable Linux build environment with the required toolchain.

## Ownership and boundaries

- `include/`: stable consumer-facing headers.
- `src/`: private implementation details.
- `tests/`: validation of public behavior.
- `examples/`: proof that install/export integration works for consumers.
- `cmake/`: reusable build-policy modules.
- `docs/`: durable project knowledge, not transient notes.
- `.github/`: collaboration and automation policy.

## Error handling and threading assumptions

The example library is deliberately simple:

- no background threading model is introduced
- no custom error domain is introduced
- operations use standard value semantics and standard-library types

That keeps the scaffold focused on structure rather than application-specific policy.

## ABI and API direction

The scaffold demonstrates an ABI-conscious shape:

- narrow public header
- hidden implementation details behind PIMPL
- export macro support for shared builds
- installed package metadata for downstream consumers

Concrete libraries cloned from this scaffold should extend this policy explicitly in their own docs.
