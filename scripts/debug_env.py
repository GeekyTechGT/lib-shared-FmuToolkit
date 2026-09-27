"""Prints diagnostics about the local toolchain this project's scripts rely on."""

from __future__ import annotations

import platform
import shutil

from prj_metadata import find_docker_executable, find_inno_setup_compiler
from rich import box
from rich.console import Console
from rich.table import Table

console = Console()


def format_tool_path(tool_name: str) -> str:
    path = shutil.which(tool_name)
    return f"[green]{path}[/green]" if path else "[red]not found[/red]"


def format_docker_path() -> str:
    path = find_docker_executable()
    return f"[green]{path}[/green]" if path else "[red]not found[/red]"


def format_iscc_path() -> str:
    path = find_inno_setup_compiler()
    if path:
        return f"[green]{path}[/green]"
    if platform.system() == "Windows":
        return "[yellow]not found (Windows-only, needed for the installer)[/yellow]"
    return "[dim]n/a (not Windows)[/dim]"


def main() -> None:
    table = Table(
        title=f"Environment ({platform.system()} {platform.release()})",
        title_style="bold cyan",
        box=box.ROUNDED,
        border_style="cyan",
    )
    table.add_column("Tool", style="bold")
    table.add_column("Path")

    table.add_row("python", format_tool_path("python"))
    table.add_row("uv", format_tool_path("uv"))
    table.add_row("git", format_tool_path("git"))
    table.add_row("cmake", format_tool_path("cmake"))
    table.add_row("ninja", format_tool_path("ninja"))
    table.add_row("ctest", format_tool_path("ctest"))
    table.add_row("docker", format_docker_path())
    table.add_row("ISCC.exe (Inno Setup)", format_iscc_path())

    console.print(table)


if __name__ == "__main__":
    main()
