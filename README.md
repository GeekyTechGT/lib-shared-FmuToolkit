# FmuToolkit

> FmuToolkit - lib-shared

![Category](https://img.shields.io/badge/category-lib--shared-blue)
![Language](https://img.shields.io/badge/language-C++-orange)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-green)
![Artifact](https://img.shields.io/badge/artifact-libfmutoolkit.dll-lightgrey)

## Table of Contents

- [Description](#description)
- [Category](#category)
- [Prerequisites](#prerequisites)
- [Setup](#setup)
- [Project Structure](#project-structure)
- [Build](#build)
- [Test](#test)
- [Deployment](#deployment)
- [Consuming the Library](#consuming-the-library)
- [Documentation](#documentation)
- [License](#license)
- [Contributing](#contributing)

## Description

**FmuToolkit** is a reusable **shared library** (`.dll` on Windows, `.so`
on Linux) written in C++. It publishes a small, stable public
API under `include/fmutoolkit/` and hides its implementation behind it, so
consumers can pick up bug fixes by replacing the binary without
recompiling - as long as the ABI is kept compatible.

Internally the library follows **hexagonal architecture**: a framework-free
`domain/`, use cases in `application/`, and outbound adapters
(`adapter/out/`: persistence, filesystem, logging) behind ports. A library
has no inbound adapters by design - a CLI or GUI belongs in an `app` or
`tool` project that depends on this one.

Symbol visibility is **hidden by default** (`-fvisibility=hidden`); only
declarations annotated with `FMUTOOLKIT_EXPORT` (generated
by CMake's `GenerateExportHeader`) are exported. That keeps the ABI surface
explicit, reduces load time and prevents accidental coupling to internals.

## Category

| Property | Value |
|---|---|
| Category | `lib-shared` - dynamically linked library |
| Artifact name | `lib-shared-FmuToolkit` (folder, deploy name and dependency name) |
| Binaries | `libfmutoolkit.dll` + import library `libfmutoolkit.dll.a` (MinGW), `libfmutoolkit.so` (Linux) |
| Public headers | `include/fmutoolkit/fmutoolkit.hpp`, `fmutoolkit_export.hpp` |
| CMake target | `fmutoolkit` (alias `fmutoolkit::fmutoolkit`) |
| Architecture | Hexagonal (ports and adapters), no inbound adapters |
| Language | C++ |
| Build system | CMake >= 3.25 (presets) + Ninja |

Naming convention: the project folder is `lib-shared-<Name>` and the binary
is `lib<name>` in lower case (`lib-shared-FmuToolkit` -> `libfmutoolkit.dll`).
The same convention lets consumers resolve the library from its name alone.

## Prerequisites

| Tool | Version | Used for |
|---|---|---|
| C/C++ toolchain | GCC 13+ / MinGW-w64 (Windows) | compiling |
| CMake | 3.25+ | configure / build / install |
| Ninja | any recent | build backend of every preset |
| Python | 3.10+ | orchestration scripts |
| [uv](https://docs.astral.sh/uv/) | latest | Python environment for `scripts/` |
| Docker Desktop | optional | `docker-*` presets (Linux build) |
| Doxygen | optional | API reference |

## Setup

```bash
uv sync                                  # Python environment for scripts/
uv run python scripts/debug_env.py       # verify compilers, CMake, Ninja
```

## Project Structure

```text
lib-shared-FmuToolkit/
├── include/fmutoolkit/          # PUBLIC API - the only headers consumers include
├── src/                      # private implementation (hexagonal)
│   ├── domain/               # model, value objects, services, ports
│   ├── application/          # commands, DTOs, mappers, services
│   ├── adapter/out/          # persistence (SQLite), filesystem, logging
│   └── config/
├── examples/                 # runnable usage examples of the public API
├── tests/                    # unit/ and integration/ CTest suites
├── docs/                     # user manual, architecture, ADRs, specs
├── cmake/                    # reusable CMake modules
├── scripts/                  # prj_exec.py orchestrator and helpers
├── CMakeLists.txt  CMakePresets.json
├── project.json              # version, artifact_name, deploy target
└── CHANGELOG.md  INFO.md  LICENSE  Dockerfile
```

## Build

```bash
uv run python scripts/prj_exec.py                                    # interactive menu
uv run python scripts/prj_exec.py --preset windows-mingw64-debug --commands 5,6
```

| # | Command | # | Command |
|---|---|---|---|
| 1 | Select preset | 8 | Run examples |
| 2 | Sync external repositories | 9 | Run tests |
| 3 | Clean current preset | 10 | Deploy |
| 4 | Clean all presets | 11 | Full workflow (clean -> configure -> build -> deploy) |
| 5 | Configure | 13 | Generate documentation PDFs |
| 6 | Build | 14 | Quit |
| 7 | Build Docker image | | |

Build outputs land in `build/<preset>/lib/`. Useful CMake options:
`FMUTOOLKIT_BUILD_TESTS`, `FMUTOOLKIT_BUILD_EXAMPLES`,
`FMUTOOLKIT_ENABLE_SANITIZERS` (ASan/UBSan, Linux).

## Test

```bash
uv run python scripts/prj_exec.py --commands 8,9    # examples, then tests
```

Tests are CTest suites (`tests/unit/`, `tests/integration/`); reports go to
`build/<preset>/test-reports/`. Windows has no rpath, so the runner prepends
`build/<preset>/lib/` to `PATH` for the DLL to be found - always run tests
and examples through `prj_exec.py`.

Test the library **through its public header** wherever possible: those
tests double as ABI/API regression tests.

## Deployment

```bash
uv run python scripts/prj_exec.py --commands 10
```

`environment: "dev"` deploys to `deploy/<preset>/`; `test`/`production`
deploy to the shared artifact store,
`<deploy_folder>/lib-shared-FmuToolkit/<version>/<preset>/`:

```text
<deploy>/
├── bin/          # libfmutoolkit.dll (+ toolchain runtime DLLs)
├── lib/          # libfmutoolkit.dll.a (import library)
├── include/      # fmutoolkit/*.hpp
├── documents/    # USER_MANUAL.pdf, CHANGELOG.pdf, INFO.md
└── license/      # LICENSE
```

`USER_MANUAL.pdf` and `CHANGELOG.pdf` are rendered at deploy time from
`docs/user_manual/USER_MANUAL.md` and `CHANGELOG.md` (`scripts/prj_docs.py`,
based on `markdown-pdf`); the Markdown sources are not copied into the
deploy folder. All five folders always exist, so every deployed artifact of
this scaffold - application or library - has the same shape.

No CMake package configuration (`lib/cmake/`) is written to the deploy
folder. It is available on demand for classic `find_package()` consumers:

```bash
cmake --install build/<preset> --prefix <dir> --component cmake_package
```

## Consuming the Library

Any project generated by the same scaffold declares the library in its
`project.json`:

```json
"dependencies": [
    {
        "name": "lib-shared-FmuToolkit",
        "version": "0.1.0",
        "copy_to": [
            { "source": "bin", "destination": "bin" },
            { "source": "lib", "destination": "lib" }
        ]
    }
]
```

At configure time `cmake/Dependencies/Dependencies.cmake` locates
`<deploy_folder>/lib-shared-FmuToolkit/<version>/<preset>/`, creates the imported
target `fmutoolkit` (a `SHARED IMPORTED` target with its import library and
DLL), and the consumer links it:

```cmake
if(TARGET fmutoolkit)
    target_link_libraries(my_target PRIVATE fmutoolkit)
endif()
```

At deploy time the consumer's `prj_exec.py` copies the folders listed in
`copy_to` into its own deploy folder, so the DLL ships next to the
executable.

## Documentation

| Document | Location |
|---|---|
| User manual | `docs/user_manual/USER_MANUAL.md` (shipped as PDF) |
| Changelog | `CHANGELOG.md` - record every API/ABI-relevant change (shipped as PDF) |
| Architecture | `docs/architecture/architecture.md` |
| Architecture decision records | `docs/adr/adr.md` |
| Specifications / Jira material | `docs/specs/`, `docs/specs/jira/` |
| API reference | `doxygen Doxyfile` (public headers are fully documented) |
| AI assistant guidelines | `CLAUDE.md`, `AGENTS.md` |

## License

Distributed under the terms in [`LICENSE`](LICENSE) (`project.json` ->
`license`).

## Contributing

1. Branch from `develop/fmutoolkit-1` (`feature/<topic>`, `fix/<topic>`).
2. Treat `include/fmutoolkit/` as a contract: follow
   [Semantic Versioning](https://semver.org/) - breaking API/ABI changes
   bump the major version in `project.json` (it also drives `SOVERSION`).
3. Never add inbound adapters; keep the public header minimal (prefer the
   pImpl idiom for classes that may evolve).
4. Every exported symbol needs Doxygen documentation, a test, and - for new
   features - an example in `examples/`.
5. Use [Conventional Commits](https://www.conventionalcommits.org/); make
   sure `scripts/prj_exec.py --commands 11,8,9` passes before opening a pull
   request.
