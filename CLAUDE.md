# CLAUDE.md - FmuToolkit (lib-shared)

This project is a shared library (`.dll`/`.so`), generated with a hexagonal
architecture. It exposes a public API under `include/fmutoolkit/` and has no
inbound adapters (no CLI, no GUI) - it is meant to be linked into other
projects.

## Structure

- `include/fmutoolkit/fmutoolkit.hpp` - public API surface, annotated with
  `FMUTOOLKIT_EXPORT`. This is the only header other projects
  should include.
- `src/domain/`, `src/application/`, `src/adapter/out/`, `src/config/` -
  implementation. `domain/` has no framework dependencies.
- `examples/basic_usage.cpp` - must genuinely exercise the public API, not
  just `return 0`.
- `tests/unit/`, `tests/integration/` - CTest scaffold.

## Rules

- Never add inbound adapters (`adapter/in/...`) - this category has none by
  design. If the project needs a CLI or GUI, it should be a `tool`/`app`
  project that depends on this library instead.
- Keep the public header (`include/fmutoolkit/fmutoolkit.hpp`) minimal and stable -
  it is the contract other projects compile against.
- On Windows there is no rpath: the built `.dll` lives in `build/<preset>/lib/`.
  Always run examples/tests through `scripts/prj_exec.py` (menu `8`/`9`), which
  extends `PATH` automatically.
- Sanitizer flags are `PUBLIC`, not `PRIVATE` (see `cmake/Quality/Sanitizers.cmake`).
- **Naming convention**: project folder / artifact name `lib-shared-FmuToolkit`
  (`project.json.artifact_name`), binary `libfmutoolkit` (`OUTPUT_NAME` +
  pinned `lib` prefix in `src/CMakeLists.txt`, no debug postfix). Consumers
  declare `"name": "lib-shared-FmuToolkit"` as a dependency and link the imported
  target `fmutoolkit` - `cmake/Dependencies/Dependencies.cmake` derives that
  base name from the dependency name, so do not rename one without the other.
- **Deploy layout** (same as `app`/`tool`): `bin/` (DLL, lib-shared only),
  `lib/` (import/static library), `include/`, `documents/` (`USER_MANUAL.pdf`
  and `CHANGELOG.pdf` rendered from Markdown by `scripts/prj_docs.py`, plus
  `INFO.md`), `license/` (`LICENSE`); nothing in the deploy root. Edit the
  Markdown sources, never the PDFs. The CMake package config is an
  opt-in install component (`cmake_package`, `EXCLUDE_FROM_ALL`) and never
  lands in the deployed `lib/`.
