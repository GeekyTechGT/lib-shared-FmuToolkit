# ADR 0002 — PIMPL for Public API

**Status:** Accepted  
**Date:** 2026-04-15

## Context

The public API (`FmuEngine`, `FmuSession`) must be stable across minor releases so that
consumers do not need to recompile when internal types change. It must also be free of
libxml2/libzip includes so Qt and other frameworks do not inherit transitive dependencies.

## Decision

Apply the **PIMPL (Pointer to Implementation)** idiom:

- Public headers declare `struct Impl;` as a forward declaration only.
- Full `Impl` definitions reside in `src/fmu_engine_impl.hpp`, which is never installed.
- Public methods are thin delegating wrappers: `return impl_->method(args...)`.

## Consequences

- Consumers see only forward declarations — no internal types in their include graph.
- Adding fields to `Impl` does not require consumer recompilation.
- A shared library update (bug fix, adapter change) is ABI-compatible as long as the
  public method signatures are unchanged.
- One additional heap allocation per `FmuEngine` instance (the `Impl`).
