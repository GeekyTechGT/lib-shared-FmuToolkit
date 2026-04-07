# Architecture Overview

## Purpose

This repository is a scaffold for reusable C++23 libraries.
Its infrastructure is designed so a user can add implementation files under `src/` and test files under `tests/` without editing the build-system file lists.

## Layers

- `include/yourlib/`: installable public headers
- `src/`: private implementation and internal headers
- `tests/`: unit tests automatically collected by the test target
- `examples/`: consumer-facing example using the exported package target
- `cmake/`: package config, compiler options, sanitizers, and chainload toolchains
- `triplets/`: overlay triplets for compiler-specific `vcpkg` consumption
- `packaging/vcpkg/`: templates used to publish the library to a private registry

## Public And Private Boundaries

The scaffold keeps the installable header surface intentionally minimal.
Users should place distributable headers under `include/yourlib/` and keep private implementation details under `src/`.

## Zero-Touch Source Discovery

The top-level `CMakeLists.txt` recursively discovers library sources under `src/`.
`tests/CMakeLists.txt` recursively discovers test sources under `tests/`.

This is a deliberate scaffold tradeoff: it removes the need to maintain file lists when onboarding a new library into the template.
