# Requirements Traceability Matrix — lib-fmu-toolkit

**Version:** 1.0
**Date:** 2026-05-31
**Status:** Baseline
**Companion document:** [srs.md](srs.md)

---

## 1. Purpose

This document provides bidirectional traceability between the requirements
defined in the [Software Requirements Specification](srs.md), the source units
that implement them, and the automated tests that verify them. It is intended as
verification evidence for integrating `lib-fmu-toolkit` into an ISO 26262
software development process.

Every functional requirement (FR) is traced to:

- **Implementation** — the source file(s) that realise the requirement. In
  addition to this file-level table, **every class, struct, enum, function and
  method** in `include/` and `src/` carries a `@req <ID>` Doxygen tag naming the
  requirement(s) it satisfies, giving fine-grained, in-code traceability that
  travels with the source.
- **Verification** — the automated test(s) that exercise the requirement, named
  by their GoogleTest `Suite.Case` identifier (identical to the `ctest` test
  name).

### In-code `@req` tags and the generated traceability page

The `@req` tag is defined as a Doxygen alias in [`../Doxyfile`](../Doxyfile):

```
ALIASES = "req=@xrefitem req_trace \"Requirements\" \"Requirements Traceability\""
```

Because it is an `@xrefitem`, running `doxygen docs/Doxyfile` produces a
**"Requirements Traceability"** page that lists, for every requirement ID, the
exact set of code entities tagged with it — the automatically generated
*code → requirement* counterpart of the *requirement → code* tables below. The
tag also prints inline (e.g. "Requirements: FR-01.3") on each entity's page.

> Coverage of the `@req` tags is enforceable in CI: every header-declared
> `class`/`struct`/`enum` already carries a `@req` tag (verified at the time of
> writing). New public or internal types/methods are expected to add one.

Non-functional requirements (NFR) and constraints (C) are traced to their
implementation and to their **verification method** (Test / Build / Review /
Inspection / ADR), since several are architectural properties not expressible as
a single runtime test.

All tests listed are part of the suite executed by `ctest` (currently **107
tests, 100 % passing** on the `windows-mingw64-debug` preset).

### Verification method legend

| Method | Meaning |
| --- | --- |
| Test | Verified by one or more automated GoogleTest cases. |
| Build | Verified by successful compilation/link under the strict warning set. |
| Review | Verified by code review against the stated property. |
| Inspection | Verified by inspecting configuration files (CMake, presets, ADR). |

---

## 2. Functional Requirements → Code → Test

### FR-01 — FMU Archive Loading

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-01.1 | `adapters/zip/zip_loader_adapter.cpp` (`extractFmu`) | `GeneratoreTest.LoadSucceeds`, `SommatoreTest.LoadSucceeds` |
| FR-01.2 | `adapters/xml/model_description_parser.cpp`; `core/usecases/fmu_importer.cpp` | `Fmi2ParserTest.ParsesModelAttributes`, `Fmi3ParserTest.DetectsFmi3Version` |
| FR-01.3 | `adapters/zip/zip_loader_adapter.cpp` (`findSharedLibrary`, `loadFmi2/3FunctionPointers`) | `GeneratoreTest.LoadSucceeds`, `FmuEngineIntegrationTest.LoadValidateAndSimulateReferenceFmu` |
| FR-01.4 | `adapters/xml/model_description_parser.cpp` (`parse` version dispatch) | `Fmi2ParserTest.DetectsFmi2Version`, `Fmi3ParserTest.DetectsFmi3Version` |
| FR-01.5 | `core/usecases/fmu_importer.cpp`; `fmu_engine.cpp` (exception mapping) | `FmuEngineTest.LoadNonExistentFmuThrowsFmuLoadError`, `FmuEngineTest.LoadWrongExtensionThrowsFmuLoadError`, `ParserErrorTest.ThrowsOnMissingFmiVersion`, `ParserErrorTest.ThrowsOnUnsupportedFmiVersion` |
| FR-01.6 | `fmu_engine_impl.hpp` (`FmuEngine::Impl::entries` handle map) | `CrossFmuTest.BothFmusLoadedInSameEngine` |

### FR-02 — FMU Metadata Inspection

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-02.1 | `fmu_engine.cpp` (`info`); `fmu_engine_impl.hpp` (`detail::toPublicInfo`) | `GeneratoreTest.InfoReportsFMI3`, `GeneratoreTest.InfoModelName`, `GeneratoreTest.InfoModelIdentifier` |
| FR-02.2 | `core/domain/model_variable.hpp`; `adapters/xml/model_description_parser.cpp` | `Fmi2ParserTest.ParsesRealOutputVariable`, `Fmi2ParserTest.ParsesInputVariable`, `Fmi2ParserTest.ParsesParameterVariable`, `Fmi3ParserTest.ParsesFloat64Variable` |
| FR-02.3 | `core/domain/fmu_package.cpp` (`inputs`/`outputs`/`parameters`/`findVariable`); `types.hpp` (public mirror) | `Fmi2ParserTest.InputsOutputsParametersFilters`, `Fmi2ParserTest.FindVariableReturnsNullptrForUnknownName`, `SommatoreTest.InfoHasTwoInputs` |
| FR-02.4 | `adapters/xml/model_description_parser.cpp` (`parseUnitDefinitions`, `parseTypeDefinitionsFmi2`) | `Fmi2ParserTest.ParsesUnitDefinitions`, `Fmi2ParserTest.ParsesTypeDefinitions`, `Fmi3ParserTest.ParsesUnitDefinitions` |
| FR-02.5 | `adapters/xml/model_description_parser.cpp` (`parseDefaultExperiment`) | `Fmi2ParserTest.ParsesDefaultExperiment`, `Fmi3ParserTest.ParsesDefaultExperiment`, `ParserEdgeTest.ModelWithNoDefaultExperimentHasNoFlags`, `GeneratoreTest.DefaultExperimentParsed` |

### FR-03 — Three-Phase FMU Validation

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-03.1 | `core/usecases/fmu_validator.cpp` (`validate`) | `GeneratoreTest.ValidationPasses`, `SommatoreTest.ValidationPasses` |
| FR-03.2 | `core/usecases/fmu_validator.cpp` (`checkStructure`) | `ValidatorStructureTest.MissingExtractedPathReportsError` |
| FR-03.3 | `core/usecases/fmu_validator.cpp` (`checkBinaryNamingConvention`) | `ValidatorBinaryDirTest.Fmi2FmuWithFmi3StyleDirReportsError`, `ValidatorBinaryDirTest.Fmi3FmuWithFmi2StyleDirReportsError`, `ValidatorBinaryDirTest.Fmi2FmuWithCorrectDirProducesNoNamingError`, `ValidatorBinaryDirTest.Fmi3FmuWithCorrectDirProducesNoNamingError` |
| FR-03.4 | `core/usecases/fmu_validator.cpp` (`checkLibraryIdentifier`) | Covered by `ValidatorStructureTest` fixtures (warning path); see §4 note |
| FR-03.5 | `adapters/xml/libxml2_validator_adapter.cpp`; `fmu_validator.cpp` (`checkSchema`) | `ValidatorSchemaTest.PassingXmlValidatorProducesNoSchemaErrors`, `ValidatorSchemaTest.EmptyRawDescriptionReportsSchemaError`, `ValidatorStructureTest.MissingSchemaReportsSchemaError` |
| FR-03.6 | `core/usecases/fmu_validator.cpp` (`checkSemantics`) | `ValidatorSemanticTest.DuplicateValueReferenceForSameTypeReportsError`, `ValidatorSemanticTest.InputConstantIsInvalid`, `ValidatorSemanticTest.OutputFixedIsInvalid`, `ValidatorSemanticTest.ParameterContinuousIsInvalid`, `ValidatorSemanticTest.NoOutputVariablesAddsWarning` |
| FR-03.7 | `core/usecases/fmu_validator.cpp`; `core/domain/validation_report.cpp` | `ValidatorSemanticTest.ValidModelPassesSemanticCheck` (no throw, findings in report) |
| FR-03.8 | `core/domain/validation_report.{hpp,cpp}` (`passed`/`hasErrors`) | `ValidatorSemanticTest.ValidModelPassesSemanticCheck`, `ValidatorSemanticTest.InputConstantIsInvalid` |

### FR-04 — Automated Co-Simulation

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-04.1 | `core/usecases/simulation_controller.cpp`; `adapters/simulation/standalone_simulation_adapter.cpp` | `SimulationControllerTest.CallsInitializeAndTerminate`, `GeneratoreTest.SimulateRamp_OutputMatchesExpected` |
| FR-04.2 | `core/usecases/simulation_controller.cpp` (`run` step loop) | `SimulationControllerTest.ProducesCorrectStepCount`, `SimulationControllerTest.TimePointsStartAtStartTime` |
| FR-04.3 | `adapters/simulation/standalone_simulation_adapter.cpp` (`initFmi2/3` parameter injection) | `SommatoreTest.SimulateWithInitialParams_OutputIsSum` |
| FR-04.4 | `core/usecases/simulation_controller.cpp` (`recordStep`, recording interval, output filter) | `SimulationControllerTest.RecordsOutputVariable`, `SimulationControllerTest.RecordingIntervalReducesDataPoints`, `SimulationControllerTest.SpecificOutputVariablesOnlyRecordRequested`, `SommatoreTest.SimulateRecordingInterval` |
| FR-04.5 | `core/usecases/simulation_controller.cpp` (progress callback) | `SimulationControllerTest.PropagatesProgressCallback` |
| FR-04.6 | `core/usecases/simulation_controller.cpp`; `fmu_engine_impl.hpp` (`detail::toPublicResult`) | `GeneratoreTest.SimulateRecordedStepCount`, `SommatoreTest.SimulateWithZeroInputs_OutputIsZero` |

### FR-05 — Step-by-Step Session

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-05.1 | `fmu_engine.cpp` (`openSession`); `fmu_engine_impl.hpp` (`FmuSession::Impl`) | `GeneratoreTest.FmuSessionStepByStep` |
| FR-05.2 | `fmu_engine.cpp` (`FmuSession::doStep`); `standalone_simulation_adapter.cpp` (`doStep`) | `GeneratoreTest.FmuSessionStepByStep` |
| FR-05.3 | `fmu_engine.cpp` (`FmuSession::setValue` overloads) | `SommatoreTest.SessionSetValueUpdatesOutput`, `SommatoreTest.SessionNegativeInputs` |
| FR-05.4 | `fmu_engine.cpp` (`FmuSession::getValue`) | `GeneratoreTest.FmuSessionStepByStep`, `SommatoreTest.SessionSetValueUpdatesOutput` |
| FR-05.5 | `fmu_engine.cpp` (`FmuSession::terminate`); `standalone_simulation_adapter.cpp` (`terminate`) | `GeneratoreTest.FmuSessionStepByStep` |
| FR-05.6 | `fmu_engine.cpp` (`FmuSession::~FmuSession`) | `SommatoreTest.SessionSetValueUpdatesOutput` (RAII path); see §4 note |

### FR-06 — Result Export

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-06.1 | `core/usecases/csv_result_exporter.cpp`; `fmu_engine.cpp` (`exportCsv`) | `CsvExporterTest.HeaderContainsTimeAndVariableNames`, `CsvExporterTest.CorrectNumberOfDataRows`, `CsvExporterTest.BooleanValuesWrittenAs0And1`, `CsvExporterTest.StringValuesAreQuoted`, `GeneratoreTest.ExportCsvContainsOutputColumn` |
| FR-06.2 | `adapters/json/json_result_exporter.cpp`; `fmu_engine.cpp` (`exportJson`) | `JsonExporterTest.ContainsAllVariableNames`, `JsonExporterTest.TimePointsArrayHasCorrectCount`, `JsonExporterTest.BooleanValuesWrittenAsTrueOrFalse`, `JsonExporterTest.StringValuesAreJsonEscaped`, `GeneratoreTest.ExportJsonContainsTimeSeries` |
| FR-06.3 | `core/usecases/csv_result_exporter.cpp`; `adapters/json/json_result_exporter.cpp` | `CsvExporterTest.ThrowsOnUnwritablePath`, `JsonExporterTest.ThrowsOnUnwritablePath` |

### FR-07 — XSD Schema Discovery

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-07.1 | `fmu_engine_impl.hpp` (`detail::resolveDefaultSchemaBasePath`, `pathFromEnvironment`, `moduleDirectory`) | Verified indirectly: `GeneratoreTest.ValidationPasses` / `SommatoreTest.ValidationPasses` require successful auto-discovery of the schema trees; see §4 note |
| FR-07.2 | `fmu_engine_impl.hpp` (`detail::schemaTreeExists`) | Verified indirectly via the validation integration tests above |

### FR-08 — JSON-Driven FMU Artifact Generation

| Req | Implementation | Verifying tests |
| --- | --- | --- |
| FR-08.1 | `core/generation/fmu_model_description.cpp` (`JsonModelDescriptionParser::parseFile`) | `FmuGenerationTest.GeneratesValidatedFmi2ModelDescriptionFromJson` |
| FR-08.2 | `core/generation/generated_model_description_builder.cpp`; `fmu_generation.cpp` (`ModelDescriptionGenerator`) | `FmuGenerationTest.GeneratesValidatedFmi2ModelDescriptionFromJson` |
| FR-08.3 | `core/generation/model_description_semantic_validator.cpp`; `fmu_generation.cpp` | `FmuGenerationTest.GeneratesValidatedFmi2ModelDescriptionFromJson` |
| FR-08.4 | `core/generation/generated_code_builder.cpp`; `fmu_generation.cpp` (`FmiCCodeGenerator`) | `FmuGenerationTest.GeneratesCGetSetHelpersFromJson` |
| FR-08.5 | `core/generation/generated_model_description_builder.cpp` (array handling) | `FmuGenerationTest.GeneratesFmi3ArraysWithoutScalarExpansion` |
| FR-08.6 | `fmu_generation.cpp` (`FmuArtifactGenerator`) | `FmuGenerationTest.FacadeCanGenerateBothArtifactsToDisk` |

---

## 3. Non-Functional Requirements & Constraints → Code → Verification

| Req | Implementation / Evidence | Method |
| --- | --- | --- |
| NFR-01.1 | `CMakeLists.txt` (`CMAKE_CXX_STANDARD 20`) | Build / Inspection |
| NFR-01.2 | `CMakePresets.json` (MinGW presets only) | Inspection / ADR-0003 |
| NFR-01.3 | `CMakePresets.json` (`windows-mingw32/64-*`) | Inspection |
| NFR-01.5 | `cmake/CompilerWarnings.cmake` (`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow …`) | Build (clean, zero warnings) |
| NFR-02.1 | `include/libfmu_toolkit/*.hpp` (no third-party includes) | Review |
| NFR-02.2 | `fmu_engine.hpp` / `fmu_engine_impl.hpp` (PIMPL) | Review / ADR-0002 |
| NFR-02.3 | Public API `std::string_view` input params | Review |
| NFR-02.4 | `fmu_engine.cpp` (out-of-line `hasErrors`/`stepCount`/`empty`) | Review / ADR-0005 |
| NFR-02.5 | `CMakeLists.txt` (`VERSION`/`SOVERSION`) | Inspection |
| NFR-02.6 | `CMakeLists.txt` (`_GLIBCXX_USE_CXX11_ABI=1` PUBLIC) | Inspection / ADR-0005 |
| NFR-03.1 | `src/core/**` has no libxml2/libzip includes (only `src/adapters/**` do) | Review / ADR-0001 |
| NFR-03.2 | Constructor injection of `IXmlValidator`/`IFmuLoader`/`ISimulationBackend` | Review + `FmuValidator`/`SimulationController` tests (stub injection) |
| NFR-03.3 | `fmu_validator.cpp` (`checkStructure`/`checkSchema`/`checkSemantics`) | Review |
| NFR-03.4 | `fmu_engine_impl.hpp` (opaque `uint64_t` handle map) | Review |
| NFR-04.1 | `tests/unit/test_FmuValidator.cpp` (`StubXmlValidator*`) | Test |
| NFR-04.2 | `tests/unit/test_FmuValidator.cpp` phase fixtures | Test |
| NFR-04.3 | `tests/integration/test_fmi3_fmus.cpp` | Test |
| NFR-04.4 | `tests/CMakeLists.txt` (GoogleTest + `ctest`) | Build / Test |
| NFR-05.* | `CMakeLists.txt`, `CMakePresets.json`, `cmake/*.cmake` | Inspection / Build |
| NFR-06.3 | `fmu_engine_impl.hpp` (`std::map` transparent comparator for variable index) | Review |
| NFR-07.2 | `adapters/xml/model_description_parser.cpp` platform handling | Test (integration) |
| NFR-08.1 | `CMakeLists.txt` link libraries (libxml2, libzip only) | Inspection |
| NFR-08.2 | `tests/CMakeLists.txt` (FetchContent GoogleTest) | Inspection |
| NFR-08.3 | `include/libfmu_toolkit/*` includes only STL + `export.hpp` | Review |
| C-01 | ADR-0003; MinGW-only presets | Inspection |
| C-02..C-05 | SRS constraints; generation scope per FR-08 | Review |

---

## 4. Verification Notes & Known Coverage Gaps

The following requirements are **not** covered by a dedicated, isolated test and
rely on indirect or review-based verification. They are listed explicitly so the
coverage picture is transparent for a safety assessment.

- **FR-03.4 (library identifier warning):** exercised through the structural
  validation fixtures but lacks a dedicated assertion on the warning text. A
  dedicated test (`ValidatorStructureTest.LibraryIdentifierMismatchWarns`) is a
  recommended addition.
- **FR-05.6 (session destructor auto-terminate):** the RAII termination path runs
  whenever a session fixture goes out of scope, but no test asserts idempotency of
  destructor-driven termination explicitly. A dedicated test is recommended.
- **FR-07.1 / FR-07.2 (schema discovery):** verified indirectly — every
  integration validation test depends on successful schema auto-discovery — but
  the individual discovery priority steps (env var, module dir, cwd) are not
  unit-tested in isolation. Dedicated tests are recommended for full ASIL
  evidence.

These gaps do not affect the 100 % pass rate of the existing suite; they identify
where additional tests would strengthen the verification argument.

---

## 5. Reverse Index — Source Unit → Requirements

| Source unit | Requirements |
| --- | --- |
| `core/usecases/fmu_importer.{hpp,cpp}` | FR-01 |
| `adapters/zip/zip_loader_adapter.{hpp,cpp}` | FR-01 |
| `adapters/xml/model_description_parser.{hpp,cpp}` | FR-01, FR-02 |
| `core/domain/fmu_package.{hpp,cpp}`, `model_variable.hpp` | FR-02 |
| `core/usecases/fmu_validator.{hpp,cpp}` | FR-03 |
| `adapters/xml/libxml2_validator_adapter.{hpp,cpp}` | FR-03.5 |
| `core/domain/validation_report.{hpp,cpp}` | FR-03.7, FR-03.8 |
| `core/usecases/simulation_controller.{hpp,cpp}` | FR-04 |
| `adapters/simulation/standalone_simulation_adapter.{hpp,cpp}` | FR-04, FR-05 |
| `fmu_engine.cpp`, `fmu_engine_impl.hpp` | FR-01..FR-07 (facade), FR-07 (schema discovery) |
| `core/usecases/csv_result_exporter.{hpp,cpp}` | FR-06.1, FR-06.3 |
| `adapters/json/json_result_exporter.{hpp,cpp}` | FR-06.2, FR-06.3 |
| `core/generation/*`, `fmu_generation.cpp` | FR-08 |
