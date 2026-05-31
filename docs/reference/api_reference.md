# API Reference

All public types and functions are declared in `include/libfmu_toolkit/`. Include the umbrella
header to pull everything in:

```cpp
#include <libfmu_toolkit/lib_fmu_toolkit.hpp>
```

---

## FmuEngine

Primary entry point. Manages one or more loaded FMUs identified by opaque `uint64_t` handles.
Thread-safe for concurrent calls on different handles; not thread-safe for the same handle.

### Constructor

```cpp
explicit FmuEngine(std::string_view xsdBasePath = {});
```

- `xsdBasePath` — path to a directory containing `fmi-standard-2.0/` and `fmi-standard-3.0/`
  subdirectories. If empty, the engine auto-discovers the schemas (see
  [Runtime XSD Schema Discovery](../README.md#runtime-xsd-schema-discovery)).

### `load`

```cpp
uint64_t load(std::string_view fmuPath);
```

Extracts the FMU ZIP archive to a temporary directory, parses `modelDescription.xml`, loads the
platform-specific shared library, and resolves all FMI function symbols.

**Returns:** An opaque handle used in subsequent calls.

**Throws:** `FmuLoadError` if any step fails (archive extraction, XML parse, DLL load, symbol
resolution).

### `info`

```cpp
FmuInfo info(uint64_t handle) const;
```

Returns a value-type snapshot of the parsed model description. All fields are copied —
modifying the returned `FmuInfo` has no effect on the loaded FMU.

### `validate`

```cpp
ValidationResult validate(uint64_t handle) const;
```

Runs the three-phase validation pipeline (Structure → Schema → Semantics). Never throws.
Returns a `ValidationResult` whose `passed` flag is `true` iff no errors were found.

### `simulate`

```cpp
SimResult simulate(
    uint64_t handle,
    const SimConfig& config,
    std::function<void(size_t step, double t, size_t totalSteps)> progress = {}
) const;
```

Runs a fully-automated co-simulation loop from `config.startTime` to `config.stopTime` with
step size `config.stepSize`. Returns a `SimResult` with recorded time points and output series.

**Throws:** `FmuSimulationError` if any FMI function returns a non-OK status.

### `openSession`

```cpp
FmuSession openSession(uint64_t handle, const SimConfig& config);
```

Opens a step-by-step simulation session. The FMU is instantiated and initialised immediately.
The caller drives the loop manually via `FmuSession::doStep()`.

**Throws:** `FmuSimulationError` if initialisation fails.

### `unload`

```cpp
void unload(uint64_t handle);
```

Terminates any active session, unloads the shared library, and removes the temporary extraction
directory. Optional — the destructor performs the same cleanup for all remaining handles.

### Static export methods

```cpp
static void exportCsv(const SimResult& result, std::string_view outputPath);
static void exportJson(const SimResult& result, std::string_view outputPath);
```

Write the simulation result to a file. CSV format follows RFC 4180. JSON format is hand-written
(no external library dependency).

**Throws:** `std::runtime_error` if the file cannot be opened for writing.

---

## FmuSession

Provides step-by-step co-simulation control. Obtained from `FmuEngine::openSession()`. Move-only
(not copyable).

### `setValue` (four overloads)

```cpp
void setValue(std::string_view variableName, double           value);
void setValue(std::string_view variableName, int64_t          value);
void setValue(std::string_view variableName, bool             value);
void setValue(std::string_view variableName, std::string_view value);
```

Sets an input or parameter variable by name. Must be called **before** `doStep()`.
All overloads accept `std::string`, `std::string_view`, or string literals without allocation.

**Throws:** `std::out_of_range` if the variable is not found. `FmuSimulationError` on FMI errors.

### `getValue`

```cpp
OutputValue getValue(std::string_view variableName) const;
```

Reads an output variable by name. Returns an `OutputValue` variant
(`double`, `int64_t`, `bool`, or `std::string`).

**Throws:** `FmuSimulationError` if the variable is not found or the FMI get call fails.

### `doStep`

```cpp
bool doStep(double currentTime, double stepSize);
```

Advances the FMU by `stepSize` seconds starting from `currentTime`.

**Returns:** `true` if the step was accepted; `false` if the FMU requested early termination.

**Throws:** `FmuSimulationError` on fatal FMI errors.

### `terminate`

```cpp
void terminate();
```

Calls `fmi2Terminate` / `fmi3Terminate` and `fmi2FreeInstance` / `fmi3FreeInstance`. Safe to
call multiple times. The session becomes inactive after the first call.

### `isActive`

```cpp
bool isActive() const noexcept;
```

Returns `true` until `terminate()` is called or `doStep()` returns `false`.

---

## FmuInfo

Value-type model description snapshot returned by `FmuEngine::info()`.

```cpp
struct FmuInfo {
    std::string   modelName;
    std::string   modelIdentifier;
    std::string   guid;               // instantiationToken for FMI 3.0
    std::string   description;
    std::string   generationTool;
    FmiVersion    fmiVersion;         // FmiVersion::FMI2 or FmiVersion::FMI3

    std::vector<VariableInfo>       variables;
    std::vector<UnitInfo>           unitDefinitions;
    std::vector<TypeDefinitionInfo> typeDefinitions;
    DefaultExperiment               defaultExperiment;

    // Convenience filters — return pointers into variables
    std::vector<const VariableInfo*> inputs()     const;
    std::vector<const VariableInfo*> outputs()    const;
    std::vector<const VariableInfo*> parameters() const;
    const VariableInfo*              findVariable(std::string_view name) const;
};
```

---

## SimConfig

```cpp
struct SimConfig {
    double startTime{0.0};
    double stopTime{1.0};
    double stepSize{0.01};
    double tolerance{1e-6};
    LogLevel logLevel{LogLevel::None};

    // Initial parameter values by variable name (applied before simulation)
    std::map<std::string, ParameterValue> initialParameters;

    // Output variable names to record (empty = record all outputs)
    std::vector<std::string> outputVariables;

    // Record outputs every N steps (1 = every step, default)
    size_t recordingInterval{1};
};
```

---

## SimResult

```cpp
struct SimResult {
    std::vector<double>                            timePoints;
    std::map<std::string, std::vector<OutputValue>> series;

    // Implemented out-of-line in the library (ABI stability — no inline STL access).
    size_t stepCount() const noexcept;
    bool   empty()     const noexcept;
};
```

`OutputValue` is `std::variant<double, int64_t, bool, std::string>`.

---

## ValidationResult

```cpp
struct ValidationIssue {
    ValidationCategory category;  // Which phase produced this entry
    std::string        message;
    int                line{-1};   // -1 if not applicable (non-schema errors)
    int                column{-1};
};

struct ValidationResult {
    bool                          passed;
    std::vector<ValidationIssue>  errors;
    std::vector<ValidationIssue>  warnings;

    // Implemented out-of-line (ABI stability).
    bool hasErrors()   const noexcept;
    bool hasWarnings() const noexcept;
};
```

---

## Enumerations

### `FmiVersion`

```cpp
enum class FmiVersion { FMI2, FMI3 };
```

### `Causality`

```cpp
enum class Causality {
    Parameter, CalculatedParameter, Input, Output, Local, Independent
};
```

### `Variability`

```cpp
enum class Variability {
    Constant, Fixed, Tunable, Discrete, Continuous
};
```

### `DataType`

```cpp
enum class DataType {
    Real,           // FMI 2.0
    Integer,        // FMI 2.0
    Boolean,
    String,
    Float32,        // FMI 3.0
    Float64,        // FMI 3.0
    Int8, Int16, Int32, Int64,      // FMI 3.0
    UInt8, UInt16, UInt32, UInt64,  // FMI 3.0
    Binary,         // FMI 3.0
    Clock           // FMI 3.0
};
```

### `ValidationCategory`

```cpp
enum class ValidationCategory : uint8_t {
    Structure,  // Phase 1 — archive layout and binary directory naming
    Schema,     // Phase 2 — XSD validation of modelDescription.xml
    Semantic    // Phase 3 — causality/variability and value-reference checks
};
```

### `LogLevel`

```cpp
enum class LogLevel { None, Warning, Info, Debug, All };
```

---

## Exceptions

All exceptions inherit from `std::runtime_error`.

```cpp
// Base class
class FmuToolkitException : public std::runtime_error { ... };

// Thrown by FmuEngine::load()
class FmuLoadError : public FmuToolkitException { ... };

// Thrown by FmuEngine::validate() if the engine itself fails (not a user model error)
class FmuValidationError : public FmuToolkitException { ... };

// Thrown by FmuEngine::simulate() and FmuSession methods
class FmuSimulationError : public FmuToolkitException { ... };
```

**Note:** `validate()` returns a `ValidationResult` containing all errors — it does not throw
for model validation failures. `FmuValidationError` is only thrown if the validation machinery
itself encounters an unexpected error.

---

## Type Aliases

```cpp
using ParameterValue = std::variant<double, int64_t, bool, std::string>;
using OutputValue    = std::variant<double, int64_t, bool, std::string>;
```

---

## FMU Generation API

Header: `<libfmu_toolkit/fmu_generation.hpp>` (also pulled in by the umbrella header).
This API generates FMI artifacts from a JSON model definition. It does **not**
compile binaries or assemble a `.fmu` ZIP container (see constraint C-05).
Requirements: **FR-08**.

### `ModelDescriptionGenerator`

Generates a validated `modelDescription.xml` from a JSON definition.

```cpp
libfmu_toolkit::ModelDescriptionGenerator gen;

// Return the XML as a string.
std::string xml = gen.generateFromJson("model.json");

// Or write it to disk and get the resulting path back.
std::filesystem::path out =
    gen.writeFromJson("model.json", "out_dir", "modelDescription.xml", /*overwrite=*/true);
```

The generated document is checked by the library's own semantic validator before
it is returned; a semantic violation throws `FmuValidationError`.

### `FmiCCodeGenerator`

Generates FMI-oriented C getter/setter accessor source from the same JSON
definition. Identifiers are sanitised from the model and variable names.

```cpp
libfmu_toolkit::FmiCCodeGenerator codeGen;
std::string cSource = codeGen.generateFromJson("model.json");
// When fileName is empty, writeFromJson derives "<modelIdentifier>_accessors.c".
std::filesystem::path c = codeGen.writeFromJson("model.json", "out_dir");
```

### `FmuArtifactGenerator`

Façade that can emit only the XML, only the C source, or both, controlled by
`FmuArtifactGenerationOptions`.

```cpp
libfmu_toolkit::FmuArtifactGenerationOptions opts;
opts.outputDirectory          = "out_dir";
opts.generateModelDescription = true;   // emit modelDescription.xml
opts.generateCSource          = true;   // emit the C accessor source
opts.overwrite                = true;

libfmu_toolkit::FmuArtifactGenerator generator;
libfmu_toolkit::GeneratedArtifactPaths paths =
    generator.generateFromJson("model.json", opts);
// paths.modelDescriptionXmlPath, paths.cSourcePath
```

See the worked examples `examples/07_generate_from_json` and
`examples/08_generate_selective`.
