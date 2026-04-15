# FMI 3.0 Standard — Summary for lib-fmu-toolkit

> Official specification: *Functional Mock-up Interface Specification, Version 3.0*
> Source: `external/fmi-standard-3.0/`

---

## Overview

FMI 3.0 is a major revision of the standard, introducing:

- **New variable data types** — Float32, Int8/16/32/64, UInt8/16/32/64, Binary, Clock.
- **Revised archive structure** — platform directory names follow the `<arch>-<os>` convention
  instead of the legacy `<os><bitwidth>` names used in FMI 2.0.
- **New instantiation token** — replaces the FMI 2.0 `guid` attribute to better support tool
  round-tripping.
- **Clocks** — a new interface for synchronising periodic and aperiodic events.
- **Terminals and Icons** — an optional structural description of connectors.
- **Build Description** — an optional XML file describing compilation flags for distributing
  source-only FMUs.

`lib-fmu-toolkit` implements the **Co-Simulation** interface for FMI 3.0, covering all standard
variable types (Float32/64, Int8/16/32/64, UInt8/16/32/64, Boolean, String, Binary, Clock).

---

## Archive Structure (§4.1)

```
<modelIdentifier>.fmu/
├── modelDescription.xml                  ← MANDATORY
├── buildDescription.xml                  ← optional (source distribution)
├── terminalsAndIcons.xml                 ← optional (connector topology)
├── documentation/                        ← optional
├── icons/                                ← optional (model.png, etc.)
├── binaries/                             ← MANDATORY for CS
│   ├── x86_64-windows/
│   │   └── <modelIdentifier>.dll
│   ├── x86-windows/
│   │   └── <modelIdentifier>.dll
│   ├── aarch64-windows/
│   │   └── <modelIdentifier>.dll
│   ├── x86_64-linux/
│   │   └── <modelIdentifier>.so
│   ├── x86-linux/
│   │   └── <modelIdentifier>.so
│   ├── aarch64-linux/
│   │   └── <modelIdentifier>.so
│   ├── x86_64-darwin/
│   │   └── <modelIdentifier>.dylib
│   └── aarch64-darwin/
│       └── <modelIdentifier>.dylib
├── sources/                              ← optional
└── resources/                            ← optional
```

**Key rules enforced by `FmuValidator`:**

- `modelDescription.xml` must exist at archive root.
- `binaries/` must exist.
- Platform directory names must follow the `<arch>-<os>` convention. Using FMI 2.0-style names
  (e.g. `win64`) inside an FMI 3.0 archive is an error.
- The shared library must be named `<modelIdentifier>.dll/.so/.dylib`.

### Platform directory name format

```
<arch>-<os>

arch: x86, x86_64, aarch64, ppc64le, s390x
os:   windows, linux, darwin
```

---

## `modelDescription.xml` (§2.4)

### Root element attributes

| Attribute | Required | Description |
| --- | --- | --- |
| `fmiVersion` | ✔ | Must be `"3.0"` |
| `modelName` | ✔ | Human-readable name |
| `instantiationToken` | ✔ | Replaces FMI 2.0 `guid`; a tool-specific token that the master must pass back at instantiation |
| `description` | — | Free-text description |
| `author` | — | Author name |
| `version` | — | Model version string |
| `copyright` | — | Copyright notice |
| `license` | — | License identifier |
| `generationTool` | — | Tool name and version |
| `generationDateAndTime` | — | ISO 8601 timestamp |
| `variableNamingConvention` | — | `flat` or `structured` |

### `<CoSimulation>` element attributes

| Attribute | Required | Description |
| --- | --- | --- |
| `modelIdentifier` | ✔ | Must match the shared library filename |
| `needsExecutionTool` | — | `false` default |
| `canHandleVariableCommunicationStepSize` | — | `false` default |
| `canReturnEarlyAfterIntermediateUpdate` | — | FMI 3.0 new: early return from step |
| `fixedInternalStepSize` | — | If set, the FMU has a fixed internal step |
| `maxOutputDerivativeOrder` | — | Maximum output derivative order supported |
| `canGetAndSetFMUstate` | — | `false` default |
| `canSerializeFMUstate` | — | `false` default |
| `providesDirectionalDerivative` | — | `false` default |
| `providesAdjointDerivatives` | — | FMI 3.0 new |
| `providesPerElementDependencies` | — | FMI 3.0 new |

---

## Variable Model (§2.4.7)

### New variable element names (replaces `<ScalarVariable>`)

In FMI 3.0, variables are declared with typed elements directly:

```xml
<Float64  name="y"    valueReference="0" causality="output" variability="continuous"/>
<Float32  name="yf"   valueReference="1" causality="output" variability="continuous"/>
<Int32    name="n"    valueReference="2" causality="parameter" variability="fixed" start="5"/>
<Int64    name="nl"   valueReference="3" causality="parameter" variability="fixed" start="0"/>
<UInt32   name="idx"  valueReference="4" causality="output" variability="discrete"/>
<Boolean  name="flag" valueReference="5" causality="output" variability="discrete"/>
<String   name="msg"  valueReference="6" causality="output" variability="discrete"/>
<Binary   name="blob" valueReference="7" causality="output" variability="discrete"/>
<Clock    name="clk"  valueReference="8" causality="output" variability="discrete"/>
```

### Supported data types

| XML element | C type | Description |
| --- | --- | --- |
| `Float64` | `fmi3Float64` (`double`) | 64-bit IEEE 754 floating point (replaces FMI 2.0 `Real`) |
| `Float32` | `fmi3Float32` (`float`) | 32-bit IEEE 754 floating point |
| `Int8` | `fmi3Int8` (`int8_t`) | 8-bit signed integer |
| `Int16` | `fmi3Int16` (`int16_t`) | 16-bit signed integer |
| `Int32` | `fmi3Int32` (`int32_t`) | 32-bit signed integer (replaces FMI 2.0 `Integer`) |
| `Int64` | `fmi3Int64` (`int64_t`) | 64-bit signed integer |
| `UInt8` | `fmi3UInt8` (`uint8_t`) | 8-bit unsigned integer |
| `UInt16` | `fmi3UInt16` (`uint16_t`) | 16-bit unsigned integer |
| `UInt32` | `fmi3UInt32` (`uint32_t`) | 32-bit unsigned integer |
| `UInt64` | `fmi3UInt64` (`uint64_t`) | 64-bit unsigned integer |
| `Boolean` | `fmi3Boolean` (`bool`) | Boolean |
| `String` | `fmi3String` (`const char*`) | UTF-8 string |
| `Binary` | `fmi3Binary` (`const fmi3Byte*`) | Opaque binary data |
| `Clock` | `fmi3Clock` (`bool`) | Synchronisation clock |

### Causality values (same as FMI 2.0, one addition)

| Value | Meaning |
| --- | --- |
| `parameter` | Set before simulation; constant during simulation |
| `calculatedParameter` | Computed from parameters at initialisation |
| `input` | Set by master at each communication point |
| `output` | Computed by FMU; read by master |
| `local` | Internal variable |
| `independent` | The independent variable (simulation time) |
| `structuralParameter` | FMI 3.0 new: parameter that influences model structure |

### Valid `causality` / `variability` combinations (FMI 3.0 §2.4.7)

Same table as FMI 2.0, with `structuralParameter` behaving like `parameter`.

---

## Co-Simulation Lifecycle (§4.2)

```
fmi3InstantiateCoSimulation(
    instanceName, instantiationToken, resourcePath,
    visible, loggingOn, eventModeUsed, earlyReturnAllowed,
    requiredIntermediateVariables, nRequiredIntermediateVariables,
    instanceEnvironment, logMessage, intermediateUpdate)
    │
    ▼
fmi3EnterInitializationMode(
    toleranceDefined, tolerance, startTime,
    stopTimeDefined, stopTime)
    │
    │  [set initial inputs and parameters via fmi3SetFloat64/Int32/…]
    │
    ▼
fmi3ExitInitializationMode()
    │
    ▼
┌── Communication step loop ────────────────────────────────────────────────┐
│                                                                           │
│   fmi3SetFloat64/Int32/…(...)     ← set inputs                          │
│       │                                                                   │
│       ▼                                                                   │
│   fmi3DoStep(currentCommunicationPoint, communicationStepSize,           │
│              noSetFMUStatePriorToCurrentPoint,                            │
│              eventEncountered, terminateSimulation,                       │
│              earlyReturn, lastSuccessfulTime)                             │
│       │                                                                   │
│       ▼                                                                   │
│   fmi3GetFloat64/Int32/…(...)     ← read outputs                        │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
    │
    ▼
fmi3Terminate()
    │
    ▼
fmi3FreeInstance()
```

**Key difference from FMI 2.0:** `fmi3DoStep` returns additional flags (`eventEncountered`,
`terminateSimulation`, `earlyReturn`, `lastSuccessfulTime`).

---

## Function Signatures (§3)

The FMI 3.0 C header is `external/fmi-standard-3.0/headers/fmi3Functions.h`.

### Key types

```c
typedef void*          fmi3Instance;
typedef unsigned int   fmi3ValueReference;
typedef double         fmi3Float64;
typedef float          fmi3Float32;
typedef int8_t         fmi3Int8;
typedef int16_t        fmi3Int16;
typedef int32_t        fmi3Int32;
typedef int64_t        fmi3Int64;
typedef uint8_t        fmi3UInt8;
typedef uint16_t       fmi3UInt16;
typedef uint32_t       fmi3UInt32;
typedef uint64_t       fmi3UInt64;
typedef bool           fmi3Boolean;
typedef const char*    fmi3String;
typedef uint8_t        fmi3Byte;
typedef const fmi3Byte* fmi3Binary;
typedef bool           fmi3Clock;
```

### Key functions (Co-Simulation subset)

```c
// Lifecycle
fmi3Instance fmi3InstantiateCoSimulation(
    fmi3String instanceName, fmi3String instantiationToken,
    fmi3String resourcePath, fmi3Boolean visible, fmi3Boolean loggingOn,
    fmi3Boolean eventModeUsed, fmi3Boolean earlyReturnAllowed,
    const fmi3ValueReference requiredIntermediateVariables[],
    size_t nRequiredIntermediateVariables,
    fmi3InstanceEnvironment instanceEnvironment,
    fmi3LogMessageCallback logMessage,
    fmi3IntermediateUpdateCallback intermediateUpdate);

fmi3Status fmi3EnterInitializationMode(fmi3Instance instance,
    fmi3Boolean toleranceDefined, fmi3Float64 tolerance,
    fmi3Float64 startTime,
    fmi3Boolean stopTimeDefined, fmi3Float64 stopTime);

fmi3Status fmi3ExitInitializationMode(fmi3Instance instance);
fmi3Status fmi3Terminate(fmi3Instance instance);
void       fmi3FreeInstance(fmi3Instance instance);

// Getters / setters (shown for Float64; pattern repeats for all types)
fmi3Status fmi3GetFloat64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[], size_t nValueReferences,
    fmi3Float64 values[], size_t nValues);

fmi3Status fmi3SetFloat64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[], size_t nValueReferences,
    const fmi3Float64 values[], size_t nValues);

// Step
fmi3Status fmi3DoStep(fmi3Instance instance,
    fmi3Float64 currentCommunicationPoint,
    fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* eventEncountered,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* earlyReturn,
    fmi3Float64* lastSuccessfulTime);
```

### Return status

Same as FMI 2.0 (`fmi3OK=0`, `fmi3Warning=1`, `fmi3Discard=2`, `fmi3Error=3`, `fmi3Fatal=4`).

---

## UnitDefinitions (§2.4.3)

```xml
<UnitDefinitions>
  <Unit name="m/s">
    <BaseUnit m="1" s="-1" factor="1" offset="0"/>
    <DisplayUnit name="km/h" factor="3.6" offset="0"/>
  </Unit>
</UnitDefinitions>
```

---

## TypeDefinitions (§2.4.4)

```xml
<TypeDefinitions>
  <Float64Type name="Velocity" unit="m/s" min="0.0" max="340.0" quantity="Velocity"/>
  <Int32Type   name="Count"   min="0"    max="1000"/>
</TypeDefinitions>
```

Variables reference named types via `declaredType="Velocity"`.

---

## XSD Schema Files

All FMI 3.0 schemas are in `external/fmi-standard-3.0/schema/`:

| File | Purpose |
| --- | --- |
| `fmi3ModelDescription.xsd` | Main schema — root element and top-level structure |
| `fmi3Variable.xsd` | All variable element definitions (Float64, Int32, etc.) |
| `fmi3Type.xsd` | TypeDefinitions element definitions |
| `fmi3Unit.xsd` | UnitDefinitions element definitions |
| `fmi3AttributeGroups.xsd` | Shared attribute group definitions |
| `fmi3VariableDependency.xsd` | ModelStructure dependencies |
| `fmi3InterfaceType.xsd` | CoSimulation / ModelExchange interface elements |
| `fmi3BuildDescription.xsd` | buildDescription.xml schema |
| `fmi3Terminal.xsd` | terminalsAndIcons.xml terminal definitions |
| `fmi3TerminalsAndIcons.xsd` | terminalsAndIcons.xml root schema |
| `fmi3LayeredStandardManifest.xsd` | Layered standard manifest schema |
| `fmi3Annotation.xsd` | Vendor annotation elements |

`lib-fmu-toolkit` validates `modelDescription.xml` against `fmi3ModelDescription.xsd` (which
imports the other schemas as needed via libxml2's schema resolver).

---

## Key Differences from FMI 2.0

| Aspect | FMI 2.0 | FMI 3.0 |
| --- | --- | --- |
| Model identity | `guid` (string) | `instantiationToken` (tool-defined string) |
| Variable types | Real, Integer, Boolean, String | Float32, Float64, Int8/16/32/64, UInt8/16/32/64, Boolean, String, Binary, Clock |
| Binary dirs | `win64`, `linux64`, `darwin64` | `x86_64-windows`, `x86_64-linux`, `x86_64-darwin` |
| DoStep return | `fmi2Status` | `fmi3Status` + event/early-return flags |
| Clocks | Not available | Synchronisation clocks for aperiodic events |
| Terminals | Not available | Optional XML topology description |
| Build description | Not available | Optional XML for source distribution |
| Intermediate update | Not available | Callback for sub-step output access |
