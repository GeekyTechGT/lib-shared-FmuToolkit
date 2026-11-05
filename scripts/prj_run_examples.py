"""Runs the example executables built under build/<preset>/examples/."""

from __future__ import annotations

import argparse
import platform
import subprocess
import sys
from pathlib import Path

from prj_metadata import PROJECT_ROOT, build_directory, load_metadata, run_cli_main, runtime_environment
from rich import box
from rich.console import Console
from rich.table import Table

console = Console()


def discover_example_executables(build_root: Path) -> list[Path]:
    examples_dir = build_root / "examples"
    if not examples_dir.exists():
        return []

    suffix = ".exe" if platform.system() == "Windows" else ""
    return sorted(
        path for path in examples_dir.iterdir()
        if path.is_file() and (suffix == "" or path.suffix == suffix) and "_example" in path.stem
    )


def run_examples(preset: str, project_root: Path | None = None) -> int:
    root = project_root or PROJECT_ROOT
    project = load_metadata(root)
    build_dir = build_directory(root, preset)

    executables = discover_example_executables(build_dir)
    if not executables:
        console.print(f"[yellow]No example executables found under {build_dir / 'examples'}.[/yellow]")
        return 0

    env = runtime_environment(root, preset, project)

    table = Table(title="Examples", title_style="bold cyan", box=box.ROUNDED, border_style="cyan")
    table.add_column("Executable", style="bold")
    table.add_column("Result", justify="center")

    overall = 0
    for executable in executables:
        result = subprocess.run([str(executable)], cwd=executable.parent, env=env)
        ok = result.returncode == 0
        overall = overall if ok else 1
        status = "[bold green]OK[/bold green]" if ok else f"[bold red]FAILED ({result.returncode})[/bold red]"
        table.add_row(executable.name, status)

    console.print(table)
    if overall == 0:
        console.print("[bold green]All examples passed.[/bold green]")
    else:
        console.print("[bold red]One or more examples failed.[/bold red]")
    return overall


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the built example executables.")
    parser.add_argument("--preset", required=True)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    sys.exit(run_examples(args.preset))


if __name__ == "__main__":
    run_cli_main(main)
