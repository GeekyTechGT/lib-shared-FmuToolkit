# FmuToolkit - User Manual

**Library:** `libfmutoolkit` (shared library) · **Version:** 1.1.0 · **Language:** C++20 or later · **Namespace:** `libfmu_toolkit`

This manual is the complete reference for **using** FmuToolkit from your own C++ application. It is
self-contained: everything you need to integrate, call and troubleshoot the library is described here. You do
not need the library's source code.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Delivered Package and Requirements](#2-delivered-package-and-requirements)
3. [Integrating the Library](#3-integrating-the-library)
4. [Core Concepts](#4-core-concepts)
5. [Quick Start](#5-quick-start)
6. [Loading and Unloading FMUs](#6-loading-and-unloading-fmus)
7. [Inspecting an FMU](#7-inspecting-an-fmu)
8. [Validating an FMU](#8-validating-an-fmu)
9. [Running a Complete Simulation](#9-running-a-complete-simulation)
10. [Step-by-Step Co-Simulation with FmuSession](#10-step-by-step-co-simulation-with-fmusession)
11. [FMI 3.0 Event Mode, Binary Variables and Clocks](#11-fmi-30-event-mode-binary-variables-and-clocks)
12. [Exporting Results](#12-exporting-results)
13. [Generating `modelDescription.xml` and C Accessors from JSON](#13-generating-modeldescriptionxml-and-c-accessors-from-json)
14. [Error Handling](#14-error-handling)
15. [Public API Reference](#15-public-api-reference)
16. [Deployment and Runtime Notes](#16-deployment-and-runtime-notes)
17. [Troubleshooting](#17-troubleshooting)
18. [Limitations and Known Constraints](#18-limitations-and-known-constraints)
19. [Glossary](#19-glossary)
20. [Further Documentation](#20-further-documentation)

---

## 1. Introduction

### 1.1 What FmuToolkit is

FmuToolkit is a C++ shared library that lets an application work with **Functional Mock-up Units (FMUs)**
conforming to the **FMI 2.0** and **FMI 3.0** standards for Co-Simulation. With a single entry-point class,
`FmuEngine`, you can:

| Capability | Entry point |
|---|---|
| Load an `.fmu` archive (extract, parse, load the platform binary, resolve FMI symbols) | `FmuEngine::load` |
| Read the full model metadata (variables, units, types, default experiment, terminals) | `FmuEngine::info` |
| Validate the FMU in three phases (archive structure, XSD schema, semantics) | `FmuEngine::validate` |
| Run a complete co-simulation and collect time series | `FmuEngine::simulate` |
| Drive a co-simulation step by step from an external master | `FmuEngine::openSession` / `FmuSession` |
| Export results to CSV or JSON | `FmuEngine::exportCsv` / `exportJson` |
| Generate `modelDescription.xml` and C accessor code from a JSON model definition | `FmuArtifactGenerator` and related classes |

### 1.2 Design guarantees

- **Small, stable public API.** The public headers use only the C++ standard library. No libxml2, libzip or
  internal types appear in them, so your application never needs those libraries.
- **Self-contained runtime.** On Windows/MinGW, libxml2 and libzip are linked *statically* into the library.
  At runtime you need only `libfmutoolkit.dll` and `libwinpthread-1.dll`.
- **Predictable errors.** All library failures derive from a single exception base class
  (`FmuToolkitException`); validation never throws, it reports.
- **Both FMI generations.** FMI 2.0 and FMI 3.0 FMUs are handled through the same API.

### 1.3 Intended audience

C++ developers integrating FMU loading, validation or co-simulation into a tool, test bench or simulation
master. Familiarity with the basic FMI concepts (model description, value references, causality, communication
step) is assumed; the [glossary](#19-glossary) summarises the terms used.

### 1.4 Conventions used in this manual

- Code samples assume `#include <lib_fmu_toolkit.hpp>` and, for brevity, omit the `libfmu_toolkit::` prefix
  where a `using namespace libfmu_toolkit;` is shown.
- *Requirement tags* such as `FR-03.1` identify the corresponding requirement in the library's Software
  Requirements Specification (see [Further Documentation](#20-further-documentation)).
- **Thread safety:** unless stated otherwise, no object of the library is thread-safe. Use one `FmuEngine` per
  thread, or synchronise externally.

---

## 2. Delivered Package and Requirements

### 2.1 Package layout

```text
<package>/
├── bin/          libfmutoolkit.dll              (Windows) / libfmutoolkit.so (Linux)
│                 libwinpthread-1.dll            (MinGW runtime, Windows)
├── lib/          libfmutoolkit.dll.a            import library (MinGW)
├── include/      lib_fmu_toolkit.hpp            umbrella header
│                 fmu_engine.hpp  fmu_generation.hpp  types.hpp
│                 exceptions.hpp  export.hpp  version.hpp
├── documents/    USER_MANUAL.pdf  CHANGELOG.pdf  INFO.md
└── license/      LICENSE
```

The headers are installed **flat** (no sub-folder). Add `<package>/include` to your include path and write
`#include <lib_fmu_toolkit.hpp>`.

### 2.2 Build-time requirements for your application

| Item | Requirement |
|---|---|
| C++ standard | C++20 or later (the API uses `std::span`, `std::string_view`, designated initialisers) |
| Compiler / ABI | The same toolchain family the library was built with (MinGW-w64 GCC 13+ on Windows, GCC on Linux), with `_GLIBCXX_USE_CXX11_ABI=1` |
| CMake (if used) | 3.25 or later |

> **ABI note.** The public API exposes standard-library types (`std::string`, `std::vector`, `std::map`,
> `std::variant`). Mixing compilers, standard-library implementations or ABI settings between your application
> and the library is **not supported** and typically fails at link time or crashes at run time. Build your
> application with a toolchain compatible with the one used to build the library.

### 2.3 Run-time requirements

| Item | Requirement |
|---|---|
| Library files | `libfmutoolkit.dll` and `libwinpthread-1.dll` next to your executable (Windows); `libfmutoolkit.so` on the loader path (Linux) |
| FMUs | Must contain a platform binary matching the host (see [section 6.2](#62-platform-binaries)) |
| XSD schemas | Required **only** for the schema phase of `validate()`; see [section 8.4](#84-locating-the-xsd-schemas) |

---

## 3. Integrating the Library

### 3.1 CMake (recommended)

If your build system exposes the deployed package as the imported target `fmutoolkit`, linking is a single line:

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE fmutoolkit)
```

For a plain CMake project that consumes the package folder directly:

```cmake
set(FMUTOOLKIT_ROOT "C:/path/to/lib-shared-FmuToolkit/1.1.0/windows-mingw64-release")

add_library(fmutoolkit SHARED IMPORTED)
set_target_properties(fmutoolkit PROPERTIES
    IMPORTED_LOCATION             "${FMUTOOLKIT_ROOT}/bin/libfmutoolkit.dll"
    IMPORTED_IMPLIB               "${FMUTOOLKIT_ROOT}/lib/libfmutoolkit.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${FMUTOOLKIT_ROOT}/include"
    INTERFACE_COMPILE_DEFINITIONS "LIB_FMU_TOOLKIT_SHARED;_GLIBCXX_USE_CXX11_ABI=1")

target_link_libraries(my_app PRIVATE fmutoolkit)
target_compile_features(my_app PRIVATE cxx_std_20)

# Windows has no rpath: place the DLLs next to the executable.
add_custom_command(TARGET my_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FMUTOOLKIT_ROOT}/bin/libfmutoolkit.dll"
            "${FMUTOOLKIT_ROOT}/bin/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:my_app>")
```

### 3.2 Manual compilation

```bash
g++ -std=c++20 -D_GLIBCXX_USE_CXX11_ABI=1 -DLIB_FMU_TOOLKIT_SHARED \
    -I<package>/include main.cpp \
    -L<package>/lib -lfmutoolkit -o my_app
```

**The macro `LIB_FMU_TOOLKIT_SHARED` must be defined when you consume the DLL on Windows.** Without it the
library's symbols are not declared `dllimport` and you will get unresolved-symbol link errors. The imported
CMake target defines it for you.

### 3.3 Headers

| Header | Contents |
|---|---|
| `lib_fmu_toolkit.hpp` | **Umbrella header.** Includes everything below. Use this unless you have a reason not to. |
| `fmu_engine.hpp` | `FmuEngine`, `FmuSession` |
| `fmu_generation.hpp` | `ModelDescriptionGenerator`, `FmiCCodeGenerator`, `FmuArtifactGenerator`, option/result structs |
| `types.hpp` | All data types: `FmuInfo`, `VariableInfo`, `SimConfig`, `SimResult`, `ValidationResult`, enumerations |
| `exceptions.hpp` | Exception hierarchy |
| `export.hpp` | Symbol-visibility macro `LIB_FMU_TOOLKIT_API` |
| `version.hpp` | Compile-time version constants |

### 3.4 Version macros

```cpp
#include <lib_fmu_toolkit.hpp>

LIB_FMU_TOOLKIT_VERSION_MAJOR   // e.g. 1
LIB_FMU_TOOLKIT_VERSION_MINOR   // e.g. 1
LIB_FMU_TOOLKIT_VERSION_PATCH   // e.g. 0
LIB_FMU_TOOLKIT_VERSION_STRING  // "1.1.0"
```

---

## 4. Core Concepts

### 4.1 The engine and handles

`FmuEngine` is the facade for the whole library. It owns every FMU you load and identifies each one by an
opaque **handle** of type `uint64_t`. You never hold a pointer to an internal object; you pass the handle back
to the engine.

```text
FmuEngine ──load()──▶ handle (uint64_t) ──▶ info() / validate() / simulate() / openSession() / unload()
```

- One engine can hold any number of FMUs at once.
- Handles are valid until `unload(handle)` or until the engine is destroyed (the destructor unloads everything).
- Passing a handle the engine does not know raises `std::out_of_range`.

### 4.2 Two ways to simulate

| Mode | Use when | API |
|---|---|---|
| **Batch** | You want the toolkit to run the whole simulation and hand you the time series | `FmuEngine::simulate` → `SimResult` |
| **Interactive** | An external master (co-simulation coordinator, test bench, real-time loop) controls time and exchanges values at every step | `FmuEngine::openSession` → `FmuSession` |

### 4.3 Value types

FMI variables are exchanged through four C++ types, collected in two `std::variant` aliases:

```cpp
using ParameterValue = std::variant<double, int64_t, bool, std::string>;  // input to a simulation
using OutputValue    = std::variant<double, int64_t, bool, std::string>;  // output of a simulation
```

| FMI variable types | C++ type used with the API |
|---|---|
| Real (FMI 2.0), Float32/Float64 (FMI 3.0) | `double` |
| Integer (FMI 2.0), Int8–Int64, UInt8–UInt64 (FMI 3.0) | `int64_t` |
| Boolean | `bool` |
| String | `std::string` / `std::string_view` |
| Binary (FMI 3.0) | `std::vector<uint8_t>` / `std::span<const uint8_t>` (see [section 11](#11-fmi-30-event-mode-binary-variables-and-clocks)) |
| Clock (FMI 3.0) | `bool` (see [section 11](#11-fmi-30-event-mode-binary-variables-and-clocks)) |

### 4.4 Strings

All path and name parameters are `std::string_view`. You may pass `std::string`, string literals or views
without conversion or allocation.

### 4.5 Lifetime rules

1. An `FmuSession` borrows the FMU of its handle. **Do not call `unload(handle)` while a session opened from
   that handle is still active** - this is undefined behaviour.
2. `FmuInfo` and `ValidationResult` are plain value snapshots; they remain valid after the FMU is unloaded.
   The pointers returned by `FmuInfo::inputs()`, `outputs()`, `parameters()` and `findVariable()` point *into
   that `FmuInfo` object* and live exactly as long as it does.
3. `FmuEngine` and `FmuSession` are **move-only** (copy is deleted).

---

## 5. Quick Start

A complete program that loads an FMU, prints its outputs, simulates it and exports the result:

```cpp
#include <lib_fmu_toolkit.hpp>

#include <iostream>
#include <variant>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " model.fmu\n"; return 2; }

    try {
        libfmu_toolkit::FmuEngine engine;

        // 1. Load
        const uint64_t handle = engine.load(argv[1]);

        // 2. Inspect
        const libfmu_toolkit::FmuInfo info = engine.info(handle);
        std::cout << info.modelName << " (FMI "
                  << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2 ? "2.0" : "3.0")
                  << ")\n";
        for (const auto* v : info.outputs())
            std::cout << "  output: " << v->name << '\n';

        // 3. Validate (never throws; findings are returned)
        const auto report = engine.validate(handle);
        std::cout << "valid: " << std::boolalpha << report.passed << '\n';

        // 4. Simulate
        libfmu_toolkit::SimConfig cfg;
        cfg.startTime = 0.0;
        cfg.stopTime  = 10.0;
        cfg.stepSize  = 0.01;
        const libfmu_toolkit::SimResult result = engine.simulate(handle, cfg);
        std::cout << result.stepCount() << " points recorded\n";

        // 5. Export
        libfmu_toolkit::FmuEngine::exportCsv (result, "result.csv");
        libfmu_toolkit::FmuEngine::exportJson(result, "result.json");

        engine.unload(handle);   // optional: the destructor would do it
        return 0;
    }
    catch (const libfmu_toolkit::FmuToolkitException& e) {
        std::cerr << "FmuToolkit error: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }
    return 1;
}
```

---

## 6. Loading and Unloading FMUs

### 6.1 `FmuEngine::load`

```cpp
[[nodiscard]] uint64_t load(std::string_view fmuPath);
```

`load` performs, in order:

1. Extracts the `.fmu` ZIP archive into a temporary directory managed by the engine.
2. Parses `modelDescription.xml` (and, for FMI 3.0, `terminalsAndIcons/terminalsAndIcons.xml` and the folders
   under `extra/`) into an internal model.
3. Loads the platform shared library `binaries/<platform>/<modelIdentifier>.dll` (or `.so`).
4. Resolves all required FMI function symbols.

| Exception | Raised when |
|---|---|
| `std::invalid_argument` | The path is invalid or the file does not exist |
| `FmuLoadError` | The archive cannot be opened/extracted, `modelDescription.xml` is unreadable, the binary cannot be loaded, or an FMI symbol is missing |

`load` does **not** validate the FMU in depth. A structurally unusual FMU may load successfully and still fail
`validate()`. Call `validate()` when you need a conformance verdict.

### 6.2 Platform binaries

The FMU must ship a binary for the host platform. The directory naming depends on the FMI version:

| FMI version | Directory naming | Examples |
|---|---|---|
| 2.0 | Legacy `<os><bits>` | `win32`, `win64`, `linux32`, `linux64`, `darwin32`, `darwin64` |
| 3.0 | `<arch>-<os>` | `x86_64-windows`, `x86_64-linux`, `aarch64-linux`, `x86-windows`, `x86_64-darwin`, `aarch64-darwin` |

The shared-library file name (without extension) must equal the `modelIdentifier` declared in the model
description. Loading fails with `FmuLoadError` if no matching binary exists for the host.

### 6.3 `FmuEngine::unload`

```cpp
void unload(uint64_t handle);
```

Releases the FMU and closes its shared library. It is optional (the engine destructor unloads all remaining
handles) but recommended when you load many FMUs over the engine's lifetime. Throws `std::out_of_range` for an
unknown handle.

### 6.4 Constructing the engine

```cpp
explicit FmuEngine(std::string_view xsdBasePath = {});
```

The optional argument tells the engine where the FMI XSD schemas are. Leave it empty to use automatic
discovery. This only affects `validate()`; see [section 8.4](#84-locating-the-xsd-schemas).

---

## 7. Inspecting an FMU

```cpp
[[nodiscard]] FmuInfo info(uint64_t handle) const;
```

`FmuInfo` is a complete snapshot of the model description.

### 7.1 `FmuInfo` fields

| Field | Type | Meaning |
|---|---|---|
| `modelName` | `std::string` | Model name |
| `modelIdentifier` | `std::string` | Identifier used to name the shared library |
| `guid` | `std::string` | FMI 2.0 `guid` / FMI 3.0 `instantiationToken` |
| `description` | `std::string` | Human-readable description |
| `generationTool`, `generationDateAndTime` | `std::string` | Authoring metadata (may be empty) |
| `fmiVersion` | `FmiVersion` | `FMI2` or `FMI3` |
| `numberOfEventIndicators` | `uint32_t` | FMI 2.0 only |
| `hasEventMode` | `bool` | FMI 3.0 `<CoSimulation hasEventMode>` |
| `mightReturnEarlyFromDoStep` | `bool` | FMI 3.0 `<CoSimulation mightReturnEarlyFromDoStep>` |
| `terminals` | `vector<TerminalInfo>` | FMI 3.0 terminals (empty if none) |
| `layeredStandards` | `vector<string>` | Folders under `extra/`, e.g. `org.fmi-standard.fmi-ls-bus` |
| `variables` | `vector<VariableInfo>` | All model variables |
| `unitDefinitions` | `vector<UnitInfo>` | Declared units (`name`, `factor`, `offset`) |
| `typeDefinitions` | `vector<TypeDefinitionInfo>` | Named types (unit, quantity, display unit, min/max) |
| `defaultExperiment` | `DefaultExperiment` | Author-recommended start/stop/step/tolerance |

### 7.2 Convenience queries

```cpp
std::vector<const VariableInfo*> inputs()     const;   // causality == Input
std::vector<const VariableInfo*> outputs()    const;   // causality == Output
std::vector<const VariableInfo*> parameters() const;   // causality == Parameter
const VariableInfo*              findVariable(std::string_view name) const;  // nullptr if absent
```

### 7.3 `VariableInfo`

| Field | Meaning |
|---|---|
| `name`, `description` | Identification |
| `valueReference` | FMI value reference |
| `causality` | `Parameter`, `CalculatedParameter`, `Input`, `Output`, `Local`, `Independent` |
| `variability` | `Constant`, `Fixed`, `Tunable`, `Discrete`, `Continuous` |
| `dataType` | `Real`, `Integer`, `Boolean`, `String`, `Float64`, `Float32`, `Int8`…`Int64`, `UInt8`…`UInt64`, `Binary`, `Clock` |
| `hasStartValue`, `startValueStr` | Declared start value, rendered as text |
| `unit`, `typeName` | Unit and referenced type definition (may be empty) |
| `mimeType`, `maxSize` | Binary variables only |
| `intervalVariability` | Clock variables only |
| `clocks` | Value references of the Clocks the variable is clocked by |

### 7.4 Using the default experiment

`DefaultExperiment` carries `has*` flags telling you which attributes the author actually declared. Only trust a
value when its flag is `true`:

```cpp
SimConfig cfg;
const auto& de = info.defaultExperiment;
cfg.startTime = de.hasStartTime ? de.startTime : 0.0;
cfg.stopTime  = de.hasStopTime  ? de.stopTime  : 1.0;
cfg.stepSize  = de.hasStepSize  ? de.stepSize  : 0.01;
```

### 7.5 Example: listing the interface

```cpp
const FmuInfo info = engine.info(handle);
for (const auto& v : info.variables) {
    std::cout << v.name << "  vr=" << v.valueReference
              << "  causality=" << static_cast<int>(v.causality)
              << "  unit=" << (v.unit.empty() ? "-" : v.unit) << '\n';
}
```

---

## 8. Validating an FMU

```cpp
[[nodiscard]] ValidationResult validate(uint64_t handle) const;
```

`validate` **never throws for validation findings**; every problem is returned in the `ValidationResult`. (It
still throws `std::out_of_range` for an unknown handle.)

### 8.1 The three phases

| Phase | `ValidationCategory` | What is checked |
|---|---|---|
| 1 | `Structure` | `modelDescription.xml` at the archive root and non-empty; `binaries/` present; a binary directory exists for the host platform; the directory naming matches the declared FMI version (FMI 2.0 legacy names vs. FMI 3.0 `<arch>-<os>` names); the shared library exists and its file name matches `modelIdentifier` |
| 2 | `Schema` | `modelDescription.xml` validated against the official FMI 2.0 or 3.0 XSD (libxml2). Errors carry line and column numbers. |
| 3 | `Semantic` | Causality/variability combinations (e.g. an input cannot be `constant`; outputs and parameters have restricted variabilities); duplicate value references per data type; a warning when the FMU has no outputs; and the FMI-LS-BUS rules described below |

**FMI-LS-BUS rules (FMI 3.0, semantic phase):** the FMU declares Event Mode; bus Binary variables are clocked
by existing Clock variables; terminal members refer to existing variables of the correct type and causality;
`Tx_Data`/`Tx_Clock` and `Rx_Data`/`Rx_Clock` are present in pairs; a terminal has at least one Tx or Rx member.

### 8.2 `ValidationResult`

```cpp
struct ValidationResult {
    bool passed;                          // true if no errors in any phase
    std::vector<ValidationIssue> errors;  // cause passed == false
    std::vector<ValidationIssue> warnings;// informational, do not affect passed
    bool hasErrors()   const noexcept;
    bool hasWarnings() const noexcept;
};

struct ValidationIssue {
    ValidationCategory category;  // Structure | Schema | Semantic
    std::string        message;
    int line   = -1;              // populated for Schema issues, otherwise -1
    int column = -1;
};
```

### 8.3 Example

```cpp
const auto report = engine.validate(handle);

auto print = [](const char* tag, const std::vector<ValidationIssue>& issues) {
    for (const auto& i : issues) {
        std::cout << tag << " [" << (i.category == ValidationCategory::Structure ? "structure"
                                   : i.category == ValidationCategory::Schema    ? "schema"
                                                                                 : "semantic") << "] "
                  << i.message;
        if (i.line >= 0) std::cout << "  (line " << i.line << ", col " << i.column << ')';
        std::cout << '\n';
    }
};
print("ERROR  ", report.errors);
print("WARNING", report.warnings);

if (!report.passed) return EXIT_FAILURE;
```

### 8.4 Locating the XSD schemas

Phase 2 needs the official FMI schema files. The engine expects a base directory containing:

```text
<base>/fmi-standard-2.0/schema/fmi2ModelDescription.xsd
<base>/fmi-standard-3.0/schema/fmi3ModelDescription.xsd
```

The base is resolved in this priority order; the first candidate in which **both** schema files exist wins:

1. The `xsdBasePath` argument given to the `FmuEngine` constructor, if non-empty.
2. The environment variable `LIB_FMU_TOOLKIT_XSD_BASE`.
3. The schema directory recorded when the library was built (relevant only on the build machine).
4. `<directory of libfmutoolkit>/../share/lib-fmu-toolkit/schemas`
5. `<directory of libfmutoolkit>/schemas`
6. `<current working directory>/external`

> **Important for deployed applications.** The delivered package does **not** include the XSD files. If you
> use `validate()` you must ship the two schema trees with your application and point the engine at them
> (constructor argument, environment variable, or one of the locations above). If the schema cannot be found,
> phase 2 does not abort validation: it adds an error `XSD schema file not found: <path>` to the result, so
> `passed` will be `false` even for a perfectly valid FMU. Phases 1 and 3 still run.
>
> The schemas are published in the open-source [`modelica/fmi-standard`](https://github.com/modelica/fmi-standard)
> repository (`schema/fmi2ModelDescription.xsd` at branch `v2.0.x`, `schema/fmi3ModelDescription.xsd` at `main`).

---

## 9. Running a Complete Simulation

```cpp
[[nodiscard]] SimResult simulate(
    uint64_t handle,
    const SimConfig& config,
    std::function<void(size_t, double, size_t)> progress = nullptr);
```

`simulate` drives the whole Co-Simulation life cycle: instantiate, set up the experiment / enter
initialisation mode, apply the initial parameters, exit initialisation, run the fixed-size communication-step
loop up to `stopTime`, then terminate and free the instance. It returns the recorded time series.

### 9.1 `SimConfig`

| Field | Default | Meaning |
|---|---|---|
| `startTime` | `0.0` | Simulation start time (s) |
| `stopTime` | `1.0` | Simulation stop time (s) |
| `stepSize` | `0.001` | Communication step size (s); fixed for the whole run |
| `tolerance` | `1e-6` | Relative tolerance passed to the FMU |
| `logLevel` | `LogLevel::Warning` | `None`, `Warning`, `Info`, `Debug`, `All`. The FMU decides how to honour it |
| `initialParameters` | empty | `map<string, ParameterValue>`; applied during initialisation, before exiting initialisation mode. Unlisted variables keep their declared start values |
| `outputVariables` | empty | Names of outputs to record. **Empty means all outputs.** Unknown names are silently ignored |
| `recordingInterval` | `1` | Record every N-th communication step. Use a larger value to reduce memory on long runs |
| `eventModeUsed` | `false` | FMI 3.0: instantiate in Event Mode (needed for Clocks and Binary buffers) |
| `earlyReturnAllowed` | `false` | FMI 3.0: allow `fmi3DoStep` to return before the requested step size |

### 9.2 Progress callback

The optional callback is invoked at each recorded step with `(recordIndex, currentTime, totalSteps)`:

```cpp
auto result = engine.simulate(handle, cfg,
    [](size_t index, double t, size_t total) {
        if (index % 100 == 0)
            std::cout << "t=" << t << "  (" << index << '/' << total << ")\r" << std::flush;
    });
```

The callback runs on the calling thread, inside `simulate`. Keep it short.

### 9.3 `SimResult`

```cpp
struct SimResult {
    std::vector<double> timePoints;                              // one entry per recorded step
    std::map<std::string, std::vector<OutputValue>> series;      // variable name -> values
    size_t stepCount() const noexcept;                           // number of recorded points
    bool   empty()     const noexcept;
};
```

Every vector in `series` has the same length as `timePoints`.

```cpp
for (const auto& [name, values] : result.series) {
    if (values.empty()) continue;
    std::cout << name << " = ";
    std::visit([](const auto& v) { std::cout << v; }, values.back());
    std::cout << '\n';
}
```

### 9.4 Setting parameters and recording selected outputs

```cpp
SimConfig cfg;
cfg.stopTime  = 5.0;
cfg.stepSize  = 0.005;
cfg.recordingInterval = 10;                                   // record every 10th step
cfg.outputVariables   = {"speed", "position"};                // only these two
cfg.initialParameters["mass"]    = 2.5;                       // double
cfg.initialParameters["mode"]    = int64_t{3};                // integer
cfg.initialParameters["enabled"] = true;                      // boolean
cfg.initialParameters["label"]   = std::string("run-A");      // string
```

> **Pitfall:** a C++ integer literal such as `3` converts to `double` or `int64_t` ambiguously inside a
> `std::variant`. Write `int64_t{3}` for integer parameters and `2.5`/`3.0` for real ones.

### 9.5 Exceptions

| Exception | Raised when |
|---|---|
| `FmuSimulationError` | A fatal FMI status is returned during the run, or a fault occurs inside the FMU binary |
| `FmuLoadError` | The FMU binary cannot be re-loaded for the simulation |
| `std::out_of_range` | Unknown handle |

---

## 10. Step-by-Step Co-Simulation with FmuSession

Use a session when **you** own the time loop: a co-simulation master, a SIL-Kit/AUTOSAR bridge, a hardware-in-
the-loop rig, or any case where inputs must be set and outputs read at every step.

### 10.1 Life cycle

```cpp
SimConfig cfg;
cfg.startTime = 0.0;
cfg.stopTime  = 10.0;
cfg.stepSize  = 0.01;

FmuSession session = engine.openSession(handle, cfg);   // FMU is instantiated AND initialised here

double t = 0.0;
while (t < cfg.stopTime && session.isActive()) {
    session.setValue("u", 1.5);                 // optional: write an input/parameter
    if (!session.doStep(t, cfg.stepSize)) break; // false => the FMU asked to stop
    t += cfg.stepSize;
    double y = std::get<double>(session.getValue("y"));
}
session.terminate();                             // explicit; the destructor would do it too
```

### 10.2 Member reference

| Member | Description |
|---|---|
| `const FmuInfo& info() const noexcept` | Metadata of the loaded FMU; valid for the lifetime of the session |
| `void setValue(name, double)` | Write a Real/Float input or parameter |
| `void setValue(name, int64_t)` | Write an integer input or parameter (all sized integer types) |
| `void setValue(name, bool)` | Write a Boolean |
| `void setValue(name, std::string_view)` | Write a String |
| `OutputValue getValue(name) const` | Read any readable variable; returns `double`, `int64_t`, `bool` or `std::string` |
| `bool doStep(double t, double h)` | Advance one communication step; `false` if the FMU requests early termination |
| `void terminate()` | `fmi*Terminate` + `fmi*FreeInstance`; safe to call repeatedly |
| `bool isActive() const noexcept` | `true` between `openSession` and `terminate` |

`setValue`/`getValue` take the variable **name**; the session resolves value references for you. Exceptions:

| Exception | Raised when |
|---|---|
| `std::out_of_range` | The variable name is not in the FMU |
| `FmuSimulationError` | The FMI call reports an error status, or the session is no longer active |

### 10.3 Choosing the right `setValue` overload

Because overloads exist for `double`, `int64_t`, `bool` and `std::string_view`, an unsuffixed integer literal can
be ambiguous. Be explicit:

```cpp
session.setValue("gain",  2.0);          // double
session.setValue("count", int64_t{4});   // integer
session.setValue("on",    true);         // bool
session.setValue("tag",   "abc");        // string
```

### 10.4 Rules to remember

- A session is **move-only**; return it from functions or store it in a `std::optional`/`std::unique_ptr`.
- Do not call `FmuEngine::unload` for the FMU while its session is active.
- Calling `doStep`, `getValue` or `setValue` after `terminate()` is invalid.
- One `FmuEngine`/`FmuSession` per thread unless you synchronise externally.

---

## 11. FMI 3.0 Event Mode, Binary Variables and Clocks

These facilities support FMI 3.0 FMUs that use Clocks and Binary variables, notably those implementing the
**FMI-LS-BUS** layered standard. They are available through `FmuSession` only and require an FMI 3.0 FMU that
declares `hasEventMode="true"`.

### 11.1 Opening a session in Event Mode

```cpp
SimConfig cfg;
cfg.eventModeUsed = true;                  // required to drive Clocks and Binary buffers
// cfg.earlyReturnAllowed = true;          // optional: let doStep return before the full step
FmuSession s = engine.openSession(handle, cfg);
```

`openSession` settles the initial discrete states and leaves the FMU in **Step Mode**.

### 11.2 Member reference

| Member | Description |
|---|---|
| `StepOutcome doStepEx(t, h)` | Like `doStep` but returns the full outcome |
| `void enterEventMode()` | Switch to Event Mode |
| `void enterStepMode()` | Switch back to Step Mode |
| `DiscreteStatesOutcome updateDiscreteStates()` | Process events; call repeatedly while `discreteStatesNeedUpdate` is `true` |
| `std::vector<uint8_t> getBinary(name) const` | Copy of a Binary variable's current buffer (empty = no data) |
| `void setBinary(name, std::span<const uint8_t>)` | Write a Binary variable's buffer |
| `bool getClock(name) const` | Read a Clock's active state |
| `void setClock(name, bool active)` | Activate/deactivate a Clock (Event Mode) |
| `ClockInterval getClockInterval(name) const` | Read a Clock's next interval and qualifier |

All of them throw `FmuSimulationError` if the FMU is not FMI 3.0, lacks the required functions, or the call is
made in the wrong mode, and `std::out_of_range` for an unknown variable.

### 11.3 Outcome structures

```cpp
struct StepOutcome {
    bool   completed;            // false: the FMU discarded the step
    bool   eventHandlingNeeded;  // the FMU needs Event Mode before the next step
    bool   terminateSimulation;  // the FMU asks to end the simulation
    bool   earlyReturn;          // the FMU stopped before the requested step size
    double lastSuccessfulTime;   // time actually reached
};

struct DiscreteStatesOutcome {
    bool   discreteStatesNeedUpdate;  // call updateDiscreteStates() again
    bool   terminateSimulation;
    bool   nextEventTimeDefined;
    double nextEventTime;
};

enum class IntervalQualifier : uint8_t { NotYetKnown, Unchanged, Changed };
struct ClockInterval { double interval; IntervalQualifier qualifier; };
```

### 11.4 Typical event-aware loop

```cpp
double t = cfg.startTime;
while (t < cfg.stopTime) {
    StepOutcome o = s.doStepEx(t, cfg.stepSize);
    if (o.terminateSimulation) break;
    t = o.lastSuccessfulTime;

    if (o.eventHandlingNeeded) {
        s.enterEventMode();
        DiscreteStatesOutcome d;
        do { d = s.updateDiscreteStates(); } while (d.discreteStatesNeedUpdate);
        s.enterStepMode();
    }
}
s.terminate();
```

### 11.5 Terminals and layered standards

`FmuInfo::terminals` lists the terminals of `terminalsAndIcons.xml` (name, `terminalKind`, `matchingRule` and
members with `variableName`, `memberName`, `variableKind`). Nested terminals appear as separate entries with a
dotted name. `FmuInfo::layeredStandards` lists the folders found under `extra/`.

### 11.6 Sized integer access

The sized integer types (`Int8`…`Int64`, `UInt8`…`UInt64`) are read and written through the ordinary
`setValue(name, int64_t)` / `getValue(name)` calls.

---

## 12. Exporting Results

Both exporters are **static**: they need a `SimResult`, not a loaded FMU.

```cpp
static void FmuEngine::exportCsv (const SimResult& result, std::string_view outputPath);
static void FmuEngine::exportJson(const SimResult& result, std::string_view outputPath);
```

An existing file at `outputPath` is overwritten. Both throw `std::ios_base::failure` if the file cannot be
opened or written.

### 12.1 CSV format

```text
time,var1,var2,...
0.000000,1.000000,0
0.010000,1.000050,1
```

One row per recorded time point. Strings are quoted per RFC 4180 and embedded double quotes are doubled.
Booleans are written as `0` / `1`.

### 12.2 JSON format

```json
{
  "timePoints": [0.0, 0.01, 0.02],
  "series": {
    "position": [0.0, 0.0001, 0.0004],
    "state":    ["idle", "idle", "run"],
    "running":  [false, false, true]
  }
}
```

Booleans are written as `true`/`false`; strings are JSON-escaped.

---

## 13. Generating `modelDescription.xml` and C Accessors from JSON

FmuToolkit can turn a JSON model definition into a validated `modelDescription.xml` and a C source file with
FMI-oriented getter/setter helpers. This is useful to scaffold FMUs without writing XML by hand.

### 13.1 API overview

| Class | Purpose |
|---|---|
| `ModelDescriptionGenerator` | JSON → `modelDescription.xml` (string or file) |
| `FmiCCodeGenerator` | JSON → C accessor source (string or file) |
| `FmuArtifactGenerator` | One call that produces either or both, controlled by `FmuArtifactGenerationOptions` |

```cpp
struct FmuArtifactGenerationOptions {
    std::filesystem::path outputDirectory{"."};
    std::string modelDescriptionFileName{"modelDescription.xml"};
    std::string cSourceFileName;            // empty => "<modelIdentifier>_accessors.c"
    bool generateModelDescription{true};
    bool generateCSource{true};
    bool overwrite{true};
};

struct GeneratedArtifactPaths {
    std::filesystem::path modelDescriptionXmlPath;   // empty if not generated
    std::filesystem::path cSourcePath;               // empty if not generated
};
```

### 13.2 Example

```cpp
libfmu_toolkit::FmuArtifactGenerator generator;

libfmu_toolkit::FmuArtifactGenerationOptions options;
options.outputDirectory = "generated/my_model";
options.generateCSource = false;                       // XML only

const auto paths = generator.generateFromJson("model.json", options);
std::cout << "written: " << paths.modelDescriptionXmlPath << '\n';
```

To obtain the content in memory instead of on disk:

```cpp
libfmu_toolkit::ModelDescriptionGenerator xml;
std::string text = xml.generateFromJson("model.json");     // throws FmuValidationError if semantically invalid

libfmu_toolkit::FmiCCodeGenerator c;
std::string source = c.generateFromJson("model.json");
```

### 13.3 JSON model definition

The root is an object with these members:

| Member | Required | Description |
|---|---|---|
| `model` | yes | Model-level metadata (below) |
| `variables` | yes | **Non-empty** array of variable objects |
| `defaultExperiment` | no | `startTime`, `stopTime`, `stepSize`, `tolerance` |
| `unitDefinitions` | no | Array of `{ "name", "factor", "offset" }` |
| `typeDefinitions` | no | Array of `{ "name", "type", "description", "unit", "quantity" }` |
| `logCategories` | no | Array of `{ "name", "description" }` |

**`model` members:** `standard` (`"fmi2"` or `"fmi3"`), `name`, `identifier`, a unique id (`guid` for FMI 2.0,
`instantiationToken` for FMI 3.0 - **required**), and optional `description`, `author`, `version`, `copyright`,
`license`, `generationTool`, `generationDateAndTime`, `variableNamingConvention` (`flat` or `structured`),
`numberOfEventIndicators` (FMI 2.0).

**Variable members:**

| Member | Notes |
|---|---|
| `name` | Required, unique |
| `type` | `Real`, `Integer`, `Boolean`, `String` (FMI 2.0); `Float32`, `Float64`, `Int8`…`Int64`, `UInt8`…`UInt64`, `Boolean`, `String` (FMI 3.0). Using an FMI 3.0-only type in an FMI 2.0 model is an error |
| `causality` | `parameter`, `calculatedParameter`, `input`, `output`, `local`, `independent` |
| `variability` | `constant`, `fixed`, `tunable`, `discrete`, `continuous` |
| `initial` | `exact`, `approx`, `calculated` |
| `valueReference` | Optional explicit value reference (unsigned 32-bit) |
| `dimensions` | Optional array of positive integers declaring an array variable |
| `start` | Scalar, or an array whose length equals the product of `dimensions` |
| `declaredType`, `description` | Optional |

Minimal FMI 2.0 example:

```json
{
  "model": {
    "standard": "fmi2",
    "name": "Adder",
    "identifier": "adder",
    "guid": "{adder-guid-0001}"
  },
  "defaultExperiment": { "startTime": 0.0, "stopTime": 5.0, "stepSize": 0.1 },
  "variables": [
    { "name": "a", "type": "Real", "causality": "input",  "variability": "continuous", "start": 0.0 },
    { "name": "b", "type": "Real", "causality": "input",  "variability": "continuous", "start": 0.0 },
    { "name": "y", "type": "Real", "causality": "output", "variability": "continuous", "initial": "calculated" }
  ]
}
```

### 13.4 Validation applied to generated models

Generation fails with `FmuValidationError` (or `FmuToolkitException` for malformed input) when, for example:

- a required key is missing, a value has the wrong JSON type, or the root is not an object;
- `standard` is not `fmi2`/`fmi3`, or an FMI 3.0 model has no `instantiationToken`;
- `variables` is missing or empty, or two variables share a name;
- a data type is not allowed in the chosen standard;
- `dimensions` contains zero/non-positive values, or the number of `start` values does not match the element
  count, or a start value has the wrong type for the variable;
- value references are duplicated within a data type;
- an `input` is `constant`, an `output` has an unsupported variability, or a `parameter` has an unsupported
  variability.

Error messages name the offending variable or key.

### 13.5 Current limits

The generator does not emit Binary or Clock variables, terminals or FMI-LS-BUS manifests.

---

## 14. Error Handling

### 14.1 Exception hierarchy

```text
std::runtime_error
 └── FmuToolkitException            base class of every library error
      ├── FmuLoadError              load failures
      ├── FmuValidationError        generation-time semantic validation failure
      └── FmuSimulationError        simulation / session failures
```

In addition the API uses standard exceptions:

| Exception | Meaning |
|---|---|
| `std::out_of_range` | Unknown engine handle or unknown variable name |
| `std::invalid_argument` | Invalid or non-existent FMU path passed to `load` |
| `std::ios_base::failure` | Result export could not write the file |

### 14.2 What each call can throw

| Call | Exceptions |
|---|---|
| `FmuEngine::load` | `std::invalid_argument`, `FmuLoadError` |
| `FmuEngine::info` | `std::out_of_range` |
| `FmuEngine::validate` | `std::out_of_range` only (findings are returned, not thrown) |
| `FmuEngine::simulate` | `FmuSimulationError`, `FmuLoadError`, `std::out_of_range` |
| `FmuEngine::openSession` | `FmuSimulationError`, `std::out_of_range` |
| `FmuEngine::unload` | `std::out_of_range` |
| `FmuEngine::exportCsv / exportJson` | `std::ios_base::failure` |
| `FmuSession::setValue / getValue` | `std::out_of_range`, `FmuSimulationError` |
| `FmuSession::doStep` and Event Mode members | `FmuSimulationError` (and `std::out_of_range` for named variables) |
| `FmuSession::terminate` | `FmuSimulationError` |
| Generators | `FmuValidationError`, `FmuToolkitException`, filesystem errors |

### 14.3 Recommended pattern

Catch specific exceptions first, then the library base class, then `std::exception`:

```cpp
try {
    // ... use the library ...
}
catch (const libfmu_toolkit::FmuLoadError& e)       { /* bad or incompatible FMU       */ }
catch (const libfmu_toolkit::FmuSimulationError& e) { /* the FMU failed while running  */ }
catch (const libfmu_toolkit::FmuToolkitException& e){ /* any other library error       */ }
catch (const std::out_of_range& e)                  { /* wrong handle or variable name */ }
catch (const std::exception& e)                     { /* everything else               */ }
```

---

## 15. Public API Reference

All declarations live in namespace `libfmu_toolkit`.

### 15.1 `FmuEngine`

| Member | Description |
|---|---|
| `explicit FmuEngine(std::string_view xsdBasePath = {})` | Construct; optional schema base directory |
| `~FmuEngine()` | Unloads every remaining FMU |
| `FmuEngine(FmuEngine&&) noexcept`, `operator=(FmuEngine&&) noexcept` | Move; copy is deleted |
| `uint64_t load(std::string_view fmuPath)` | Load an FMU, return its handle |
| `FmuInfo info(uint64_t handle) const` | Metadata snapshot |
| `ValidationResult validate(uint64_t handle) const` | Three-phase validation |
| `SimResult simulate(uint64_t, const SimConfig&, std::function<void(size_t,double,size_t)> = nullptr)` | Full simulation |
| `FmuSession openSession(uint64_t handle, const SimConfig&)` | Initialised interactive session |
| `void unload(uint64_t handle)` | Release an FMU |
| `static void exportCsv(const SimResult&, std::string_view)` | CSV export |
| `static void exportJson(const SimResult&, std::string_view)` | JSON export |

### 15.2 `FmuSession`

See [section 10.2](#102-member-reference) and [section 11.2](#112-member-reference).

### 15.3 Enumerations

| Enumeration | Values |
|---|---|
| `LogLevel` | `None`, `Warning`, `Info`, `Debug`, `All` |
| `FmiVersion` | `FMI2 = 2`, `FMI3 = 3` |
| `Causality` | `Parameter`, `CalculatedParameter`, `Input`, `Output`, `Local`, `Independent` |
| `Variability` | `Constant`, `Fixed`, `Tunable`, `Discrete`, `Continuous` |
| `DataType` | `Real`, `Integer`, `Boolean`, `String`, `Float64`, `Float32`, `Int8`, `Int16`, `Int32`, `Int64`, `UInt8`, `UInt16`, `UInt32`, `UInt64`, `Binary`, `Clock` |
| `Initial` | `Exact`, `Approx`, `Calculated`, `None` |
| `ValidationCategory` | `Structure`, `Schema`, `Semantic` |
| `IntervalQualifier` | `NotYetKnown`, `Unchanged`, `Changed` |

### 15.4 Data structures

| Type | Purpose |
|---|---|
| `FmuInfo`, `VariableInfo`, `UnitInfo`, `TypeDefinitionInfo`, `DefaultExperiment` | Model metadata |
| `TerminalInfo`, `TerminalMemberInfo` | FMI 3.0 terminals |
| `ValidationResult`, `ValidationIssue` | Validation output |
| `SimConfig`, `SimResult` | Simulation input/output |
| `StepOutcome`, `DiscreteStatesOutcome`, `ClockInterval` | Event Mode results |
| `FmuArtifactGenerationOptions`, `GeneratedArtifactPaths` | Generation options/output |

---

## 16. Deployment and Runtime Notes

1. **Ship the DLLs.** Distribute `libfmutoolkit.dll` and `libwinpthread-1.dll` together with your executable.
   Windows has no rpath: the loader finds them only in the executable's folder or on `PATH`.
2. **No other native dependencies.** libxml2 and libzip are linked into the library; you do not need
   `libxml2.dll` or `libzip.dll`.
3. **Ship the XSD schemas** if your application calls `validate()` ([section 8.4](#84-locating-the-xsd-schemas)).
4. **FMU binaries must match the host.** An FMU containing only a Linux binary cannot be loaded on Windows (and
   vice versa); the load fails with `FmuLoadError`.
5. **Temporary files.** `load` extracts each FMU into a temporary directory under the system temp location.
   Ensure it is writable and has enough space for the unpacked archive.
6. **FMU code runs in your process.** An FMU binary is native code loaded into your application. Only load FMUs
   from sources you trust.
7. **Diagnostics.** FMU log messages at warning and error level are written to the standard error stream.

---

## 17. Troubleshooting

| Symptom | Likely cause | Remedy |
|---|---|---|
| Link error: undefined reference to `libfmu_toolkit::...` on Windows | `LIB_FMU_TOOLKIT_SHARED` not defined | Add `-DLIB_FMU_TOOLKIT_SHARED` (or link the CMake imported target) |
| Link error mentioning `std::__cxx11::basic_string` | ABI mismatch | Compile with `-D_GLIBCXX_USE_CXX11_ABI=1` and a compatible GCC |
| Program exits immediately, "DLL not found" (exit code `0xC0000135`) | `libfmutoolkit.dll` or `libwinpthread-1.dll` not beside the executable | Copy both DLLs next to the `.exe` |
| `FmuLoadError` mentioning the shared library | No binary for the host platform, wrong `modelIdentifier`, or a dependency of the FMU binary is missing | Check the `binaries/` folder inside the `.fmu`; inspect with `validate()` |
| `validate()` fails only with `XSD schema file not found` | Schemas not discoverable | Provide them ([section 8.4](#84-locating-the-xsd-schemas)) |
| `validate()` reports an FMI 2.0 FMU using `x86_64-windows` | FMI 2.0 requires legacy names (`win64`) | Rename the binary directory in the FMU |
| `std::out_of_range` from `getValue`/`setValue` | Variable name misspelled or absent | Check `info.findVariable(name)` |
| `std::out_of_range` from an engine call | Handle already unloaded or never issued | Do not reuse a handle after `unload` |
| `FmuSimulationError` on Event Mode calls | FMU is FMI 2.0, does not declare `hasEventMode`, or session not opened with `eventModeUsed = true` | See [section 11](#11-fmi-30-event-mode-binary-variables-and-clocks) |
| Integer parameter has no effect / compile ambiguity | Integer literal interpreted as `double`, or ambiguous overload | Use `int64_t{n}` |
| Output file empty or missing columns | `outputVariables` listed names that are not outputs | Names that do not match an output are ignored; check `info.outputs()` |
| Crash after `unload` | A session for that handle was still active | Terminate the session first |

---

## 18. Limitations and Known Constraints

- **Co-Simulation only.** Model Exchange and Scheduled Execution interfaces are not supported.
- **Fixed communication step.** `simulate` uses the constant `stepSize` from `SimConfig`; there is no adaptive
  step control.
- **Scalar-oriented value access.** Variables are exchanged as `double`, `int64_t`, `bool`, `std::string`
  (plus Binary and Clock through dedicated calls).
- **Not thread-safe.** One engine/session per thread, or external locking.
- **Generator scope.** Binary and Clock variables, terminals and FMI-LS-BUS manifests are not generated.
- **Platform binaries.** Only the host platform's binary inside the FMU is used.
- **Compiler compatibility.** Source/ABI compatibility is guaranteed only within the same toolchain family.

---

## 19. Glossary

| Term | Meaning |
|---|---|
| **FMI** | Functional Mock-up Interface, the open standard for exchanging and co-simulating dynamic models |
| **FMU** | Functional Mock-up Unit: a ZIP archive (`.fmu`) with `modelDescription.xml`, platform binaries and resources |
| **Co-Simulation** | FMU interface in which the FMU integrates its own equations and the master only advances time in communication steps |
| **Communication step** | The time interval passed to one `doStep` call |
| **Value reference** | Numeric identifier of a variable in FMI get/set calls |
| **Causality** | Role of a variable: parameter, input, output, local, … |
| **Variability** | How often a variable may change: constant, fixed, tunable, discrete, continuous |
| **Handle** | Opaque `uint64_t` identifying a loaded FMU inside an `FmuEngine` |
| **Session** | An `FmuSession`: an initialised FMU instance advanced step by step by the caller |
| **Event Mode** | FMI 3.0 mode in which discrete events and Clocks are processed |
| **Clock** | FMI 3.0 variable type representing a time-triggered or event-triggered tick |
| **Binary variable** | FMI 3.0 variable carrying an opaque byte buffer |
| **FMI-LS-BUS** | FMI layered standard for bus communication, built on Binary variables, Clocks and terminals |
| **XSD** | XML Schema Definition; used to validate `modelDescription.xml` |

---

## 20. Further Documentation

| Document | Content |
|---|---|
| `CHANGELOG.pdf` (in `documents/`) | Release history |
| `INFO.md` (in `documents/`) | Package information |
| Software Requirements Specification | Functional and non-functional requirements referenced by the `FR-xx` / `NFR-xx` tags |
| Architecture and ADRs | Internal design (hexagonal architecture, PIMPL public API, three-phase validation, ABI policy) |
| Example programs | Eight sources demonstrating the API: basic simulation, step-by-step session, validation, CSV/JSON export, two-FMU chaining, batch validation, generation from JSON (full and selective) |
| FMI standard | <https://fmi-standard.org> |
