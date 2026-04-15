# ADR 0004 — Three-Phase FMU Validation

**Status:** Accepted  
**Date:** 2026-04-15

## Context

An FMU can be invalid in multiple independent ways: bad archive structure, invalid XML, and
semantic inconsistencies. Stopping at the first error would hide downstream problems and require
multiple validation runs to get a complete picture.

## Decision

Implement three **independent validation phases** that all run unconditionally:

1. **Structure** — verifies archive layout and FMI-standard binary directory naming conventions.
2. **Schema** — validates XML against the official FMI XSD schema.
3. **Semantics** — checks cross-variable invariants (duplicate value references,
   causality/variability compatibility).

All phases append to a single `ValidationReport`. The `passed` flag is set at the end based
on the presence of any errors.

## Consequences

- A single `validate()` call returns the complete diagnostic picture.
- Each phase is a focused private method (`checkStructure`, `checkSchema`, `checkSemantics`),
  which is independently unit-testable via stub injection.
- Structural checks are extended to cover FMI version-specific binary directory naming
  conventions (FMI 2.0 vs FMI 3.0), preventing silent cross-version misuse.
