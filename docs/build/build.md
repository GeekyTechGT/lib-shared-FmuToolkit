# Build System Reference

## Prerequisites

| Tool | Minimum version | Notes |
| --- | --- | --- |
| CMake | 3.25 | Required for preset support (`--preset`) |
| Ninja | any | Strongly recommended; Makefiles also work |
| GCC | 11 | C++20 support required |
| Clang | 13 | C++20 support required |
| libxml2 | 2.9 | XML parsing and XSD validation |
| libzip | 1.8 | FMU ZIP archive extraction |
| GoogleTest | 1.12 | Auto-fetched if not found |

> **Note:** MSVC is intentionally not supported. The library targets cross-platform
> toolchains (GCC and Clang) that work identically on Windows (via MSYS2/MinGW) and Linux.
> This is a prerequisite for use in Qt6 projects, which provide official MinGW packages for
> Windows.

---

## Available Presets

Presets are defined in `CMakePresets.json`. All presets use Ninja and set
`CMAKE_EXPORT_COMPILE_COMMANDS=ON` for clang tooling.

### Generic presets (no explicit toolchain)

| Preset | Build type | Notes |
| --- | --- | --- |
| `dev-debug` | Debug | AddressSanitizer + UBSan enabled |
| `dev-release` | Release | — |
| `vcpkg-debug` | Debug | vcpkg toolchain + sanitizers |
| `vcpkg-release` | Release | vcpkg toolchain |

### Linux presets

| Preset | Compiler | Build type | vcpkg |
| --- | --- | --- | --- |
| `linux-gcc-debug` | `/usr/bin/g++` | Debug | — |
| `linux-gcc-release` | `/usr/bin/g++` | Release | — |
| `linux-gcc-vcpkg-debug` | `/usr/bin/g++` | Debug | ✔ (`x64-linux-gcc`) |
| `linux-gcc-vcpkg-release` | `/usr/bin/g++` | Release | ✔ (`x64-linux-gcc`) |
| `linux-clang-debug` | `/usr/bin/clang++` | Debug | — |
| `linux-clang-release` | `/usr/bin/clang++` | Release | — |
| `linux-clang-vcpkg-debug` | `/usr/bin/clang++` | Debug | ✔ (`x64-linux-clang`) |
| `linux-clang-vcpkg-release` | `/usr/bin/clang++` | Release | ✔ (`x64-linux-clang`) |

### Windows presets (MSYS2 required)

| Preset | Compiler | Build type | vcpkg |
| --- | --- | --- | --- |
| `windows-mingw-debug` | MinGW `g++.exe` | Debug | — |
| `windows-mingw-release` | MinGW `g++.exe` | Release | — |
| `windows-mingw-vcpkg-debug` | MinGW `g++.exe` | Debug | ✔ (`x64-mingw-dynamic`) |
| `windows-mingw-vcpkg-release` | MinGW `g++.exe` | Release | ✔ (`x64-mingw-dynamic`) |
| `windows-clang-debug` | MSYS2 `clang++.exe` | Debug | — |
| `windows-clang-release` | MSYS2 `clang++.exe` | Release | — |
| `windows-clang-vcpkg-debug` | MSYS2 `clang++.exe` | Debug | ✔ (`x64-mingw-clang-dynamic`) |
| `windows-clang-vcpkg-release` | MSYS2 `clang++.exe` | Release | ✔ (`x64-mingw-clang-dynamic`) |

MSYS2 paths are `C:/eng_apps/msys64/`. Adjust in `CMakePresets.json` if installed elsewhere.

---

## Build Options

Pass these as `-D<OPTION>=ON/OFF` on the command line, or set them in a preset's
`cacheVariables` block.

| Option | Default | Description |
| --- | --- | --- |
| `LIB_FMU_TOOLKIT_BUILD_SHARED` | ON | Build the shared library (`lib-fmu-toolkit.dll/.so`) |
| `LIB_FMU_TOOLKIT_BUILD_STATIC` | ON | Build the static library (`lib-fmu-toolkit.a/.lib`) |
| `LIB_FMU_TOOLKIT_BUILD_TESTS` | ON | Build the GoogleTest test executable |
| `LIB_FMU_TOOLKIT_BUILD_EXAMPLES` | ON | Build the six example executables |
| `LIB_FMU_TOOLKIT_ENABLE_WARNINGS` | ON | Pass strict warning flags to the compiler |
| `LIB_FMU_TOOLKIT_ENABLE_WERROR` | OFF | Treat all warnings as errors |
| `LIB_FMU_TOOLKIT_ENABLE_SANITIZERS` | OFF | Enable ASan + UBSan (Debug builds only) |
| `LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME` | ON | Link MinGW runtime statically (`-static-libgcc -static-libstdc++`) |

At least one of `BUILD_SHARED` or `BUILD_STATIC` must be ON.

### ABI compile definitions (automatically set)

The following compile definitions are set **automatically** by the CMake target and propagate to
all consumers via `target_link_libraries`. You do not need to set them manually.

| Definition | Value | Purpose |
| --- | --- | --- |
| `_GLIBCXX_USE_CXX11_ABI` | `1` | Pins libstdc++ to the C++11 (non-COW) string/list ABI (GCC ≥ 5 default). Prevents silent mismatches with code compiled on legacy GCC 4 systems where the default was `0`. Applied on all non-MSVC toolchains. |

If your downstream project defines `_GLIBCXX_USE_CXX11_ABI=0` explicitly, CMake will emit a
redefinition warning. Remove the explicit definition from your project and let the library
propagate the correct value.

---

## Dependency Resolution

### With vcpkg

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset linux-gcc-vcpkg-release
cmake --build --preset linux-gcc-vcpkg-release
```

vcpkg reads `vcpkg.json` (the manifest) and automatically installs `libxml2` and `libzip`.
GoogleTest is fetched via CMake's `FetchContent` if not already provided by vcpkg.

### Without vcpkg — Linux system packages

```bash
# Ubuntu / Debian
sudo apt install libxml2-dev libzip-dev

# Arch Linux
sudo pacman -S libxml2 libzip

# Fedora
sudo dnf install libxml2-devel libzip-devel
```

Then use a non-vcpkg preset:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

### Without vcpkg — Windows (MSYS2 packages)

```bash
# In MSYS2 MinGW64 shell
pacman -S mingw-w64-x86_64-libxml2 mingw-w64-x86_64-libzip
```

Then use a non-vcpkg Windows preset:

```bat
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
```

---

## Static vs Shared Library

The CMake build produces up to two targets:

| Target | Alias | Output |
| --- | --- | --- |
| `lib_fmu_toolkit_shared` | `lib_fmu_toolkit::shared` | `lib-fmu-toolkit.dll` / `.so` |
| `lib_fmu_toolkit_static` | `lib_fmu_toolkit::static` | `lib-fmu-toolkit.a` / `.lib` |
| — | `lib_fmu_toolkit::lib_fmu_toolkit` | resolves to shared if present, else static |

The `lib_fmu_toolkit::lib_fmu_toolkit` alias is the recommended target for consumers. It
resolves to shared by default, which is suitable for production deployments. The examples and
tests link against `lib_fmu_toolkit::static` to gain access to internal symbols without
requiring DLL export annotations on every private class.

### MinGW static runtime linkage

When `LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME=ON` (the default on Windows), the MinGW C++
runtime is linked statically into the output binary. This makes the `.exe` and `.dll` fully
self-contained — no `libstdc++-6.dll` or `libgcc_s_seh-1.dll` needs to be distributed alongside
the binaries.

---

## Installation

```bash
cmake --install build/<preset> --prefix /opt/lib-fmu-toolkit
```

The install tree:

```
/opt/lib-fmu-toolkit/
├── include/libfmu_toolkit/       # Public headers
├── lib/
│   ├── lib-fmu-toolkit.so        # Shared library (Linux)
│   ├── lib-fmu-toolkit.a         # Static library
│   └── cmake/lib_fmu_toolkit/    # CMake package files
└── share/lib-fmu-toolkit/
    └── schemas/
        ├── fmi-standard-2.0/     # FMI 2.0 XSD schemas
        └── fmi-standard-3.0/     # FMI 3.0 XSD schemas
```

---

## Qt6 Integration

This library is Qt-agnostic (no Qt dependency), but it is designed to be embedded in Qt6
applications. The cross-platform MinGW/Clang toolchain choice ensures compatibility with Qt's
official Windows packages.

### CMake integration with a Qt6 project

```cmake
# Find Qt6 (must be in CMAKE_PREFIX_PATH)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Find the library
find_package(lib_fmu_toolkit CONFIG REQUIRED)

# Link both
target_link_libraries(my_qt_app
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        lib_fmu_toolkit::lib_fmu_toolkit)
```

### Qt Creator configuration

Open Qt Creator → Projects → Build → CMake:

- Add `CMAKE_PREFIX_PATH` pointing to the Qt installation.
- Select the MinGW or Clang kit matching one of the presets above.
- Use the preset name as the CMake build configuration name.

---

## Compile Commands

All presets set `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. The resulting
`build/<preset>/compile_commands.json` enables:

- **clangd** language server (VS Code, CLion).
- **clang-tidy** static analysis.
- **clang-format** code formatting.

```bash
# Run clang-tidy on all source files
scripts/pm_lint.bat build/windows-mingw-debug

# Run clang-format on all source files
scripts/pm_format.bat
```
