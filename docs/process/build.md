# Build Guide

## Build Layout

All structured builds live under `build/`.
`CMakePresets.json` uses:

```text
${sourceDir}/build/${presetName}
```

Do not create ad-hoc build directories at repository root.

## Standard Flow

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

Release build:

```bash
cmake --preset dev-release
cmake --build --preset dev-release
```

Install:

```bash
cmake --install build/dev-release
```

## Library Outputs

The scaffold supports building both library forms in the same configure/build.
Default behavior:

- `YOUR_LIBRARY_BUILD_SHARED=ON`
- `YOUR_LIBRARY_BUILD_STATIC=ON`

Generated outputs depend on platform and toolchain, but on Windows MinGW the key artifacts are:

- shared library: `bin/libyour_library.dll`
- import library for the shared build: `lib/libyour_library.dll.a`
- static library: `lib/libyour_library_static.a`

## Preset Semantics

Top-level presets behave as follows:

- `dev-*`: no `vcpkg` toolchain, compiler selected from the active environment
- `vcpkg-*`: uses the `vcpkg` CMake toolchain file
- compiler-specific presets such as `linux-gcc-*`, `linux-clang-*`, `windows-msvc-*`, `windows-mingw-*`, and `windows-clang-*` pin the compiler family explicitly

## vcpkg And Compiler Alignment

The scaffold aligns compiler-specific `vcpkg` builds through overlay triplets under `triplets/` and chainload toolchain files under `cmake/toolchains/`.

Compiler-specific `vcpkg` presets map to these triplets:

- `linux-gcc-vcpkg-*` -> `x64-linux-gcc`
- `linux-clang-vcpkg-*` -> `x64-linux-clang`
- `windows-msvc-vcpkg-*` -> `x64-windows`
- `windows-mingw-vcpkg-*` -> `x64-mingw-dynamic`
- `windows-clang-vcpkg-*` -> `x64-mingw-clang-dynamic`

## Tests

Unit tests are enabled by default through `YOUR_LIBRARY_BUILD_TESTS=ON`.
GoogleTest is discovered from the active environment or fetched via `FetchContent` when necessary.

## Docker

The Docker image provides a Linux build environment with CMake, Ninja, GCC, Clang, and Python 3.

## Windows Helper Scripts

`project_manager.bat` is the menu-driven Windows entry point.
The scripts in `scripts/` wrap preset-based configure, build, test, deploy, and `vcpkg` flows.
