# Roadmap

## Completed

### Core architecture

- Hexagonal (Ports & Adapters) architecture with domain, use-cases, ports, and adapters
- PIMPL facade (`FmuEngine`, `FmuSession`) — ABI-stable public API, no internal types exposed
- Dependency injection throughout: `IXmlValidator`, `IFmuLoader`, `ISimulationBackend`, `IResultExporter`

### FMI support

- FMI 2.0 co-simulation: full lifecycle, all getter/setter types (Real, Integer, Boolean, String)
- FMI 3.0 co-simulation: Float32/64, Int8-64, UInt8-64, Bool, String, Binary, Clock
- `modelDescription.xml` parsing: variables, UnitDefinitions, TypeDefinitions, DefaultExperiment
- Three-phase validation: structure (archive layout + FMI version-specific binary naming), XSD schema, semantics

### Build system

- CMake ≥ 3.25 with preset matrix (Linux GCC/Clang, Windows MinGW/Clang, vcpkg variants)
- MSVC explicitly removed (ADR-0003): targets GCC + Clang only for Qt6 compatibility
- Static and shared library targets; MinGW static runtime linkage
- vcpkg manifest + private registry templates

### ABI stability

- All input-only string parameters changed to `std::string_view`
- Inline STL-accessing methods moved out-of-line (`hasErrors`, `hasWarnings`, `stepCount`, `empty`)
- `std::less<>` transparent comparator on `varIndex` — zero-allocation `find` with `string_view`
- `_GLIBCXX_USE_CXX11_ABI=1` pinned as `PUBLIC` CMake compile definition

### Test data

- `data/fmi2/SimpleAdder.fmu` — valid FMI 2.0 stub
- `data/fmi3/Generatore.fmu`, `Sommatore.fmu`, `SimpleRamp.fmu` — valid FMI 3.0 archives
- `data/fmi2/corrupted/` and `data/fmi3/corrupted/` — 4 corrupted FMUs each (xml, binaries, model desc, platform dir)

### Examples (6)

- `01_basic_simulate`, `02_step_by_step`, `03_validate_fmu`, `04_export_results`
- `05_generatore_sommatore` — two-FMU orchestrated co-simulation
- `06_batch_validate` — batch validation sweep over `data/`

### Documentation

- Full `docs/` hierarchy: `architecture/`, `standards/`, `reference/`, `build/`, `requirements/`, `adr/`
- `docs/requirements/srs.md` — functional and non-functional requirements
- ADRs 0001–0005: hexagonal arch, PIMPL, no MSVC, three-phase validation, ABI stability

---

## Next candidates

- **CI pipeline** — GitHub Actions matrix covering `linux-gcc-debug`, `linux-clang-debug`,
  `windows-mingw-debug`; upload test results and code coverage
- **FMI 2.0 model exchange** — extend `ISimulationBackend` for ME mode (requires ODE solver port)
- **C API layer** (`c_api.h`) — `extern "C"` facade for Python/Rust/C# language bindings
- **Result streaming** — callback-based output recording to avoid large `SimResult` allocations
  for long-running simulations
- **FMU validation report serialisation** — export `ValidationResult` to JSON/XML for CI integration
- **vcpkg binary caching** — document and optionally automate cache configuration for CI
