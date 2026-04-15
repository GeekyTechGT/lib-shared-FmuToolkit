# ADR 0005 — ABI Stability for a Source-Distributed C++ Library

**Status:** Accepted  
**Date:** 2026-04-15

## Context

A code review identified several patterns that are hazardous for binary-distributed C++ shared
libraries:

- `std::string`, `std::vector`, `std::map` in public API signatures and return types  
- `std::variant` and `std::function` as parameters  
- Inline methods in public headers that access STL container internals  
- No explicit `_GLIBCXX_USE_CXX11_ABI` pin  

These patterns cause **heap corruption, silent UB, or crashes** when the library and its consumer
are compiled with different STL implementations or different GCC ABI settings.

## Context Qualifier — Source Distribution via vcpkg

`lib-fmu-toolkit` is distributed as source via vcpkg (consumers recompile the library from
source). This eliminates the most severe binary-compatibility hazards:

- The consumer always uses the **same STL implementation** as the library (same `libstdc++`
  headers, same ABI version) because they compile from the same source tree.
- There are no precompiled binaries to mismatch against.

A full C-ABI (`extern "C"`) redesign is therefore **not required** at this time.

## Decision

Apply four targeted mitigations that cover the real risk for a source-distributed library while
preserving a modern, ergonomic C++20 API:

### 1 — Move inline STL-accessing methods out of headers

`ValidationResult::hasErrors()`, `ValidationResult::hasWarnings()`,
`SimResult::stepCount()`, and `SimResult::empty()` are moved from inline definitions in
`types.hpp` to out-of-line definitions in `fmu_engine.cpp`.

**Why:** Inline methods in public headers are compiled into the **caller's** translation unit.
If caller and library ever diverge in STL version (e.g., mixed vcpkg + system packages, or a
future binary distribution), the caller would execute `std::vector::empty()` using its own STL
layout on a vector filled by the library's STL layout. Moving the body to the library ensures
only one copy of the code executes, using the library's STL layout consistently.

### 2 — Replace `const std::string&` input parameters with `std::string_view`

All input-only string parameters in the public API (`FmuEngine::load`,
`FmuEngine::exportCsv/Json`, `FmuSession::setValue`, `FmuSession::getValue`,
`FmuInfo::findVariable`, `FmuEngine` constructor) are changed to `std::string_view`.

**Why:** `std::string_view` is a non-owning reference to a contiguous char sequence. Its layout
(`pointer + size`) is trivial and identical across all C++ implementations. It avoids:
- The copy that `const std::string&` forces when called with a string literal.
- The STL layout dependency when passing a `std::string` from one translation unit to another
  (the `std::string` stays in the caller, only its `.data()` and `.size()` cross the boundary).

**Internal map lookups** are updated to use `std::less<>` (transparent comparator) on the
`FmuSession::Impl::varIndex` map, enabling `std::map::find(std::string_view)` without
constructing a temporary `std::string` key.

### 3 — SONAME versioning (already present, confirmed)

`CMakeLists.txt` already sets `VERSION "${PROJECT_VERSION}"` and
`SOVERSION "${PROJECT_VERSION_MAJOR}"` on both shared library targets. This ensures that if a
binary distribution is ever added, the OS dynamic linker enforces major-version consistency.
`write_basic_package_version_file` is configured with `SameMajorVersion` compatibility.

### 4 — Pin `_GLIBCXX_USE_CXX11_ABI=1`

Added as a `PUBLIC` compile definition for all non-MSVC targets.

**Why:** GCC ≥ 5 defaults to `_GLIBCXX_USE_CXX11_ABI=1` (C++11 SSO string ABI), but the
default can be overridden system-wide or per-project on older distributions. If the library is
compiled with ABI=1 and a consumer with ABI=0, any `std::string` passed across the boundary
(even via pointer/reference) has a different memory layout → **heap corruption**. Pinning it
explicitly and propagating via CMake's transitive dependency system ensures consistency without
requiring consumer action.

## Consequences

- The public API now accepts `std::string`, `std::string_view`, and string literals uniformly
  at call sites — no source changes required for existing callers that pass `std::string`.
- Methods like `hasErrors()` and `stepCount()` are no longer `constexpr`/`inline`, but this
  has no measurable performance impact (they are trivial delegating wrappers).
- Future binary distribution of precompiled artifacts is safer, but would still require a full
  C-ABI layer (`c_api.h`) to be robust across MSVC or libc++ consumers.
- If MSVC support is ever reinstated (reverting ADR-0003), a full C-ABI redesign will be
  required. This is explicitly deferred pending that decision.
