# FmuToolkit

> C++ toolkit for loading, parsing, validating, and simulating FMI 2.0/3.0 FMUs

![Category](https://img.shields.io/badge/category-lib--shared-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B23-orange)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-green)
![Artifact](https://img.shields.io/badge/artifact-libfmutoolkit.dll-lightgrey)
![Tests](https://img.shields.io/badge/tests-107%20passing-brightgreen)

## Table of Contents

- [Description](#description)
- [Category](#category)
- [Compatibility contract](#compatibility-contract)
- [Prerequisites](#prerequisites)
- [Setup](#setup)
- [Project Structure](#project-structure)
- [Build](#build)
- [Test](#test)
- [Deployment](#deployment)
- [Consuming the Library](#consuming-the-library)
- [FMU Test Data](#fmu-test-data)
- [Documentation](#documentation)
- [License](#license)
- [Contributing](#contributing)

## Description

**FmuToolkit** is a reusable **shared library** (`libfmutoolkit.dll` on
Windows, `libfmutoolkit.so` on Linux) for loading, parsing, XSD/semantic
validating, and co-simulating **FMI 2.0 and FMI 3.0** Functional Mock-up
Units. It publishes a small, stable, PIMPL-based public API under `include/`
and hides libxml2/libzip/dlopen entirely behind it.

Internally it follows **hexagonal architecture**: a framework-free
`domain/`, use cases in `application/service/`, five driven ports
(`domain/port/out/`), and outbound adapters (`adapter/out/xml`, `zip`,
`simulation`, `json`) that are the only places libxml2/libzip are used. See
[`docs/architecture/architecture.md`](docs/architecture/architecture.md) for
the full layer map and data-flow diagrams, and
[`docs/adr/`](docs/adr/) for the design decisions behind it (hexagonal
architecture, PIMPL, MinGW-only toolchains, the three-phase validation
pipeline, ABI stability).

This is a **port** onto `tool-ProjectScaffold`'s `lib-shared` category of a
mature, already-tested pre-scaffold project - the domain logic, ~1500 lines
of adapters/use cases, 107 tests and 8 examples were carried over with only
their folder names and `#include` paths mechanically updated to this
scaffold's hexagonal convention. See [`CHANGELOG.md`](CHANGELOG.md) for
exactly what the port changed and [`CLAUDE.md`](CLAUDE.md) for the
compatibility contract that constrains further changes.

## Category

| Property | Value |
|---|---|
| Category | `lib-shared` - dynamically linked library |
| Artifact name | `lib-shared-FmuToolkit` (folder, deploy name and dependency name) |
| Binaries | `libfmutoolkit.dll` + import library `libfmutoolkit.dll.a` (MinGW) |
| Public headers | flat: `include/lib_fmu_toolkit.hpp` (umbrella), `fmu_engine.hpp`, `types.hpp`, `exceptions.hpp`, `fmu_generation.hpp`, `export.hpp`, `version.hpp` |
| Public namespace | `libfmu_toolkit` |
| CMake target | `fmutoolkit` (alias `fmutoolkit::fmutoolkit`) |
| Architecture | Hexagonal (ports and adapters), no inbound adapters |
| Language | C++23 (built as C++20 originally; both compile unchanged) |
| Build system | CMake >= 3.25 (presets) + Ninja |
| Consumed by | `tool-FmuRunner` (`apps/cli/main.cpp`) |

## Compatibility contract

Unlike a fresh `lib-shared` project, this one's public API is **not** this
scaffold's usual `include/fmutoolkit/`, `FMUTOOLKIT_EXPORT`,
`fmutoolkit::`-namespaced shape - it is the pre-scaffold project's own,
verbatim, because `tool-FmuRunner` already compiles against it as-is
(`#include <lib_fmu_toolkit.hpp>`, `libfmu_toolkit::FmuEngine`). Only the
**internal** folder names and the **CMake target/artifact naming** follow
this scaffold's convention. See [`CLAUDE.md`](CLAUDE.md#compatibility-contract-with-tool-fmurunner-do-not-break)
for the full, binding list before changing anything under `include/` or the
install steps in the root `CMakeLists.txt`.

## Prerequisites

| Tool | Version | Used for |
|---|---|---|
| C/C++ toolchain | GCC 13+ / MinGW-w64 (Windows) | compiling |
| CMake | 3.25+ | configure / build / install |
| Ninja | any recent | build backend of every preset |
| Python | 3.10+ | orchestration scripts |
| [uv](https://docs.astral.sh/uv/) | latest | Python environment for `scripts/` |
| `mingw-w64-x86_64-libxml2`, `mingw-w64-x86_64-libzip` (MSYS2) | any | XML validation / FMU archive extraction |
| Doxygen | optional | API reference |

libxml2 and libzip are linked **statically** into `libfmutoolkit.dll` on
MinGW (`FMUTOOLKIT_MINGW_STATIC_RUNTIME`, default `ON`) - they are needed to
*build* the library, but a consumer only needs `libfmutoolkit.dll` and
`libwinpthread-1.dll` at runtime, never `libxml2.dll`/`libzip.dll`.

## Setup

```bash
uv sync                                  # Python environment for scripts/
uv run python scripts/debug_env.py       # verify compilers, CMake, Ninja
uv run python scripts/prj_exec.py --commands 2   # fetch the fmi-standard-2.0/3.0 XSD schemas
```

The last step is not optional: `FmuValidator`'s schema phase and 5+ tests
need `external/fmi-standard-{2.0,3.0}/schema/*.xsd`, fetched as `project.json`
`submodules[]` (`kind: "git"`) from `modelica/fmi-standard` - see
[FMU Test Data](#fmu-test-data) below.

## Project Structure

```text
lib-shared-FmuToolkit/
├── include/libfmu_toolkit/    # PUBLIC API (flat once deployed - see Compatibility contract)
├── src/                       # private implementation (hexagonal)
│   ├── domain/model/          # FmuPackage, ModelVariable, ValidationReport, ...
│   ├── domain/exception/      # internal exception hierarchy
│   ├── domain/port/out/       # IFmuLoader, IXmlValidator, ISimulationBackend, ...
│   ├── application/service/   # FmuImporter, FmuValidator, SimulationController, generation/
│   ├── adapter/out/{xml,zip,simulation,json}/
│   └── fmu_engine.cpp, fmu_engine_impl.hpp, fmu_generation.cpp   # PIMPL composition root
├── examples/                  # 8 runnable examples of the public API (01-08)
├── tests/{unit,integration}/  # 107 GoogleTest cases; tests/data/*.fmu fixtures
├── data/generation/           # FR-08 JSON model-definition examples
├── external/                  # fmi-standard-2.0/3.0 (git submodules, not committed)
├── docs/                      # architecture, ADRs, SRS, traceability, API/validation reference
├── cmake/                     # reusable CMake modules + FmuToolkitThirdParty.cmake
├── scripts/                   # prj_exec.py orchestrator and helpers
├── CMakeLists.txt  CMakePresets.json
├── project.json                # version 1.0.0, environment "production", deploy target
└── CHANGELOG.md  INFO.md  LICENSE  Dockerfile
```

## Build

```bash
uv run python scripts/prj_exec.py                                    # interactive menu
uv run python scripts/prj_exec.py --preset windows-mingw64-debug --commands 2,5,6
```

| # | Command | # | Command |
|---|---|---|---|
| 1 | Select preset | 9 | Run tests |
| 2 | Sync external repositories | 10 | Deploy |
| 3 | Clean current preset | 11 | Full workflow (clean -> configure -> build -> deploy) |
| 4 | Clean all presets | 13 | Generate documentation PDFs |
| 5 | Configure | 14 | Quit |
| 6 | Build | | |
| 8 | Run examples | | |

Build outputs land in `build/<preset>/lib/` (the DLL included - this
category's `RUNTIME_OUTPUT_DIRECTORY` is pinned to `lib/`, not `bin/`).
Useful CMake options: `FMUTOOLKIT_BUILD_TESTS`, `FMUTOOLKIT_BUILD_EXAMPLES`,
`FMUTOOLKIT_MINGW_STATIC_RUNTIME` (default `ON`).

## Test

```bash
uv run python scripts/prj_exec.py --commands 8,9    # examples, then tests
```

107 GoogleTest cases across two executables, `fmutoolkit_unit_tests` and
`fmutoolkit_integration_tests` - both compile the library's own hexagonal
sources directly in (see `CLAUDE.md`) rather than linking the DLL, so
internal classes are directly testable. Reports go to
`build/<preset>/test-reports/`.

| Suite | Covers |
|---|---|
| `GeneratoreTest` / `SommatoreTest` / `CrossFmuTest` | Full FMU lifecycle: load, inspect, validate, simulate, export, step-by-step, unload |
| `FmuEngineTest` / `FmuEngineIntegrationTest` | Engine construction, error paths, end-to-end pipeline |
| `ValidatorStructureTest` / `ValidatorBinaryDirTest` / `ValidatorSchemaTest` / `ValidatorSemanticTest` | The three-phase validation pipeline (structure, XSD schema, semantics) |
| `Fmi2ParserTest` / `Fmi3ParserTest` / `ParserErrorTest` / `ParserEdgeTest` | `modelDescription.xml` parsing, both FMI versions, malformed/edge inputs |
| `CsvExporterTest` / `JsonExporterTest` | Result export correctness |
| `SimulationControllerTest` | Step logic, recording interval, cancellation |
| `FmuGeneration` (FR-08) | JSON-driven `modelDescription.xml` + C-accessor generation |

## Deployment

```bash
uv run python scripts/prj_exec.py --commands 10
```

`project.json`'s `environment` is `"production"` (not this scaffold's usual
`"dev"` default - see [Compatibility contract](#compatibility-contract)), so
deploy writes straight to the shared artifact store consumers read from,
`<deploy_folder>/lib-shared-FmuToolkit/1.0.0/<preset>/`:

```text
<deploy>/
├── bin/          # libfmutoolkit.dll (+ toolchain runtime DLLs)
├── lib/          # libfmutoolkit.dll.a (import library)
├── include/      # flat public headers (incl. version.hpp)
├── documents/    # USER_MANUAL.pdf, CHANGELOG.pdf, INFO.md
└── license/      # LICENSE
```

`USER_MANUAL.pdf` and `CHANGELOG.pdf` are rendered at deploy time from
`docs/user_manual/USER_MANUAL.md` and `CHANGELOG.md`
(`scripts/prj_docs.py`, `markdown-pdf`).

## Consuming the Library

`tool-FmuRunner`'s `project.json` already does this:

```json
"dependencies": [
    {
        "name": "lib-shared-FmuToolkit",
        "version": "1.0.0",
        "copy_to": [
            { "source": "bin", "destination": "bin" },
            { "source": "lib", "destination": "lib" }
        ]
    }
]
```

At configure time `cmake/Dependencies/Dependencies.cmake` locates
`<deploy_folder>/lib-shared-FmuToolkit/1.0.0/<preset>/`, creates the
imported target `fmutoolkit`, and the consumer links it:

```cmake
if(TARGET fmutoolkit)
    target_link_libraries(my_target PRIVATE fmutoolkit)
endif()
```

```cpp
#include <lib_fmu_toolkit.hpp>   // master include

libfmu_toolkit::FmuEngine engine;
uint64_t handle = engine.load("model.fmu");
auto validation = engine.validate(handle);
libfmu_toolkit::SimConfig cfg{.startTime = 0.0, .stopTime = 10.0, .stepSize = 0.01};
auto result = engine.simulate(handle, cfg);
engine.exportJson(result, "output.json");
```

## FMU Test Data

`tests/data/{Generatore,Sommatore,BouncingBall}.fmu` are hand-crafted FMI 3.0
FMUs with **both Windows and Linux platform binaries** already embedded
(`binaries/x86_64-{windows,linux}/`), so tests and examples run unmodified on
either OS. `data/generation/*.json` are FR-08 model definitions consumed by
`example_07_generate_from_json`/`08_generate_selective` and
`test_FmuGeneration.cpp`.

The `external/fmi-standard-{2.0,3.0}/` XSD schemas (needed by
`FmuValidator`'s schema phase) are **not** committed - `scripts/prj_exec.py`
menu `2` clones them from `modelica/fmi-standard` per `project.json`'s
`submodules[]` (see [Setup](#setup)).

## Documentation

| Document | Location |
|---|---|
| User manual | `docs/user_manual/USER_MANUAL.md` (shipped as PDF) |
| Changelog | `CHANGELOG.md` - the pre-scaffold project's history plus the port itself |
| Architecture | `docs/architecture/architecture.md` - layer map, PIMPL pattern, data flows |
| Architecture decision records | `docs/adr/0001`-`0005` |
| Requirements & traceability | `docs/requirements/srs.md`, `docs/requirements/traceability.md` |
| API reference | `docs/reference/api_reference.md`, `docs/reference/validation.md`; `doxygen Doxyfile` |
| FMI standard notes | `docs/standards/fmi2_standard.md`, `docs/standards/fmi3_standard.md` |
| AI assistant guidelines | `CLAUDE.md` (the binding compatibility contract), `AGENTS.md` |

## License

Distributed under the terms in [`LICENSE`](LICENSE) (`project.json` ->
`license`).

## Contributing

1. Branch from `develop/fmutoolkit-1` (`feature/<topic>`, `fix/<topic>`).
2. Treat the public headers under `include/` as a contract - see
   [Compatibility contract](#compatibility-contract) before touching them.
   Breaking API/ABI changes bump the major version in `project.json` (it
   also drives `SOVERSION`) and must be coordinated with `tool-FmuRunner`.
3. Never add inbound adapters; keep the public header minimal.
4. Every exported symbol needs Doxygen documentation, a test, and - for new
   features - an example in `examples/`. Update the SRS/traceability docs
   for any new or changed requirement.
5. Use [Conventional Commits](https://www.conventionalcommits.org/); make
   sure `scripts/prj_exec.py --commands 2,11,8,9` passes before opening a
   pull request.
