# Design Notes

## Why the example uses PIMPL

The `Greeter` type is intentionally small, but it demonstrates a pattern that scales to larger libraries:

- public headers remain lightweight
- implementation details are hidden from consumers
- rebuild impact is reduced when internals change
- ABI-sensitive libraries get a cleaner separation point

## Copy and move semantics

`Greeter` implements:

- explicit move construction and move assignment
- deep-copy construction and copy assignment

This is useful in a scaffold because it shows how owning PIMPL types can still offer normal value semantics.

## Export and install design

The repository includes:

- Windows export macros in `include/yourlib/export.hpp`
- generated package config files from `cmake/your_libraryConfig.cmake.in`
- `install()` rules in the root `CMakeLists.txt`

Together, these show the intended path from local build to downstream reuse.

## Warning and sanitizer policy

Compiler warnings and sanitizers are split into dedicated CMake modules instead of being inlined in the root build script. That keeps target declaration readable and makes policy reuse straightforward.

## Example scope

The example is intentionally narrow:

- one public type
- one source implementation unit
- one private implementation header
- one test executable
- one consumer example

That keeps the scaffold easy to rename and adapt while still proving all major infrastructure paths.
