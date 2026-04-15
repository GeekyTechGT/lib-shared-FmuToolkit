# Documentation Index

This directory contains the technical documentation for `lib-fmu-toolkit`.

## Requirements

| Document | Description |
| --- | --- |
| [requirements/srs.md](requirements/srs.md) | Software Requirements Specification — functional and non-functional requirements |

## Architecture

| Document | Description |
| --- | --- |
| [architecture/architecture.md](architecture/architecture.md) | Layer map, class responsibilities, data-flow diagrams |

## Reference

| Document | Description |
| --- | --- |
| [reference/api_reference.md](reference/api_reference.md) | Complete public API reference (FmuEngine, FmuSession, types, exceptions) |
| [reference/validation.md](reference/validation.md) | Three-phase validation pipeline: checks, error messages, test data |

## Standards

| Document | Description |
| --- | --- |
| [standards/fmi2_standard.md](standards/fmi2_standard.md) | FMI 2.0 archive structure, XML schema, lifecycle, function signatures |
| [standards/fmi3_standard.md](standards/fmi3_standard.md) | FMI 3.0 archive structure, new types, lifecycle, differences from FMI 2.0 |

## Build

| Document | Description |
| --- | --- |
| [build/build.md](build/build.md) | Build presets, CMake options, dependency resolution, Qt6 integration |

## Architecture Decision Records

| ADR | Decision |
| --- | --- |
| [adr/0001-hexagonal-architecture.md](adr/0001-hexagonal-architecture.md) | Why hexagonal architecture (Ports & Adapters) |
| [adr/0002-pimpl-public-api.md](adr/0002-pimpl-public-api.md) | Why PIMPL for the public API |
| [adr/0003-cross-platform-toolchains-only.md](adr/0003-cross-platform-toolchains-only.md) | Why MSVC is not supported |
| [adr/0004-three-phase-fmu-validation.md](adr/0004-three-phase-fmu-validation.md) | Why validation runs all three phases unconditionally |
| [adr/0005-abi-stability-source-distribution.md](adr/0005-abi-stability-source-distribution.md) | ABI stability mitigations for source-distributed C++ (string_view, out-of-line methods, ABI pin) |

## External Standards

The FMI specifications and XSD schemas are included as git submodules:

| Path | Content |
| --- | --- |
| `external/fmi-standard-2.0/` | FMI 2.0 specification, headers, and XSD schemas |
| `external/fmi-standard-3.0/` | FMI 3.0 specification, headers, and XSD schemas |
