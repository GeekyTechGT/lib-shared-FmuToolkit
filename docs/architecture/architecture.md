# Architecture Overview

## Design Philosophy

`lib-fmu-toolkit` is built on three foundational principles:

- **Hexagonal Architecture (Ports & Adapters)** — the domain and application layers are
  completely decoupled from infrastructure concerns (ZIP extraction, XML parsing, DLL loading).
- **PIMPL (Pointer to Implementation)** — the public API headers expose no internal types,
  ensuring ABI stability and zero include-path pollution for consumers.
- **Dependency Injection** — every use-case class receives its dependencies through interfaces,
  enabling mock injection for unit testing and alternative adapter substitution.

---

## Layer Map

```
┌────────────────────────────────────────────────────────────────────────────┐
│  PUBLIC API — include/libfmu_toolkit/                                      │
│                                                                            │
│  FmuEngine (PIMPL)     FmuSession (PIMPL)                                 │
│  types.hpp             exceptions.hpp       export.hpp   version.hpp      │
└───────────────────────────────────┬────────────────────────────────────────┘
                                    │  (internal — never installed)
┌───────────────────────────────────▼────────────────────────────────────────┐
│  USE CASES — src/core/usecases/                                            │
│                                                                            │
│  FmuImporter           FmuValidator         SimulationController           │
│  CsvResultExporter                          (uses ISimulationBackend)     │
│                        (uses IXmlValidator)                                │
└───────────────────┬───────────────────────────────────────────────────────┘
                    │  depends on ports (never on adapters)
┌───────────────────▼───────────────────────────────────────────────────────┐
│  DOMAIN — src/core/domain/                                                 │
│                                                                            │
│  FmuPackage   ModelVariable   ValidationReport   SimulationConfig         │
│  SimulationResult   UnitDef   TypeDef   DomainDefaultExperiment           │
└────────────────────────────────────────────────────────────────────────────┘
         ▲                ▲                ▲
┌────────┴────┐  ┌────────┴──────┐  ┌─────┴──────────────────────────────┐
│  PORTS      │  │  PORTS        │  │  PORTS                             │
│  (abstract) │  │  (abstract)   │  │  (abstract)                       │
│             │  │               │  │                                    │
│ IFmuLoader  │  │ IXmlValidator │  │ ISimulationBackend                 │
│             │  │               │  │ IResultExporter                    │
│             │  │               │  │ ILogger                            │
└────────┬────┘  └────────┬──────┘  └─────┬──────────────────────────────┘
         │                │                │  (adapters implement ports)
┌────────▼────┐  ┌────────▼──────┐  ┌─────▼──────────────────────────────┐
│  ADAPTERS   │  │  ADAPTERS     │  │  ADAPTERS                          │
│             │  │               │  │                                    │
│ ZipLoader-  │  │ Libxml2Vali-  │  │ StandaloneSimulationAdapter        │
│ Adapter     │  │ datorAdapter  │  │ JsonResultExporter                 │
│             │  │               │  │ CsvResultExporter                  │
│             │  │ ModelDesc-    │  │                                    │
│             │  │ riptionParser │  │                                    │
└─────────────┘  └───────────────┘  └────────────────────────────────────┘
```

---

## Layer Descriptions

### Public API (`include/libfmu_toolkit/`)

The public surface is a **thin PIMPL façade**. Every public class (`FmuEngine`, `FmuSession`)
holds only a forward-declared `struct Impl*`. The actual implementation is defined in the private
header `src/fmu_engine_impl.hpp`, which is never installed.

**Files:**

| File | Purpose |
| --- | --- |
| `fmu_engine.hpp` | `FmuEngine` and `FmuSession` declarations |
| `types.hpp` | All public data types (`FmuInfo`, `SimConfig`, `SimResult`, `ValidationResult`, enums) |
| `exceptions.hpp` | `FmuLoadError`, `FmuValidationError`, `FmuSimulationError` |
| `export.hpp` | `LIB_FMU_TOOLKIT_API` DLL-export macro |
| `lib_fmu_toolkit.hpp` | Umbrella include |
| `version.hpp.in` | Version constants (configured by CMake) |

**Design rule:** No libxml2, libzip, or internal domain types may appear in any of these headers.

---

### Domain (`src/core/domain/`)

Pure C++ structs representing the **FMI problem domain**. No third-party dependencies; no
virtual methods; no heap allocations beyond standard containers.

| Type | Role |
| --- | --- |
| `FmuPackage` | Aggregates everything extracted from one FMU archive: parsed metadata, variable list, extracted path, library handle, FMI function pointers. Non-copyable, movable. |
| `ModelVariable` | Single variable: name, valueReference, causality, variability, dataType, start value, unit, declaredType. |
| `ValidationReport` | Three lists (passed flag, `ValidationError[]`, `ValidationWarning[]`). Categorised by `ValidationCategory` (Structure / Schema / Semantic). |
| `SimulationConfig` | Simulation parameters: time range, step size, tolerance, initial parameters map, output filter, recording interval. |
| `SimulationResult` | Time series output: `vector<double> timePoints`, `map<string, vector<OutputValue>> series`. |
| `UnitDef` | Physical unit: name, SI conversion factor, offset. |
| `TypeDef` | Named type: name, dataType, unit, quantity, displayUnit, min/max strings. |
| `DomainDefaultExperiment` | Recommended simulation parameters from XML. |

---

### Use Cases (`src/core/usecases/`)

**Application-layer orchestration.** Each use-case class:
- Receives its dependencies via constructor injection (Dependency Inversion Principle).
- Is non-copyable and non-movable (owns injected references).
- Has a single, narrow public method.

| Class | Dependency | Public method |
| --- | --- | --- |
| `FmuImporter` | `IFmuLoader&` | `import(path)` → `FmuPackage` |
| `FmuValidator` | `IXmlValidator&`, two schema paths | `validate(pkg)` → `ValidationReport` |
| `SimulationController` | `ISimulationBackend&` | `run(pkg, config, progress)` → `SimulationResult` |
| `CsvResultExporter` | — | `exportResult(result, path)` |

#### FmuValidator — three-phase pipeline

```
validate(pkg)
├── checkStructure(pkg, report)
│   ├── check extractedPath exists
│   ├── check modelDescription.xml present
│   ├── check binaries/ directory present
│   ├── check host platform directory present (warn if missing)
│   ├── checkBinaryNamingConvention(pkg, report)   ← FMI 2.0 vs 3.0 dir names
│   └── checkLibraryIdentifier(pkg, report)         ← library name == modelIdentifier
├── checkSchema(pkg, report)
│   ├── select XSD (fmi2ModelDescription.xsd / fmi3ModelDescription.xsd)
│   └── delegate to IXmlValidator
└── checkSemantics(pkg, report)
    ├── duplicate valueReference per DataType
    ├── invalid causality/variability combinations
    └── no output variables (warning)
```

---

### Ports (`src/ports/`)

Pure abstract interfaces. No implementations reside here. The port layer is the **dependency
inversion boundary** — use cases depend upward on ports, adapters depend downward on ports.

| Port | Contract |
| --- | --- |
| `IFmuLoader` | `load(path)` → `FmuPackage` |
| `IXmlValidator` | `validate(xmlContent, xsdPath)` → `ValidationReport` |
| `ISimulationBackend` | `initialize`, `setInput`, `getOutput`, `doStep`, `terminate` |
| `IResultExporter` | `exportResult(result, path)` |
| `ILogger` | `log(level, message)` |

---

### Adapters (`src/adapters/`)

Concrete implementations of the ports. All third-party library usage is confined here.

| Adapter | Port | Library |
| --- | --- | --- |
| `ZipLoaderAdapter` | `IFmuLoader` | libzip — extracts the FMU archive, locates the platform binary, loads the DLL, resolves FMI function pointers |
| `Libxml2ValidatorAdapter` | `IXmlValidator` | libxml2 — validates XML content against an XSD schema, extracts line/column error positions |
| `ModelDescriptionParser` | — (called internally) | libxml2 — parses `modelDescription.xml` DOM into `FmuPackage` fields |
| `StandaloneSimulationAdapter` | `ISimulationBackend` | FMI function pointers (stored in `FmuPackage`) — implements the full FMI 2.0 and FMI 3.0 co-simulation lifecycle |
| `JsonResultExporter` | `IResultExporter` | (none) — hand-written JSON serialiser |

---

## PIMPL Pattern

```
Consumer code
    │
    ▼
FmuEngine  ──────────────────────────────────────────────────────────────────┐
(include/libfmu_toolkit/fmu_engine.hpp)                                      │
                                                                             │
  class FmuEngine {                                                          │
      struct Impl;                    // only the forward declaration here   │
      std::unique_ptr<Impl> impl_;                                          │
  public:                                                                    │
      uint64_t load(path);           // thin wrapper calling impl_->load()  │
      ValidationResult validate(h);  // thin wrapper                        │
      SimResult simulate(h, cfg);    // thin wrapper                        │
  };                                                                         │
                                                                             │
  ─────────────────── ABI boundary ──────────────────────────────────────── │
                                                                             │
FmuEngine::Impl                                                              │
(src/fmu_engine_impl.hpp — never installed)                                  │
                                                                             │
  struct FmuEngine::Impl {                                                   │
      std::map<uint64_t, Entry> entries_;                                   │
      std::unique_ptr<IXmlValidator> xmlValidator_;                         │
      // ... ZipLoaderAdapter, FmuValidator, etc.                           │
  };                                                                         │
└────────────────────────────────────────────────────────────────────────────┘
```

This guarantees:
- Consumers only see `struct Impl;` — never the full definition.
- Internal refactoring (adding fields, changing dependency wiring) never requires consumer recompilation.
- The shared library can be updated (bug fixes, new adapter implementations) without breaking
  consumer binaries, as long as the public method signatures are unchanged.

---

## Dependency Injection at the Composition Root

`FmuEngine::Impl` is the **composition root** — the one place where ports are wired to adapters:

```cpp
// Simplified from src/fmu_engine_impl.hpp
struct FmuEngine::Impl {
    explicit Impl(std::filesystem::path xsdBase,
                  std::unique_ptr<IXmlValidator> validator = nullptr)
    {
        if (!validator)
            xmlValidator_ = std::make_unique<Libxml2ValidatorAdapter>();
        else
            xmlValidator_ = std::move(validator);
        // ... resolve schema directories from xsdBase
    }

    uint64_t load(const std::string& path) {
        ZipLoaderAdapter loader;              // concrete adapter, local scope
        FmuImporter importer(loader);         // use-case receives port interface
        auto pkg = importer.import(path);     // FmuPackage returned by value
        // ... store in entries_
    }
};
```

The constructor accepts an `IXmlValidator` override for testing — allowing the entire XML layer
to be replaced with a stub without modifying any use-case code.

---

## Data Flow: Loading an FMU

```
engine.load("model.fmu")
│
├─ ZipLoaderAdapter::load(path)
│   ├─ libzip: extract archive → temp directory
│   ├─ find platform binary directory (win64 / x86_64-linux / …)
│   ├─ dlopen / LoadLibrary the shared library
│   ├─ dlsym all FMI function symbols → FmuPackage::Fmi2Functions or Fmi3Functions
│   └─ return FmuPackage (moved out)
│
└─ ModelDescriptionParser::parse(rawXml)
    ├─ libxml2: parse XML DOM
    ├─ read fmiVersion, modelName, guid / instantiationToken, modelIdentifier
    ├─ parse all ScalarVariable (FMI 2.0) / Float64, Int32, … (FMI 3.0) nodes
    ├─ parse UnitDefinitions, TypeDefinitions, DefaultExperiment, LogCategories
    └─ populate FmuPackage fields
```

## Data Flow: Validating an FMU

```
engine.validate(handle)
│
└─ FmuValidator::validate(pkg)
    ├─ checkStructure(pkg, report)
    │   ├─ verify extractedPath exists
    │   ├─ verify modelDescription.xml at root
    │   ├─ verify binaries/ directory
    │   ├─ checkBinaryNamingConvention → verify FMI-version-appropriate dir names
    │   └─ checkLibraryIdentifier     → verify library name == modelIdentifier
    │
    ├─ checkSchema(pkg, report)
    │   └─ Libxml2ValidatorAdapter::validate(rawXml, xsdPath)
    │       └─ libxml2: validate XML against XSD, collect errors with line/col
    │
    └─ checkSemantics(pkg, report)
        ├─ scan variables for duplicate valueReference per DataType
        ├─ check invalid causality/variability combinations
        └─ warn if no output variables exist
```

## Data Flow: Running a Co-Simulation

```
engine.simulate(handle, config, progressCallback)
│
└─ SimulationController::run(pkg, config, progress)
    │
    ├─ StandaloneSimulationAdapter::initialize(pkg, config)
    │   ├─ fmi2/3Instantiate
    │   ├─ fmi2SetupExperiment / fmi3EnterInitializationMode
    │   ├─ set initial parameters by valueReference
    │   └─ fmi2/3ExitInitializationMode
    │
    ├─ build variable name→(vr, type) index (O(1) lookup per step)
    │
    └─ simulation loop (t = startTime … stopTime, step = stepSize)
        ├─ StandaloneSimulationAdapter::doStep(t, stepSize)
        ├─ record outputs (filtered by outputVariables, every recordingInterval steps)
        ├─ invoke progressCallback(step, t, total)
        └─ check cancellation flag (atomic, thread-safe)
```
