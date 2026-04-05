# Documentation

This directory contains the long-lived technical documentation for the scaffold.

## What each document is for

- `architecture.md`: explains how the repository is split across public API, implementation, build logic, tests, examples, and automation.
- `design.md`: explains why the example API uses PIMPL and which design tradeoffs the scaffold is trying to model.
- `build.md`: explains presets, CLI flow, Windows `project_manager.bat`, Docker usage, install/deploy flow, and quality tools.
- `release.md`: explains what “release ready” means in this scaffold and how the placeholder release workflow should be specialized.
- `dependencies.md`: lists actual runtime, build, test, and developer-tool dependencies used by the repository.
- `specs/`: stores concrete feature or library specifications.
- `adr/`: stores architectural decisions with rationale and consequences.

## Recommended reading order

1. `architecture.md`
2. `design.md`
3. `build.md`
4. `dependencies.md`
5. `specs/library-spec.md`
6. `adr/0001-use-pimpl.md`
