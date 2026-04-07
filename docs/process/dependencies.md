# Dependencies

## Runtime

At source level, the scaffold library depends only on the C++ standard library.

## Build-Time

Required:

- CMake 3.25+
- a C++23-capable compiler
- Ninja for preset-driven workflows

Supported compiler families in the shipped presets:

- GCC
- Clang
- MSVC
- MinGW-w64 GCC
- MSYS2 Clang targeting MinGW

## Test-Time

- GoogleTest

The test build first attempts `find_package(GTest CONFIG QUIET)` and falls back to `FetchContent` when necessary.

## Packaging

The scaffold ships private-registry-oriented `vcpkg` assets under `packaging/vcpkg/` and overlay triplets under `triplets/`.

Important naming split:

- library/package config name: `your_library`
- vcpkg port name: `your-library`
