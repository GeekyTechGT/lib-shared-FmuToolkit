# `your_library` scaffold

Professional scaffold for modern C++23 libraries with:

- target-based CMake
- portable `CMakePresets.json`
- `vcpkg` support in manifest mode
- a real PIMPL-based example
- install/export package support for `find_package(...)`
- unit tests with GoogleTest
- strict warnings and optional sanitizers
- project governance, documentation, CI, Docker, and a Windows project manager

## What is in the repository

```text
.
|-- .github/
|-- cmake/
|-- docs/
|-- examples/
|-- include/yourlib/
|-- scripts/
|-- src/
|-- tests/
|-- .clang-format
|-- .clang-tidy
|-- .clangd
|-- .editorconfig
|-- .gitattributes
|-- .gitignore
|-- CHANGELOG.md
|-- CMakeLists.txt
|-- CMakePresets.json
|-- CMakeUserPresets.json.example
|-- CODE_OF_CONDUCT.md
|-- CONTRIBUTING.md
|-- Dockerfile
|-- LICENSE
|-- README.md
|-- SECURITY.md
|-- project_manager.bat
`-- vcpkg.json
```

## Repository inventory and actual use

### Build and packaging

- [CMakeLists.txt](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CMakeLists.txt): defines the `your_library` target, install rules, exported package config, generated version header, and optional test build.
- [CMakePresets.json](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CMakePresets.json): standard configure, build, and test presets for local development on Linux, macOS, and Windows.
- [CMakeUserPresets.json.example](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CMakeUserPresets.json.example): template for machine-local overrides that should not be committed.
- [cmake/ProjectOptions.cmake](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/cmake/ProjectOptions.cmake): applies warnings and sanitizers consistently to library and tests.
- [cmake/CompilerWarnings.cmake](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/cmake/CompilerWarnings.cmake): central warning profile for MSVC and non-MSVC builds.
- [cmake/Sanitizers.cmake](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/cmake/Sanitizers.cmake): enables ASan and UBSan for supported debug builds.
- [cmake/your_libraryConfig.cmake.in](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/cmake/your_libraryConfig.cmake.in): package config template used by `cmake --install` so downstream consumers can call `find_package(your_library REQUIRED)`.

### Library code

- [include/yourlib/yourlib.hpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/include/yourlib/yourlib.hpp): public API, currently a `Greeter` example.
- [include/yourlib/export.hpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/include/yourlib/export.hpp): export/import macros for shared-library builds on Windows.
- [include/yourlib/version.hpp.in](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/include/yourlib/version.hpp.in): template for the generated version header produced at configure time.
- [src/yourlib.cpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/src/yourlib.cpp): implementation of the public API and copy/move semantics.
- [src/yourlib_impl.hpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/src/yourlib_impl.hpp): private PIMPL definition hidden from consumers.

### Tests and examples

- [tests/CMakeLists.txt](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/tests/CMakeLists.txt): builds the GoogleTest-based test target and discovers tests with CTest.
- [tests/tests_greeter.cpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/tests/tests_greeter.cpp): verifies the example API behavior.
- [examples/minimal/CMakeLists.txt](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/examples/minimal/CMakeLists.txt): demonstrates consumer-side `find_package` integration.
- [examples/minimal/main.cpp](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/examples/minimal/main.cpp): minimal executable using the installed library.
- [examples/README.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/examples/README.md): explains what the example proves and how to use it.

### Project orchestration and developer tooling

- [project_manager.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/project_manager.bat): interactive Windows entry point for configure, build, rebuild, test, format, lint, deploy, and custom vcpkg-registry tasks.
- [scripts/pm_select_preset.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_select_preset.bat): preset selection helper used by the manager.
- [scripts/pm_configure.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_configure.bat): runs `cmake --preset`.
- [scripts/pm_build.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_build.bat): runs `cmake --build --preset`.
- [scripts/pm_rebuild.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_rebuild.bat): cleans the build directory, then reconfigures and rebuilds.
- [scripts/pm_clean.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_clean.bat): deletes a requested directory.
- [scripts/pm_test.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_test.bat): runs `ctest --preset`.
- [scripts/pm_pipeline.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_pipeline.bat): configure, build, and test in one flow.
- [scripts/pm_format.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_format.bat): formats code under `include/`, `src/`, `tests/`, and `examples/` with `clang-format`.
- [scripts/pm_lint.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_lint.bat): runs `clang-tidy` against project sources using the selected build directory.
- [scripts/pm_deploy.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_deploy.bat): installs the configured build into `deploy/<preset>` via `cmake --install`.
- [scripts/pm_status.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_status.bat): reports whether current build and deploy folders exist.
- [scripts/pm_vcpkg_status.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_vcpkg_status.bat): shows the metadata and local configuration used to publish the library to a custom vcpkg registry.
- [scripts/pm_vcpkg_init_registry.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_vcpkg_init_registry.bat): initializes the required `ports/`, `versions/`, and `baseline.json` layout in a target Git registry repository.
- [scripts/pm_vcpkg_publish_registry.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/pm_vcpkg_publish_registry.bat): renders the port files into the target registry and runs `vcpkg x-add-version` against that registry.
- [scripts/vcpkg_registry_config.example.bat](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/scripts/vcpkg_registry_config.example.bat): template for local registry publishing configuration.

### Vcpkg registry packaging assets

- [packaging/vcpkg/portfile.cmake.in](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/packaging/vcpkg/portfile.cmake.in): template for the registry portfile that fetches the library source and installs it with CMake.
- [packaging/vcpkg/vcpkg.json.in](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/packaging/vcpkg/vcpkg.json.in): template for the registry port manifest.
- [packaging/vcpkg/usage.in](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/packaging/vcpkg/usage.in): usage text installed with the port.
- [packaging/vcpkg/vcpkg-configuration.git.example.json](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/packaging/vcpkg/vcpkg-configuration.git.example.json): consumer-side example for referencing a custom Git registry.

### Automation and repository policy

- [Dockerfile](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/Dockerfile): Ubuntu-based developer container with compilers, CMake, Ninja, and clang tools.
- [vcpkg.json](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/vcpkg.json): manifest-mode dependency definition, currently declaring `gtest`.
- [.github/workflows/ci.yml](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/workflows/ci.yml): configures, builds, and tests on Ubuntu using the Linux GCC debug preset.
- [.github/workflows/release.yml](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/workflows/release.yml): placeholder release workflow to be specialized by the concrete library.
- [.github/dependabot.yml](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/dependabot.yml): update automation for GitHub-managed dependencies.
- [.github/ISSUE_TEMPLATE/bug_report.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/ISSUE_TEMPLATE/bug_report.md): bug-report template.
- [.github/ISSUE_TEMPLATE/feature_request.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/ISSUE_TEMPLATE/feature_request.md): feature-request template.
- [.github/pull_request_template.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.github/pull_request_template.md): pull-request checklist and structure.

### Repository defaults and governance

- [.clang-format](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.clang-format): formatting policy used by `pm_format.bat`.
- [.clang-tidy](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.clang-tidy): static-analysis policy used by `pm_lint.bat`.
- [.clangd](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.clangd): editor integration defaults for clangd-based IDEs.
- [.editorconfig](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.editorconfig): cross-editor whitespace and line-ending conventions.
- [.gitattributes](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.gitattributes): repository-wide Git file handling rules.
- [.gitignore](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/.gitignore): ignores generated local files such as builds and IDE artifacts.
- [CHANGELOG.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CHANGELOG.md): human-readable project history.
- [CONTRIBUTING.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CONTRIBUTING.md): contributor workflow and expectations.
- [SECURITY.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/SECURITY.md): vulnerability reporting policy.
- [CODE_OF_CONDUCT.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/CODE_OF_CONDUCT.md): collaboration standards.
- [LICENSE](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/LICENSE): legal distribution terms.

## Quick start

### Standard CLI flow

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

### Windows project manager flow

Run:

```bat
project_manager.bat
```

The manager is intended as the Windows entry point for common local workflows. Each menu action delegates to a dedicated script under `scripts/`, including the custom vcpkg-registry helpers for init and publish.

### Publish to a custom vcpkg registry

1. Copy `scripts/vcpkg_registry_config.example.bat` to `scripts/vcpkg_registry_config.bat`.
2. Set the target registry repository path, the source repository URL, and the source ref/tag.
3. Initialize the target registry layout with `scripts\pm_vcpkg_init_registry.bat` or the corresponding project-manager menu option.
4. Publish the port with `scripts\pm_vcpkg_publish_registry.bat`.

The publish script renders `packaging/vcpkg/*` into `ports/<port-name>/` in the target registry and then runs `vcpkg x-add-version` using that registry's `ports/` and `versions/` directories.

### Build with vcpkg

Prerequisite: set `VCPKG_ROOT` to your local `vcpkg` installation root.

```bash
cmake --preset vcpkg-debug
cmake --build --preset vcpkg-debug
ctest --preset vcpkg-debug
```

### Build inside Docker

```bash
docker build -t your-library-dev .
docker run --rm -it -v "$(pwd):/workspace" -w /workspace your-library-dev
```

Inside the container:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

## Presets

- `dev-debug`
- `dev-release`
- `vcpkg-debug`
- `vcpkg-release`
- `linux-gcc-debug`
- `linux-clang-debug`
- `linux-gcc-vcpkg-debug`
- `linux-clang-vcpkg-debug`
- `macos-clang-debug`
- `macos-clang-vcpkg-debug`
- `windows-msvc-debug`
- `windows-msvc-vcpkg-debug`
- `windows-mingw-debug`
- `windows-mingw-vcpkg-debug`

## Install and consume

Install from a configured build tree:

```bash
cmake --install build/dev-release
```

A downstream consumer can then use:

```cmake
find_package(your_library REQUIRED)
target_link_libraries(app PRIVATE your_library::your_library)
```

The installed-package pattern is demonstrated by [examples/minimal/CMakeLists.txt](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/examples/minimal/CMakeLists.txt).

## Documentation map

- [docs/README.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/README.md): documentation index and scope
- [docs/architecture.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/architecture.md): repository and code structure
- [docs/design.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/design.md): design rationale behind the example API and build choices
- [docs/build.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/build.md): build, test, tooling, and deploy workflows
- [docs/dependencies.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/dependencies.md): dependency and tool inventory
- [docs/release.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/release.md): release and packaging expectations
- [docs/specs/README.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/specs/README.md): specs index
- [docs/specs/library-spec.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/specs/library-spec.md): functional and non-functional expectations
- [docs/adr/README.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/adr/README.md): ADR index
- [docs/adr/0001-use-pimpl.md](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/docs/adr/0001-use-pimpl.md): accepted decision to use PIMPL in the example API

## Reuse checklist

Rename these elements consistently when cloning the scaffold:

1. CMake project name: `your_library`
2. C++ namespace: `yourlib`
3. Public headers directory: `include/yourlib`
4. Exported target: `your_library::your_library`
5. Export/version macros: `YOUR_LIBRARY_*`
6. `vcpkg` manifest package name and dependencies
7. Documentation placeholders, governance contacts, and release workflow details
