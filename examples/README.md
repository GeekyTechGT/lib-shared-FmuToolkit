# Examples

The examples directory exists to prove consumer-facing usage, not to duplicate unit tests.

## Current example

- `minimal/`: a downstream-style CMake project that calls `find_package(your_library REQUIRED)` and links `your_library::your_library`

## What the minimal example proves

- install/export metadata is generated correctly
- the package can be found by another CMake project
- the public header layout is usable from a consumer executable

## Typical usage

1. Configure and build the library.
2. Install it with `cmake --install` or with the deploy option from `project_manager.bat`.
3. Point a consumer project at the installed package prefix.
4. Build the example or an equivalent downstream application.
