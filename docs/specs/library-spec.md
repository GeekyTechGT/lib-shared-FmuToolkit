# Library Specification

## Scope

The scaffold provides a small but complete example of a reusable C++ library repository. It is expected to demonstrate:

- a public API under `include/`
- a hidden implementation under `src/`
- build, test, and install support through CMake
- downstream consumption through exported package metadata
- Windows project orchestration through `project_manager.bat`

## Functional requirements

- The repository shall build a library target named `your_library`.
- The repository shall export an alias target `your_library::your_library`.
- The public `Greeter` API shall allow greeting a name with a configurable prefix.
- The repository shall provide at least one automated unit test for the public API.
- The repository shall support `cmake --install` for downstream consumption.
- The repository shall include at least one consumer example that uses `find_package(your_library REQUIRED)`.

## Non-functional requirements

- The project shall require CMake 3.25 or newer.
- The project shall target C++23.
- The public API shall remain separated from implementation details through PIMPL in the example.
- Warning and sanitizer policy shall be centralized in reusable CMake modules.
- The repository shall document developer workflows and repository purpose.
- Local Windows development shall have a single menu-driven orchestration entry point.
