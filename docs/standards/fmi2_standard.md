# FMI 2.0 Standard — Summary for lib-fmu-toolkit

> Official specification: *Functional Mock-up Interface for Model Exchange and Co-Simulation,
> Version 2.0*
> Source: `external/fmi-standard-2.0/`

---

## Overview

FMI 2.0 defines two interface types:

- **Model Exchange (ME)** — the FMU provides differential equations; an external solver drives
  integration. Not implemented in this library (co-simulation only).
- **Co-Simulation (CS)** — the FMU bundles both the model and a solver; the master sends time
  steps and exchanges inputs/outputs.

`lib-fmu-toolkit` implements the **Co-Simulation** interface only.

---

## Archive Structure (§4.1)

An FMU is a ZIP archive with the extension `.fmu`. After extraction it must contain:

```
<modelIdentifier>.fmu/
├── modelDescription.xml            ← MANDATORY — model description
├── binaries/                       ← MANDATORY for CS
│   ├── win32/
│   │   └── <modelIdentifier>.dll
│   ├── win64/
│   │   └── <modelIdentifier>.dll
│   ├── linux32/
│   │   └── <modelIdentifier>.so
│   ├── linux64/
│   │   └── <modelIdentifier>.so
│   ├── darwin32/
│   │   └── <modelIdentifier>.dylib
│   └── darwin64/
│       └── <modelIdentifier>.dylib
├── sources/                        ← optional — portable C source
├── resources/                      ← optional — additional files
└── documentation/                  ← optional
```

**Key rules enforced by `FmuValidator`:**

- `modelDescription.xml` must exist at archive root.
- `binaries/` must exist.
- The platform directory name must follow the legacy format: `<os><bitwidth>` (e.g. `win64`,
  `linux64`). Using FMI 3.0-style names (e.g. `x86_64-windows`) inside an FMI 2.0 archive is
  an error.
- The shared library must be named `<modelIdentifier>.dll/.so/.dylib`.

---

## `modelDescription.xml` (§2.2)

### Root element attributes

| Attribute | Required | Description |
| --- | --- | --- |
| `fmiVersion` | ✔ | Must be `"2.0"` |
| `modelName` | ✔ | Human-readable name of the model |
| `guid` | ✔ | Globally unique identifier (must match the compiled binary) |
| `description` | — | Free-text description |
| `author` | — | Author name |
| `version` | — | Model version string |
| `copyright` | — | Copyright notice |
| `license` | — | License identifier |
| `generationTool` | — | Tool that generated the FMU |
| `generationDateAndTime` | — | ISO 8601 timestamp |
| `variableNamingConvention` | — | `flat` (default) or `structured` |
| `numberOfContinuousStates` | — | Used for ME; informational for CS |
| `numberOfEventIndicators` | — | Used for ME |

### `<CoSimulation>` element attributes

| Attribute | Required | Description |
| --- | --- | --- |
| `modelIdentifier` | ✔ | Must match the shared library filename (without extension) |
| `needsExecutionTool` | — | `false` (default) — FMU is self-contained |
| `canHandleVariableCommunicationStepSize` | — | `false` (default) — step size is fixed |
| `canInterpolateInputs` | — | `false` (default) |
| `canRunAsynchronuously` | — | `false` (default) |
| `canBeInstantiatedOnlyOncePerProcess` | — | `false` (default) |
| `canGetAndSetFMUstate` | — | `false` (default) — no checkpointing |
| `canSerializeFMUstate` | — | `false` (default) |
| `providesDirectionalDerivative` | — | `false` (default) |

---

## Variable Model (§2.2.7)

### `<ScalarVariable>` attributes

| Attribute | Required | Description |
| --- | --- | --- |
| `name` | ✔ | Unique variable name |
| `valueReference` | ✔ | Non-negative integer, unique per type |
| `description` | — | Free-text description |
| `causality` | — | See table below (default: `local`) |
| `variability` | — | See table below (default: `continuous`) |
| `initial` | — | See table below |

### Causality values

| Value | Meaning |
| --- | --- |
| `parameter` | Set by master before simulation; constant during simulation |
| `calculatedParameter` | Computed from other parameters at initialisation |
| `input` | Set by master at each communication step |
| `output` | Computed by FMU; read by master |
| `local` | Internal variable (not accessible from outside) |
| `independent` | The independent variable (usually time) |

### Variability values

| Value | Meaning |
| --- | --- |
| `constant` | Never changes during or after initialisation |
| `fixed` | Set during initialisation, constant thereafter |
| `tunable` | Can change between communication points |
| `discrete` | Changes only at event instants |
| `continuous` | Changes continuously (default for Real variables) |

### Valid `causality` / `variability` combinations

| | constant | fixed | tunable | discrete | continuous |
| --- | --- | --- | --- | --- | --- |
| **parameter** | — | ✔ | ✔ | — | — |
| **calculatedParameter** | — | ✔ | ✔ | — | — |
| **input** | — | — | — | ✔ | ✔ |
| **output** | ✔ | — | — | ✔ | ✔ |
| **local** | ✔ | ✔ | ✔ | ✔ | ✔ |
| **independent** | — | — | — | — | ✔ |

`FmuValidator` enforces the error cases from this table.

### Type sub-elements

Each `<ScalarVariable>` contains exactly one of:

```xml
<Real    start="0.0" unit="m/s" relativeQuantity="false" min="0.0" max="100.0"/>
<Integer start="0"   min="0"    max="10"/>
<Boolean start="false"/>
<String  start="hello"/>
<Enumeration start="0"/>
```

---

## Co-Simulation Lifecycle (§4.2)

```
fmi2Instantiate(...)
    │
    ▼
fmi2SetupExperiment(toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime)
    │
    ▼
fmi2EnterInitializationMode()
    │
    │  [set initial inputs and parameters via fmi2SetReal/Integer/Boolean/String]
    │
    ▼
fmi2ExitInitializationMode()
    │
    ▼
┌── Communication step loop ────────────────────────────────────────────────┐
│                                                                           │
│   fmi2SetReal/Integer/Boolean/String(...)   ← set inputs                │
│       │                                                                   │
│       ▼                                                                   │
│   fmi2DoStep(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint)
│       │                                                                   │
│       ▼                                                                   │
│   fmi2GetReal/Integer/Boolean/String(...)   ← read outputs              │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
    │
    ▼
fmi2Terminate()
    │
    ▼
fmi2FreeInstance()
```

**Important:** At the end of step 0 (after `ExitInitializationMode` but before the first
`DoStep`), outputs reflect the FMU's initial values, not computed values. The first computed
output is available after the first `DoStep`.

---

## Function Signatures (§3)

The FMI 2.0 C header is `external/fmi-standard-2.0/headers/fmi2Functions.h`.

### Key types

```c
typedef void*          fmi2Component;
typedef unsigned int   fmi2ValueReference;
typedef double         fmi2Real;
typedef int            fmi2Integer;
typedef int            fmi2Boolean;
typedef const char*    fmi2String;
typedef void*          fmi2FMUstate;
```

### Key functions (Co-Simulation subset)

```c
// Lifecycle
fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType,
    fmi2String fmuGUID, fmi2String fmuResourceLocation,
    const fmi2CallbackFunctions* functions,
    fmi2Boolean visible, fmi2Boolean loggingOn);

fmi2Status fmi2SetupExperiment(fmi2Component c,
    fmi2Boolean toleranceDefined, fmi2Real tolerance,
    fmi2Real startTime,
    fmi2Boolean stopTimeDefined, fmi2Real stopTime);

fmi2Status fmi2EnterInitializationMode(fmi2Component c);
fmi2Status fmi2ExitInitializationMode(fmi2Component c);
fmi2Status fmi2Terminate(fmi2Component c);
void       fmi2FreeInstance(fmi2Component c);

// Getters / setters
fmi2Status fmi2GetReal   (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real    value[]);
fmi2Status fmi2SetReal   (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);
fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]);
fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]);
fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]);
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]);
fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]);
fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]);

// Step
fmi2Status fmi2DoStep(fmi2Component c,
    fmi2Real currentCommunicationPoint,
    fmi2Real communicationStepSize,
    fmi2Boolean noSetFMUStatePriorToCurrentPoint);
```

### Return status

| Code | Value | Meaning |
| --- | --- | --- |
| `fmi2OK` | 0 | Success |
| `fmi2Warning` | 1 | Non-critical issue, simulation continues |
| `fmi2Discard` | 2 | Step rejected; master should retry with smaller step |
| `fmi2Error` | 3 | Unrecoverable error in this instance |
| `fmi2Fatal` | 4 | Unrecoverable error in all instances |
| `fmi2Pending` | 5 | Asynchronous operation pending (not used by this library) |

---

## UnitDefinitions (§2.2.2)

```xml
<UnitDefinitions>
  <Unit name="m/s">
    <BaseUnit m="1" s="-1"/>
    <DisplayUnit name="km/h" factor="3.6"/>
  </Unit>
</UnitDefinitions>
```

The `factor` and `offset` fields convert from SI base units:
`displayValue = factor × baseValue + offset`

---

## TypeDefinitions (§2.2.3)

```xml
<TypeDefinitions>
  <SimpleType name="Velocity">
    <Real quantity="Velocity" unit="m/s" min="0" max="340"/>
  </SimpleType>
</TypeDefinitions>
```

Variables can reference a type via `declaredType="Velocity"` to inherit its constraints.

---

## XSD Schema Validation

The official XSD schema for FMI 2.0 is located at:

```
external/fmi-standard-2.0/schema/fmi2ModelDescription.xsd
```

Supporting schemas (imported by the main XSD):

```
fmi2Annotation.xsd
fmi2AttributeGroups.xsd
fmi2ScalarVariable.xsd
fmi2Type.xsd
fmi2Unit.xsd
fmi2VariableDependency.xsd
```

`lib-fmu-toolkit` uses libxml2 to validate `modelDescription.xml` against this schema tree.
