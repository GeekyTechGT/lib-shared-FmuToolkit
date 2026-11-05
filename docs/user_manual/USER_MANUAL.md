# FmuToolkit - User Manual

## Overview

FmuToolkit - lib-shared

## Installation

Consume this library via CMake's `find_package(FmuToolkit CONFIG)` once
deployed, or link `include/fmutoolkit/fmutoolkit.hpp` directly against the built
`.dll`/`.so` under `deploy/<preset>/`.

## API

See `include/fmutoolkit/fmutoolkit.hpp` and `examples/basic_usage.cpp`.

## FMI 3.0 Event Mode, Binary and Clock (1.1.0)

Open the session with `SimConfig::eventModeUsed = true` to drive Clocks and Binary variables (FMI-LS-BUS buses). `openSession()` settles the
initial discrete states and leaves the FMU in Step Mode. Then:

- `doStepEx(t, h)` returns a `StepOutcome` (`completed`, `eventHandlingNeeded`, `terminateSimulation`, `earlyReturn`, `lastSuccessfulTime`);
- `enterEventMode()`, `updateDiscreteStates()` (call it until `discreteStatesNeedUpdate` is false) and `enterStepMode()` move between the modes;
- `getBinary()` / `setBinary()` copy a Binary variable's buffer, `getClock()` / `setClock()` read and activate a Clock, `getClockInterval()` reads a
  Clock's next interval.

`FmuInfo` now also reports `hasEventMode`, `mightReturnEarlyFromDoStep`, the `terminals` of `terminalsAndIcons.xml` (name, kind, matching rule,
members) and the `layeredStandards` found under `extra/`; `VariableInfo` carries `mimeType`, `maxSize`, `intervalVariability` and `clocks`.
`validate()` checks the FMI-LS-BUS rules the XSD cannot express (Event Mode declared, bus Binary variables clocked by existing Clock variables,
network terminal members of the right type and causality, Tx/Rx data and clock in pairs). The sized integer types (Int8 to UInt64) can now be read
and written through `setValue` / `getValue`.
