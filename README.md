# FmuToolkit

> A C++ shared library for loading, inspecting, validating and co-simulating **FMI 2.0 / FMI 3.0** Functional Mock-up Units.

![Category](https://img.shields.io/badge/category-lib--shared-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B20%2B-orange)
![FMI](https://img.shields.io/badge/FMI-2.0%20%7C%203.0-informational)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-green)
![Artifact](https://img.shields.io/badge/artifact-libfmutoolkit.dll-lightgrey)

**Version 1.0.0** · Namespace `libfmu_toolkit` · Consumed by `tool-FmuRunner`

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Example](#quick-example)
- [Using the Library](#using-the-library)
- [Documentation](#documentation)
- [Architecture at a Glance](#architecture-at-a-glance)
- [Building from Source](#building-from-source)
- [Testing](#testing)
- [Deployment](#deployment)
- [Repository Layout](#repository-layout)
- [Compatibility Contract](#compatibility-contract)
- [Contributing](#contributing)
- [License](#license)

## Overview

FmuToolkit gives an application a single, small entry point (`FmuEngine`) to work with FMUs: load the archive,
read its model description, validate it against the standard, run a co-simulation, or drive one step by step
from an external master. The public API is built on the C++ standard library only; libxml2, libzip and the
dynamic-loading machinery are hidden behind a PIMPL facade and, on Windows/MinGW, linked statically into the
DLL. Consumers need only `libfmutoolkit.dll` and `libwinpthread-1.dll` at run time.

> **If you want to *use* the library, start with the [User Manual](docs/user_manual/USER_MANUAL.md).** It is the
> complete reference for integration, every API call, error handling and troubleshooting. This README is aimed at
> people who build, test or maintain the library.

## Features

| Area | Capability |
|---|---|
| **Loading** | Extracts `.fmu` archives, parses `modelDescription.xml`, loads the platform binary and resolves FMI symbols |
| **Inspection** | Variables, causality/variability, units, type definitions, default experiment, FMI 3.0 terminals and layered standards |
| **Validation** | Three phases: archive structure, XSD schema (libxml2), semantics - including FMI-LS-BUS rules; findings are returned, never thrown |
| **Co-simulation** | Batch `simulate()` with progress callback, parameter injection, output selection and recording interval |
| **Interactive control** | `FmuSession` for step-by-step masters; FMI 3.0 Event Mode, Binary variables and Clocks |
| **Export** | CSV (RFC 4180) and JSON result writers |
| **Generation** | JSON-driven `modelDescription.xml` and C accessor generation for FMI 2.0 and 3.0 |
| **Platforms** | Windows (MinGW-w64) and Linux; FMI 2.0 and FMI 3.0 Co-Simulation |

## Quick Example

```cpp
#include <lib_fmu_toolkit.hpp>

libfmu_toolkit::FmuEngine engine;
const uint64_t handle = engine.load("model.fmu");

const auto report = engine.validate(handle);          // never throws for findings

libfmu_toolkit::SimConfig cfg;
cfg.stopTime = 10.0;
cfg.stepSize = 0.01;

const auto result = engine.simulate(handle, cfg);
libfmu_toolkit::FmuEngine::exportCsv(result, "result.csv");
```

Eight complete, runnable programs are in [`examples/`](examples/).

## Using the Library

Link the imported CMake target and include the umbrella header:

```cmake
target_link_libraries(my_app PRIVATE fmutoolkit)
```

```cpp
#include <lib_fmu_toolkit.hpp>
```

The deployed package contains `bin/` (`libfmutoolkit.dll`), `lib/` (`libfmutoolkit.dll.a`), `include/` (flat
public headers), `documents/` and `license/`. Manual compilation flags, the required `LIB_FMU_TOOLKIT_SHARED`
define on Windows, ABI requirements and schema (XSD) deployment for `validate()` are all covered in the
[User Manual](docs/user_manual/USER_MANUAL.md#3-integrating-the-library).

## Documentation

| Document | Location | Audience |
|---|---|---|
| **User Manual** | [`docs/user_manual/USER_MANUAL.md`](docs/user_manual/USER_MANUAL.md) (shipped as PDF) | Library users |
| Changelog | [`CHANGELOG.md`](CHANGELOG.md) | Everyone |
| Architecture | [`docs/architecture/architecture.md`](docs/architecture/architecture.md) | Maintainers |
| Design decisions | [`docs/adr/`](docs/adr/) (ADR-0001 to ADR-0005) | Maintainers |
| Requirements (SRS) | [`docs/specs/srs.md`](docs/specs/srs.md) | Maintainers, QA |
| FMI standard notes | [`docs/standards/`](docs/standards/) | Maintainers |
| Project rules and compatibility contract | [`CLAUDE.md`](CLAUDE.md) | Contributors |

## Architecture at a Glance

The library follows a hexagonal (ports-and-adapters) design with no inbound adapters:

```text
 public PIMPL facade  (FmuEngine, FmuSession, generators)        include/libfmu_toolkit/
            │
 application services (import, validate, simulate, export,        src/application/service/
                       generation)
            │  depend only on ports
 domain               (FMI model, exceptions, 5 driven ports)     src/domain/
            │
 outbound adapters    (libxml2, libzip, dlopen/FMI backend, JSON) src/adapter/out/
```

Rationale and trade-offs are recorded in the ADRs: hexagonal architecture, PIMPL public API, MinGW/GCC-only
toolchains, the three-phase validation pipeline, and the ABI/source-distribution policy.

## Building from Source

### Prerequisites

| Tool | Version | Purpose |
|---|---|---|
| GCC / MinGW-w64 | 13+ | Compiler |
| CMake | 3.25+ | Configure, build, install |
| Ninja | recent | Build backend used by every preset |
| Python | 3.10+ and [uv](https://docs.astral.sh/uv/) | Orchestration scripts |
| libxml2, libzip (MSYS2: `mingw-w64-x86_64-libxml2`, `mingw-w64-x86_64-libzip`) | any | Needed to **build**; statically linked into the DLL |
| Doxygen | optional | API reference |

### Steps

```bash
uv sync                                                  # Python environment
uv run python scripts/debug_env.py                       # verify compilers, CMake, Ninja
uv run python scripts/prj_exec.py --commands 2           # fetch the FMI XSD schemas (external/)
uv run python scripts/prj_exec.py --preset windows-mingw64-debug --commands 5,6   # configure + build
```

Running `scripts/prj_exec.py` without arguments opens the interactive menu:

| # | Action | # | Action |
|---|---|---|---|
| 1 | Select preset | 9 | Run tests |
| 2 | Sync external repositories | 10 | Deploy |
| 3 | Clean current preset | 11 | Full workflow (clean, configure, build, deploy) |
| 4 | Clean all presets | 13 | Generate documentation PDFs |
| 5 | Configure | 14 | Quit |
| 6 | Build | | |
| 8 | Run examples | | |

The schema step is required on a fresh clone: `external/fmi-standard-2.0` and `external/fmi-standard-3.0` are
declared in `project.json` (`submodules[]`) and fetched from `modelica/fmi-standard`, not committed.

Build outputs are written to `build/<preset>/lib/` (the DLL included). Useful CMake options:
`FMUTOOLKIT_BUILD_TESTS`, `FMUTOOLKIT_BUILD_EXAMPLES`, `FMUTOOLKIT_MINGW_STATIC_RUNTIME` (default `ON`).

## Testing

```bash
uv run python scripts/prj_exec.py --commands 8,9         # examples, then tests
```

GoogleTest suites are split into `fmutoolkit_unit_tests` and `fmutoolkit_integration_tests`. Both compile the
library's own sources directly (rather than linking the DLL) so that internal, non-exported classes can be
tested. Reports are written to `build/<preset>/test-reports/`.

| Area | Covered by |
|---|---|
| Full FMU lifecycle (load, inspect, validate, simulate, export, step-by-step) | Generator/Adder FMU tests, cross-FMU tests |
| Three-phase validation | Structure, binary directory, schema and semantic validator tests |
| `modelDescription.xml` parsing, FMI 2.0 and 3.0, malformed input | Parser tests |
| Result export | CSV and JSON exporter tests |
| Step logic, recording interval, cancellation | Simulation controller tests |
| FMI 3.0 FMUs, Event Mode, Binary/Clock | Integration tests |
| JSON-driven generation | Generation tests |

Test FMUs (`tests/data/*.fmu`) embed both Windows and Linux binaries, so the suites run on either OS.

## Deployment

```bash
uv run python scripts/prj_exec.py --commands 10
```

`project.json` sets `environment` to `production`, so the deploy step writes to the shared artifact store read by
consumers: `<deploy_folder>/lib-shared-FmuToolkit/<version>/<preset>/`.

```text
├── bin/        libfmutoolkit.dll (+ libwinpthread-1.dll)
├── lib/        libfmutoolkit.dll.a
├── include/    flat public headers (incl. version.hpp)
├── documents/  USER_MANUAL.pdf, CHANGELOG.pdf, INFO.md
└── license/    LICENSE
```

The PDFs are rendered from `docs/user_manual/USER_MANUAL.md` and `CHANGELOG.md`; edit the Markdown sources, never
the PDFs.

## Repository Layout

```text
lib-shared-FmuToolkit/
├── include/libfmu_toolkit/   public API (installed flat)
├── src/                      domain/, application/service/, adapter/out/, PIMPL composition root
├── examples/                 01_basic_simulate … 08_generate_selective
├── tests/                    unit/, integration/, data/*.fmu
├── data/generation/          JSON model-definition samples
├── external/                 FMI XSD schemas (fetched, not committed)
├── docs/                     user_manual/, architecture/, adr/, specs/, standards/
├── cmake/                    CMake modules incl. FmuToolkitThirdParty.cmake
├── scripts/                  prj_exec.py orchestrator and helpers
└── project.json  CMakeLists.txt  CMakePresets.json  CHANGELOG.md
```

## Compatibility Contract

`tool-FmuRunner` compiles against this library's public API as-is. The following are therefore fixed and must not
be changed without updating FmuRunner in the same change (details in [`CLAUDE.md`](CLAUDE.md)):

- namespace `libfmu_toolkit`;
- flat installed headers (`#include <lib_fmu_toolkit.hpp>`) and the `LIB_FMU_TOOLKIT_API` export macro;
- CMake target `fmutoolkit` producing `libfmutoolkit.dll` / `libfmutoolkit.dll.a`;
- `project.json` `version` aligned with consumers' `dependencies[].version`, and `environment` set to
  `production`;
- static linking of libxml2/libzip on MinGW and `_GLIBCXX_USE_CXX11_ABI=1` as a `PUBLIC` definition.

## Contributing

1. Branch from `develop/fmutoolkit-1` (`feature/<topic>`, `fix/<topic>`).
2. Treat `include/` as a contract. Breaking API/ABI changes bump the major version in `project.json` and must be
   coordinated with `tool-FmuRunner`.
3. Do not add inbound adapters; keep the public headers free of third-party types.
4. Every exported symbol needs Doxygen documentation and a test; new features need an example. Update the SRS,
   ADRs, architecture document and **User Manual** with any behavioural change.
5. Use [Conventional Commits](https://www.conventionalcommits.org/) and make sure
   `scripts/prj_exec.py --commands 2,11,8,9` passes before opening a pull request.

## License

See [`LICENSE`](LICENSE); the license identifier is recorded in `project.json`.
