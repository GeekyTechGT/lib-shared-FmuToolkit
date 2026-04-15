# ADR 0001 — Hexagonal Architecture (Ports & Adapters)

**Status:** Accepted  
**Date:** 2026-04-15

## Context

The library must support two FMI versions (2.0 and 3.0), two archive formats, multiple XML
parsers, and eventually multiple simulation backends. It must also be fully unit-testable without
requiring real FMU binaries.

## Decision

Use **hexagonal architecture** (Ports & Adapters):

- The **domain** layer holds pure C++ structs with no third-party dependencies.
- The **use-case** layer orchestrates domain objects via abstract port interfaces.
- The **port** layer defines `IFmuLoader`, `IXmlValidator`, `ISimulationBackend`,
  `IResultExporter`, and `ILogger` as pure virtual interfaces.
- The **adapter** layer provides concrete implementations (libzip, libxml2, FMI function
  pointer tables) that depend on third-party libraries.

## Consequences

- Adding a new FMI version requires only a new adapter; no use-case code changes.
- Unit tests can inject stub adapters without disk I/O or FMU binaries.
- libxml2 and libzip are confined to `src/adapters/` and never leak into public headers.
