# your_library

[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=c%2B%2B&logoColor=white)](#build-and-test)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C?logo=cmake&logoColor=white)](#build-and-test)
[![Scaffold](https://img.shields.io/badge/Template-Zero--Touch%20Library%20Scaffold-0A7E3B)](#overview)
[![Targets](https://img.shields.io/badge/CMake-Shared%20%7C%20Static-444)](#cmake-package-consumption)
[![vcpkg](https://img.shields.io/badge/vcpkg-Registry%20Ready-0E639C)](#vcpkg-consumer-guide)

`your_library` is a reusable C++23 library scaffold.
It is structured so a user can clone the folder, add library sources under `src/`, add tests under `tests/`, and keep the build, packaging, docs, presets, and install/export flow working without editing the infrastructure files.

## Overview

The scaffold provides:

- installable CMake package exports for downstream reuse
- simultaneous shared and static library builds
- preset-driven builds under `build/<preset>`
- compiler-specific local and `vcpkg` workflows for Linux GCC, Linux Clang, Windows MSVC, Windows MinGW, and Windows Clang via MSYS2
- recursive source and test auto-discovery so new `.cpp` files are picked up automatically
- GoogleTest integration with `FetchContent` fallback
- Windows helper scripts for configure, build, test, deploy, and private-registry publication
- private-registry-oriented `vcpkg` templates, triplets, and consumer documentation

## Zero-Touch Reuse Model

The intended scaffold workflow is:

- keep the infrastructure files as they are
- add or replace public headers under `include/yourlib/`
- add library implementation `.cpp` files anywhere under `src/`
- add test `.cpp` files anywhere under `tests/`
- optionally replace the placeholder example API with the real library API once the concrete library is defined

The core CMake files automatically discover:

- all library `.cpp` files under `src/`
- all test `.cpp` files under `tests/`, excluding `tests/CMakeLists.txt`

That means users should not need to edit `CMakeLists.txt` or `tests/CMakeLists.txt` just to register new source files.

## Public API

The installable header surface is intentionally minimal:

- `include/yourlib/yourlib.hpp`
- `include/yourlib/export.hpp`
- generated `include/yourlib/version.hpp`

The placeholder public API is deliberately neutral and build-oriented rather than domain-specific.
It gives the scaffold a working example while remaining easy to replace with a real library API.

## Repository Layout

```text
.
|-- .github/
|-- cmake/
|-- docs/
|-- examples/
|-- include/yourlib/
|-- packaging/vcpkg/
|-- scripts/
|-- src/
|-- tests/
|-- triplets/
|-- CHANGELOG.md
|-- CMakeLists.txt
|-- CMakePresets.json
|-- CODE_OF_CONDUCT.md
|-- CONTRIBUTING.md
|-- Dockerfile
|-- LICENSE
|-- README.md
|-- SECURITY.md
|-- project_manager.bat
`-- vcpkg.json
```

## Build And Test

All configured builds are expected under `build/<preset>`.

Generic debug flow:

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

Generic release flow:

```bash
cmake --preset dev-release
cmake --build --preset dev-release
ctest --preset dev-release
```

Recommended Windows MinGW flow:

```bash
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug
```

Recommended Windows MSVC flow:

```bash
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Recommended Windows Clang flow:

```bash
cmake --preset windows-clang-debug
cmake --build --preset windows-clang-debug
ctest --preset windows-clang-debug
```

## Library Outputs

Default build options:

- `YOUR_LIBRARY_BUILD_SHARED=ON`
- `YOUR_LIBRARY_BUILD_STATIC=ON`

If both are enabled, the scaffold produces:

- a shared library / DLL
- the shared import library on Windows
- a static library

On MinGW builds the runtime is statically linked by default through `YOUR_LIBRARY_MINGW_STATIC_RUNTIME=ON`.

## CMake Package Consumption

Exported targets:

- `your_library::your_library`
- `your_library::your_library_shared`
- `your_library::your_library_static`

`your_library::your_library` is the stable convenience target and resolves to the preferred primary library exported by the package.

```cmake
find_package(your_library CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE your_library::your_library)
```

## vcpkg Consumer Guide

Customer applications should consume the concrete library generated from this scaffold through a private `vcpkg` registry, not by vendoring the source repository.

The detailed guide is documented in `docs/process/customer-vcpkg-consumption.md` and covers:

- private registry setup with `vcpkg-configuration.json`
- declaring `your-library` in the consumer `vcpkg.json`
- selecting the correct triplet for MSVC, MinGW, MSYS2 Clang, Linux GCC, or Linux Clang
- integrating `find_package(your_library CONFIG REQUIRED)` in the consumer CMake project
- version pinning through the registry baseline commit
- shared versus static consumption rules

Minimum consumer CMake usage remains:

```cmake
find_package(your_library CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE your_library::your_library)
```

Minimum consumer include remains:

```cpp
#include <yourlib/yourlib.hpp>
```

## Example

The example under `examples/minimal/` demonstrates the installed-package consumer flow and uses the placeholder scaffold API without depending on any domain-specific behavior.

## Packaging

- library and package name: `your_library`
- vcpkg port name: `your-library`
- compiler-specific `vcpkg` overlay triplets live under `triplets/`

Private-registry templates and helper scripts live under `packaging/vcpkg/` and `scripts/`.

## Documentation

- `docs/README.md` for the documentation index
- `docs/reference/api-reference.md` for the public API reference
- `docs/architecture/overview.md` and `docs/architecture/design.md` for architecture details
- `docs/process/build.md`, `docs/process/dependencies.md`, `docs/process/customer-vcpkg-consumption.md`, and `docs/process/release.md` for workflow details
- `docs/specs/library-spec.md` for the scaffold scope and constraints

## Reuse Checklist

When cloning the scaffold for a concrete library, rename these consistently:

- CMake project name: `your_library`
- C++ namespace: `yourlib`
- public include folder: `include/yourlib`
- exported target: `your_library::your_library`
- export/version macros: `YOUR_LIBRARY_*`
- `vcpkg` package name and registry metadata
- documentation placeholders, governance contacts, and release workflow details
