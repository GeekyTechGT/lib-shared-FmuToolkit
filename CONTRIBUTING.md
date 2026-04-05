# Contributing

## Development workflow

1. Configure with a preset.
2. Build the project.
3. Run tests.
4. Run formatting and static analysis before opening a pull request.
5. Update documentation when behavior, build flow, or repository structure changes.

## Recommended commands

### CLI flow

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

### Windows flow

```bat
project_manager.bat
```

## Coding guidelines

- Keep the public API minimal and stable.
- Prefer PIMPL when ABI stability or rebuild isolation matters.
- Keep headers lightweight.
- Add tests for behavioral changes.
- Avoid introducing platform-specific assumptions into the public interface.
- Keep build policy centralized in `cmake/` modules where practical.

## Pull requests

- Keep changes scoped.
- Update documentation when behavior changes.
- Add or update tests when fixing bugs or adding features.
- Keep examples aligned with the installed-consumer story when public API changes.
