"""Inserts a copyright header into source files that don't already have one."""

from __future__ import annotations

import argparse
import datetime
from dataclasses import dataclass, field
from pathlib import Path

from prj_metadata import run_cli_main

SOURCE_DIRS = ("src", "include", "apps", "tests", "examples", "scripts", "frontend", "backend", "static")
C_LIKE_SUFFIXES = {".c", ".h", ".cpp", ".hpp", ".ts", ".tsx", ".js", ".jsx"}
PYTHON_SUFFIXES = {".py"}
# Substituted by the generator (tool-ProjectScaffold COPYRIGHT_HOLDER).
COPYRIGHT_HOLDER = "Cristian Tesconi"
EXCLUDED_DIR_NAMES = {".venv", "build", "deploy", "__pycache__", ".git", "node_modules", "dist"}


@dataclass
class RunSummary:
    scanned: int = 0
    updated: list[Path] = field(default_factory=list)
    skipped: list[Path] = field(default_factory=list)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Insert a copyright header into source files missing one.")
    parser.add_argument("--project", type=Path, default=Path(__file__).absolute().parent.parent)
    parser.add_argument("--year", type=int, default=datetime.date.today().year)
    parser.add_argument("--write", action="store_true", help="Apply changes (default is dry-run).")
    return parser.parse_args()


def source_files(project_root: Path) -> list[Path]:
    files: list[Path] = []
    for dir_name in SOURCE_DIRS:
        base = project_root / dir_name
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if not path.is_file():
                continue
            if any(part in EXCLUDED_DIR_NAMES for part in path.parts):
                continue
            if path.suffix in C_LIKE_SUFFIXES | PYTHON_SUFFIXES:
                files.append(path)
    return sorted(files)


def has_copyright_header(text: str, suffix: str) -> bool:
    head = text[:400]
    return "Copyright" in head


def newline_for(text: str) -> str:
    return "\r\n" if "\r\n" in text else "\n"


def c_or_cpp_header(newline: str, year: int) -> str:
    lines = [
        f"// Copyright (c) {year} {COPYRIGHT_HOLDER}.",
        "// All rights reserved.",
        "",
    ]
    return newline.join(lines) + newline


def python_header(newline: str, year: int) -> str:
    lines = [
        f"# Copyright (c) {year} {COPYRIGHT_HOLDER}.",
        "# All rights reserved.",
        "",
    ]
    return newline.join(lines) + newline


def split_python_preamble(text: str) -> tuple[str, str]:
    """Splits off a leading shebang/encoding-declaration preamble so the
    header can be inserted after it rather than before."""
    lines = text.splitlines(keepends=True)
    preamble_end = 0
    for index, line in enumerate(lines[:2]):
        if line.startswith("#!") or "coding:" in line or "coding=" in line:
            preamble_end = index + 1
        else:
            break
    preamble = "".join(lines[:preamble_end])
    rest = "".join(lines[preamble_end:])
    return preamble, rest


def add_copyright_header(path: Path, year: int, write: bool) -> bool:
    text = path.read_text(encoding="utf-8")
    if has_copyright_header(text, path.suffix):
        return False

    newline = newline_for(text)

    if path.suffix in PYTHON_SUFFIXES:
        preamble, rest = split_python_preamble(text)
        new_text = preamble + python_header(newline, year) + rest
    else:
        new_text = c_or_cpp_header(newline, year) + text

    if write:
        path.write_text(new_text, encoding="utf-8")
    return True


def process_sources(project_root: Path, year: int, write: bool) -> RunSummary:
    summary = RunSummary()
    for path in source_files(project_root):
        summary.scanned += 1
        try:
            if add_copyright_header(path, year, write):
                summary.updated.append(path)
            else:
                summary.skipped.append(path)
        except (OSError, UnicodeDecodeError):
            summary.skipped.append(path)
    return summary


def main() -> int:
    from rich.console import Console

    console = Console()

    args = parse_args()
    summary = process_sources(args.project, args.year, args.write)

    if args.write:
        mode_color, mode = "green", "applied"
    else:
        mode_color, mode = "yellow", "dry-run (pass --write to apply)"

    console.print(
        f"Scanned [bold]{summary.scanned}[/bold] file(s), "
        f"[bold {mode_color}]{len(summary.updated)}[/bold {mode_color}] "
        f"{'updated' if args.write else 'would be updated'} [{mode_color}][{mode}][/{mode_color}]."
    )
    for path in summary.updated:
        console.print(f"  [green]+[/green] {path.relative_to(args.project)}")

    return 0


if __name__ == "__main__":
    run_cli_main(main)
