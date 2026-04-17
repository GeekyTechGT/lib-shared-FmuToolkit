# lib-fmu-toolkit

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.25-064F8C.svg)](https://cmake.org/)
[![FMI 2.0](https://img.shields.io/badge/FMI-2.0-green.svg)](https://svn.modelica.org/fmi/branches/public/specifications/v2.0/)
[![FMI 3.0](https://img.shields.io/badge/FMI-3.0-brightgreen.svg)](https://svn.modelica.org/fmi/branches/public/specifications/v3.0/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A C++20 library for loading, parsing, validating, and co-simulating FMI 2.0 and FMI 3.0
Functional Mock-up Units (FMUs). Designed as the installable, ABI-stable C++ alternative to
FMPy.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Repository Layout](#repository-layout)
4. [Prerequisites](#prerequisites)
   - [Windows](#windows)
   - [Linux / WSL](#linux--wsl)
   - [Docker](#docker)
5. [Quick Start — Windows](#quick-start--windows)
6. [Quick Start — Linux / WSL](#quick-start--linux--wsl)
7. [Quick Start — Docker](#quick-start--docker)
8. [Project Metadata — project.json](#project-metadata--projectjson)
9. [Python Virtual Environment — pyvenv](#python-virtual-environment--pyvenv)
10. [Project Manager — project_manager.py](#project-manager--project_managerpy)
11. [CMake Presets](#cmake-presets)
12. [CMake Options](#cmake-options)
13. [Shared Library Dependencies — _shared_lib](#shared-library-dependencies--_shared_lib)
14. [Deploy Layout](#deploy-layout)
15. [Runtime Distribution](#runtime-distribution)
16. [FMU Test Data and Cross-Platform Binaries](#fmu-test-data-and-cross-platform-binaries)
17. [Running the Tests](#running-the-tests)
18. [Running the Examples](#running-the-examples)
19. [Library Public API](#library-public-api)
20. [Building Manually with CMake](#building-manually-with-cmake)
21. [CI — GitHub Actions](#ci--github-actions)
22. [Troubleshooting](#troubleshooting)

---

## Overview

| Feature | Status |
|---|---|
| FMI 2.0 co-simulation | ✓ full |
| FMI 3.0 co-simulation | ✓ full — all scalar types (Float32/64, Int8–64, UInt8–64), Boolean, String, Binary, Clock |
| XSD-validated `modelDescription.xml` | ✓ libxml2 |
| CSV and JSON result export | ✓ |
| Static C++ runtime (no `libstdc++.dll`) | ✓ MinGW (`-static-libgcc -static-libstdc++`) |
| Windows build (32-bit and 64-bit) | ✓ MSYS2 MinGW |
| Linux build | ✓ system GCC |
| Docker build (Ubuntu 24.04) | ✓ |
| WSL build | ✓ |
| Qt ABI compatibility | ✓ MinGW only — Clang presets removed intentionally |
| 103 tests, Windows + Docker | ✓ |

---

## Architecture

The library follows a **Hexagonal (Ports & Adapters)** architecture, keeping all platform
and third-party details out of the domain core:

```
include/                        ← Public API headers (deployed flat into include/)
├── exceptions.hpp
├── export.hpp
├── fmu_engine.hpp
├── lib_fmu_toolkit.hpp
├── types.hpp
└── version.hpp

src/
├── core/
│   ├── domain/                 ← Pure domain entities: FmuPackage, ModelVariable, …
│   ├── usecases/               ← FmuImporter, FmuValidator, SimulationController, exporters
│   └── exceptions/             ← Internal exception hierarchy
├── ports/                      ← Abstract interfaces: IFmuLoader, IXmlValidator, …
├── adapters/
│   ├── xml/                    ← libxml2 parser and validator
│   ├── zip/                    ← libzip extractor
│   ├── simulation/             ← FMI 2.0 and 3.0 dlopen adapter
│   └── json/                   ← JSON result exporter (nlohmann/json, header-only)
└── fmu_engine.cpp              ← PIMPL facade — the only file in the public boundary
```

All public API symbols are hidden by default (`-fvisibility=hidden`); only those annotated
with `LIB_FMU_TOOLKIT_API` are exported. This prevents accidental ABI leakage.

---

## Repository Layout

```
lib-fmu-toolkit/
├── include/                    ← Public headers (deployed flat)
├── src/                        ← Private implementation
├── tests/
│   ├── unit/
│   ├── integration/
│   └── data/                   ← Generatore.fmu, Sommatore.fmu (Win + Linux binaries)
├── examples/                   ← 01–06 example applications
├── external/
│   ├── fmi-standard-2.0/       ← FMI 2.0 XSD schemas + headers
│   └── fmi-standard-3.0/       ← FMI 3.0 XSD schemas + headers
├── cmake/
│   ├── ProjectOptions.cmake
│   ├── SharedLibDeps.cmake     ← Resolves _shared_lib dependencies
│   └── lib_fmu_toolkitConfig.cmake.in
├── scripts/
│   ├── project_manager.py      ← Interactive TUI (replaces all .bat scripts)
│   └── build_linux_fmu_binaries.py  ← Builds Linux .so files for test FMUs
├── .github/workflows/ci.yml
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
├── project.json                ← Project metadata (name, version, deploy paths)
├── requirements.txt            ← Python deps (rich, colorama)
└── VERSION                     ← Version string (must match project.json)
```

---

## Prerequisites

### Windows

| Tool | Required version | Default installation path |
|---|---|---|
| MSYS2 | latest | `C:\eng_apps\msys64\` |
| MinGW-W64 64-bit toolchain | GCC ≥ 13 | `C:\eng_apps\msys64\mingw64\` |
| MinGW-W32 32-bit toolchain | GCC ≥ 13 | `C:\eng_apps\msys64\mingw32\` (optional) |
| CMake | ≥ 3.25 | `C:\eng_apps\cmake\cmake-4.2.0\` or in PATH |
| Python | ≥ 3.10 | In PATH |
| Docker Desktop | any | Required only for Docker presets |
| WSL 2 | any | Required only for WSL presets |

**Install MSYS2 packages for the 64-bit toolchain** (open the *MSYS2 MinGW64* terminal):

```bash
pacman -Syu   # update the package database first

pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-pkg-config \
          mingw-w64-x86_64-libxml2 \
          mingw-w64-x86_64-libzip
```

**Install MSYS2 packages for the optional 32-bit toolchain** (open the *MSYS2 MinGW32* terminal):

```bash
pacman -S mingw-w64-i686-gcc \
          mingw-w64-i686-ninja \
          mingw-w64-i686-cmake \
          mingw-w64-i686-pkg-config \
          mingw-w64-i686-libxml2 \
          mingw-w64-i686-libzip
```

> **Important:** The project assumes MSYS2 is installed at `C:\eng_apps\msys64\`. If your
> installation is elsewhere, update the `PATH` entries in `CMakePresets.json` and the
> `_MINGW_*` constants in `scripts/project_manager.py`.

### Linux / WSL

```bash
sudo apt-get update && sudo apt-get install -y \
    g++ \
    ninja-build \
    cmake \
    pkg-config \
    libxml2-dev \
    libzip-dev \
    python3 \
    python3-venv \
    python3-pip
```

Tested on Ubuntu 22.04 and 24.04. The minimum CMake version is 3.25; on older Ubuntu releases
install it from the [official Kitware APT repository](https://apt.kitware.com/).

### Docker

Only Docker Engine / Docker Desktop is required on the host machine. The `Dockerfile` at the
project root builds a self-contained Ubuntu 24.04 image with all build dependencies:

```bash
# Build the image once (from the project root)
docker build -t lib-fmu-toolkit-builder .
```

The image installs: `g++`, `gcc`, `ninja-build`, `cmake`, `pkg-config`, `libxml2-dev`,
`libzip-dev`, `python3`, `python3-venv`, `python3-pip`.

---

## Quick Start — Windows

These instructions use a plain Windows terminal (cmd, PowerShell, or VS Code integrated
terminal). You do **not** need to open an MSYS2 shell.

```bat
:: 1. Clone the repository
git clone https://github.com/your-org/lib-fmu-toolkit.git
cd lib-fmu-toolkit

:: 2. Create the Python virtual environment
python -m venv pyvenv

:: 3. Activate it
pyvenv\Scripts\activate

:: 4. Install Python dependencies
pip install -r requirements.txt

:: 5. Launch the interactive project manager
python scripts\project_manager.py
```

Inside the project manager:

1. Press **1** → select **windows-mingw64-debug** (or release)
2. Press **2** → Configure
3. Press **3** → Build
4. Press **6** → Run tests (103 tests, all green)
5. Press **7** → Deploy to `_shared_lib`
6. Press **0** → Quit

> **Why not use the MSYS2 shell?**
> The project manager automatically prepends `C:\eng_apps\msys64\mingw64\bin` to the
> subprocess PATH before invoking CMake and Ninja. This means MinGW's linker (`ld.exe`) and
> support tools are always found correctly, regardless of which terminal you use.

---

## Quick Start — Linux / WSL

```bash
# In a Linux terminal or WSL session
cd /mnt/d/GeekyTechRepos/lib-fmu-toolkit    # WSL path
# or
cd ~/projects/lib-fmu-toolkit               # native Linux path

# Install build tools (once per machine)
sudo apt-get update && sudo apt-get install -y \
    g++ ninja-build cmake pkg-config libxml2-dev libzip-dev \
    python3 python3-venv python3-pip

# Create the Python virtual environment
python3 -m venv pyvenv
source pyvenv/bin/activate

# Install Python dependencies
pip install -r requirements.txt

# Launch the project manager
python scripts/project_manager.py
```

Inside the project manager:

1. Press **1** → select **linux-mingw-debug** (or release)
2. Press **2** → Configure
3. Press **3** → Build
4. Press **6** → Run tests
5. Press **7** → Deploy

---

## Quick Start — Docker

```bash
# 1. Build the Docker image (one-time setup)
docker build -t lib-fmu-toolkit-builder .

# 2. Launch the project manager from Windows (cmd / PowerShell)
python scripts\project_manager.py
```

Inside the project manager:

1. Press **1** → select **docker-mingw-debug** (or release)
2. Press **2** → Configure  *(cmake runs inside the container)*
3. Press **3** → Build       *(cmake --build runs inside the container)*
4. Press **6** → Run tests   *(ctest runs inside the container)*
5. Press **7** → Deploy      *(cmake --install runs inside the container; output is copied to `_shared_lib`)*

Alternatively, run cmake directly without the project manager:

```bash
# Windows — Docker Desktop volumes use forward slashes
docker run --rm \
    -v "D:/GeekyTechRepos/lib-fmu-toolkit:/workspace" \
    -w /workspace \
    lib-fmu-toolkit-builder \
    bash -c "cmake --preset docker-mingw-debug && cmake --build build/docker-mingw-debug -- -j$(nproc)"

# Run tests
docker run --rm \
    -v "D:/GeekyTechRepos/lib-fmu-toolkit:/workspace" \
    lib-fmu-toolkit-builder \
    ctest --test-dir /workspace/build/docker-mingw-debug --output-on-failure
```

> **Note for Windows users:** When running `docker run` from an MSYS2/Git Bash shell,
> the shell may silently convert `/workspace` to a Windows path (e.g., `C:/Git/workspace`).
> Always use the project manager or Python's `subprocess` module to invoke Docker — Python
> does **not** perform this path conversion.

---

## Project Metadata — project.json

`project.json` at the repository root is the single source of truth for project identity.
It is read both by `scripts/project_manager.py` (Python `json.load`) and by
`cmake/SharedLibDeps.cmake` (CMake `string(JSON)`):

```json
{
    "name":         "lib-fmu-toolkit",
    "version":      "0.1.0",
    "description":  "C++ toolkit for loading, parsing, validating, and simulating FMI 2.0/3.0 FMUs",
    "license":      "MIT",
    "deploy_folder":   "D:/GeekyTechRepos/_shared_lib",
    "shared_lib_base": "D:/GeekyTechRepos/_shared_lib",
    "docker_image":    "lib-fmu-toolkit-builder",
    "dependencies": []
}
```

| Field | Used by | Description |
|---|---|---|
| `name` | both | Library name; becomes part of the deploy path |
| `version` | both | Must match the `VERSION` file at the repo root |
| `deploy_folder` | Python | Root into which `cmake --install` writes artifacts |
| `shared_lib_base` | CMake | Root from which dependency headers and archives are read |
| `docker_image` | Python | Name of the Docker image used for Docker presets |
| `dependencies` | CMake | Array of `{name, version}` objects declaring user-library deps |

> **Version synchronisation:** Whenever you bump the version, update **both** `project.json`
> and the `VERSION` file. The CMake `project()` call reads `VERSION` at configure time via
> `file(READ VERSION ...)`.

---

## Python Virtual Environment — pyvenv

The virtual environment is named `pyvenv/` and lives at the project root. It is excluded
from version control by `.gitignore`.

```bash
# ── Create ────────────────────────────────────────────────────────────────────
python  -m venv pyvenv      # Windows
python3 -m venv pyvenv      # Linux / WSL / Docker

# ── Activate ──────────────────────────────────────────────────────────────────
pyvenv\Scripts\activate          # Windows cmd
pyvenv\Scripts\Activate.ps1      # Windows PowerShell
source pyvenv/bin/activate       # Linux / WSL / Git Bash

# ── Install dependencies ──────────────────────────────────────────────────────
pip install -r requirements.txt

# ── Deactivate ────────────────────────────────────────────────────────────────
deactivate
```

`requirements.txt` declares:

| Package | Minimum | Purpose |
|---|---|---|
| `rich` | 13.7.0 | Colored tables, panels, progress bars, prompts |
| `colorama` | 0.4.6 | ANSI escape-code compatibility on Windows |

---

## Project Manager — project_manager.py

`scripts/project_manager.py` replaces all legacy `.bat` scripts with a single, interactive,
colored TUI. Always activate `pyvenv` before running it.

```bash
python scripts/project_manager.py
```

### Main menu

| Key | Action | Details |
|---|---|---|
| **1** | Select Preset | Tabular view of all 8 presets with availability indicators |
| **2** | Configure | `cmake --preset <name>` |
| **3** | Build | `cmake --build --preset <name>` |
| **4** | Clean preset | Delete `build/<preset>/` |
| **5** | Clean ALL | Delete the entire `build/` directory |
| **6** | Run Tests + HTML report | Runs `ctest`, parses JUnit XML, generates an HTML report and opens it in the browser |
| **7** | Deploy to `_shared_lib` | Installs the library into `<deploy_folder>/<name>/<version>/<preset>/` |
| **8** | Build Docker image | `docker build -t <docker_image> .` |
| **0** | Quit | |

### Executor model

Each preset has an assigned **executor** that determines how the cmake/ctest commands are
wrapped:

| Executor | When used | How it works |
|---|---|---|
| `native` | Windows presets | Runs cmake directly; injects `mingw64/bin` (or `mingw32/bin`) into the subprocess PATH so the MinGW linker is always found |
| `wsl` | `linux-mingw-*` | On Windows, wraps the command with `wsl.exe bash -lc "cd '/mnt/…' && cmake …"` |
| `docker` | `docker-mingw-*` | Wraps with `docker run --rm -v <volume>:/workspace -w /workspace <image> cmake …` |

### CMake / CTest discovery (Windows)

The script searches for `cmake.exe` and `ctest.exe` in this order:

1. System `PATH`
2. `C:/eng_apps/cmake/cmake-4.2.0/bin/`
3. `C:/Program Files/CMake/bin/`

### PATH injection for native MinGW presets

When running native Windows presets from any terminal (cmd, PowerShell, VS Code), the script
automatically prepends the correct MSYS2 bin directory to the child process environment:

- `mingw64` presets → `C:\eng_apps\msys64\mingw64\bin` + `C:\eng_apps\msys64\usr\bin`
- `mingw32` presets → `C:\eng_apps\msys64\mingw32\bin` + `C:\eng_apps\msys64\usr\bin`

This ensures that `g++.exe`, `ld.exe`, `as.exe`, `ninja.exe`, and `pkg-config.exe` are all
resolved correctly, even when the MSYS2 directories are not in the system-wide PATH.

---

## CMake Presets

All eight presets are defined in `CMakePresets.json`. The Clang presets that existed in
earlier versions have been removed — MinGW is the only supported toolchain, which is
required for ABI compatibility with Qt6's Windows build.

| Preset name | Target OS | Toolchain | Build type | Executor |
|---|---|---|---|---|
| `windows-mingw32-debug` | Windows | MSYS2 MinGW32 (`C:\eng_apps\msys64\mingw32`) | Debug | native |
| `windows-mingw32-release` | Windows | MSYS2 MinGW32 | Release | native |
| `windows-mingw64-debug` | Windows | MSYS2 MinGW64 (`C:\eng_apps\msys64\mingw64`) | Debug | native |
| `windows-mingw64-release` | Windows | MSYS2 MinGW64 | Release | native |
| `linux-mingw-debug` | Linux / WSL | System GCC (`/usr/bin/g++`) | Debug | wsl / native |
| `linux-mingw-release` | Linux / WSL | System GCC | Release | wsl / native |
| `docker-mingw-debug` | Docker (Ubuntu 24.04) | System GCC | Debug | docker |
| `docker-mingw-release` | Docker (Ubuntu 24.04) | System GCC | Release | docker |

Every preset:

- Uses **Ninja** as the generator
- Sets `LIB_FMU_TOOLKIT_PRESET_NAME` cache variable to its own name (used by `SharedLibDeps.cmake`)
- Places all binaries in `build/<preset-name>/bin/`, libraries in `build/<preset-name>/lib/`

---

## CMake Options

These variables can be passed on the command line or set in a `CMakeUserPresets.json` overlay:

| Variable | Default | Description |
|---|---|---|
| `LIB_FMU_TOOLKIT_BUILD_SHARED` | `ON` | Build the shared library (`.dll` / `.so`) |
| `LIB_FMU_TOOLKIT_BUILD_STATIC` | `ON` | Build the static archive (`.a`) |
| `LIB_FMU_TOOLKIT_BUILD_TESTS` | `ON` | Build the unit and integration test executable |
| `LIB_FMU_TOOLKIT_BUILD_EXAMPLES` | `ON` | Build the six example applications |
| `LIB_FMU_TOOLKIT_ENABLE_WARNINGS` | `ON` | Enable strict compiler warnings (`-Wall -Wextra …`) |
| `LIB_FMU_TOOLKIT_ENABLE_WERROR` | `OFF` | Treat all warnings as errors |
| `LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME` | `ON` | Link the MinGW C++ runtime statically (recommended) |
| `LIB_FMU_TOOLKIT_PRESET_NAME` | `""` | Set automatically by each preset; do not set manually |

Example — disable tests for a faster configure:

```bash
cmake --preset windows-mingw64-release -DLIB_FMU_TOOLKIT_BUILD_TESTS=OFF
```

---

## Shared Library Dependencies — _shared_lib

When `lib-fmu-toolkit` depends on another user-written library (not a system package), declare
it in `project.json`:

```json
{
    "dependencies": [
        { "name": "lib-my-signal-bus", "version": "2.0.0" }
    ]
}
```

`cmake/SharedLibDeps.cmake` reads this at configure time and expects the dependency to be
already deployed under `shared_lib_base` in the following layout:

```
D:/GeekyTechRepos/_shared_lib/
└── lib-my-signal-bus/
    └── 2.0.0/
        └── windows-mingw64-debug/    ← must match LIB_FMU_TOOLKIT_PRESET_NAME exactly
            ├── include/              ← directory is added to the compiler's include path
            │   └── *.hpp
            ├── lib/
            │   └── *.a              ← preferred: statically linked
            └── bin/
                └── *.dll            ← fallback if no .a is found (with a warning)
```

CMake raises a fatal error if a declared dependency is missing from disk, so you will catch
missing deps at configure time rather than at link time.

---

## Deploy Layout

After running option **7** (Deploy), the install tree is written to:

```
<deploy_folder>/<name>/<version>/<preset>/
```

For example, deploying `windows-mingw64-release` produces:

```
D:/GeekyTechRepos/_shared_lib/
└── lib-fmu-toolkit/
    └── 0.1.0/
        └── windows-mingw64-release/
            ├── bin/
            │   ├── lib-fmu-toolkit.dll       ← shared library
            │   └── libwinpthread-1.dll        ← MinGW threading runtime (bundled automatically)
            ├── include/                       ← headers installed FLAT — no subdirectory
            │   ├── exceptions.hpp
            │   ├── export.hpp
            │   ├── fmu_engine.hpp
            │   ├── lib_fmu_toolkit.hpp
            │   ├── types.hpp
            │   └── version.hpp
            └── lib/
                ├── lib-fmu-toolkit.a          ← static archive
                └── liblib-fmu-toolkit.dll.a   ← Windows import library
```

And for `docker-mingw-debug` (Linux shared object):

```
D:/GeekyTechRepos/_shared_lib/
└── lib-fmu-toolkit/
    └── 0.1.0/
        └── docker-mingw-debug/
            ├── include/                       ← headers installed flat
            │   ├── exceptions.hpp
            │   ├── export.hpp
            │   ├── fmu_engine.hpp
            │   ├── lib_fmu_toolkit.hpp
            │   ├── types.hpp
            │   └── version.hpp
            └── lib/
                ├── lib-fmu-toolkit.so         ← symlink → lib-fmu-toolkit.so.1
                ├── lib-fmu-toolkit.so.1       ← symlink → lib-fmu-toolkit.so.1.0.0
                ├── lib-fmu-toolkit.so.1.0.0   ← versioned shared object
                └── lib-fmu-toolkit.a          ← static archive
```

> **Flat headers:** Public headers are installed directly into `include/`, not into a
> `include/libfmu_toolkit/` subdirectory. Consumer CMake projects should add
> `<prefix>/include` to their include path and include headers without a prefix:
>
> ```cpp
> #include "fmu_engine.hpp"
> #include "types.hpp"
> ```

---

## Runtime Distribution

| Runtime component | Windows status | Linux status | Notes |
|---|---|---|---|
| `libstdc++-6.dll` | Statically linked | n/a | `-static-libstdc++` |
| `libgcc_s_seh-1.dll` | Statically linked | n/a | `-static-libgcc` |
| `libwinpthread-1.dll` | **Bundled** (by deploy step) | n/a | MinGW POSIX threading; copied from `mingw64/bin/` by `project_manager.py` |
| `msvcrt.dll` | Dynamic (system) | n/a | Always present on any Windows installation |
| `libxml2.so` | n/a | Linked from system | `libxml2-dev` package |
| `libzip.so` | n/a | Linked from system | `libzip-dev` package |

**End users on Windows** need only two files: `lib-fmu-toolkit.dll` and
`libwinpthread-1.dll`. No MSYS2 or MinGW installation is required on the target machine.

**On Linux**, `libxml2` and `libzip` are linked as shared libraries and must be installed
via the distribution package manager.

---

## FMU Test Data and Cross-Platform Binaries

The test suite uses two hand-crafted FMI 3.0 FMUs located in `tests/data/`:

| FMU | Model | Inputs | Outputs |
|---|---|---|---|
| `Generatore.fmu` | Linear ramp: `y = t × 5.0` | — | `y` (Float64) |
| `Sommatore.fmu` | Adder: `y = u1 + u2` | `u1`, `u2` (Float64) | `y` (Float64) |

Both FMUs contain **platform binaries for both Windows and Linux** in the standard
FMI 3.0 directory layout:

```
Generatore.fmu (zip)
├── modelDescription.xml
├── binaries/
│   ├── x86_64-windows/
│   │   └── Generatore.dll          ← pre-built Windows DLL
│   └── x86_64-linux/
│       └── Generatore.so           ← pre-built Linux shared object
└── sources/
    └── generatore.c                ← C source (FMI 3.0)
```

### Rebuilding the Linux FMU binaries

If you need to regenerate the Linux `.so` files from the C sources — for example after
pulling the repository on a machine that previously only had the Windows binaries — run:

```bash
# From the project root (pyvenv must be active)
python scripts/build_linux_fmu_binaries.py
```

This script:

1. Opens each `.fmu` archive and extracts the C source file.
2. Compiles the source inside the `lib-fmu-toolkit-builder` Docker container using
   `gcc -shared -fPIC -I/workspace/external/fmi-standard-3.0/headers`.
3. Injects the resulting `.so` back into the FMU zip archive under
   `binaries/x86_64-linux/`.

The script is idempotent — if a Linux binary is already present in the archive, it prints
a message and skips recompilation.

> **Prerequisite:** The Docker image (`lib-fmu-toolkit-builder`) must exist. Build it once
> with option **8** in the project manager, or directly:
> ```bash
> docker build -t lib-fmu-toolkit-builder .
> ```

---

## Running the Tests

### Via the project manager (recommended)

Select option **6** after building. The project manager runs `ctest`, captures output,
parses the JUnit XML file, generates an HTML report in `build/<preset>/test_results.html`,
and offers to open it automatically in the default browser.

### Directly with ctest

```bash
# Windows — debug preset
ctest --test-dir build/windows-mingw64-debug --output-on-failure

# Linux / WSL — debug preset
ctest --test-dir build/linux-mingw-debug --output-on-failure

# Docker — debug preset (from Windows, via project_manager or Python)
docker run --rm \
    -v "D:/GeekyTechRepos/lib-fmu-toolkit:/workspace" \
    lib-fmu-toolkit-builder \
    ctest --test-dir /workspace/build/docker-mingw-debug --output-on-failure
```

### Test suite breakdown

The suite contains **103 tests** covering:

| Test suite | Count | What it covers |
|---|---|---|
| `GeneratoreTest` | 13 | Load, inspect, validate, simulate, export, step-by-step, unload |
| `SommatoreTest` | 12 | Same lifecycle tests for the two-input adder FMU |
| `CrossFmuTest` | 1 | Two FMUs loaded simultaneously in the same engine |
| `FmuEngineTest` | 8 | Engine construction, error paths, empty export |
| `FmuEngineIntegrationTest` | 1 | Full pipeline: load → validate → simulate |
| `ValidatorStructureTest` | 2 | Missing path / missing schema detection |
| `ValidatorBinaryDirTest` | 4 | FMI 2.0 vs 3.0 binary directory naming checks |
| `ValidatorSchemaTest` | 2 | XSD validation pass/fail |
| `ValidatorSemanticTest` | 5 | Duplicate VRs, invalid causality/variability combinations |
| `Fmi2ParserTest` | 14 | All FMI 2.0 XML parsing paths |
| `Fmi3ParserTest` | 9 | All FMI 3.0 XML parsing paths |
| `ParserErrorTest` | 4 | Malformed / missing XML |
| `ParserEdgeTest` | 2 | Edge cases (no variables, no default experiment) |
| `CsvExporterTest` | 7 | CSV output correctness |
| `JsonExporterTest` | 7 | JSON output correctness |
| `SimulationControllerTest` | 9 | Step logic, recording interval, cancellation |

---

## Running the Examples

All example executables land in `build/<preset>/bin/`. On Windows, `lib-fmu-toolkit.dll`
and `libwinpthread-1.dll` are copied next to each executable at build time so they can be
run without modifying `PATH`.

```bat
:: Windows — from the project root
build\windows-mingw64-debug\bin\example_01_basic_simulate.exe tests\data\Generatore.fmu

build\windows-mingw64-debug\bin\example_02_step_by_step.exe tests\data\Generatore.fmu

build\windows-mingw64-debug\bin\example_03_validate_fmu.exe tests\data\Generatore.fmu

build\windows-mingw64-debug\bin\example_04_export_results.exe tests\data\Generatore.fmu results.json

build\windows-mingw64-debug\bin\example_05_generatore_sommatore.exe ^
    tests\data\Generatore.fmu tests\data\Sommatore.fmu

build\windows-mingw64-debug\bin\example_06_batch_validate.exe tests\data\
```

| Example | Description |
|---|---|
| `01_basic_simulate` | Load an FMU → inspect metadata → run a full co-simulation → print result columns |
| `02_step_by_step` | Manual `doStep` loop using `FmuSession` — demonstrates per-step I/O |
| `03_validate_fmu` | XSD schema + semantic validation only, no simulation |
| `04_export_results` | Simulate and export the result series to JSON |
| `05_generatore_sommatore` | Two-FMU composition: feed the ramp generator's output into the adder |
| `06_batch_validate` | Scan a directory for `.fmu` files and validate each one |

---

## Library Public API

Include headers directly (no subdirectory prefix after deploy):

```cpp
#include "lib_fmu_toolkit.hpp"   // master include — pulls in everything
// or individually:
#include "fmu_engine.hpp"
#include "types.hpp"
#include "exceptions.hpp"
```

### Complete usage example

```cpp
#include "lib_fmu_toolkit.hpp"
#include <iostream>

int main()
{
    libfmu_toolkit::FmuEngine engine;

    // ── Load ──────────────────────────────────────────────────────────────
    uint64_t handle = engine.load("path/to/model.fmu");

    // ── Inspect metadata ──────────────────────────────────────────────────
    libfmu_toolkit::FmuInfo info = engine.info(handle);
    std::cout << "Model: " << info.modelName
              << "  FMI: " << info.fmiVersion << "\n";
    for (const auto& var : info.variables)
        std::cout << "  " << var.name << " [" << var.valueReference << "]\n";

    // ── Validate ──────────────────────────────────────────────────────────
    auto vr = engine.validate(handle);
    if (vr.hasErrors()) {
        for (const auto& e : vr.errors)
            std::cerr << "[ERROR] " << e.message << "\n";
        return 1;
    }

    // ── Simulate (one-shot) ───────────────────────────────────────────────
    libfmu_toolkit::SimConfig cfg;
    cfg.startTime       = 0.0;
    cfg.stopTime        = 10.0;
    cfg.stepSize        = 0.01;
    cfg.recordingInterval = 0.1;   // record every 0.1 s instead of every step

    auto result = engine.simulate(handle, cfg);
    // result.series["y"] → std::vector<libfmu_toolkit::ScalarValue>

    // ── Export ────────────────────────────────────────────────────────────
    engine.exportCsv(result, "output.csv");
    engine.exportJson(result, "output.json");

    // ── Step-by-step session ──────────────────────────────────────────────
    auto session = engine.openSession(handle, cfg);
    while (!session.done()) {
        session.setReal("u1", 3.14);
        session.doStep();
        double y = session.getReal("y");
        std::cout << "y = " << y << "\n";
    }
    session.close();

    // ── Unload ────────────────────────────────────────────────────────────
    engine.unload(handle);
}
```

### Linking against the deployed library (CMake consumer)

```cmake
# In the consuming project's CMakeLists.txt
set(LIB_FMU_TOOLKIT_DIR
    "D:/GeekyTechRepos/_shared_lib/lib-fmu-toolkit/0.1.0/windows-mingw64-release")

add_library(lib_fmu_toolkit::shared SHARED IMPORTED)
set_target_properties(lib_fmu_toolkit::shared PROPERTIES
    IMPORTED_LOCATION          "${LIB_FMU_TOOLKIT_DIR}/bin/lib-fmu-toolkit.dll"
    IMPORTED_IMPLIB            "${LIB_FMU_TOOLKIT_DIR}/lib/liblib-fmu-toolkit.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${LIB_FMU_TOOLKIT_DIR}/include"
)

target_link_libraries(my_app PRIVATE lib_fmu_toolkit::shared)
```

---

## Building Manually with CMake

Use this approach when you want to bypass the project manager or integrate with your own
build orchestration.

```bash
# ── Configure ─────────────────────────────────────────────────────────────────
cmake --preset windows-mingw64-debug

# ── Build ──────────────────────────────────────────────────────────────────────
cmake --build build/windows-mingw64-debug

# ── Test ───────────────────────────────────────────────────────────────────────
ctest --test-dir build/windows-mingw64-debug --output-on-failure

# ── Deploy ─────────────────────────────────────────────────────────────────────
cmake --install build/windows-mingw64-debug \
      --prefix "D:/GeekyTechRepos/_shared_lib/lib-fmu-toolkit/0.1.0/windows-mingw64-debug"
```

> **Windows PATH requirement:** CMake calls Ninja which calls `g++.exe` and `ld.exe`. These
> tools are found only if `C:\eng_apps\msys64\mingw64\bin` is in PATH. The project manager
> handles this automatically. If you run cmake manually from a plain Windows terminal, add
> the MinGW bin directory to PATH first:
>
> ```bat
> set PATH=C:\eng_apps\msys64\mingw64\bin;C:\eng_apps\msys64\usr\bin;%PATH%
> cmake --preset windows-mingw64-debug
> ```

---

## CI — GitHub Actions

The workflow at `.github/workflows/ci.yml` runs on Ubuntu Latest for every push and pull
request to `main` / `master`:

```yaml
- name: Install toolchain
  run: |
    sudo apt-get update
    sudo apt-get install -y ninja-build g++ cmake pkg-config libxml2-dev libzip-dev

- name: Configure
  run: cmake --preset linux-mingw-debug

- name: Build
  run: cmake --build --preset linux-mingw-debug

- name: Test
  run: ctest --test-dir build/linux-mingw-debug --output-on-failure
```

No vcpkg, no Clang, no Docker in CI — the native GCC toolchain on the GitHub runner is
sufficient.

---

## Troubleshooting

### 1. Linker exits silently with code 1 on Windows (no error message)

**Symptom:** `cmake --build` fails at the link step. The Ninja output shows the full linker
command but no error text. The DLL is not created.

**Cause:** CMake wraps the linker command in `cmd.exe /C "…"`. When MinGW's `ld.exe` cannot
find its support executables (`cc1plus.exe`, `as.exe`, etc.), it exits with code 1 without
printing anything to stderr — because `cmd.exe` swallows the output stream.

**Root cause:** `C:\eng_apps\msys64\mingw64\bin` is not in the PATH of the calling process.

**Fix (project manager):** The project manager always injects the correct MSYS2 bin directory
into the subprocess environment. Use the project manager instead of running cmake directly.

**Fix (manual cmake):** Add the MSYS2 bin directories to PATH before calling cmake:

```bat
set PATH=C:\eng_apps\msys64\mingw64\bin;C:\eng_apps\msys64\usr\bin;%PATH%
cmake --build build/windows-mingw64-debug
```

---

### 2. Docker `docker run` converts `/workspace` to a Windows path

**Symptom:** Running `docker run … -w /workspace …` from an MSYS2 bash or Git Bash
terminal fails with:

```
docker: Error response from daemon: the working directory
'C:/eng_apps/Git/workspace' is invalid
```

**Cause:** MSYS2/Git Bash intercepts absolute Unix-style paths in command-line arguments
and converts them to Windows paths before passing them to the process.

**Fix:** Always invoke `docker run` from Python's `subprocess` module (as the project
manager does), or from a plain Windows cmd / PowerShell session where no path conversion
occurs. Alternatively, set `MSYS_NO_PATHCONV=1` in your shell session:

```bash
MSYS_NO_PATHCONV=1 docker run --rm \
    -v "D:/GeekyTechRepos/lib-fmu-toolkit:/workspace" \
    -w /workspace lib-fmu-toolkit-builder bash
```

---

### 3. Integration tests fail in Docker — "shared library not found in binaries/"

**Symptom:** Tests that load `Generatore.fmu` or `Sommatore.fmu` fail inside Docker with:

```
ZipLoaderAdapter::findSharedLibrary — shared library for model 'Generatore'
not found in: /tmp/libfmu_toolkit/.../Generatore_.../binaries
```

**Cause:** The FMU archive does not contain a `binaries/x86_64-linux/Generatore.so` entry.
This happens if the FMU was originally created on Windows and never had its Linux binaries
added.

**Fix:** Run the Linux FMU binary builder:

```bash
python scripts/build_linux_fmu_binaries.py
```

This script uses Docker to compile the FMU's C sources and injects the resulting `.so`
files into the FMU archives. The Docker image must exist first:

```bash
docker build -t lib-fmu-toolkit-builder .
```

---

### 4. `python scripts/project_manager.py` fails with `ModuleNotFoundError: No module named 'rich'`

**Cause:** The virtual environment is not activated, or `pip install -r requirements.txt`
was never run.

**Fix:**

```bash
# Windows
pyvenv\Scripts\activate
pip install -r requirements.txt

# Linux / WSL
source pyvenv/bin/activate
pip install -r requirements.txt
```

If `pyvenv/` does not exist, create it first:

```bash
python  -m venv pyvenv   # Windows
python3 -m venv pyvenv   # Linux / WSL
```

---

### 5. CMake cannot find `libxml2` or `libzip` on Windows

**Symptom:** Configure step fails with:

```
Could not find a package configuration file provided by "LibXml2"
```

or `pkg_check_modules` fails for `libxml-2.0` or `libzip`.

**Cause:** The MinGW64 packages `mingw-w64-x86_64-libxml2` and `mingw-w64-x86_64-libzip`
are not installed, or `PKG_CONFIG_PATH` / `PKG_CONFIG` are not pointing to the MinGW64
sysroot.

**Fix:**

```bash
# Open the MSYS2 MinGW64 terminal (not MSYS2 UCRT64 or MSYS2 MSYS)
pacman -S mingw-w64-x86_64-libxml2 mingw-w64-x86_64-libzip
```

Verify that the presets set the correct `PKG_CONFIG` and `PKG_CONFIG_PATH` in
`CMakePresets.json`. The MinGW64 entries should read:

```json
"PKG_CONFIG":      "C:/eng_apps/msys64/mingw64/bin/pkg-config.exe",
"PKG_CONFIG_PATH": "C:/eng_apps/msys64/mingw64/lib/pkgconfig"
```

---

### 6. Tests exit with code `0xC0000135` (STATUS_DLL_NOT_FOUND) on Windows

**Symptom:** `ctest` output shows tests failing with exit code `0xC0000135`.

**Cause:** `libwinpthread-1.dll` (or another DLL) is not in the same directory as the test
executable.

**Fix:** The CMake build is configured to copy `libwinpthread-1.dll` next to each
executable at build time via a `POST_BUILD` command. If the DLL is missing, rebuild:

```bash
cmake --build build/windows-mingw64-debug -- -j4
```

If the error persists, verify that the DLL exists in the compiler's bin directory:

```
C:\eng_apps\msys64\mingw64\bin\libwinpthread-1.dll
```

If it is missing, reinstall the `mingw-w64-x86_64-winpthreads-git` package from MSYS2.

---

### 7. WSL configure fails — `cmake: command not found`

**Symptom:** Selecting a `linux-mingw-*` preset in the project manager and pressing
**Configure** prints `bash: cmake: command not found` inside WSL.

**Cause:** CMake is not installed inside the WSL distribution.

**Fix:**

```bash
# Inside WSL
sudo apt-get update
sudo apt-get install -y cmake ninja-build g++ pkg-config libxml2-dev libzip-dev
```

---

### 8. Deploy produces `include/libfmu_toolkit/` subdirectory (old layout)

**Symptom:** After deploying, headers land at `include/libfmu_toolkit/fmu_engine.hpp`
instead of the expected flat `include/fmu_engine.hpp`.

**Cause:** The build directory was configured with an older version of `CMakeLists.txt`
that used `FILE_SET public_headers` with a `BASE_DIRS` pointing to `include/`. The old
ninja cache is stale.

**Fix:** Delete the build directory and reconfigure from scratch:

```bash
# Via project manager
# Option 4 → Clean current preset
# Then Option 2 → Configure, Option 3 → Build, Option 7 → Deploy

# Or manually:
rm -rf build/windows-mingw64-release
cmake --preset windows-mingw64-release
cmake --build build/windows-mingw64-release
cmake --install build/windows-mingw64-release --prefix "..."
```

---

### 9. Docker image not found when running Docker presets

**Symptom:** Selecting a `docker-mingw-*` preset shows `✗` in the Ready column or the
configure step fails with:

```
Error response from daemon: No such image: lib-fmu-toolkit-builder
```

**Fix:** Build the Docker image using option **8** in the project manager, or manually:

```bash
docker build -t lib-fmu-toolkit-builder .
```

The image is based on `ubuntu:24.04` and is approximately 350 MB after build.

---

### 10. `g++` produces an object file but the DLL is not created, no error shown

This is a variant of issue #1 above. To diagnose it independently:

```bash
# Run the linker with MSYS PATH from Git Bash or MSYS2
export PATH=/c/eng_apps/msys64/mingw64/bin:$PATH
cd build/windows-mingw64-debug

# Re-run cmake build — now g++ and ld are on PATH and errors surface normally
cmake --build . -- -j1
```

If `ld.exe` reports `undefined reference` errors, the object files exist but some source
files were not compiled (check for compilation errors earlier in the build log). If `ld.exe`
reports `file truncated` on `/dev/null`, your MSYS2 terminal is trying to write to the
MSYS `/dev/null` device — always use a real file path as the output target.
