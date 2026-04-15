# FMU Validation — Technical Reference

## Overview

`FmuValidator` implements a **three-phase validation pipeline** that runs unconditionally on every
call to `engine.validate(handle)`. All phases execute even when earlier phases find errors, so
a single call returns the complete diagnostic picture.

```
Phase 1 — Structure
Phase 2 — Schema
Phase 3 — Semantics
```

Errors from all phases are collected into a `ValidationReport` whose `passed` flag is `true`
only when no errors exist across all three phases. Warnings do not affect the `passed` flag.

---

## Phase 1 — Structure

Checks the FMU archive layout against the FMI standard.

### Checks performed

| Check | Failure mode | FMI version |
| --- | --- | --- |
| Extracted root directory exists | Error | Both |
| `modelDescription.xml` exists at archive root | Error | Both |
| `binaries/` directory exists | Error | Both |
| At least one platform directory exists for the current host | Warning | Both |
| Binary directory names follow FMI 2.0 convention (`win64`, `linux64`, etc.) | Error if only FMI 3.0 names found | FMI 2.0 |
| Binary directory names follow FMI 3.0 convention (`x86_64-windows`, etc.) | Error if only FMI 2.0 names found | FMI 3.0 |
| Shared library filename matches `modelIdentifier` | Warning | Both |
| Shared library file exists at resolved path | Error | Both |

### FMI version naming convention check

This is the most distinctive new check in lib-fmu-toolkit v1.0+.

**FMI 2.0 valid directory names:**

```
win32    win64
linux32  linux64
darwin32 darwin64
```

**FMI 3.0 valid directory names:**

```
x86-windows      x86_64-windows   aarch64-windows
x86-linux        x86_64-linux     aarch64-linux
x86_64-darwin    aarch64-darwin
```

**Error cases:**

- An FMI 2.0 FMU that contains **only** FMI 3.0-style directories → **error**
- An FMI 3.0 FMU that contains **only** FMI 2.0-style directories → **error**
- Mixed styles present → **warning** (non-standard but not strictly invalid)

This catches a common mistake when exporting FMUs from tools that upgraded their naming
convention between FMI 2.0 and FMI 3.0 support.

### Library identifier check

Per FMI standard §4.1, the shared library must be named `<modelIdentifier>.<ext>`, where
`modelIdentifier` is the value of the `modelIdentifier` attribute in `<CoSimulation>`.

Example: if `modelIdentifier="Generatore"`, the library must be:
- `Generatore.dll` on Windows
- `Generatore.so` on Linux
- `Generatore.dylib` on macOS

A mismatch generates a **warning** (not an error, since the library was already successfully
loaded, but the naming convention violation could cause problems in other tools).

---

## Phase 2 — Schema

Validates the raw `modelDescription.xml` content against the official FMI XSD schema.

### Schema file selection

| FMI version | Schema file |
| --- | --- |
| FMI 2.0 | `external/fmi-standard-2.0/schema/fmi2ModelDescription.xsd` |
| FMI 3.0 | `external/fmi-standard-3.0/schema/fmi3ModelDescription.xsd` |

### Checks performed

| Check | Failure mode |
| --- | --- |
| `rawModelDescription` content is non-empty | Error |
| XSD schema file exists | Error |
| XML is valid against the XSD schema | Error (one per violation, with line/column) |

### Error detail level

Schema errors include **line and column numbers** extracted from the libxml2 parser, pointing
directly to the XML element or attribute that violated the schema.

Example output:
```
[Schema] (line 5:12) Element 'fmiModelDescription': The attribute 'guid' is required.
[Schema] (line 18:3) Element 'Outputs': Missing child element(s). Expected is 'Unknown'.
```

---

## Phase 3 — Semantics

Checks variable-model invariants that cannot be expressed in XSD because they involve
cross-variable relationships or the FMI variable-combination rules.

### Duplicate valueReference check

Within a single `DataType`, each `valueReference` must be unique. The FMI standard (§2.2.7 /
§2.4.7) requires this uniqueness per type, not globally.

Example error:
```
[Semantic] Duplicate valueReference 2 for type Real
```

### Causality / variability combination check

The FMI standard defines a validity table for `(causality, variability)` pairs. Invalid
combinations are errors:

| Combination | Rule |
| --- | --- |
| `input` + `constant` | Error — inputs cannot be constant |
| `output` + `fixed` | Error — outputs cannot be fixed |
| `output` + `tunable` | Error — outputs cannot be tunable |
| `parameter` + `discrete` | Error — parameters cannot be discrete |
| `parameter` + `continuous` | Error — parameters cannot be continuous |

Example error:
```
[Semantic] Variable 'gain': causality=input combined with variability=constant is invalid
```

### No output variables (warning)

An FMU with no output variables is technically valid per the schema but practically useless in
co-simulation. A warning is issued:

```
[WARN/Semantic] FMU has no output variables
```

---

## Using the Validator from Code

```cpp
libfmu_toolkit::FmuEngine engine;
const uint64_t handle = engine.load("path/to/model.fmu");

const libfmu_toolkit::ValidationResult result = engine.validate(handle);

// Check overall pass/fail
if (result.passed) {
    std::cout << "Validation PASSED\n";
} else {
    std::cout << "Validation FAILED\n";
}

// Iterate errors with category and position
for (const auto& err : result.errors) {
    std::cout << "[";
    switch (err.category) {
        case libfmu_toolkit::ValidationCategory::Structure: std::cout << "Structure"; break;
        case libfmu_toolkit::ValidationCategory::Schema:    std::cout << "Schema";    break;
        case libfmu_toolkit::ValidationCategory::Semantic:  std::cout << "Semantic";  break;
    }
    std::cout << "]";
    if (err.line > 0)   std::cout << " (line " << err.line << ":" << err.column << ")";
    std::cout << " " << err.message << "\n";
}

// Iterate warnings
for (const auto& warn : result.warnings) {
    std::cout << "[WARN] " << warn.message << "\n";
}
```

---

## Testing with Corrupted FMUs

The `data/` directory provides ready-made corrupted archives for each error category.

### data/fmi2/corrupted/

| File | Expected error | Phase |
| --- | --- | --- |
| `corrupted_xml.fmu` | XML parse error | Schema |
| `missing_binaries.fmu` | `binaries/ directory not found` | Structure |
| `missing_model_desc.fmu` | Load fails before validation | — |
| `wrong_platform_dir.fmu` | `FMI 2.0 FMU contains FMI 3.0-style binary directory 'x86_64-windows'` | Structure |

### data/fmi3/corrupted/

| File | Expected error | Phase |
| --- | --- | --- |
| `corrupted_xml.fmu` | XML parse error | Schema |
| `missing_binaries.fmu` | `binaries/ directory not found` | Structure |
| `missing_model_desc.fmu` | Load fails before validation | — |
| `wrong_platform_dir.fmu` | `FMI 3.0 FMU contains FMI 2.0-style binary directory 'win64'` | Structure |

Run a complete batch validation:

```bash
example_06_batch_validate data
```

---

## Injecting a Custom XML Validator

`FmuValidator` receives its `IXmlValidator` through the `FmuEngine` constructor, which accepts
a custom validator override:

```cpp
// Internal API — for testing only
class MyStubValidator final : public libfmu_toolkit::IXmlValidator {
public:
    libfmu_toolkit::ValidationReport validate(
        const std::string& xmlContent,
        const std::filesystem::path& schemaPath) const override
    {
        libfmu_toolkit::ValidationReport r;
        r.passed = true;
        return r;
    }
};
```

This pattern is used in the unit test suite (`tests/unit/test_FmuValidator.cpp`) to isolate
structural and semantic checks from the real libxml2 validation layer.
