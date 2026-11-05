# CLAUDE.md - FmuToolkit (lib-shared)

A shared library (`libfmutoolkit.dll` + import lib, or `.so` on Linux) for
loading, parsing, validating, and co-simulating FMI 2.0/3.0 FMUs. Ported onto
this scaffold's `lib-shared` category from a mature, pre-scaffold project
(`lib-fmu-toolkit`) that already had ~1500 lines of tested hexagonal code, 107
tests and 8 examples - see `docs/adr/` for the original design decisions
(still accurate) and `CHANGELOG.md` for what the port itself changed.

**Consumed by `tool-FmuRunner`** (`apps/cli/main.cpp`, `libfmu_toolkit::FmuEngine`/
`FmuSession`) via `"name": "lib-shared-FmuToolkit"` in its `project.json`
`dependencies[]`. That is a real, working dependency, not aspirational -
breaking the compatibility points below breaks FmuRunner's build.

## Compatibility contract with tool-FmuRunner (do not break)

The pre-scaffold project's public API surface is kept **verbatim**, not
adapted to this scaffold's usual `lib-shared` conventions, because FmuRunner
already compiles against it exactly as it was:

- **Namespace is `libfmu_toolkit`**, not `fmutoolkit` (this scaffold's usual
  `{libname}` convention). Every public and internal source file already uses
  it; never rename it opportunistically.
- **Public headers are flat**: `#include <lib_fmu_toolkit.hpp>` (the umbrella
  header), `<fmu_engine.hpp>`, `<types.hpp>`, etc., with **no** `fmutoolkit/`
  or `libfmu_toolkit/` subfolder in the *installed* copy. Internally, the
  same headers live at `include/libfmu_toolkit/*.hpp` (source tree) and are
  `#include <libfmu_toolkit/xxx.hpp>`'d that way by the library's own code
  (`src/fmu_engine.cpp`, tests, examples) - the flattening happens only in
  the root `CMakeLists.txt`'s explicit `install(FILES ...)` block, which
  replaces `gt_install_target()`'s normal `INCLUDE_DIRS` (that would install
  the nested folder as-is). The generated `version.hpp` is flat too, at
  `include/version.hpp`: `src/CMakeLists.txt` generates it directly into
  `<build>/src/generated/version.hpp` (no `libfmu_toolkit/` subfolder), and
  `lib_fmu_toolkit.hpp` includes it as `#include "version.hpp"` (quoted, like
  its sibling includes) rather than `<libfmu_toolkit/version.hpp>` - see the
  single `install(FILES ...)` block in the root `CMakeLists.txt`.
- **Export macro is the hand-written `LIB_FMU_TOOLKIT_API`**
  (`include/libfmu_toolkit/export.hpp`), not this scaffold's
  `generate_export_header()`/`FMUTOOLKIT_EXPORT`. `src/CMakeLists.txt` only
  defines `LIB_FMU_TOOLKIT_SHARED` (`PUBLIC`) / `LIB_FMU_TOOLKIT_BUILDING_SHARED`
  (`PRIVATE`) on the `fmutoolkit` target so that macro resolves correctly;
  it does not call `generate_export_header()` at all.
- **CMake target/artifact naming still follows this scaffold's rule**:
  the target is `fmutoolkit`, `OUTPUT_NAME "fmutoolkit"` + `PREFIX "lib"` in
  `src/CMakeLists.txt`, producing `libfmutoolkit.dll` / `libfmutoolkit.dll.a`
  - this part **does** need to match, since `cmake/Dependencies/Dependencies.cmake`
    derives exactly that base name (`fmutoolkit`) from FmuRunner's dependency
    name `lib-shared-FmuToolkit` and looks for `lib<base>.dll.a`/`.dll`/`.a`.
    Do not add an `OUTPUT_NAME`/`PREFIX` override that would produce anything
    else (e.g. the old project's own `lib-fmu-toolkit.dll`).
- **`project.json.version` is pinned at `1.0.0`** because FmuRunner's
  `dependencies[].version` already says `"1.0.0"` - bump both together.
- **`project.json.environment` is `"production"`**, not this scaffold's usual
  `"dev"` default - `scripts/prj_metadata.py`'s `deploy_directory()` only
  writes to the shared `deploy_folder` (where FmuRunner's build looks) when
  `environment` is `test`/`production`; `dev` deploys to a local `deploy/`
  folder inside this repo instead, which no consumer will ever see.

## Structure

Only the layer *names* were renamed to this scaffold's hexagonal convention
(a 1:1, mechanical rename of folders and `#include` paths - internal
namespacing and every class's own code were left untouched); the shape and
substance are the pre-scaffold project's own mature ports-and-adapters
design, described in full in `docs/architecture/architecture.md` and
`docs/adr/0001-hexagonal-architecture.md`:

- `include/libfmu_toolkit/` - the public PIMPL facade (`FmuEngine`,
  `FmuSession`, `types.hpp`, `exceptions.hpp`) plus `export.hpp` and the
  `version.hpp.in` template. No libxml2/libzip/internal types ever appear
  here (design rule from ADR-0001/0002).
- `src/domain/model/` - pure FMI domain structs (`FmuPackage`,
  `ModelVariable`, `ValidationReport`, `SimulationConfig`/`Result`, ...). No
  third-party dependencies.
- `src/domain/exception/` - the internal exception hierarchy
  (`fmu_exception.hpp`).
- `src/domain/port/out/` - the five driven-port interfaces (`IFmuLoader`,
  `IXmlValidator`, `ISimulationBackend`, `IResultExporter`, `ILogger`). Use
  cases depend on these, never on the adapters below directly.
- `src/application/service/` - the use cases (`FmuImporter`, `FmuValidator`,
  `SimulationController`, `CsvResultExporter`) plus `generation/` (the FR-08
  JSON-driven `modelDescription.xml`/C-accessor generator).
- `src/adapter/out/{xml,zip,simulation,json}/` - the concrete port
  implementations; all libxml2/libzip/dlopen usage is confined here.
- `src/fmu_engine.cpp`, `src/fmu_engine_impl.hpp`, `src/fmu_generation.cpp` -
  the PIMPL boundary (composition root: wires ports to adapters) - kept at
  `src/` root, exactly where the pre-scaffold project had them, picked up by
  `gt_discover_hexagonal_sources()`'s root-level `src/*.cpp` glob.
- `src/config/`, `src/domain/service/`, `src/domain/valueobject/`,
  `src/application/{command,dto,mapper}/` - unused by this port, kept as
  empty layer folders (`.gitkeep`) per this scaffold's convention.
- `tests/unit/`, `tests/integration/`, `tests/data/*.fmu` - 107 GoogleTest
  cases (`fmutoolkit_unit_tests` / `fmutoolkit_integration_tests`, this
  scaffold's two-suite naming convention - the pre-scaffold project used one
  combined executable). **Both compile the same hexagonal `.cpp` sources
  directly into the test binary** (`tests/CMakeLists.txt`'s
  `FMUTOOLKIT_LIB_SOURCES`), rather than linking the `fmutoolkit` shared
  target: this category only builds one library variant (`library_type` in
  `project.json`), and tests reach into internal, non-exported classes
  (`FmuValidator`, adapters, ...) that `CXX_VISIBILITY_PRESET hidden` would
  otherwise hide across a real DLL boundary. The pre-scaffold project solved
  the same problem by also building a static library variant just for tests
  - this is the single-variant equivalent.
- `examples/01_basic_simulate/` … `08_generate_selective/` - all 8 ported
  as-is; unlike tests, these link the real `fmutoolkit` DLL (public API
  only), matching `examples/CMakeLists.txt`'s `fmutoolkit_configure_example()`.
- `data/generation/*.json` - FR-08 example model definitions, referenced by
  `tests/unit/test_FmuGeneration.cpp` and `example_07`/`08` via
  `LIB_FMU_TOOLKIT_SOURCE_DIR`.
- `cmake/Dependencies/FmuToolkitThirdParty.cmake` - libxml2/libzip/nlohmann_json
  resolution and the MinGW static-runtime helpers, ported from the
  pre-scaffold project's own root `CMakeLists.txt`. Kept as its own file
  (not folded into the shared `cmake/Dependencies/ThirdParty.cmake`) because
  none of it is reusable by other categories - same reasoning tool-FmuRunner
  applies to its own `SILKIT_DIR` resolution.

## External schemas: submodules, not vendored files

`external/fmi-standard-2.0/` and `external/fmi-standard-3.0/` (FMI XSD
schemas used by `FmuValidator`'s schema phase, resolved via
`LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR`) are declared in `project.json`'s
`submodules[]` as `"kind": "git"` clones of `modelica/fmi-standard` at refs
`v2.0.x` / `main` respectively - **not** committed files. Run `scripts/prj_exec.py`
menu `2` (or it runs automatically before `build`) before configuring on a
fresh clone. `schemaTreeExists()` in `src/fmu_engine_impl.hpp` checks for
`schema/fmi2ModelDescription.xsd` / `schema/fmi3ModelDescription.xsd`
specifically - verified present at both refs.

## Rules

- **Never rename the `libfmu_toolkit` namespace or flatten/nest headers
  differently** without updating tool-FmuRunner's `apps/cli/main.cpp` and
  `apps/cli/CMakeLists.txt` in the same change - see the compatibility
  contract above.
- **This category has no inbound adapters by design** (no CLI, no GUI) - a
  project needing one should be a `tool`/`app` depending on this library,
  not a change to this repo's shape.
- On Windows there is no rpath: `fmutoolkit`'s `RUNTIME_OUTPUT_DIRECTORY` is
  `build/<preset>/lib/`, not `bin/` - examples and tests each get their own
  `POST_BUILD` copy step (see `fmutoolkit_copy_winpthread()`/the DLL-copy
  block in `examples/CMakeLists.txt`) rather than relying on a shared output
  directory. Always run examples/tests through `scripts/prj_exec.py`
  (menu `8`/`9`).
- **libxml2 and libzip are statically linked into `libfmutoolkit.dll` on
  MinGW** (`FMUTOOLKIT_MINGW_STATIC_RUNTIME`, default `ON`) - a consumer
  needs only `libfmutoolkit.dll` and `libwinpthread-1.dll`, never
  `libxml2.dll`/`libzip.dll`. Do not switch this to dynamic linking without
  updating the deploy story (FmuRunner's `copy_to` only copies `bin/`/`lib/`
  from this project's own deployed folder).
- `_GLIBCXX_USE_CXX11_ABI=1` is set `PUBLIC` on `fmutoolkit`
  (`FmuToolkitThirdParty.cmake`) - keep it that way so consumers linking
  against the DLL stay ABI-consistent; do not move it to `PRIVATE`.
- Keep `docs/architecture/architecture.md`, the ADRs (`docs/adr/0001`-`0005`),
  the SRS/traceability (`docs/requirements/`) and the API reference
  (`docs/reference/`) consistent with the code - they describe a real,
  already-verified design, not aspirational documentation; update them
  alongside any behavioural change, the same way `docs/reference/traceability.md`
  expects.
- **Deploy layout** (same as `app`/`tool`): `bin/` (`libfmutoolkit.dll`),
  `lib/` (`libfmutoolkit.dll.a`), `include/` (flat public headers, see
  above), `documents/` (`USER_MANUAL.pdf`/`CHANGELOG.pdf` rendered from
  Markdown by `scripts/prj_docs.py`, plus `INFO.md`), `license/` (`LICENSE`);
  nothing else in the deploy root. Edit the Markdown sources, never the PDFs.
