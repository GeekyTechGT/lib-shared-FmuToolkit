"""Main orchestration entry point copied into every generated CMake project.

Interactive menu (also drivable non-interactively via --commands):
    1  select preset          6  build                 11 full workflow
    2  sync external repos    7  build Docker image     12 build installer
    3  clean current preset   8  run examples            13 generate documentation PDFs
    4  clean all              9  run tests               14 quit
    5  configure              10 deploy                  15 run from deploy folder (GUI only)
                                                         16 API documentation (Doxygen)
                                                         17 static analysis (clang-tidy)
                                                         18 coverage report (gcovr)

Command numbers are a stable contract (scripts, CI and docs refer to them),
which is why "run from deploy folder" is appended as 15 rather than
renumbering. For app and tool (gui/both), the full workflow (11) ends by
launching the deployed GUI - pass --no-launch to skip that step (CI, smoke
tests).
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import stat
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from prj_docs import project_documents, render_documents
from prj_metadata import (
    PROJECT_ROOT,
    build_directory,
    default_preset,
    deploy_directory,
    docker_environment,
    docker_run_command,
    ensure_basic_files,
    ensure_deploy_directory,
    ensure_docker_daemon_running,
    find_docker_executable_or_fail,
    gui_executable_name,
    has_gui,
    is_docker_preset,
    load_metadata,
    python_command,
    run_cli_main,
    run_command,
    runtime_environment,
    sync_one_repository,
)
from prj_models import ProjectModel
from rich import box
from rich.console import Console
from rich.panel import Panel
from rich.prompt import Prompt
from rich.table import Table

console = Console()


# --------------------------------------------------------------------------
# CMake / process helpers
# --------------------------------------------------------------------------

def _docker_workspace_path(project_root: Path, path: Path) -> str:
    relative = Path(path).resolve().relative_to(project_root.resolve())
    return "/workspace" if str(relative) == "." else "/workspace/" + relative.as_posix()


def _run_cmake(project_root: Path, preset: str, args: list[str]) -> int:
    command = ["cmake", *args]
    if is_docker_preset(preset):
        ensure_docker_daemon_running()
        project = load_metadata(project_root)
        image = project.docker_image or f"{project.name}-builder"
        return run_command(docker_run_command(project_root, image, [" ".join(command)]), cwd=project_root)
    return run_command(command, cwd=project_root)


def _run_cmake_install(project_root: Path, preset: str, prefix: Path) -> int:
    build_dir = build_directory(project_root, preset)
    if is_docker_preset(preset):
        ensure_docker_daemon_running()
        project = load_metadata(project_root)
        image = project.docker_image or f"{project.name}-builder"
        inner = (
            f"cmake --install {_docker_workspace_path(project_root, build_dir)} "
            f"--prefix {_docker_workspace_path(project_root, prefix)}"
        )
        return run_command(docker_run_command(project_root, image, [inner]), cwd=project_root)
    return run_command(["cmake", "--install", str(build_dir), "--prefix", str(prefix)], cwd=project_root)


def _run_python_script(project_root: Path, script_name: str, args: list[str]) -> int:
    command = python_command(project_root) + [str(project_root / "scripts" / script_name), *args]
    return run_command(command, cwd=project_root)


def load_configure_presets() -> list[str]:
    presets_path = PROJECT_ROOT / "CMakePresets.json"
    payload = json.loads(presets_path.read_text(encoding="utf-8"))
    return [preset["name"] for preset in payload.get("configurePresets", []) if not preset.get("hidden")]


# --------------------------------------------------------------------------
# Configure / build / clean
# --------------------------------------------------------------------------

def warn_missing_dependencies(project_root: Path, preset: str) -> None:
    """Configure succeeds without a dependency (consumers guard with
    if(TARGET ...)), which silently drops features - say so up front."""
    project = load_metadata(project_root)
    for dependency in project.dependencies:
        deployed = Path(project.deploy_folder) / dependency.name / dependency.version / preset
        offline = project_root / "external" / dependency.name
        if not deployed.exists() and not offline.exists():
            console.print(Panel(
                f"[bold]{dependency.name} {dependency.version}[/bold] is not deployed for "
                f"[bold]{preset}[/bold].\nExpected: {deployed}\n"
                "Features that depend on it will be compiled out. Deploy it with its own "
                "prj_exec.py (environment: test/production) or place a copy under external/.",
                title="Missing dependency", border_style="yellow",
            ))


def configure(project_root: Path, preset: str) -> int:
    warn_missing_dependencies(project_root, preset)
    return _run_cmake(project_root, preset, ["--preset", preset])


def build(project_root: Path, preset: str) -> int:
    code = sync_external(project_root)
    if code != 0:
        return code
    return _run_cmake(project_root, preset, ["--build", "--preset", preset])


def _rmtree_onerror(func, path, exc_info) -> None:
    del exc_info
    Path(path).chmod(stat.S_IWRITE)
    func(path)


def clean_current(project_root: Path, preset: str) -> int:
    build_dir = build_directory(project_root, preset)
    if build_dir.exists():
        shutil.rmtree(build_dir, onerror=_rmtree_onerror)
        console.print(f"[yellow]Removed[/yellow] {build_dir}")
    return 0


def clean_all(project_root: Path) -> int:
    for directory_name in ("build", "deploy"):
        directory = project_root / directory_name
        if directory.exists():
            shutil.rmtree(directory, onerror=_rmtree_onerror)
            console.print(f"[yellow]Removed[/yellow] {directory}")
    return 0


# --------------------------------------------------------------------------
# External repositories (project.json submodules)
# --------------------------------------------------------------------------

def sync_external(project_root: Path) -> int:
    project = load_metadata(project_root)
    if not project.submodules:
        console.print("[dim]No external repositories declared in project.json.[/dim]")
        return 0

    overall = 0
    for submodule in project.submodules:
        code = sync_one_repository(project_root, submodule)
        overall = overall or code
    return overall


# --------------------------------------------------------------------------
# Examples / tests / docs
# --------------------------------------------------------------------------

def run_examples(project_root: Path, preset: str) -> int:
    return _run_python_script(project_root, "prj_run_examples.py", ["--preset", preset])


def run_tests(project_root: Path, preset: str) -> int:
    return _run_python_script(project_root, "prj_run_tests.py", ["--preset", preset])


def documents_build_directory(project_root: Path, preset: str) -> Path:
    """build/<preset>/documents - a sibling of build/<preset>/bin, so a GUI
    run straight from the build tree finds its PDFs at <exe dir>/../documents,
    exactly like a deployed one."""
    return build_directory(project_root, preset) / "documents"


def render_project_documents(project_root: Path, project: ProjectModel, output_dir: Path) -> int:
    """Renders USER_MANUAL.md and CHANGELOG.md to PDF into `output_dir`.
    Missing PDF tooling is a warning, not a failure: the project still
    deploys (an app's Help menu then just reports the document as missing)."""
    specs = project_documents(project_root, project.application_name or project.artifact_name or project.name)
    for spec, failure in render_documents(specs, output_dir):
        if failure is None:
            console.print(f"[green]Rendered[/green] {output_dir / spec.pdf_name}")
        else:
            console.print(f"[yellow]Could not render {spec.pdf_name}: {failure}[/yellow]")
    return 0


def generate_documentation_pdfs(project_root: Path, preset: str) -> int:
    project = load_metadata(project_root)
    return render_project_documents(project_root, project, documents_build_directory(project_root, preset))


def build_docker_image(project_root: Path) -> int:
    project = load_metadata(project_root)
    dockerfile = project_root / "Dockerfile"
    if not dockerfile.exists():
        console.print("[yellow]No Dockerfile in this project; skipping.[/yellow]")
        return 0
    ensure_docker_daemon_running()
    docker = find_docker_executable_or_fail()
    image = project.docker_image or f"{project.name}-builder"
    return run_command([docker, "build", "-t", image, "-f", str(dockerfile), "."],
                        cwd=project_root, env=docker_environment())


def find_tool(name: str) -> str | None:
    """A tool on PATH, or installed into this project's venv by `uv sync`
    (clang-tidy, gcovr) - the venv's Scripts/bin folder is not necessarily on
    PATH when prj_exec.py runs."""
    found = shutil.which(name)
    if found:
        return found
    venv_bin = Path(sys.executable).parent
    for candidate in (venv_bin / name, venv_bin / f"{name}.exe"):
        if candidate.exists():
            return str(candidate)
    return None


def generate_api_documentation(project_root: Path) -> int:
    doxygen = find_tool("doxygen")
    if doxygen is None:
        console.print("[yellow]doxygen not found on PATH - install it (https://www.doxygen.nl) to generate "
                      "the API reference.[/yellow]")
        return 0
    code = run_command([doxygen, "Doxyfile"], cwd=project_root)
    if code == 0:
        console.print(f"[green]API reference:[/green] {project_root / 'build' / 'docs' / 'html' / 'index.html'}")
    return code


STATIC_ANALYSIS_ROOTS = ("src", "apps", "include")
STATIC_ANALYSIS_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
GCC_ONLY_FLAGS = ("-fno-keep-inline-dllexport",)


def static_analysis(project_root: Path, preset: str) -> int:
    """clang-tidy (.clang-tidy profile) over every translation unit of the
    project, in parallel, using the preset's compile_commands.json."""
    clang_tidy = find_tool("clang-tidy")
    if clang_tidy is None:
        console.print("[red]clang-tidy not found - run `uv sync` (it is a dev dependency) or install LLVM.[/red]")
        return 1
    build_dir = build_directory(project_root, preset)
    if not (build_dir / "compile_commands.json").exists():
        console.print(f"[red]{build_dir / 'compile_commands.json'} not found - configure (5) first.[/red]")
        return 1

    sources = sorted(
        path for root in STATIC_ANALYSIS_ROOTS if (project_root / root).exists()
        for path in (project_root / root).rglob("*") if path.suffix in STATIC_ANALYSIS_SUFFIXES
    )
    base = [clang_tidy, "-p", str(build_dir), "--quiet"]
    if platform.system() == "Windows":
        # The compile database comes from MinGW g++; tell clang-tidy's driver
        # so it resolves the MinGW headers instead of looking for MSVC ones.
        base.append("--extra-arg=--target=x86_64-w64-mingw32")
        # GCC-only flags CMake adds for MinGW (VISIBILITY_INLINES_HIDDEN)
        # that clang's driver rejects.
        base += [f"--removed-arg={flag}" for flag in GCC_ONLY_FLAGS]
    env = runtime_environment(project_root, preset, load_metadata(project_root))

    def analyse(source: Path) -> tuple[Path, int, str]:
        result = subprocess.run([*base, str(source)], capture_output=True, text=True, env=env)
        return source, result.returncode, (result.stdout + result.stderr).strip()

    failures = 0
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for source, code, output in pool.map(analyse, sources):
            relative = source.relative_to(project_root)
            if code != 0:
                failures += 1
                console.print(f"[red]x[/red] {relative}\n{output}")
            elif "warning:" in output:
                console.print(f"[yellow]![/yellow] {relative}\n{output}")
            else:
                console.print(f"[green]ok[/green] {relative}")
    console.print(f"[bold]{len(sources)} file(s) analysed, {failures} with errors.[/bold]")
    return 1 if failures else 0


def coverage_preset() -> str:
    return "windows-mingw64-coverage" if platform.system() == "Windows" else "linux-gcc-coverage"


def coverage_report(project_root: Path) -> int:
    """Instrumented build (the *-coverage preset) -> tests -> gcovr HTML."""
    preset = coverage_preset()
    if preset not in load_configure_presets():
        console.print(f"[yellow]No '{preset}' preset in CMakePresets.json - coverage is not available here.[/yellow]")
        return 0
    gcovr = find_tool("gcovr")
    if gcovr is None:
        console.print("[red]gcovr not found - run `uv sync` (it is a dev dependency).[/red]")
        return 1

    for step in (lambda: configure(project_root, preset), lambda: build(project_root, preset),
                 lambda: run_tests(project_root, preset)):
        code = step()
        if code != 0:
            return code

    build_dir = build_directory(project_root, preset)
    report = build_dir / "coverage" / "index.html"
    report.parent.mkdir(parents=True, exist_ok=True)
    command = [
        gcovr, "--root", str(project_root), "--object-directory", str(build_dir),
        "--filter", "src/", "--filter", "include/", "--filter", "apps/",
        "--html-details", str(report), "--print-summary",
    ]
    env = runtime_environment(project_root, preset, load_metadata(project_root))  # gcov on PATH
    code = run_command(command, cwd=project_root, env=env)
    if code == 0:
        console.print(f"[green]Coverage report:[/green] {report}")
    return code


def build_installer(project_root: Path, preset: str) -> int:
    return _run_python_script(project_root, "build_installer.py", ["--preset", preset])


# --------------------------------------------------------------------------
# Deploy
# --------------------------------------------------------------------------

def is_test_executable(name: str) -> bool:
    stem = Path(name).stem
    return stem.endswith("_unit_tests") or stem.endswith("_integration_tests")


def copy_qt_runtime(project_root: Path, preset: str, deploy_bin: Path) -> None:
    """Copies windeployqt's output (Qt DLLs + plugin subdirectories such as
    platforms/, styles/, imageformats/) into the deploy folder. windeployqt
    already ran as a POST_BUILD step and dropped everything next to the GUI
    executable in build/<preset>/bin/ - but `cmake --install` only installs
    the declared targets (the .exe files themselves), not files/directories
    a POST_BUILD step happened to leave alongside them.
    """
    build_bin = build_directory(project_root, preset) / "bin"
    if not build_bin.exists():
        return

    deploy_bin.mkdir(parents=True, exist_ok=True)
    for item in build_bin.iterdir():
        if item.is_file() and item.suffix.lower() == ".exe":
            continue  # the real targets are already installed via cmake --install
        if item.is_dir():
            shutil.copytree(item, deploy_bin / item.name, dirs_exist_ok=True)
        elif item.is_file():
            shutil.copy2(item, deploy_bin / item.name)


def _copy_dependency_legacy_bin(dependency_root: Path, deploy_bin: Path) -> None:
    """Default when a dependency declares no `copy_to` mappings: flat-copy
    files directly under its own bin/ into the consumer's bin/ (no
    recursion)."""
    dependency_bin = dependency_root / "bin"
    if not dependency_bin.exists():
        return
    deploy_bin.mkdir(parents=True, exist_ok=True)
    for item in dependency_bin.iterdir():
        if item.is_file():
            shutil.copy2(item, deploy_bin / item.name)


# A dependency's lib/cmake/ holds find_package() configs with paths baked in
# for *its* install prefix - meaningless (and misleading) once copied into a
# consumer's deploy folder, so it is never propagated.
_DEPENDENCY_COPY_IGNORE = shutil.ignore_patterns("cmake")


def _copy_dependency_path(source: Path, destination: Path) -> None:
    if source.is_dir():
        shutil.copytree(source, destination, dirs_exist_ok=True, ignore=_DEPENDENCY_COPY_IGNORE)
    elif source.is_file():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    else:
        console.print(f"[yellow]Dependency copy_to source not found, skipping: {source}[/yellow]")


def copy_dependency_artifacts(project: ProjectModel, preset: str, prefix: Path) -> None:
    """Copies each dependency's deployed artifacts into this project's own
    deploy prefix. A dependency with explicit `copy_to` mappings is copied
    exactly as declared (files or directories, anywhere under `prefix`);
    otherwise falls back to `_copy_dependency_legacy_bin()`.
    """
    for dependency in project.dependencies:
        dependency_root = Path(project.deploy_folder) / dependency.name / dependency.version / preset
        if not dependency_root.exists():
            console.print(
                f"[yellow]Dependency {dependency.name} {dependency.version} is not deployed for "
                f"'{preset}' ({dependency_root}) - nothing to copy.[/yellow]"
            )
            continue
        if not dependency.copy_to:
            _copy_dependency_legacy_bin(dependency_root, prefix / "bin")
            continue
        for mapping in dependency.copy_to:
            _copy_dependency_path(dependency_root / mapping.source, prefix / mapping.destination)


def _copy_file_if_exists(source: Path, destination: Path) -> None:
    if source.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def remove_test_executables(bin_dir: Path) -> None:
    if not bin_dir.exists():
        return
    for item in list(bin_dir.iterdir()):
        if item.is_file() and is_test_executable(item.name):
            item.unlink()


def copy_toolchain_runtimes(project: ProjectModel, preset: str, bin_dir: Path) -> None:
    """Copies the compiler runtime DLLs declared in project.json's
    runtimes[<preset>] (libstdc++, libgcc, winpthread for MinGW)."""
    runtime = project.runtimes.get(preset)
    if not runtime:
        return
    for source_dir_str in runtime.source_path.split(","):
        source_dir = Path(source_dir_str.strip())
        if not source_dir.exists():
            continue
        for dll_name in runtime.dlls:
            dll_path = source_dir / dll_name
            if dll_path.exists():
                shutil.copy2(dll_path, bin_dir / dll_name)


# Deployed layout, identical for app, tool, lib-shared and lib-static:
#   bin/        executables / DLL, Qt runtime (windeployqt), dependency DLLs
#   lib/        import/static library, dependency libraries (copy_to)
#   include/    public headers (libraries), dependency headers (copy_to)
#   documents/  USER_MANUAL.pdf, CHANGELOG.pdf, INFO.md
#   license/    LICENSE
# All five folders are always created, even when a category leaves one empty
# (e.g. bin/ of a lib-static), so every deploy has the same predictable shape.
DEPLOY_LAYOUT = ("bin", "lib", "include", "documents", "license")


def deploy_standard_layout(project_root: Path, project: ProjectModel, prefix: Path) -> None:
    for directory in DEPLOY_LAYOUT:
        (prefix / directory).mkdir(parents=True, exist_ok=True)

    documents = prefix / "documents"
    render_project_documents(project_root, project, documents)
    _copy_file_if_exists(project_root / "INFO.md", documents / "INFO.md")
    _copy_file_if_exists(project_root / "LICENSE", prefix / "license" / "LICENSE")

    # Earlier deploys put the Markdown sources in the prefix root - drop them
    # so a re-deploy into an existing folder converges on the new layout.
    for stale in ("CHANGELOG.md", "INFO.md", "USER_MANUAL.md", "USER_MANUAL.pdf"):
        stale_path = prefix / stale
        if stale_path.is_file():
            stale_path.unlink()


def deploy_app(project_root: Path, preset: str, project: ProjectModel, prefix: Path) -> int:
    return_code = _run_cmake_install(project_root, preset, prefix)
    if return_code != 0:
        return return_code

    bin_dir = prefix / "bin"
    remove_test_executables(bin_dir)
    copy_qt_runtime(project_root, preset, bin_dir)
    if bin_dir.exists():
        copy_toolchain_runtimes(project, preset, bin_dir)
    copy_dependency_artifacts(project, preset, prefix)

    deploy_standard_layout(project_root, project, prefix)
    return 0


def deploy(project_root: Path, preset: str) -> int:
    code = sync_external(project_root)
    if code != 0:
        return code
    project = load_metadata(project_root)
    prefix = deploy_directory(project_root, preset, project)
    error = ensure_deploy_directory(prefix)
    if error:
        console.print(f"[red]{error}[/red]")
        return 1
    return deploy_app(project_root, preset, project, prefix)


# --------------------------------------------------------------------------
# Run from the deploy folder
# --------------------------------------------------------------------------

def run_from_deploy(project_root: Path, preset: str) -> int:
    """Launches the deployed GUI executable from <deploy>/bin, detached, so
    the menu stays usable while the application runs. Running the deployed
    copy (rather than build/<preset>/bin) is the real end-to-end check: it
    only works if Deploy gathered every DLL, Qt plugin and document."""
    project = load_metadata(project_root)
    executable_name = gui_executable_name(project)
    if executable_name is None:
        console.print(f"[yellow]'{project.name}' has no GUI executable to run (category {project.category}).[/yellow]")
        return 0

    bin_dir = deploy_directory(project_root, preset, project) / "bin"
    executable = bin_dir / executable_name
    if not executable.exists():
        console.print(f"[red]{executable} not found - run Deploy (10) first.[/red]")
        return 1

    kwargs: dict = {"cwd": bin_dir}
    if platform.system() == "Windows":
        kwargs["creationflags"] = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        kwargs["start_new_session"] = True
    try:
        process = subprocess.Popen([str(executable)], **kwargs)
    except OSError as exc:
        console.print(f"[red]Could not launch {executable}: {exc}[/red]")
        return 1
    console.print(f"[green]Launched[/green] {executable} [dim](pid {process.pid})[/dim]")
    return 0


def launch_disabled() -> bool:
    """--no-launch sets PRJ_EXEC_NO_LAUNCH; the variable can also be set
    directly (CI) to keep the full workflow headless."""
    return os.environ.get("PRJ_EXEC_NO_LAUNCH", "").strip().lower() in {"1", "true", "yes"}


def full_workflow(project_root: Path, preset: str) -> int:
    steps = [
        lambda: clean_current(project_root, preset),
        lambda: configure(project_root, preset),
        lambda: build(project_root, preset),
        lambda: deploy(project_root, preset),
    ]
    if has_gui(load_metadata(project_root)) and not launch_disabled():
        steps.append(lambda: run_from_deploy(project_root, preset))

    for step in steps:
        code = step()
        if code != 0:
            return code
    return 0


# --------------------------------------------------------------------------
# Menu
# --------------------------------------------------------------------------

# Each section is (title, color, [(number, label), ...]). Numbers are stable
# CLI contract (--commands "5,6,9" etc. is documented in docs/EXAMPLES.md) -
# only the *display* is grouped/colored, dispatch() below is unaffected.
def menu_sections(project: ProjectModel) -> list[tuple[str, str, list[tuple[str, str]]]]:
    gui = has_gui(project)
    workflow_label = (
        "Full workflow (clean -> configure -> build -> deploy -> run)"
        if gui
        else "Full workflow (clean -> configure -> build -> deploy)"
    )
    deploy_items = [
        ("10", "Deploy"),
        ("11", workflow_label),
        ("12", "Build installer"),
        ("13", "Generate documentation PDFs"),
    ]
    if gui:
        deploy_items.append(("15", "Run from deploy folder"))

    return [
        ("Environment", "cyan", [
            ("1", "Select preset"),
            ("2", "Sync external repositories"),
        ]),
        ("Clean", "yellow", [
            ("3", "Clean current preset"),
            ("4", "Clean all presets"),
        ]),
        ("Build", "green", [
            ("5", "Configure"),
            ("6", "Build"),
            ("7", "Build Docker image"),
        ]),
        ("Test", "blue", [
            ("8", "Run examples"),
            ("9", "Run tests"),
        ]),
        ("Quality", "bright_cyan", [
            ("16", "API documentation (Doxygen)"),
            ("17", "Static analysis (clang-tidy)"),
            ("18", "Coverage report (gcovr)"),
        ]),
        ("Deploy & package", "magenta", deploy_items),
        ("Exit", "red", [
            ("14", "Quit"),
        ]),
    ]


def print_header(project_root: Path, project_name: str, project_version: str, description: str, preset: str) -> None:
    del project_root
    console.print(Panel(
        f"[bold cyan]{project_name}[/bold cyan] [dim]{project_version}[/dim]\n"
        f"[italic]FmuToolkit - lib-shared[/italic]\n\n"
        f"Preset: [bold green]{preset}[/bold green]",
        border_style="cyan",
    ))


def print_preset_table(current_preset: str) -> None:
    table = Table(title="Available presets", title_style="bold cyan", box=box.ROUNDED, border_style="cyan")
    table.add_column("#", justify="right", style="bold")
    table.add_column("Preset")
    presets = load_configure_presets()
    for index, preset in enumerate(presets, start=1):
        is_current = preset == current_preset
        style = "bold green" if is_current else ""
        marker = " [dim](current)[/dim]" if is_current else ""
        table.add_row(str(index), f"[{style}]{preset}[/{style}]{marker}" if style else f"{preset}{marker}")
    console.print(table)


def select_preset(current_preset: str) -> str:
    presets = load_configure_presets()
    print_preset_table(current_preset)
    choice = Prompt.ask("[bold cyan]Select preset number[/bold cyan] (blank to keep current)", default="")
    if not choice.strip():
        return current_preset
    try:
        index = int(choice) - 1
        return presets[index]
    except (ValueError, IndexError):
        console.print("[red]Invalid selection, keeping current preset.[/red]")
        return current_preset


def print_menu(preset: str, project_root: Path) -> None:
    project = load_metadata(project_root)
    table = Table(
        title=f"Menu  (preset: [bold green]{preset}[/bold green])",
        show_header=True,
        header_style="bold",
        box=box.ROUNDED,
    )
    table.add_column("#", justify="right", width=3)
    table.add_column("Command")
    table.add_column("Category")

    for name, color, items in menu_sections(project):
        last_index = len(items) - 1
        for index, (number, label) in enumerate(items):
            table.add_row(
                f"[bold {color}]{number}[/bold {color}]",
                f"[{color}]{label}[/{color}]",
                f"[dim {color}]{name}[/dim {color}]",
                end_section=(index == last_index),
            )
    console.print(table)


def dispatch(choice: str, project_root: Path, preset: str) -> tuple[str, int, bool]:
    if choice == "1":
        return select_preset(preset), 0, True
    if choice == "2":
        return preset, sync_external(project_root), True
    if choice == "3":
        return preset, clean_current(project_root, preset), True
    if choice == "4":
        return preset, clean_all(project_root), True
    if choice == "5":
        return preset, configure(project_root, preset), True
    if choice == "6":
        return preset, build(project_root, preset), True
    if choice == "7":
        return preset, build_docker_image(project_root), True
    if choice == "8":
        return preset, run_examples(project_root, preset), True
    if choice == "9":
        return preset, run_tests(project_root, preset), True
    if choice == "10":
        return preset, deploy(project_root, preset), True
    if choice == "11":
        return preset, full_workflow(project_root, preset), True
    if choice == "12":
        return preset, build_installer(project_root, preset), True
    if choice == "13":
        return preset, generate_documentation_pdfs(project_root, preset), True
    if choice == "14":
        return preset, 0, False
    if choice == "15":
        return preset, run_from_deploy(project_root, preset), True
    if choice == "16":
        return preset, generate_api_documentation(project_root), True
    if choice == "17":
        return preset, static_analysis(project_root, preset), True
    if choice == "18":
        return preset, coverage_report(project_root), True

    console.print(f"[red]Unknown choice: {choice}[/red]")
    return preset, 1, True


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Project orchestration menu.")
    parser.add_argument("--preset", default=None, help="Preset to use. Defaults to the platform default.")
    parser.add_argument("--commands", default=None, help="Comma-separated menu numbers to run non-interactively.")
    parser.add_argument(
        "--no-launch",
        action="store_true",
        help="Do not launch the deployed GUI at the end of the full workflow (11).",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.no_launch:
        os.environ["PRJ_EXEC_NO_LAUNCH"] = "1"
    ensure_basic_files(PROJECT_ROOT)
    project = load_metadata(PROJECT_ROOT)
    preset = args.preset or default_preset(project)

    if args.commands:
        overall = 0
        for choice in (c.strip() for c in args.commands.split(",") if c.strip()):
            preset, code, _keep_going = dispatch(choice, PROJECT_ROOT, preset)
            overall = overall or code
            if code != 0:
                console.print(f"[bold red]Command {choice} failed with exit code {code}.[/bold red]")
                break
        else:
            console.print("[bold green]All commands completed successfully.[/bold green]")
        sys.exit(overall)

    keep_going = True
    while keep_going:
        print_header(PROJECT_ROOT, project.name, project.version, project.description, preset)
        print_menu(preset, PROJECT_ROOT)
        choice = Prompt.ask("[bold cyan]Choice[/bold cyan]")
        preset, code, keep_going = dispatch(choice, PROJECT_ROOT, preset)
        if not keep_going:
            break
        if code != 0:
            console.print(f"[bold red]Command failed with exit code {code}.[/bold red]")
        else:
            console.print("[green]Done.[/green]")
        # Leave the command's output on screen until the user is ready to see
        # the menu again, instead of redrawing over it immediately.
        console.input("\n[dim]Press Enter to return to the menu...[/dim]")


if __name__ == "__main__":
    run_cli_main(main)
