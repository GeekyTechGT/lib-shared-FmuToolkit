# Customer vcpkg Consumption Guide

## Purpose

This guide explains how customer applications should consume a library generated from this scaffold through a private `vcpkg` registry.

## Naming Rules

Keep the two public names distinct:

- vcpkg port name: `your-library`
- CMake package name: `your_library`

## 1. Register The Private Registry

In the customer application repository, create or update `vcpkg-configuration.json`:

```json
{
  "default-registry": {
    "kind": "builtin",
    "baseline": "<builtin-vcpkg-baseline-commit>"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/<org>/<registry-repo>.git",
      "baseline": "<custom-registry-baseline-commit>",
      "packages": [
        "your-library"
      ]
    }
  ]
}
```

## 2. Declare The Dependency

In the customer application's `vcpkg.json`:

```json
{
  "name": "customer-app",
  "version-string": "0.1.0",
  "dependencies": [
    "your-library"
  ]
}
```

## 3. Configure CMake With vcpkg

```cmake
find_package(your_library CONFIG REQUIRED)
target_link_libraries(customer_app PRIVATE your_library::your_library)
```

## 4. Choose The Triplet That Matches The Application Compiler

Recommended mapping:

- Windows MSVC application -> `x64-windows`
- Windows MinGW application -> `x64-mingw-dynamic` or `x64-mingw-static`
- Windows MSYS2 Clang / MinGW-runtime application -> `x64-mingw-clang-dynamic` or `x64-mingw-clang-static`
- Linux GCC application -> `x64-linux-gcc`
- Linux Clang application -> `x64-linux-clang`
