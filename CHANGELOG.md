# Changelog

All notable changes to FmuToolkit will be documented in this file.

## [1.0.0] - Unreleased

### FMI 3.0 Event Mode, Binary and Clock support

The building blocks of FMI-LS-BUS.

- **Fix the FMI 3.0 backend ABI**: `fmi3InstantiateCoSimulation` is now called with its 12 parameters (the
  intermediate-update callback was missing), `fmi3Boolean` is `bool` (it was `int`), `eventModeUsed` /
  `earlyReturnAllowed` come from `SimConfig` instead of being fixed to 0, and the FMU's own log messages are no longer
  dropped (warnings and errors go to stderr).
- **`FmuSession`**: `doStepEx()` (full step outcome), `enterEventMode()`, `enterStepMode()`, `updateDiscreteStates()`,
  `getBinary()` / `setBinary()`, `getClock()` / `setClock()`, `getClockInterval()`. `SimConfig::eventModeUsed` opens the
  session in Step Mode after settling the initial discrete states. Purely additive: existing calls are unchanged.
- **Parsing**: `<CoSimulation>` capability flags, Binary `mimeType`/`maxSize`, Clock `intervalVariability`, the
  `clocks` attribute, `terminalsAndIcons/terminalsAndIcons.xml` (`FmuInfo::terminals`) and the folders under `extra/`
  (`FmuInfo::layeredStandards`).
- **Validation**: FMI-LS-BUS rules in the Semantic phase (Event Mode declared, bus Binary variables clocked by Clock
  variables, Network terminal members exist with the right type and causality, Tx/Rx data+clock pairs).
- Not yet done: the FMU generator does not emit Binary/Clock variables, terminals or LS-BUS manifests; the zip
  extraction limits (zip-slip / zip-bomb) of the audit are untouched.

### Port onto the lib-shared scaffold

Ported from `lib-fmu-toolkit-old` onto `tool-ProjectScaffold`'s `lib-shared`
category. The library's domain logic, use cases, ports, adapters (~1500
lines), 107 tests and 8 examples are carried over unchanged in substance;
this entry records what the port itself changed.

- **Folder layout remapped to this scaffold's hexagonal convention** (a
  mechanical rename, no logic changes): `src/core/domain` -> `src/domain/model`,
  `src/core/exceptions` -> `src/domain/exception`, `src/ports` ->
  `src/domain/port/out`, `src/core/usecases`/`src/core/generation` ->
  `src/application/service`, `src/adapters/*` -> `src/adapter/out/*`.
  `#include` paths were updated to match; internal namespacing was not
  touched.
- **Public API kept verbatim** for compatibility with the already-written
  `tool-FmuRunner` consumer: the `libfmu_toolkit` namespace, the flat
  installed header layout (`#include <lib_fmu_toolkit.hpp>`), and the
  hand-written `LIB_FMU_TOOLKIT_API` export macro are all unchanged - see
  `CLAUDE.md`'s compatibility contract. Public headers still live nested at
  `include/libfmu_toolkit/*.hpp` in the source tree (matching every internal
  `#include <libfmu_toolkit/...>` in the library's own code); the root
  `CMakeLists.txt` now flattens them explicitly on `install()`, replacing
  the old project's own `install(FILES ...)` block with one shaped for this
  scaffold's `gt_install_target()`.
- **Dual shared+static library build dropped.** This scaffold's `lib-shared`
  category builds one variant (`project.json`'s `library_type`, here
  `"shared"`). Tests, which reach into internal non-exported classes, now
  compile the library's own hexagonal sources directly into each test
  executable instead of linking a static variant - see `tests/CMakeLists.txt`
  and `CLAUDE.md`. Examples are unaffected: they always linked the shared
  target.
- **Test executables split in two**, `fmutoolkit_unit_tests` and
  `fmutoolkit_integration_tests`, matching this scaffold's two-suite
  convention (the old project built one combined `lib_fmu_toolkit_tests`).
  All 107 cases pass.
- **CMake target/artifact naming switched to this scaffold's convention**:
  the target is `fmutoolkit` (was `lib_fmu_toolkit_shared`/`_static`),
  producing `libfmutoolkit.dll`/`.dll.a` (was `lib-fmu-toolkit.dll`/`.a`) -
  required for `tool-FmuRunner`'s already-written
  `cmake/Dependencies/Dependencies.cmake` lookup
  (`lib-shared-FmuToolkit` -> imported target `fmutoolkit`) to resolve at
  all.
- **libxml2/libzip/nlohmann_json resolution ported to its own CMake module**,
  `cmake/Dependencies/FmuToolkitThirdParty.cmake` (static linking on MinGW,
  the MinGW static-runtime and `libwinpthread-1.dll`-copy helpers) - kept
  separate from the shared `cmake/Dependencies/ThirdParty.cmake` since none
  of it is reusable by other categories.
- **`external/fmi-standard-2.0`/`3.0` (FMI XSD schemas) are now `project.json`
  `submodules[]` entries** (`"kind": "git"`, `modelica/fmi-standard` at refs
  `v2.0.x`/`main`) fetched by `scripts/prj_exec.py`, rather than files
  expected to already exist on disk.
- **`project.json`**: `version` set to `1.0.0` and `environment` to
  `"production"` to match what `tool-FmuRunner`'s dependency declaration
  already expected and to make `scripts/prj_exec.py`'s deploy step write to
  the shared artifact store consumers actually read from (this scaffold's
  `"dev"` default deploys locally only).
- Custom Python tooling (`scripts/project_manager.py`,
  `scripts/build_linux_fmu_binaries.py`, the `pyvenv/` virtual environment)
  replaced by this scaffold's standard `scripts/prj_exec.py` orchestrator
  (`uv`-managed). The Linux-`.so`-in-FMU rebuild step was not ported - the
  committed `tests/data/*.fmu` already carry both platforms' binaries.
- `docs/build/build.md` (entirely about the old custom tooling) was not
  carried over; every other `docs/` file was ported, with folder-path
  references updated the same way as the source tree.
- License changed from MIT to Proprietary (Cristian Tesconi), matching this
  session's other ported projects (`tool-FmuRunner`); flag if the intent was
  to keep this component open source.
