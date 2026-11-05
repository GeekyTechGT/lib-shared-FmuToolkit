# FmuToolkit - Software Requirements Specification

| | |
|---|---|
| **Project** | FmuToolkit (`lib-shared-FmuToolkit`) |
| **Category** | `lib-shared` - dynamically linked library (`.dll`/`.so`) |
| **Version** | Tracks `project.json`'s `version` (also `SOVERSION`) |
| **Status** | Draft |
| **Owner** | _fill in_ |

## 1. Introduction

### 1.1 Purpose

This Software Requirements Specification (SRS) defines the functional and
non-functional requirements for FmuToolkit. It is the reference other
engineers - and any AI assistant working in this repository - use to
understand *what* the library must do and *how well* it must do it, as
distinct from *how* it does it (see
[`docs/architecture/architecture.md`](../architecture/architecture.md) and
[`CLAUDE.md`](../../CLAUDE.md) for the *how*). For a library, the public
header is the requirement - every behaviour promised here should be
observable through `include/fmutoolkit/` alone.

### 1.2 Scope

In scope: the public API surface under `include/fmutoolkit/` and the runtime
behaviour of `libfmutoolkit.dll`/`.so`. Out of scope: internal, non-exported
implementation details (free to change between patch versions without
notice), and any consuming project's own requirements.

### 1.3 Definitions, Acronyms, Abbreviations

| Term | Meaning |
|---|---|
| FR | Functional Requirement |
| NFR | Non-Functional Requirement |
| ABI | Application Binary Interface - the contract a rebuilt DLL must preserve for existing consumer binaries |
| PIMPL | Pointer-to-Implementation idiom |
| SemVer | Semantic Versioning (`MAJOR.MINOR.PATCH`) |

### 1.4 References

- [`docs/architecture/architecture.md`](../architecture/architecture.md) - how the system is built
- [`docs/adr/adr.md`](../adr/adr.md) - why it's built that way
- [`CLAUDE.md`](../../CLAUDE.md) - conventions this repository follows
- [`CHANGELOG.md`](../../CHANGELOG.md) - what changed, and when (record every API/ABI-relevant change here)

## 2. Overall Description

### 2.1 Product Perspective

FmuToolkit is a reusable component, not a standalone product - it is
built to be linked into other applications, tools, or services generated
by this same scaffold (or any CMake consumer). It has no inbound adapters
(no CLI, no GUI) by design; every requirement here is a promise made to a
future consumer, not to an end user.

### 2.2 Product Functions

<Summarize the library's public capabilities as a bullet list - what can a
consumer *do* by linking this library, in one line each.>

### 2.3 User and Actor Classes

| Actor | Description |
|---|---|
| Consuming project | Any `app`/`tool`/`svc-*` project declaring this library in its `project.json` `dependencies[]`. Almost every requirement's real actor. |
| Maintainer | Extends the library; consumes this document and `CLAUDE.md` together. |

### 2.4 Operating Environment

Windows (MinGW) and Linux (system GCC) via CMake presets; no Qt or GUI
framework dependency unless explicitly added. Consumers link the imported
target this scaffold's `Dependencies.cmake` creates from the deployed
`bin/`/`lib`/`include/` layout.

### 2.5 Design and Implementation Constraints

- Must follow the hexagonal layering and category rules in `CLAUDE.md` -
  no inbound adapters, symbol visibility hidden by default
  (`FMUTOOLKIT_EXPORT` only).
- The public header is a contract: a breaking change bumps the major
  version (see NFR-COMPAT.1) and is called out explicitly in
  `CHANGELOG.md`.

### 2.6 Assumptions and Dependencies

- Consumers build with a compatible C++ ABI (see NFR-COMPAT.2 if the
  library pins a specific `_GLIBCXX_USE_CXX11_ABI` or similar).

## 3. Functional Requirements

Each functional requirement has a stable identifier (`FR-<area>.<n>`, never
reused or renumbered once published - a superseded requirement is marked
"Superseded by FR-x.y", not deleted or renumbered) so it can be traced to
the source that implements it and the test(s) that verify it (see
[Traceability](#6-traceability)). Add new requirements below using this
same shape:

> **FR-\<area>.\<n> - \<short title>**
> **Description:** what the library must do, stated as a single testable behaviour, observable through the public header.
> **Rationale:** why this requirement exists.
> **Acceptance Criteria:** a bullet list of observable, checkable conditions.
> **Priority:** Must / Should / Could (MoSCoW).
> **Status:** Proposed / Approved / Implemented / Verified.

### 3.1 Public API (worked example)

> **FR-API.1 - \<Core capability name>**
> **Description:** \<what calling the public API accomplishes>.
> **Rationale:** \<why a consumer needs this>.
> **Acceptance Criteria:**
> - \<observable behaviour 1, expressed as an example call and its result>
> - \<error behaviour: what happens on invalid input>
> **Priority:** Must
> **Status:** Proposed

### 3.2 \<Next area - add your own>

## 4. Non-Functional Requirements

Grouped by quality characteristic (ISO/IEC 25010); the same identifier
scheme as functional requirements, prefixed `NFR-<characteristic>.<n>`.

### 4.1 Performance Efficiency

_No requirements yet - add them here as they arise (e.g. a maximum call
latency, an allocation budget on a hot path)._

### 4.2 Reliability

> **NFR-REL.1 - No undefined behaviour on invalid input**
> **Description:** Every public entry point either succeeds or reports a well-defined error (exception/error code) for invalid input; it never invokes undefined behaviour.
> **Rationale:** A library crash or memory-safety bug takes down its consumer, not just itself.
> **Acceptance Criteria:** Fuzzing/boundary tests on every public entry point find no UB (ASan/UBSan clean).
> **Priority:** Must
> **Status:** Proposed

### 4.3 Usability

> **NFR-USE.1 - Self-explanatory public headers**
> **Description:** Every exported symbol carries a Doxygen comment explaining its contract (preconditions, postconditions, thread-safety).
> **Rationale:** The public header is the primary documentation surface for consumers.
> **Acceptance Criteria:** `doxygen Doxyfile` produces no "undocumented" warnings for public symbols.
> **Priority:** Should
> **Status:** Proposed

### 4.4 Security

_No requirements yet - add them here as they arise (e.g. safe handling of
untrusted input passed across the ABI boundary)._

### 4.5 Maintainability

_No requirements yet - add them here as they arise (e.g. minimum test
coverage for `src/`)._

### 4.6 Compatibility

> **NFR-COMPAT.1 - Semantic Versioning across the ABI**
> **Description:** A binary-incompatible change (removed/changed exported symbol, changed struct layout in a public header) bumps `project.json`'s major version; a backward-compatible addition bumps minor; a bug fix with no API/ABI change bumps patch.
> **Rationale:** Consumers pin an exact version in their own `project.json` and rely on this contract to know when it is safe to bump.
> **Acceptance Criteria:** `CHANGELOG.md` states which kind of change each release contains; the version bump matches.
> **Priority:** Must
> **Status:** Proposed

> **NFR-COMPAT.2 - Consistent C++ ABI**
> **Description:** \<state the pinned ABI switch, if any, e.g. `_GLIBCXX_USE_CXX11_ABI`> is applied consistently and propagated `PUBLIC` so consumers cannot silently mismatch it.
> **Rationale:** An ABI mismatch on `std::string`/`std::vector` layout causes silent heap corruption, not a link error.
> **Acceptance Criteria:** The ABI define is set `PUBLIC` on the library's CMake target.
> **Priority:** Must
> **Status:** Proposed

## 5. Constraints

- No inbound adapters, ever - a CLI or GUI need belongs in a separate
  `app`/`tool` project depending on this library (see `CLAUDE.md`).
- The CMake package config (`find_package()` support) is an opt-in install
  component, not the primary consumption path - do not make a requirement
  depend on it being installed by default.

## 6. Traceability

As the requirement set grows, maintain a traceability matrix (requirement
ID -> implementing source file(s) -> verifying test(s)) alongside this
document, e.g. `docs/requirements/traceability.md`. For a small library, a
`@req FR-x.y` reference in the relevant public header's own Doxygen comment,
grep-able on demand, is a lighter-weight alternative - pick one convention
and apply it consistently.

## Appendix A: Requirement ID Prefixes Used In This Document

| Prefix | Area |
|---|---|
| `FR-API` | Public API |
| `NFR-REL` | Reliability |
| `NFR-USE` | Usability |
| `NFR-COMPAT` | Compatibility (API/ABI/SemVer) |
