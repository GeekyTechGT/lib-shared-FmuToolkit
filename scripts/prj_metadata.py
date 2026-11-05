"""Shared helpers for the runtime scripts copied into every generated CMake
project (prj_exec.py, prj_run_tests.py, prj_run_examples.py, build_installer.py,
debug_env.py). Operates on *this* project's own project.json - never on the
generator's.
"""

from __future__ import annotations

import os
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

from prj_models import ProjectModel, ProjectModelError, load_project_model

PROJECT_ROOT = Path(__file__).absolute().parent.parent


def run_cli_main(main) -> None:
    """Every script's `if __name__ == "__main__":` line should read
    `run_cli_main(main)` instead of calling `main()` directly - the one place
    Ctrl+C is handled for all of them. A build/configure/deploy step can run
    for minutes; without this, Ctrl+C during one dumps a raw Python
    traceback (`KeyboardInterrupt`) instead of just stopping. Piped/closed
    stdin at an interactive `Prompt.ask()` raises `EOFError` instead - the
    same "just stop cleanly" outcome applies, so it is caught alongside
    `KeyboardInterrupt`. `main`'s return value is still used as the process
    exit code, exactly as calling it directly would (`None` -> 0); a
    `sys.exit()`/`SystemExit` raised from inside `main` itself is left
    untouched."""
    try:
        raise SystemExit(main())
    except (KeyboardInterrupt, EOFError):
        print("\nInterrupted by user (Ctrl+C).", file=sys.stderr)
        raise SystemExit(130) from None


def find_docker_executable() -> str | None:
    return shutil.which("docker")


def find_docker_executable_or_fail() -> str:
    docker = find_docker_executable()
    if docker is None:
        raise RuntimeError("Docker was not found on PATH. Install Docker Desktop and re-run.")
    return docker


def ensure_docker_daemon_running() -> None:
    docker = find_docker_executable_or_fail()
    result = subprocess.run([docker, "info"], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            "Docker is installed but the daemon does not respond. "
            "Start Docker Desktop and re-run this command.\n" + (result.stderr or "")
        )


def docker_environment() -> dict[str, str]:
    env = os.environ.copy()
    env.setdefault("DOCKER_BUILDKIT", "1")
    return env


def find_inno_setup_compiler() -> str | None:
    if platform.system() != "Windows":
        return None
    default_path = Path("C:/Program Files (x86)/Inno Setup 6/ISCC.exe")
    if default_path.exists():
        return str(default_path)
    return shutil.which("ISCC.exe") or shutil.which("iscc")


def load_metadata(project_root: Path | None = None) -> ProjectModel:
    root = project_root or PROJECT_ROOT
    try:
        return load_project_model(root / "project.json")
    except ProjectModelError as exc:
        raise RuntimeError(str(exc)) from exc


def default_preset(project: ProjectModel) -> str:
    del project  # not currently used, kept for signature stability
    return "windows-mingw64-debug" if platform.system() == "Windows" else "linux-gcc-debug"


def build_directory(project_root: Path, preset: str) -> Path:
    return project_root / "build" / preset


def stale_build_version(project_root: Path, preset: str, project: ProjectModel) -> str | None:
    """Compares the version baked into the build tree's generated version
    header against project.json's current version. Returns the stale (baked)
    version if they differ, or None if the build is up to date / not found.
    """
    # The header lives under generated/include/<snake>/ - located by glob
    # rather than derived from project.name, which is not guaranteed to map
    # 1:1 onto the C++ identifier.
    headers = sorted((build_directory(project_root, preset) / "generated" / "include").glob("*/version.hpp"))
    if not headers:
        return None
    version_header = headers[0]

    text = version_header.read_text(encoding="utf-8")
    marker = 'kVersion = "'
    start = text.find(marker)
    if start == -1:
        return None
    start += len(marker)
    end = text.find('"', start)
    if end == -1:
        return None

    baked_version = text[start:end]
    return baked_version if baked_version != project.version else None


def artifact_name(project: ProjectModel) -> str:
    """Folder name under deploy_folder - the name consumers declare in their
    own project.json dependencies (e.g. "lib-shared-QtCustomWidgets")."""
    return project.artifact_name or project.name


def deploy_directory(project_root: Path, preset: str, project: ProjectModel) -> Path:
    if project.environment == "dev":
        return project_root / "deploy" / preset
    return Path(project.deploy_folder) / artifact_name(project) / project.version / preset


def has_gui(project: ProjectModel) -> bool:
    """True when the project builds a Qt GUI executable (app, app-qml, tool
    gui/both)."""
    if project.category in {"app", "app-qml"}:
        return True
    return project.category == "tool" and project.tool_variant in {"gui", "both"}


def gui_executable_name(project: ProjectModel) -> str | None:
    """File name of the GUI executable under bin/ (OUTPUT_NAME in
    apps/gui/CMakeLists.txt): "<name>" for app, "<name>-gui" for tool."""
    if not has_gui(project):
        return None
    stem = f"{project.name}-gui" if project.category == "tool" else project.name
    return f"{stem}.exe" if platform.system() == "Windows" else stem


def main_executable_name(project: ProjectModel) -> str:
    """The executable an installer shortcut should launch: the GUI when
    there is one, otherwise the CLI ("<name>")."""
    gui = gui_executable_name(project)
    if gui:
        return gui
    return f"{project.name}.exe" if platform.system() == "Windows" else project.name


def ensure_deploy_directory(path: Path) -> str | None:
    """Creates `path` (with parents). Returns None on success, or a clear,
    actionable message instead of letting a raw OSError/FileNotFoundError
    traceback surface - the most common real-world cause is deploy_folder
    (project.json) pointing at a drive/share that isn't currently mounted
    on this machine."""
    try:
        path.mkdir(parents=True, exist_ok=True)
        return None
    except OSError as exc:
        return (
            f"Could not create deploy directory '{path}': {exc}. "
            "Check that project.json's deploy_folder points to a drive/path "
            "that exists and is reachable on this machine."
        )


def runtime_environment(project_root: Path, preset: str, project: ProjectModel) -> dict[str, str]:
    """Environment for running this project's own binaries (tests, examples)
    outside a deploy folder. On Windows (no rpath) PATH is extended with the
    build tree's lib/ (a lib-shared DLL) and the toolchain runtime folders
    declared in project.json's runtimes[<preset>] (libstdc++, libgcc, ...)."""
    env = os.environ.copy()
    if platform.system() != "Windows":
        return env

    extra = [str(build_directory(project_root, preset) / "lib")]
    runtime = project.runtimes.get(preset)
    if runtime:
        extra += [path.strip() for path in runtime.source_path.split(",") if path.strip()]
    env["PATH"] = os.pathsep.join([*extra, env.get("PATH", "")])
    return env


def python_command(project_root: Path) -> list[str]:
    venv_dir = project_root / ".venv"
    venv_python = (
        venv_dir / "Scripts" / "python.exe" if platform.system() == "Windows" else venv_dir / "bin" / "python"
    )
    if venv_python.exists():
        return [str(venv_python)]
    return [sys.executable]


def is_docker_preset(preset: str) -> bool:
    return preset.startswith("docker-")


def docker_run_command(project_root: Path, image: str, inner_command: list[str]) -> list[str]:
    return [
        find_docker_executable_or_fail(),
        "run", "--rm",
        "-v", f"{project_root}:/workspace",
        "-w", "/workspace",
        image,
        "bash", "-lc", " ".join(inner_command),
    ]


def run_command(command: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> int:
    try:
        result = subprocess.run(command, cwd=cwd, env=env)
    except FileNotFoundError:
        print(f"Error: '{command[0]}' was not found on PATH.", file=sys.stderr)
        return 1
    return result.returncode


def ensure_basic_files(project_root: Path) -> None:
    required = ["CMakeLists.txt", "CMakePresets.json", "project.json"]
    missing = [name for name in required if not (project_root / name).exists()]
    if missing:
        raise FileNotFoundError(
            f"Missing required project file(s): {', '.join(missing)}. "
            "Is this a generated project root?"
        )


def sync_one_repository(project_root: Path, submodule) -> int:
    target = project_root / submodule.destination
    if submodule.kind == "download_zip":
        return _sync_zip_archive(target, submodule)
    return _sync_git_repository(target, submodule)


def _sync_git_repository(target: Path, submodule) -> int:
    if target.exists():
        return run_command(["git", "pull", "--ff-only"], cwd=target)

    args = ["git", "clone", "--depth", "1"]
    ref = submodule.ref or submodule.branch or submodule.tag
    if ref:
        args += ["--branch", ref]
    args += [submodule.url, str(target)]
    return run_command(args)


def _sync_zip_archive(target: Path, submodule) -> int:
    """Downloads and extracts a released archive (a vendor SDK, a spec
    bundle, ...) once. Unlike a git clone, a fixed archive has nothing to
    "pull" - an existing, non-empty target is left alone."""
    if target.exists() and any(target.iterdir()):
        print(f"{submodule.name}: already present at {target}, skipping download.")
        return 0

    print(f"{submodule.name}: downloading {submodule.url}")
    try:
        with tempfile.TemporaryDirectory(prefix=f"{submodule.name}-download-") as scratch:
            archive_path = Path(scratch) / "archive.zip"
            urllib.request.urlretrieve(submodule.url, archive_path)

            extract_dir = Path(scratch) / "extracted"
            with zipfile.ZipFile(archive_path) as archive:
                archive.extractall(extract_dir)

            # A single wrapping top-level folder (the common case for
            # GitHub-style release archives) is unwrapped so `target` holds
            # the SDK's own contents directly, not an extra nested folder.
            entries = list(extract_dir.iterdir())
            source = entries[0] if len(entries) == 1 and entries[0].is_dir() else extract_dir

            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                shutil.rmtree(target, onerror=_rmtree_onerror)
            shutil.move(str(source), str(target))
    except (OSError, zipfile.BadZipFile, urllib.error.URLError) as exc:
        print(f"Error: failed to download/extract {submodule.name}: {exc}", file=sys.stderr)
        return 1

    print(f"{submodule.name}: extracted to {target}")
    return 0


def _rmtree_onerror(func, path, exc_info) -> None:
    del exc_info
    Path(path).chmod(stat.S_IWRITE)
    func(path)
