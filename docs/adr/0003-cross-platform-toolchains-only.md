# ADR 0003 — Cross-Platform Toolchains Only (No MSVC)

**Status:** Accepted  
**Date:** 2026-04-15

## Context

The library must build on both Windows and Linux. Qt6's official Windows packages are built
with MinGW, not MSVC. Maintaining MSVC-specific workarounds (MSVC STL quirks, DLL export
macros, Windows SDK paths) adds maintenance burden without adding value for the target use case.

## Decision

Support only **GCC and Clang** via MSYS2/MinGW on Windows and native toolchains on Linux.
Remove all MSVC presets from `CMakePresets.json` and `pm_select_preset.bat`.

## Consequences

- Windows users must install MSYS2 (`C:/eng_apps/msys64/`).
- Qt6 integration is straightforward: both the library and Qt are built with the same MinGW
  toolchain, eliminating runtime library mismatches.
- MSVC users cannot build the library. This is an acceptable trade-off given the project's
  target audience (embedded simulation, Qt GUI applications).
