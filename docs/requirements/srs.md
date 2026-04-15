# Software Requirements Specification — lib-fmu-toolkit

**Version:** 1.0  
**Date:** 2026-04-15  
**Status:** Baseline

---

## 1. Introduction

### 1.1 Purpose

This document specifies the functional and non-functional requirements for `lib-fmu-toolkit`, a
C++ library for loading, parsing, validating, and co-simulating Functional Mock-up Units (FMUs)
compliant with the FMI 2.0 and FMI 3.0 standards.

### 1.2 Scope

`lib-fmu-toolkit` provides:

- An API for programmatic FMU lifecycle management (load, validate, simulate, unload).
- A three-phase validation pipeline (structural, XSD schema, semantic).
- A co-simulation loop engine and a step-by-step session interface for external time
  coordinators.
- Export utilities for simulation results (CSV and JSON).
- A hexagonal architecture with dependency-injected ports for testability and extensibility.

The library does **not** provide a graphical interface, a co-simulation master protocol
(e.g. DCP, SSP, SIL-Kit), or FMU authoring/export functionality.

### 1.3 Definitions

| Term | Definition |
| --- | --- |
| FMU | Functional Mock-up Unit — a ZIP archive containing a model description XML and platform-specific shared libraries |
| FMI | Functional Mock-up Interface — the standard defining FMU structure and API |
| Handle | An opaque `uint64_t` identifier returned by `FmuEngine::load()` |
| Phase | One of the three independent validation stages: Structure, Schema, Semantics |
| Session | An active co-simulation context bound to one FMU handle |

---

## 2. Functional Requirements

### FR-01 — FMU Archive Loading

| ID | Requirement |
| --- | --- |
| FR-01.1 | The library shall accept a file system path to an `.fmu` file and extract its contents to a temporary directory. |
| FR-01.2 | The library shall parse `modelDescription.xml` from the extracted archive and expose all model metadata through `FmuInfo`. |
| FR-01.3 | The library shall load the platform-specific shared library (`binaries/<platform>/<modelIdentifier>.<ext>`) and resolve all required FMI function symbols. |
| FR-01.4 | The library shall support both FMI 2.0 and FMI 3.0 FMUs, auto-detected from the `fmiVersion` attribute of `modelDescription.xml`. |
| FR-01.5 | The library shall throw `FmuLoadError` if the archive cannot be opened, extracted, or if the shared library or its symbols cannot be resolved. |
| FR-01.6 | Multiple FMUs may be loaded concurrently in the same `FmuEngine` instance, each identified by its own handle. |

### FR-02 — FMU Metadata Inspection

| ID | Requirement |
| --- | --- |
| FR-02.1 | `FmuEngine::info(handle)` shall return a value-type `FmuInfo` struct containing model name, identifier, GUID, FMI version, and all model variables. |
| FR-02.2 | Each `VariableInfo` entry shall expose: name, description, value reference, causality, variability, data type, unit, declared type name, and start value string. |
| FR-02.3 | `FmuInfo` shall expose convenience filters: `inputs()`, `outputs()`, `parameters()`, and `findVariable(name)`. |
| FR-02.4 | `FmuInfo` shall include `UnitInfo` records from `<UnitDefinitions>` and `TypeDefinitionInfo` records from `<TypeDefinitions>` / `<SimpleType>`. |
| FR-02.5 | `FmuInfo` shall include `DefaultExperiment` values with `has*` flags indicating which attributes were present in the XML. |

### FR-03 — Three-Phase FMU Validation

| ID | Requirement |
| --- | --- |
| FR-03.1 | `FmuEngine::validate(handle)` shall run all three phases unconditionally and return a single `ValidationResult`. |
| FR-03.2 | **Phase 1 — Structure:** the validator shall verify that `modelDescription.xml` is present, that a `binaries/` directory exists, and that at least one platform sub-directory follows the naming convention for the declared FMI version. |
| FR-03.3 | **Phase 1 — Binary naming convention:** FMI 2.0 FMUs shall use legacy platform directory names (`win32`, `win64`, `linux32`, `linux64`, `darwin32`, `darwin64`). FMI 3.0 FMUs shall use arch-OS format (`x86_64-windows`, `x86_64-linux`, `x86_64-darwin`, `aarch64-linux`, etc.). A version mismatch shall be reported as a Structure error. |
| FR-03.4 | **Phase 1 — Library identifier:** the shared library filename stem shall match the `modelIdentifier` attribute. A mismatch shall be reported as a Structure warning. |
| FR-03.5 | **Phase 2 — Schema:** `modelDescription.xml` shall be validated against the official FMI XSD schema (FMI 2.0 or 3.0 as appropriate). All XSD errors shall include line and column numbers. |
| FR-03.6 | **Phase 3 — Semantics:** the validator shall check causality/variability compatibility rules and the uniqueness of `valueReference` values within each data type. |
| FR-03.7 | `validate()` shall never throw for model-level errors — all findings shall be returned in `ValidationResult.errors` and `ValidationResult.warnings`. |
| FR-03.8 | `ValidationResult.passed` shall be `true` if and only if `errors` is empty. |

### FR-04 — Automated Co-Simulation

| ID | Requirement |
| --- | --- |
| FR-04.1 | `FmuEngine::simulate(handle, config)` shall drive the FMU through the complete co-simulation lifecycle: instantiate → setup experiment → enter/exit initialization mode → step loop → terminate → free instance. |
| FR-04.2 | The simulation step loop shall advance from `config.startTime` to `config.stopTime` with step size `config.stepSize`. |
| FR-04.3 | Initial parameter values specified in `config.initialParameters` shall be applied before `ExitInitializationMode`. |
| FR-04.4 | Output variable values shall be recorded at every `config.recordingInterval`-th communication step. When `config.outputVariables` is non-empty, only variables in that list shall be recorded. |
| FR-04.5 | An optional `progress` callback shall be invoked after each recorded step, receiving the record index, current time, and total step count. |
| FR-04.6 | `simulate()` shall return a `SimResult` containing `timePoints` and per-variable time series. |

### FR-05 — Step-by-Step Session

| ID | Requirement |
| --- | --- |
| FR-05.1 | `FmuEngine::openSession(handle, config)` shall instantiate and initialize the FMU and return a live `FmuSession`. |
| FR-05.2 | `FmuSession::doStep(currentTime, stepSize)` shall advance the FMU by one communication step and return `true` on success, `false` on early termination. |
| FR-05.3 | `FmuSession::setValue(name, value)` shall write a value to a named input or parameter variable before or after a step. Supported types: `double`, `int64_t`, `bool`, `std::string_view`. |
| FR-05.4 | `FmuSession::getValue(name)` shall read the current value of any readable variable and return it as `OutputValue`. |
| FR-05.5 | `FmuSession::terminate()` shall call `fmi*Terminate` and `fmi*FreeInstance`. It shall be safe to call multiple times (idempotent). |
| FR-05.6 | The `FmuSession` destructor shall call `terminate()` automatically if the session is still active. |

### FR-06 — Result Export

| ID | Requirement |
| --- | --- |
| FR-06.1 | `FmuEngine::exportCsv(result, path)` shall write a `SimResult` to a CSV file. The first row shall be a header with `time` followed by variable names. Boolean values shall be written as `0`/`1`. String values shall be RFC-4180 quoted. |
| FR-06.2 | `FmuEngine::exportJson(result, path)` shall write a `SimResult` to a JSON file containing `"timePoints"` array and `"series"` object. Boolean values shall be JSON `true`/`false`. No external JSON library shall be required. |
| FR-06.3 | Both export methods shall throw `std::ios_base::failure` if the output file cannot be opened or written. |

### FR-07 — XSD Schema Discovery

| ID | Requirement |
| --- | --- |
| FR-07.1 | When `FmuEngine` is constructed without an explicit `xsdBasePath`, the library shall auto-discover the XSD schema trees in this priority order: (1) `LIB_FMU_TOOLKIT_XSD_BASE` environment variable; (2) compile-time source `external/` path; (3) `<module_dir>/../share/lib-fmu-toolkit/schemas`; (4) `${cwd}/external`. |
| FR-07.2 | The library shall require both `fmi-standard-2.0/schema/fmi2ModelDescription.xsd` and `fmi-standard-3.0/schema/fmi3ModelDescription.xsd` to be present under the resolved base path. |

---

## 3. Non-Functional Requirements

### NFR-01 — Language Standard and Toolchain Compatibility

| ID | Requirement |
| --- | --- |
| NFR-01.1 | The library shall be written in C++20. |
| NFR-01.2 | Supported compilers: GCC ≥ 11, Clang ≥ 13. MSVC is explicitly not supported (see ADR-0003). |
| NFR-01.3 | On Windows, only MSYS2/MinGW toolchains shall be supported (`mingw64`, `clang64` environments). |
| NFR-01.4 | On Linux, native GCC and Clang system installations shall be supported. |
| NFR-01.5 | The library shall compile without warnings under `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow`. |

### NFR-02 — ABI and API Stability

| ID | Requirement |
| --- | --- |
| NFR-02.1 | Public headers shall expose no libxml2, libzip, or internal domain types. |
| NFR-02.2 | The `FmuEngine` and `FmuSession` classes shall use the PIMPL idiom. Adding fields to `Impl` shall not require consumer recompilation. |
| NFR-02.3 | All input-only string parameters in the public API shall accept `std::string_view` to avoid STL layout dependency at call sites. |
| NFR-02.4 | Methods that access STL container internals (e.g., `hasErrors()`, `stepCount()`) shall be implemented out-of-line in the library, not as inline definitions in public headers. |
| NFR-02.5 | The shared library shall be versioned with `VERSION` and `SOVERSION` matching the project semantic version. |
| NFR-02.6 | The compile definition `_GLIBCXX_USE_CXX11_ABI=1` shall be set as a PUBLIC CMake definition on all non-MSVC targets to prevent silent ABI mismatches with legacy GCC4 consumers. |
| NFR-02.7 | `CompatibilityVersion` for the CMake package shall be `SameMajorVersion`. |

### NFR-03 — Architecture and Design

| ID | Requirement |
| --- | --- |
| NFR-03.1 | The library shall follow hexagonal architecture (Ports & Adapters). The domain and use-case layers shall have zero compile-time dependencies on libxml2 or libzip. |
| NFR-03.2 | All use-case classes (`FmuValidator`, `FmuImporter`, `SimulationController`) shall receive their dependencies via constructor injection using port interfaces (`IXmlValidator`, `IFmuLoader`, `ISimulationBackend`). |
| NFR-03.3 | Each class shall have a single responsibility. The three validation phases shall be isolated in private methods (`checkStructure`, `checkSchema`, `checkSemantics`). |
| NFR-03.4 | The FMU handle system shall use opaque `uint64_t` IDs. No raw pointers to internal state shall be exposed in the public API. |

### NFR-04 — Testability

| ID | Requirement |
| --- | --- |
| NFR-04.1 | The `FmuValidator` shall be unit-testable by injecting stub `IXmlValidator` implementations without requiring real XSD schema files or real FMU archives. |
| NFR-04.2 | Unit tests shall test each validation phase in isolation via stub injection (`StubXmlValidatorPass`, `StubXmlValidatorFail`). |
| NFR-04.3 | Integration tests shall exercise the full load → validate → simulate pipeline against real FMU binaries (`tests/data/Generatore.fmu`, `tests/data/Sommatore.fmu`). |
| NFR-04.4 | The test suite shall use GoogleTest and be runnable via `ctest --preset <preset>`. |

### NFR-05 — Build System

| ID | Requirement |
| --- | --- |
| NFR-05.1 | The build system shall use CMake ≥ 3.25 with preset support (`CMakePresets.json`). |
| NFR-05.2 | The library shall build both shared (`SHARED`) and static (`STATIC`) targets. |
| NFR-05.3 | Dependencies (`libxml2`, `libzip`) shall be resolvable via either vcpkg or system package managers. |
| NFR-05.4 | When `LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME=ON`, the MinGW C++ runtime shall be linked statically, producing self-contained binaries. |
| NFR-05.5 | All presets shall set `CMAKE_EXPORT_COMPILE_COMMANDS=ON` for clang tooling integration. |
| NFR-05.6 | AddressSanitizer and UBSan shall be enabled for debug presets on Linux and for `dev-debug` presets. |

### NFR-06 — Performance

| ID | Requirement |
| --- | --- |
| NFR-06.1 | `FmuEngine::load()` shall complete ZIP extraction and XML parsing in linear time proportional to the archive size and XML document size respectively. |
| NFR-06.2 | The co-simulation step loop overhead (time spent in the library between FMI `doStep` calls) shall be negligible relative to the FMU computation time for step sizes ≥ 1 ms. |
| NFR-06.3 | Variable lookups in `FmuSession::setValue()` and `FmuSession::getValue()` shall complete in O(log N) time where N is the number of model variables (implemented via `std::map` with transparent comparator). |

### NFR-07 — Portability

| ID | Requirement |
| --- | --- |
| NFR-07.1 | The library shall build and pass all tests on Windows 10+ (MSYS2/MinGW) and Linux (Ubuntu 22.04+, Arch Linux). |
| NFR-07.2 | The library shall support FMUs with platform binaries for: `win64`, `linux64` (FMI 2.0) and `x86_64-windows`, `x86_64-linux` (FMI 3.0). |
| NFR-07.3 | No hardcoded absolute paths shall appear in installed CMake package files. All paths shall be relative to the install prefix using `$<INSTALL_INTERFACE:...>` generator expressions. |

### NFR-08 — Dependency Constraints

| ID | Requirement |
| --- | --- |
| NFR-08.1 | libxml2 and libzip shall be the only third-party runtime dependencies of the library binary. |
| NFR-08.2 | GoogleTest shall be the only test-time dependency and shall be auto-fetched if not provided by vcpkg. |
| NFR-08.3 | The public headers (`include/libfmu_toolkit/`) shall include only C++ standard library headers and `export.hpp`. No third-party headers shall be transitively included by the public API. |

---

## 4. Constraints

| ID | Constraint |
| --- | --- |
| C-01 | MSVC is not supported (ADR-0003). All code must compile under GCC and Clang only. |
| C-02 | The library does not support FMU model exchange mode — only co-simulation mode is implemented. |
| C-03 | The library does not implement a co-simulation master protocol. External time coordination (e.g. SIL-Kit) must use `FmuSession` directly. |
| C-04 | FMU binaries with incorrect `modelIdentifier` in their shared library filename will pass loading but receive a validation warning, not an error. |
| C-05 | The library does not support FMU packaging (creating `.fmu` archives). It is a consumer-side toolkit only. |
