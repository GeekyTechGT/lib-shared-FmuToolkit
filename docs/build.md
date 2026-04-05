# Build Guide

## Primary entry points

Use one of these two workflows:

- direct CLI commands with `cmake`, `cmake --build`, and `ctest`
- `project_manager.bat` on Windows for an interactive menu-driven flow

## Preset-based CLI workflow

Standard local debug flow:

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

Release-oriented local build:

```bash
cmake --preset dev-release
cmake --build --preset dev-release
```

## Windows project manager workflow

Run:

```bat
project_manager.bat
```

The menu delegates to these scripts under `scripts/`:

- `pm_select_preset.bat`
- `pm_configure.bat`
- `pm_build.bat`
- `pm_rebuild.bat`
- `pm_clean.bat`
- `pm_test.bat`
- `pm_pipeline.bat`
- `pm_format.bat`
- `pm_lint.bat`
- `pm_deploy.bat`
- `pm_status.bat`
- `pm_vcpkg_status.bat`
- `pm_vcpkg_init_registry.bat`
- `pm_vcpkg_publish_registry.bat`

## Presets

The repository ships presets for:

- generic developer debug and release builds
- `vcpkg`-backed builds
- Linux GCC and Clang builds
- macOS Clang builds
- Windows MSVC and MinGW builds

Use `CMakePresets.json` as the canonical source of available preset names.

## Tooling expectations

Required for normal build/test work:

- CMake 3.25+
- a C++23-capable compiler
- Ninja for the preset-driven workflows in this scaffold

Optional but supported:

- `vcpkg` through `VCPKG_ROOT`
- `clang-format` for formatting
- `clang-tidy` for linting
- Docker for Linux-containerized builds

## Tests

Tests are enabled by default through `YOUR_LIBRARY_BUILD_TESTS=ON` in the base preset. The test target is built from `tests/tests_greeter.cpp` and registered with CTest.

## Install and deploy

The repository supports CMake install rules.

CLI install example:

```bash
cmake --install build/dev-release
```

Windows manager deploy flow:

- configure and build a preset
- run the deploy option
- artifacts are installed into `deploy/<preset>`

The deploy action is a thin wrapper around `cmake --install`.

## vcpkg flow

When using `vcpkg-*` presets, CMake loads:

```cmake
$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
```

The manifest file is [vcpkg.json](/mnt/d/GeekyTechRepos/cpp_lib_skaffold/vcpkg.json).

## Docker flow

Build the image:

```bash
docker build -t your-library-dev .
```

Start a shell in the container:

```bash
docker run --rm -it -v "$(pwd):/workspace" -w /workspace your-library-dev
```

Then run the normal preset workflow inside the container.

## Custom vcpkg registry publishing

The repository now contains the files required to publish this library to a separate Git-based vcpkg registry:

- `packaging/vcpkg/portfile.cmake.in`
- `packaging/vcpkg/vcpkg.json.in`
- `packaging/vcpkg/usage.in`
- `scripts/vcpkg_registry_config.example.bat`
- `scripts/pm_vcpkg_init_registry.bat`
- `scripts/pm_vcpkg_publish_registry.bat`

The expected flow is:

1. Configure `scripts/vcpkg_registry_config.bat` locally.
2. Ensure the target registry repo is cloned locally and is a Git repository.
3. Run the init script once for the registry layout.
4. Run the publish script to render the port and update `versions/` via `vcpkg x-add-version`.
