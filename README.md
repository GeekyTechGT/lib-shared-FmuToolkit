# lib-fmu-toolkit

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.25-064F8C.svg)](https://cmake.org/)
[![FMI 2.0](https://img.shields.io/badge/FMI-2.0-green.svg)](https://svn.modelica.org/fmi/branches/public/specifications/v2.0/)
[![FMI 3.0](https://img.shields.io/badge/FMI-3.0-brightgreen.svg)](https://svn.modelica.org/fmi/branches/public/specifications/v3.0/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)](https://fmi-standard.org/)
[![vcpkg](https://img.shields.io/badge/vcpkg-compatible-blueviolet.svg)](https://vcpkg.io/)

A production-grade, header-clean C++ library for loading, parsing, validating, and simulating
[Functional Mock-up Units (FMUs)](https://fmi-standard.org/) following both the **FMI 2.0** and
**FMI 3.0** co-simulation standards.

`lib-fmu-toolkit` was extracted and generalised from the `fmu-studio` and `fmu-forge` toolchain
projects. It is designed to be embeddable as a static or shared library, consumable via vcpkg
private registries, and architecturally clean enough to serve as a foundation for higher-level
simulation frameworks and Qt6-based GUI applications.

---

## Table of Contents

1. [Features](#features)
2. [Architecture](#architecture)
3. [Repository Layout](#repository-layout)
4. [Test Data](#test-data)
5. [Dependencies](#dependencies)
6. [Building](#building)
7. [Toolchain Support](#toolchain-support)
8. [Consuming via CMake](#consuming-via-cmake)
9. [Consuming via vcpkg](#consuming-via-vcpkg)
10. [API Overview](#api-overview)
11. [FMU Validation In Depth](#fmu-validation-in-depth)
12. [Examples](#examples)
13. [Runtime XSD Schema Discovery](#runtime-xsd-schema-discovery)
14. [Build Options](#build-options)
15. [Testing](#testing)
16. [Documentation](#documentation)
17. [Contributing](#contributing)

---

## Features

- **Full FMI 2.0 and FMI 3.0 co-simulation support** — instantiate, initialize, step, terminate,
  and free, following the complete lifecycle mandated by both standards. All variable data types
  are supported: Real/Float32/Float64, Integer/Int8…Int64/UInt8…UInt64, Boolean, String, Binary,
  Clock.

- **ZIP archive extraction** — FMU archives are extracted transparently using libzip before the
  shared library and XML descriptor are accessed. The extraction path is configurable.

- **Comprehensive XML parsing** — `modelDescription.xml` is fully parsed for all standard
  attributes:
  - Model metadata: `modelName`, `guid` (FMI 2.0) / `instantiationToken` (FMI 3.0),
    `fmiVersion`, `description`, `generationTool`, `generationDateAndTime`,
    `variableNamingConvention`, `numberOfEventIndicators`.
  - `ModelVariables`: causality, variability, initial, data type, start values, unit,
    declaredType for every variable.
  - `UnitDefinitions`: physical unit names with SI base-unit conversion factors and offsets.
  - `TypeDefinitions` (FMI 2.0 `SimpleType` / FMI 3.0 `TypeDefinitions`): named types with
    quantity, unit, displayUnit, min/max constraints.
  - `DefaultExperiment`: recommended start time, stop time, step size, and tolerance.
  - `LogCategories`: all declared logging categories.

- **Three-phase FMU validation** — a single `validate()` call runs all three phases:
  1. **Structure Phase** — checks the FMU archive layout against the FMI standard:
     - `modelDescription.xml` must exist at the archive root.
     - `binaries/` directory must be present.
     - Binary subdirectory names must follow the naming convention for the declared FMI version:
       - FMI 2.0: `win32`, `win64`, `linux32`, `linux64`, `darwin32`, `darwin64`
       - FMI 3.0: `x86_64-windows`, `x86_64-linux`, `x86_64-darwin`, `aarch64-linux`, etc.
     - A version mismatch (e.g. FMI 3.0-style directories inside an FMI 2.0 FMU) is an error.
     - The shared library filename must match the `modelIdentifier` attribute.
  2. **Schema Phase** — validates `modelDescription.xml` against the official FMI XSD schema
     using libxml2, with line- and column-accurate error reporting.
  3. **Semantic Phase** — checks invariants beyond what XSD covers: duplicate value references,
     invalid causality/variability combinations, absence of output variables (warning).

- **Co-simulation execution** — `SimulationController` drives the FMI co-simulation loop with:
  - Configurable recording intervals (record every Nth step).
  - Selective output variable recording (empty list = record all outputs).
  - Initial parameter injection by variable name.
  - Atomic cancellation mechanism (thread-safe).
  - Progress callback for UI integration.

- **Step-by-step session control** — `FmuSession` exposes `doStep()` / `setValue()` /
  `getValue()` for scenarios where an external time coordinator (e.g., SIL-Kit, AUTOSAR,
  co-simulation master, or Qt timer) drives the simulation clock.

- **Result export** — `SimResult` can be exported to:
  - **CSV** (RFC-4180 compliant, with string escaping).
  - **JSON** (hand-written, no third-party JSON library dependency).

- **Static and shared builds** — both `lib_fmu_toolkit_shared` and `lib_fmu_toolkit_static`
  CMake targets are built from the same sources. The `lib_fmu_toolkit::lib_fmu_toolkit` alias
  resolves to shared by default; consumers can target `::shared` or `::static` explicitly.

- **Hexagonal architecture** — the public API is a thin PIMPL facade over a
  domain/use-case/ports/adapters stack. No third-party types leak into public headers.

- **Stable ABI surface** — public headers expose only standard C++ and the library's own
  `LIB_FMU_TOOLKIT_API`-decorated types. libxml2, libzip, and all internal domain types are
  entirely hidden behind the PIMPL boundary.

- **Installable CMake package** — ships `lib_fmu_toolkitConfig.cmake`,
  `lib_fmu_toolkitConfigVersion.cmake`, and `lib_fmu_toolkitTargets.cmake` for `find_package`
  consumption by downstream projects, including Qt6-based applications.

- **vcpkg private registry support** — the `packaging/vcpkg/` directory contains parameterised
  portfile and manifest templates ready for private registry publication.

---

## Architecture

The library follows a **hexagonal architecture** (Ports and Adapters) combined with the **PIMPL
idiom** at the public API boundary. This separation ensures that:

- The **domain layer** (pure data structures and business rules) has zero dependencies on
  third-party libraries.
- The **use-case layer** orchestrates validation and simulation logic by depending only on
  abstract port interfaces — never on concrete adapters.
- The **adapter layer** contains all third-party library usage (libxml2, libzip, platform
  DLL-loading APIs) and is invisible to consumers.
- The **public API** (`FmuEngine`, `FmuSession`) exposes only standard C++ types, making it
  trivially consumable from Qt and other frameworks without include-path entanglement.

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│  PUBLIC API  (include/libfmu_toolkit/)                                       │
│                                                                              │
│   FmuEngine ──PIMPL──► FmuEngine::Impl       FmuSession ──PIMPL──► Impl    │
│   (fmu_engine.hpp)                                                           │
│   types.hpp · exceptions.hpp · export.hpp · version.hpp                     │
└──────────────────────────┬───────────────────────────────────────────────────┘
                           │  (internal only — not shipped to consumers)
┌──────────────────────────▼───────────────────────────────────────────────────┐
│  USE CASES  (src/core/usecases/)                                             │
│                                                                              │
│   FmuImporter            FmuValidator          SimulationController          │
│   CsvResultExporter      (uses IXmlValidator)  (uses ISimulationBackend)    │
└────────┬─────────────────────────────────────────────────────────────────────┘
         │  depends on port abstractions only
┌────────▼─────────────────────────────────────────────────────────────────────┐
│  DOMAIN  (src/core/domain/)                                                  │
│                                                                              │
│   FmuPackage · ModelVariable · SimulationConfig · SimulationResult           │
│   ValidationReport · UnitDef · TypeDef · DomainDefaultExperiment            │
└──────────────────────────────────────────────────────────────────────────────┘
         ▲                ▲                ▲
         │                │                │
┌────────┴────┐  ┌────────┴──────┐  ┌─────┴───────────────────────────────────┐
│  PORTS      │  │  PORTS        │  │  PORTS                                  │
│             │  │               │  │                                         │
│ IFmuLoader  │  │ IXmlValidator │  │ ISimulationBackend · IResultExporter    │
└────────┬────┘  └────────┬──────┘  └─────┬───────────────────────────────────┘
         │                │                │  (adapters implement ports)
┌────────▼────┐  ┌────────▼──────┐  ┌─────▼───────────────────────────────────┐
│  ADAPTERS   │  │  ADAPTERS     │  │  ADAPTERS                               │
│             │  │               │  │                                         │
│ ZipLoader-  │  │ Libxml2Vali-  │  │ StandaloneSimulation-   JsonResult-     │
│ Adapter     │  │ datorAdapter  │  │ Adapter                 Exporter        │
└─────────────┘  └───────────────┘  └─────────────────────────────────────────┘
```

**Key design decisions:**

- `FmuEngine` and `FmuSession` expose only `struct Impl;` forward declarations in their public
  headers. The full `Impl` definitions — which include all heavy internal types — live exclusively
  in `src/fmu_engine_impl.hpp` and are never installed.
- All use-cases depend on port interfaces (`IXmlValidator`, `IFmuLoader`, `ISimulationBackend`,
  `IResultExporter`), never on concrete adapters. This enables injecting test doubles or
  alternative implementations without modifying any use-case code (Dependency Inversion Principle).
- `SimulationController` builds a single `unordered_map<name, ModelVariable*>` index before the
  simulation loop, giving O(1) variable lookup per step regardless of model size.

---

## Repository Layout

```text
lib-fmu-toolkit/
├── include/libfmu_toolkit/       # Public API headers (installed)
│   ├── export.hpp                # DLL export macros
│   ├── exceptions.hpp            # Public exception hierarchy
│   ├── fmu_engine.hpp            # FmuEngine and FmuSession declarations
│   ├── lib_fmu_toolkit.hpp       # Umbrella include
│   ├── types.hpp                 # All public data types
│   └── version.hpp.in            # Version header template (configured by CMake)
│
├── src/                          # Private implementation (not installed)
│   ├── fmu_engine.cpp            # FmuEngine and FmuSession method bodies
│   ├── fmu_engine_impl.hpp       # Impl struct definitions (internal only)
│   │
│   ├── core/
│   │   ├── domain/               # Pure domain entities (no third-party types)
│   │   │   ├── fmu_package.hpp/.cpp
│   │   │   ├── model_variable.hpp
│   │   │   ├── fmu_variable_types.hpp
│   │   │   ├── validation_report.hpp/.cpp
│   │   │   ├── simulation_config.hpp
│   │   │   ├── simulation_result.hpp
│   │   │   └── log_category.hpp
│   │   ├── exceptions/           # Internal exception types
│   │   └── usecases/             # Application-layer orchestration
│   │       ├── fmu_validator.hpp/.cpp      (three-phase validator)
│   │       ├── fmu_importer.hpp/.cpp
│   │       ├── simulation_controller.hpp/.cpp
│   │       └── csv_result_exporter.hpp/.cpp
│   │
│   ├── ports/                    # Abstract port interfaces (dependency inversion)
│   │   ├── i_fmu_loader.hpp
│   │   ├── i_xml_validator.hpp
│   │   ├── i_simulation_backend.hpp
│   │   ├── i_result_exporter.hpp
│   │   └── i_logger.hpp
│   │
│   └── adapters/                 # Concrete adapter implementations
│       ├── xml/                  # Libxml2ValidatorAdapter, ModelDescriptionParser
│       ├── zip/                  # ZipLoaderAdapter
│       ├── simulation/           # StandaloneSimulationAdapter
│       └── json/                 # JsonResultExporter
│
├── tests/
│   ├── CMakeLists.txt
│   ├── data/                     # FMU fixtures for integration tests
│   ├── unit/                     # GoogleTest unit tests
│   └── integration/              # Integration tests (require real FMU binaries)
│
├── examples/
│   ├── 01_basic_simulate/        # Load, configure, simulate, inspect results
│   ├── 02_step_by_step/          # FmuSession manual step loop
│   ├── 03_validate_fmu/          # Three-phase validation with detailed output
│   ├── 04_export_results/        # CSV and JSON export side-by-side
│   ├── 05_generatore_sommatore/  # Two-FMU orchestrated co-simulation
│   └── 06_batch_validate/        # Batch validation of valid and corrupted FMUs
│
├── data/                         # Sample FMU archives (not linked by tests)
│   ├── fmi2/
│   │   ├── SimpleAdder.fmu               (valid FMI 2.0 stub, no binary)
│   │   └── corrupted/
│   │       ├── corrupted_xml.fmu         (malformed XML)
│   │       ├── missing_binaries.fmu      (no binaries/ dir)
│   │       ├── missing_model_desc.fmu    (no modelDescription.xml)
│   │       └── wrong_platform_dir.fmu    (FMI 3.0 dir naming in FMI 2.0 FMU)
│   └── fmi3/
│       ├── Generatore.fmu                (valid FMI 3.0, real binary)
│       ├── Sommatore.fmu                 (valid FMI 3.0, real binary)
│       ├── SimpleRamp.fmu                (valid FMI 3.0 stub, no binary)
│       └── corrupted/
│           ├── corrupted_xml.fmu         (malformed XML)
│           ├── missing_binaries.fmu      (no binaries/ dir)
│           ├── missing_model_desc.fmu    (no modelDescription.xml)
│           └── wrong_platform_dir.fmu    (FMI 2.0 dir naming in FMI 3.0 FMU)
│
├── external/
│   ├── fmi-standard-2.0/schema/  # FMI 2.0 XSD schemas (submodule)
│   └── fmi-standard-3.0/schema/  # FMI 3.0 XSD schemas (submodule)
│
├── cmake/
│   ├── lib_fmu_toolkitConfig.cmake.in
│   ├── CompilerWarnings.cmake
│   ├── ProjectOptions.cmake
│   └── Sanitizers.cmake
│
├── packaging/vcpkg/              # vcpkg port templates for private registries
├── triplets/                     # Custom vcpkg triplets
├── CMakeLists.txt
└── CMakePresets.json
```

---

## Test Data

The `data/` directory contains sample FMU archives organised by FMI version and validity status.
They are intended for manual testing and demonstration — the automated test suite uses its own
fixtures under `tests/data/`.

### FMI 2.0 FMUs (`data/fmi2/`)

| File | Description |
| --- | --- |
| `SimpleAdder.fmu` | Valid FMI 2.0 co-simulation FMU. Contains `modelDescription.xml` and platform directories (`win64/`, `linux64/`, `darwin64/`) with placeholder binaries. No functional binary is included; it validates fully but cannot be simulated. |
| `corrupted/corrupted_xml.fmu` | Intentionally malformed `modelDescription.xml`. The Schema phase reports a parse error. |
| `corrupted/missing_binaries.fmu` | Archive with no `binaries/` directory. The Structure phase reports a missing directory error. |
| `corrupted/missing_model_desc.fmu` | Archive with no `modelDescription.xml`. Fails at load time. |
| `corrupted/wrong_platform_dir.fmu` | Valid FMI 2.0 XML, but the binary directory uses FMI 3.0-style naming (`x86_64-windows/` instead of `win64/`). The Structure phase reports a version-convention mismatch error. |

### FMI 3.0 FMUs (`data/fmi3/`)

| File | Description |
| --- | --- |
| `Generatore.fmu` | Valid FMI 3.0 co-simulation FMU (real binary). Implements a ramp: `y = t × 5.0`. Used by Example 05. |
| `Sommatore.fmu` | Valid FMI 3.0 co-simulation FMU (real binary). Implements a two-input adder: `y = u1 + u2`. Used by Example 05. |
| `SimpleRamp.fmu` | Valid FMI 3.0 stub. Contains `modelDescription.xml` and platform directories (`x86_64-windows/`, `x86_64-linux/`, `x86_64-darwin/`). No functional binary. |
| `corrupted/corrupted_xml.fmu` | Intentionally malformed `modelDescription.xml`. Schema phase reports a parse error. |
| `corrupted/missing_binaries.fmu` | Archive with no `binaries/` directory. Structure phase reports a missing directory error. |
| `corrupted/missing_model_desc.fmu` | Archive with no `modelDescription.xml`. Fails at load time. |
| `corrupted/wrong_platform_dir.fmu` | Valid FMI 3.0 XML, but the binary directory uses FMI 2.0-style naming (`win64/` instead of `x86_64-windows/`). Structure phase reports a version-convention mismatch error. |

Use Example 06 to run the validator over all of these in one pass:

```bash
example_06_batch_validate data
```

---

## Dependencies

| Dependency | Role | Minimum version |
| --- | --- | --- |
| [libxml2](https://gitlab.gnome.org/GNOME/libxml2) | XML DOM parsing and XSD schema validation | 2.9 |
| [libzip](https://libzip.org/) | FMU ZIP archive extraction | 1.8 |
| [GoogleTest](https://github.com/google/googletest) | Unit and integration tests | 1.12 |

All three dependencies are resolved automatically when using vcpkg. Without vcpkg, they must be
available as system packages (`apt`, `pacman`, `brew`) or discoverable via CMake's `find_package`
or `PkgConfig`.

---

## Building

### Prerequisites

- CMake ≥ 3.25
- Ninja (strongly recommended; Make is also supported)
- A C++20-capable compiler — see [Toolchain Support](#toolchain-support)
- libxml2 and libzip (system packages or via vcpkg)

### Quick start — Linux (system packages)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install libxml2-dev libzip-dev

# Debug build with sanitizers
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure

# Release build
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

### Quick start — Linux (vcpkg)

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset linux-gcc-vcpkg-debug
cmake --build --preset linux-gcc-vcpkg-debug
ctest --preset linux-gcc-vcpkg-debug --output-on-failure
```

### Quick start — Windows (MinGW via MSYS2)

```bat
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug --output-on-failure
```

### Quick start — Windows (MinGW + vcpkg)

```bat
set VCPKG_ROOT=C:\path\to\vcpkg
cmake --preset windows-mingw-vcpkg-debug
cmake --build --preset windows-mingw-vcpkg-debug
ctest --preset windows-mingw-vcpkg-debug --output-on-failure
```

### Quick start — Windows (Clang via MSYS2)

```bat
cmake --preset windows-clang-debug
cmake --build --preset windows-clang-debug
```

All `*-release` variants omit sanitizers and produce optimised binaries. All `*-vcpkg-*` variants
read the toolchain file from `$VCPKG_ROOT` and resolve all dependencies automatically.

### Using the Project Manager (Windows)

The interactive `project_manager.bat` provides a menu-driven interface to all build operations:

```bat
project_manager.bat
```

Menu options:

- **[1] Select Preset** — choose from all available presets
- **[2] Configure** — run `cmake --preset`
- **[3] Build** — run `cmake --build --preset`
- **[4] Rebuild** — clean + configure + build
- **[5] Clean** — remove the current build directory
- **[6] Clean All** — remove all build directories
- **[7] Run Tests** — run `ctest --preset`
- **[8] Pipeline** — configure + build + test in one shot
- **[9] clang-format** — format all source files
- **[10] clang-tidy** — static analysis
- **[11] Deploy** — `cmake --install`
- **[13–15] vcpkg** — registry management

---

## Toolchain Support

Only cross-platform, open-source toolchains are supported. MSVC is intentionally excluded to
keep the library buildable on both Windows and Linux with the same compiler family, which is a
prerequisite for Qt6 integration (Qt's official packages for Windows ship with MinGW binaries).

| Toolchain | Windows | Linux | Qt6 compatible |
| --- | --- | --- | --- |
| GCC (MSYS2 MinGW64 / system GCC) | ✔ (MinGW) | ✔ | ✔ |
| Clang (MSYS2 clang64 / system Clang) | ✔ | ✔ | ✔ |

### Qt6 integration

When consuming this library from a Qt6 application, add the Qt installation to `CMAKE_PREFIX_PATH`
before calling `find_package`:

```cmake
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.x.y/mingw_64")
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(lib_fmu_toolkit CONFIG REQUIRED)

target_link_libraries(my_qt_app
    PRIVATE
        Qt6::Core
        lib_fmu_toolkit::lib_fmu_toolkit)
```

Or pass it on the command line:

```bat
cmake --preset windows-mingw-release -DCMAKE_PREFIX_PATH="C:/Qt/6.x.y/mingw_64"
```

---

## Consuming via CMake

After installing the library (`cmake --install`) or adding it as a subdirectory, use the standard
`find_package` mechanism:

```cmake
find_package(lib_fmu_toolkit CONFIG REQUIRED)

target_link_libraries(my_app
    PRIVATE lib_fmu_toolkit::lib_fmu_toolkit)
```

The `lib_fmu_toolkit::lib_fmu_toolkit` alias resolves to the shared build when both targets are
present. To target a specific flavour explicitly:

```cmake
target_link_libraries(my_app PRIVATE lib_fmu_toolkit::shared)
# or
target_link_libraries(my_app PRIVATE lib_fmu_toolkit::static)
```

Include the umbrella header to pull in all public types and the engine:

```cpp
#include <libfmu_toolkit/lib_fmu_toolkit.hpp>
```

Or include individual headers:

```cpp
#include <libfmu_toolkit/fmu_engine.hpp>   // FmuEngine, FmuSession
#include <libfmu_toolkit/types.hpp>        // SimConfig, SimResult, FmuInfo, ...
#include <libfmu_toolkit/exceptions.hpp>   // FmuLoadError, FmuSimulationError, ...
```

---

## Consuming via vcpkg

The `packaging/vcpkg/` directory contains templates for publishing this library as a vcpkg port
in a private registry. Parameterise the templates, commit them to your registry, then add your
registry URL to your consuming project's `vcpkg-configuration.json`:

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://your-registry/vcpkg-registry.git",
      "baseline": "<commit-sha>",
      "packages": ["lib-fmu-toolkit"]
    }
  ]
}
```

Then declare a dependency in `vcpkg.json`:

```json
{
  "dependencies": ["lib-fmu-toolkit"]
}
```

---

## API Overview

### FmuEngine — primary entry point

`FmuEngine` manages one or more loaded FMUs identified by opaque `uint64_t` handles.

```cpp
libfmu_toolkit::FmuEngine engine;

// Load: extracts ZIP, parses XML, loads shared library, resolves FMI symbols.
uint64_t handle = engine.load("path/to/model.fmu");

// Inspect metadata.
libfmu_toolkit::FmuInfo info = engine.info(handle);
std::cout << info.modelName << " [FMI "
          << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2 ? "2" : "3") << "]\n";

// Enumerate variables.
for (const auto* v : info.inputs())
    std::cout << "input: " << v->name << " (" << v->unit << ")\n";

// Access UnitDefinitions.
for (const auto& unit : info.unitDefinitions)
    std::cout << unit.name << ": factor=" << unit.factor << "\n";

// Validate (three phases: structure + XSD schema + semantics).
auto report = engine.validate(handle);
if (!report.passed) {
    for (const auto& err : report.errors)
        std::cerr << "[" << err.line << ":" << err.column << "] " << err.message << "\n";
}

// Configure and run a co-simulation.
libfmu_toolkit::SimConfig cfg;
cfg.startTime         = 0.0;
cfg.stopTime          = 10.0;
cfg.stepSize          = 0.01;
cfg.tolerance         = 1e-6;
cfg.initialParameters["K"] = 2.5;              // set named parameter before run
cfg.outputVariables   = {"position", "speed"}; // empty = record all outputs
cfg.recordingInterval = 10;                    // record every 10th step

auto result = engine.simulate(handle, cfg,
    [](size_t step, double t, size_t total) {
        std::cout << "t=" << t << " (" << step << "/" << total << ")\n";
    });

// Export results.
libfmu_toolkit::FmuEngine::exportCsv(result,  "output.csv");
libfmu_toolkit::FmuEngine::exportJson(result, "output.json");

engine.unload(handle); // optional — destructor cleans up all handles
```

### FmuSession — step-by-step co-simulation

`FmuSession` gives fine-grained control for external time coordinators (Qt timers, SIL-Kit,
AUTOSAR middleware, etc.):

```cpp
libfmu_toolkit::SimConfig cfg;
cfg.startTime = 0.0;
cfg.stopTime  = 5.0;
cfg.stepSize  = 0.01;

auto session = engine.openSession(handle, cfg);

double t = cfg.startTime;
while (session.isActive() && t < cfg.stopTime) {
    // Write inputs (must happen before doStep)
    session.setValue("u", computeInput(t));

    // Advance the FMU by one communication step
    if (!session.doStep(t, cfg.stepSize)) break;

    // Read outputs after the step
    auto y = std::get<double>(session.getValue("y"));
    recordOutput(t, y);

    t += cfg.stepSize;
}

session.terminate(); // safe to call even if session is already inactive
```

### FmuInfo — metadata helpers

```cpp
auto info = engine.info(handle);

// Convenience filters — return pointers into the variable list.
auto inputs  = info.inputs();    // std::vector<const VariableInfo*>
auto outputs = info.outputs();
auto params  = info.parameters();

// By-name lookup.
const auto* var = info.findVariable("position"); // nullptr if not found

// DefaultExperiment values.
if (info.defaultExperiment.hasStopTime)
    cfg.stopTime = info.defaultExperiment.stopTime;
if (info.defaultExperiment.hasStepSize)
    cfg.stepSize = info.defaultExperiment.stepSize;
```

---

## FMU Validation In Depth

### Overview

The `FmuValidator` implements three validation phases in a single `validate()` call. All phases
run unconditionally, so you always receive the full diagnostic picture — not just the first
problem found.

```cpp
auto result = engine.validate(handle);

std::cout << (result.passed ? "PASSED" : "FAILED") << "\n";

for (const auto& err : result.errors)
    std::cout << "[" << categoryName(err.category) << "] " << err.message << "\n";

for (const auto& warn : result.warnings)
    std::cout << "[WARN] " << warn.message << "\n";
```

### Phase 1 — Structure

Verifies that the FMU archive follows the directory layout mandated by the FMI standard.

**FMI 2.0 required structure:**

```text
<FMU>/
├── modelDescription.xml          ← mandatory
└── binaries/                     ← mandatory
    ├── win32/   or  win64/       ← platform-specific (at least one)
    ├── linux32/ or  linux64/
    └── darwin32/ or darwin64/
```

**FMI 3.0 required structure:**

```text
<FMU>/
├── modelDescription.xml                 ← mandatory
└── binaries/                            ← mandatory
    ├── x86_64-windows/  (or x86-windows, aarch64-windows)
    ├── x86_64-linux/    (or x86-linux, aarch64-linux)
    └── x86_64-darwin/   (or aarch64-darwin)
```

**Version mismatch detection:** If an FMI 2.0 FMU contains only FMI 3.0-style directories (e.g.
`x86_64-windows/` instead of `win64/`), or vice-versa, a structure **error** is raised. This
catches a common mistake when exporting FMUs from tools that upgraded their naming convention.

**Library naming:** The shared library inside the platform directory must be named
`<modelIdentifier>.<ext>`. A mismatch produces a **warning**.

### Phase 2 — Schema

Validates `modelDescription.xml` against the official FMI XSD schema. The appropriate schema is
selected automatically based on the `fmiVersion` attribute:

- FMI 2.0 → `external/fmi-standard-2.0/schema/fmi2ModelDescription.xsd`
- FMI 3.0 → `external/fmi-standard-3.0/schema/fmi3ModelDescription.xsd`

Line and column numbers are included in all schema errors for precise navigation.

### Phase 3 — Semantics

Checks variable-model invariants that cannot be expressed in XSD:

| Check | Severity |
| --- | --- |
| Duplicate `valueReference` within the same `DataType` | Error |
| `causality=input` + `variability=constant` | Error |
| `causality=output` + `variability=fixed` or `tunable` | Error |
| `causality=parameter` + `variability=discrete` or `continuous` | Error |
| No output variables defined | Warning |

### Testing corrupted FMUs

The `data/` directory provides ready-made corrupted FMU archives for each category of structural
defect. Use Example 06 to run a batch validation sweep:

```bash
example_06_batch_validate data
```

Expected output for the corrupted archives:

```text
[Structure] FMI 2.0 FMU contains FMI 3.0-style binary directory 'x86_64-windows'...
[Structure] binaries/ directory not found...
[Schema]    Element 'fmiModelDescription': The attribute 'guid' is required...
```

---

## Examples

Six example applications are provided in `examples/`. They are built automatically when
`LIB_FMU_TOOLKIT_BUILD_EXAMPLES=ON` (the default).

| # | Example | Description |
| --- | --- | --- |
| 01 | `basic_simulate` | Full `simulate()` workflow: load → inspect → configure → run → print results |
| 02 | `step_by_step` | `FmuSession` manual step loop for external time coordinators |
| 03 | `validate_fmu` | Three-phase validation with detailed per-phase diagnostic output |
| 04 | `export_results` | CSV and JSON export of a simulation result, side by side |
| 05 | `generatore_sommatore` | Two-FMU orchestrated co-simulation (Generatore → Sommatore) |
| 06 | `batch_validate` | Batch validation of the `data/` directory: valid + corrupted FMUs |

### Running examples

After a successful build (`cmake --build --preset <preset>`), the example binaries are in
`build/<preset>/bin/`:

```bash
# Single FMU validation
build/windows-mingw-debug/bin/example_03_validate_fmu.exe data/fmi3/Generatore.fmu

# Validate all sample data
build/windows-mingw-debug/bin/example_06_batch_validate.exe data

# Orchestrated co-simulation
build/windows-mingw-debug/bin/example_05_generatore_sommatore.exe \
    data/fmi3/Generatore.fmu data/fmi3/Sommatore.fmu

# Export to CSV + JSON
build/windows-mingw-debug/bin/example_04_export_results.exe data/fmi3/Generatore.fmu 30 1
```

On Linux, omit the `.exe` extension.

---

## Runtime XSD Schema Discovery

When `FmuEngine` is constructed without an explicit XSD base path, the library searches for the
FMI schema trees in the following priority order:

1. **Environment variable** `LIB_FMU_TOOLKIT_XSD_BASE` — if set and non-empty, this path is used
   directly, bypassing all other candidates.
2. **Compile-time source path** `LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR` — defined by CMake as the
   `external/` directory under the project source root. This enables development builds without
   installation.
3. **Installed schema directory** — relative to the loaded library module:
   `<module_dir>/../share/lib-fmu-toolkit/schemas` and `<module_dir>/schemas`.
4. **Current working directory** — `${cwd}/external`.

The library requires two schema sub-trees at the resolved path:

```text
<xsdBasePath>/fmi-standard-2.0/schema/fmi2ModelDescription.xsd
<xsdBasePath>/fmi-standard-3.0/schema/fmi3ModelDescription.xsd
```

When installed, `cmake --install` copies these trees to
`share/lib-fmu-toolkit/schemas/fmi-standard-{2,3}.0/` automatically.

To override the schema path at runtime without recompilation:

```bash
export LIB_FMU_TOOLKIT_XSD_BASE=/opt/my-schemas
```

Or pass it explicitly at construction time:

```cpp
libfmu_toolkit::FmuEngine engine("/opt/my-schemas");
```

---

## Build Options

| CMake option | Default | Description |
| --- | --- | --- |
| `LIB_FMU_TOOLKIT_BUILD_SHARED` | `ON` | Build the shared library target |
| `LIB_FMU_TOOLKIT_BUILD_STATIC` | `ON` | Build the static library target |
| `LIB_FMU_TOOLKIT_BUILD_TESTS` | `ON` | Build the GoogleTest test suite |
| `LIB_FMU_TOOLKIT_BUILD_EXAMPLES` | `ON` | Build the six example applications |
| `LIB_FMU_TOOLKIT_ENABLE_WARNINGS` | `ON` | Enable strict compiler warning flags |
| `LIB_FMU_TOOLKIT_ENABLE_WERROR` | `OFF` | Treat warnings as errors |
| `LIB_FMU_TOOLKIT_ENABLE_SANITIZERS` | `OFF` | Enable AddressSanitizer / UBSan in Debug builds |
| `LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME` | `ON` | Statically link the MinGW C++ runtime (Windows) |

At least one of `LIB_FMU_TOOLKIT_BUILD_SHARED` or `LIB_FMU_TOOLKIT_BUILD_STATIC` must be `ON`;
a CMake fatal error is raised otherwise.

---

## Testing

The test suite lives in `tests/unit/` and `tests/integration/`. Unit tests use injected test
doubles (stubs) to exercise every layer without requiring real FMU binaries on disk.

```bash
# Run all tests with sanitizers (Linux)
ctest --preset linux-gcc-debug --output-on-failure

# Run all tests (Windows MinGW)
ctest --preset windows-mingw-debug --output-on-failure

# Run a specific test binary and filter by name
./build/linux-gcc-debug/bin/lib_fmu_toolkit_tests --gtest_filter="Validator*"
```

### Test coverage

| Test file | What is covered |
| --- | --- |
| `test_FmuValidator.cpp` | Structure (missing paths, binaries/, FMI 2.0 vs 3.0 directory naming), Schema (missing XSD, empty XML), Semantics (duplicate VR, invalid causality/variability, no outputs) |
| `test_ModelDescriptionParser.cpp` | FMI 2.0 and 3.0 XML parsing: all variable types, UnitDefinitions, TypeDefinitions, DefaultExperiment, error cases |
| `test_SimulationController.cpp` | Step count, recording interval, initial parameter injection, cancellation, progress callback |
| `test_ResultExporters.cpp` | CSV and JSON exporters: header rows, row counts, boolean encoding, string escaping, unwritable path error |
| `test_FmuEngine_load.cpp` | FMU loading workflow: archive extraction, DLL loading, symbol resolution |
| `test_fmi3_fmus.cpp` (integration) | 25+ real-FMU tests against Generatore.fmu and Sommatore.fmu |

---

## Documentation

In-depth technical documentation lives in the [`docs/`](docs/) directory:

| Document | Description |
| --- | --- |
| [requirements/srs.md](docs/requirements/srs.md) | Software Requirements Specification |
| [architecture/architecture.md](docs/architecture/architecture.md) | Layer map, class responsibilities, data-flow diagrams |
| [standards/fmi2_standard.md](docs/standards/fmi2_standard.md) | FMI 2.0 archive structure, XML schema, lifecycle, function signatures |
| [standards/fmi3_standard.md](docs/standards/fmi3_standard.md) | FMI 3.0 archive structure, new types, lifecycle, differences from FMI 2.0 |
| [reference/validation.md](docs/reference/validation.md) | Three-phase validation pipeline: checks, error messages, test data |
| [reference/api_reference.md](docs/reference/api_reference.md) | Complete public API reference (FmuEngine, FmuSession, types, exceptions) |
| [build/build.md](docs/build/build.md) | Build presets, CMake options, dependency resolution, Qt6 integration |

Architecture Decision Records:

| ADR | Decision |
| --- | --- |
| [0001-hexagonal-architecture.md](docs/adr/0001-hexagonal-architecture.md) | Why hexagonal architecture (Ports & Adapters) |
| [0002-pimpl-public-api.md](docs/adr/0002-pimpl-public-api.md) | Why PIMPL for the public API |
| [0003-cross-platform-toolchains-only.md](docs/adr/0003-cross-platform-toolchains-only.md) | Why MSVC is not supported |
| [0004-three-phase-fmu-validation.md](docs/adr/0004-three-phase-fmu-validation.md) | Why validation runs all three phases unconditionally |
| [0005-abi-stability-source-distribution.md](docs/adr/0005-abi-stability-source-distribution.md) | ABI stability mitigations (`std::string_view`, out-of-line methods, `_GLIBCXX_USE_CXX11_ABI`) |

---

## Contributing

Contributions, bug reports, and feature requests are welcome via the issue tracker and pull
requests.

The project follows these design principles rigorously:

- **SOLID** — each class has exactly one reason to change. Use-case classes depend on abstract
  port interfaces; concrete adapters are wired only at the composition root (`FmuEngine::Impl`).
  The FmuValidator is partitioned into three clearly-bounded private methods (one per phase).

- **DRY** — shared logic (FMI 2.0 / FMI 3.0 binary directory lists, platform detection macros)
  is defined once in named constants. The two FMI code paths in the simulation adapter share
  common initialization structure while preserving type-system differences.

- **KISS** — no speculative abstractions. Complexity lives where it genuinely belongs (the FMI
  lifecycle is inherently complex; the XSD/binary naming lookup needs exhaustive enumeration).
  The public API surface is minimal and orthogonal.

- **Hexagonal architecture** — the domain and use-case layers have zero dependencies on
  third-party libraries. Only adapters depend on libxml2 and libzip; this isolation is enforced
  by CMake target visibility (`PRIVATE` linkage in `src/`).

- **PIMPL** — public headers ship no internal types, ensuring ABI stability across minor releases
  and compatibility with Qt's include model.
