# AGENTS.md - FmuToolkit (lib-shared)

Instructions for any AI coding agent working in this repository.

## What this is

A shared library (`.dll`/`.so`) with a hexagonal architecture and no inbound
adapters. Public API: `include/fmutoolkit/fmutoolkit.hpp`.

## Build / test

```bash
uv run python scripts/prj_exec.py --preset <preset> --commands 5,6,9
```

Presets: `windows-mingw64-{debug,release}`, `linux-gcc-{debug,release}`,
`docker-gcc-{debug,release}`.

## Conventions

- No `adapter/in/` - this category never has inbound adapters.
- `examples/basic_usage.cpp` must call real API and assert on the result.
- Public header only under `include/fmutoolkit/`; everything else is private.
- Run built binaries through `scripts/prj_exec.py`, not directly - it sets up
  `PATH` for the DLL on Windows.
