#!/usr/bin/env python3
"""
project_manager.py — interactive build manager for lib-fmu-toolkit.

Activate the virtual environment first, then run:
    python scripts/project_manager.py

Supports three execution modes:
  native  — cmake runs directly in the current process
  wsl     — cmake runs inside WSL (wsl.exe on Windows, native on Linux)
  docker  — cmake runs inside Docker (docker run …)
"""

from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess
import sys
import textwrap
import webbrowser
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Bootstrap check
# ---------------------------------------------------------------------------
try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.table import Table
    from rich.prompt import Prompt, Confirm
    from rich.rule import Rule
    from rich import box
except ImportError:
    print(
        "\n[ERROR] 'rich' is not installed.\n"
        "Activate the virtual environment first:\n"
        "  Windows : pyvenv\\Scripts\\activate\n"
        "  Linux   : source pyvenv/bin/activate\n"
        "Then: pip install -r requirements.txt\n"
    )
    sys.exit(1)

# ---------------------------------------------------------------------------
# Globals
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROJECT_JSON = PROJECT_ROOT / "project.json"
CONSOLE = Console()
_HOST = platform.system()   # "Windows" or "Linux"

# CMake/CTest fallback paths on Windows
_WIN_CMAKE = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/cmake.exe",
    "C:/Program Files/CMake/bin/cmake.exe",
]
_WIN_CTEST = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/ctest.exe",
    "C:/Program Files/CMake/bin/ctest.exe",
]

# ---------------------------------------------------------------------------
# Preset catalogue
# Each entry has:
#   executor  "native" | "wsl" | "docker"
#   available  True/False — whether it can run on the current host
# ---------------------------------------------------------------------------
PRESETS: list[dict] = [
    {
        "name":       "windows-mingw32-debug",
        "display":    "Windows MinGW32  [Debug]",
        "executor":   "native",
        "arch":       "32-bit",
        "build_type": "Debug",
        "host":       "Windows",       # requires Windows
    },
    {
        "name":       "windows-mingw32-release",
        "display":    "Windows MinGW32  [Release]",
        "executor":   "native",
        "arch":       "32-bit",
        "build_type": "Release",
        "host":       "Windows",
    },
    {
        "name":       "windows-mingw64-debug",
        "display":    "Windows MinGW64  [Debug]",
        "executor":   "native",
        "arch":       "64-bit",
        "build_type": "Debug",
        "host":       "Windows",
    },
    {
        "name":       "windows-mingw64-release",
        "display":    "Windows MinGW64  [Release]",
        "executor":   "native",
        "arch":       "64-bit",
        "build_type": "Release",
        "host":       "Windows",
    },
    {
        "name":       "linux-mingw-debug",
        "display":    "Linux/WSL GCC    [Debug]",
        "executor":   "wsl",           # WSL on Windows, native on Linux
        "arch":       "native",
        "build_type": "Debug",
        "host":       "any",
    },
    {
        "name":       "linux-mingw-release",
        "display":    "Linux/WSL GCC    [Release]",
        "executor":   "wsl",
        "arch":       "native",
        "build_type": "Release",
        "host":       "any",
    },
    {
        "name":       "docker-mingw-debug",
        "display":    "Docker GCC       [Debug]",
        "executor":   "docker",
        "arch":       "native",
        "build_type": "Debug",
        "host":       "any",
    },
    {
        "name":       "docker-mingw-release",
        "display":    "Docker GCC       [Release]",
        "executor":   "docker",
        "arch":       "native",
        "build_type": "Release",
        "host":       "any",
    },
]


def preset_is_available(p: dict) -> bool:
    """True if the preset can be run on the current host."""
    if p["host"] == "Windows" and _HOST != "Windows":
        return False
    if p["executor"] == "wsl" and _HOST == "Windows":
        return shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    if p["executor"] == "docker":
        return shutil.which("docker") is not None
    return True


def executor_label(p: dict) -> str:
    """Short label shown in the preset table."""
    if p["executor"] == "native":
        return "[cyan]native[/]"
    if p["executor"] == "wsl":
        if _HOST == "Linux":
            return "[cyan]native[/]"
        return "[yellow]WSL[/]"
    if p["executor"] == "docker":
        return "[magenta]Docker[/]"
    return p["executor"]


# ---------------------------------------------------------------------------
# Tool discovery
# ---------------------------------------------------------------------------

def _find_exe(name: str, extra: list[str] | None = None) -> Optional[str]:
    found = shutil.which(name)
    if found:
        return found
    for c in (extra or []):
        if Path(c).is_file():
            return c
    return None


def find_cmake() -> Optional[str]:
    return _find_exe("cmake", _WIN_CMAKE if _HOST == "Windows" else [])


def find_ctest() -> Optional[str]:
    return _find_exe("ctest", _WIN_CTEST if _HOST == "Windows" else [])


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def _win_to_wsl_path(p: Path) -> str:
    """D:\\foo\\bar  →  /mnt/d/foo/bar"""
    parts = p.parts                             # ('D:\\', 'foo', 'bar')
    drive = parts[0].rstrip(":\\").lower()      # 'D:' → 'd'
    rest = "/".join(parts[1:])
    return f"/mnt/{drive}/{rest}"


def _win_to_docker_volume(p: Path) -> str:
    """D:\\foo\\bar  →  D:/foo/bar  (Docker Desktop on Windows)"""
    return str(p).replace("\\", "/")


# ---------------------------------------------------------------------------
# Command builders — wrap a cmake/ctest command for WSL or Docker
# ---------------------------------------------------------------------------

def _build_native_cmd(base_cmd: list[str]) -> list[str]:
    return base_cmd


def _build_wsl_cmd(base_cmd: list[str]) -> list[str]:
    """Wrap a cmake command for execution inside WSL."""
    if _HOST == "Linux":
        return base_cmd                         # already native
    wsl_path = _win_to_wsl_path(PROJECT_ROOT)
    shell_cmd = " ".join(f'"{a}"' if " " in a else a for a in base_cmd)
    return ["wsl.exe", "bash", "-lc", f"cd '{wsl_path}' && {shell_cmd}"]


def _build_docker_cmd(base_cmd: list[str], meta: dict) -> list[str]:
    """Wrap a cmake command for execution inside Docker."""
    image = meta.get("docker_image", "lib-fmu-toolkit-builder")
    if _HOST == "Windows":
        volume = _win_to_docker_volume(PROJECT_ROOT)
    else:
        volume = str(PROJECT_ROOT)
    return [
        "docker", "run", "--rm",
        "-v", f"{volume}:/workspace",
        "-w", "/workspace",
        image,
    ] + base_cmd


def build_cmd(preset: dict, base_cmd: list[str], meta: dict) -> list[str]:
    """Return the full command list for the given preset's executor."""
    ex = preset["executor"]
    if ex == "native":
        return _build_native_cmd(base_cmd)
    if ex == "wsl":
        return _build_wsl_cmd(base_cmd)
    if ex == "docker":
        return _build_docker_cmd(base_cmd, meta)
    return base_cmd


# ---------------------------------------------------------------------------
# Project metadata
# ---------------------------------------------------------------------------

def load_metadata() -> dict:
    if not PROJECT_JSON.exists():
        CONSOLE.print(f"[bold red]ERROR[/] project.json not found at {PROJECT_JSON}")
        sys.exit(1)
    with open(PROJECT_JSON, encoding="utf-8") as fh:
        return json.load(fh)


# ---------------------------------------------------------------------------
# Subprocess helpers
# ---------------------------------------------------------------------------

def _make_env(preset_name: str | None = None) -> dict:
    """
    Build an environment dict suitable for running native MinGW cmake/ninja.

    MinGW g++ needs its own bin/ directory in PATH so it can locate ld.exe,
    cc1plus.exe and other support tools.  When project_manager.py is launched
    from a plain Windows terminal (cmd / PowerShell / VS Code) those directories
    are absent, causing the linker to exit 1 with no error message.

    We detect which toolchain a preset uses and prepend the matching MSYS2
    bin/ directory to the subprocess PATH.
    """
    env = os.environ.copy()
    if _HOST != "Windows" or preset_name is None:
        return env

    mingw64_bin = "C:/eng_apps/msys64/mingw64/bin"
    mingw32_bin = "C:/eng_apps/msys64/mingw32/bin"
    msys_usr_bin = "C:/eng_apps/msys64/usr/bin"

    if "mingw32" in preset_name:
        prepend = [mingw32_bin, msys_usr_bin]
    else:
        prepend = [mingw64_bin, msys_usr_bin]

    sep = os.pathsep
    existing = env.get("PATH", "")
    env["PATH"] = sep.join(prepend) + sep + existing
    return env


def run(cmd: list[str], preset_name: str | None = None) -> int:
    """Stream a command to the console. Returns exit code."""
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    return subprocess.run(cmd, cwd=PROJECT_ROOT, env=_make_env(preset_name)).returncode


def run_capture(cmd: list[str], preset_name: str | None = None) -> tuple[int, str]:
    """Capture stdout+stderr. Returns (exit_code, output)."""
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    r = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True,
                       text=True, encoding="utf-8", errors="replace",
                       env=_make_env(preset_name))
    return r.returncode, r.stdout + r.stderr


# ---------------------------------------------------------------------------
# HTML test report
# ---------------------------------------------------------------------------

def _parse_junit(xml_path: Path) -> tuple[int, int, list[dict]]:
    tests: list[dict] = []
    total = failures = 0
    try:
        root = ET.parse(xml_path).getroot()
        for tc in root.iter("testcase"):
            total += 1
            f_el = tc.find("failure")
            e_el = tc.find("error")
            s_el = tc.find("skipped")
            if f_el is not None or e_el is not None:
                failures += 1
                status = "FAIL"
                msg = (f_el or e_el).get("message", "")  # type: ignore[union-attr]
            elif s_el is not None:
                status = "SKIP"
                msg = ""
            else:
                status = "PASS"
                msg = ""
            tests.append({
                "name": tc.get("name", "?"),
                "classname": tc.get("classname", ""),
                "time": float(tc.get("time", "0")),
                "status": status,
                "message": msg,
            })
    except Exception as exc:
        CONSOLE.print(f"[yellow]Warning: could not parse JUnit XML: {exc}[/]")
    return total, failures, tests


def _generate_html(out: Path, raw: str, junit: Optional[Path],
                   preset_name: str, meta: dict) -> None:
    total, failures, cases = 0, 0, []
    if junit and junit.exists():
        total, failures, cases = _parse_junit(junit)
    passed = total - failures
    ok = failures == 0
    sc = "#28a745" if ok else "#dc3545"

    rows = ""
    for tc in cases:
        bg = {"PASS": "#d4edda", "FAIL": "#f8d7da", "SKIP": "#fff3cd"}.get(tc["status"], "#fff")
        rows += (
            f"<tr style='background:{bg}'>"
            f"<td>{tc['classname']}</td><td>{tc['name']}</td>"
            f"<td style='text-align:center;font-weight:bold'>{tc['status']}</td>"
            f"<td style='text-align:right'>{tc['time']:.3f}s</td>"
            f"<td style='font-size:.8em;color:#555'>{tc['message'][:120]}</td>"
            f"</tr>\n"
        )
    tbl = (
        "<table border='1' cellspacing='0' cellpadding='6' "
        "style='border-collapse:collapse;width:100%;font-family:monospace;font-size:.9em'>"
        "<thead style='background:#343a40;color:white'>"
        "<tr><th>Suite</th><th>Test</th><th>Status</th><th>Time</th><th>Message</th></tr>"
        f"</thead><tbody>{rows}</tbody></table>"
    ) if cases else "<p><em>No test detail (JUnit XML unavailable).</em></p>"

    esc = raw.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    lbl = "ALL PASSED" if ok else f"{failures} FAILED"

    html = textwrap.dedent(f"""<!DOCTYPE html>
    <html lang="en"><head><meta charset="UTF-8"/>
    <title>Test Report — {meta['name']} {meta['version']}</title>
    <style>
      body{{font-family:'Segoe UI',Arial,sans-serif;margin:24px;background:#f5f5f5}}
      h1{{color:#1a1a2e}}
      .summary{{background:{sc};color:white;padding:12px 20px;border-radius:6px;
                font-size:1.2em;margin-bottom:20px}}
      .badge{{display:inline-block;padding:4px 12px;border-radius:12px;
              font-weight:bold;font-size:.85em}}
      .pass{{background:#28a745;color:white}} .fail{{background:#dc3545;color:white}}
      pre{{background:#1e1e1e;color:#d4d4d4;padding:16px;border-radius:6px;
           overflow-x:auto;font-size:.85em}}
      details summary{{cursor:pointer;color:#007bff;font-weight:bold;margin-top:20px}}
    </style></head>
    <body>
    <h1>Test Report — {meta['name']} {meta['version']}</h1>
    <div class="summary">
      Preset: <strong>{preset_name}</strong> &nbsp;|&nbsp;
      Total: <strong>{total}</strong> &nbsp;|&nbsp;
      <span class="badge {'pass' if ok else 'fail'}">{lbl}</span>
      &nbsp;({passed} passed, {failures} failed)
    </div>
    {tbl}
    <details open><summary>Raw CTest output</summary><pre>{esc}</pre></details>
    </body></html>
    """)
    out.write_text(html, encoding="utf-8")


# ---------------------------------------------------------------------------
# WSL / Docker setup helpers
# ---------------------------------------------------------------------------

def _check_wsl() -> bool:
    if _HOST == "Linux":
        return True
    wsl = shutil.which("wsl") or shutil.which("wsl.exe")
    if not wsl:
        CONSOLE.print("[red]WSL not found. Install WSL: wsl --install[/]")
        return False
    return True


def _check_docker(meta: dict) -> bool:
    docker = shutil.which("docker")
    if not docker:
        CONSOLE.print("[red]docker not found in PATH. Is Docker Desktop running?[/]")
        return False
    image = meta.get("docker_image", "lib-fmu-toolkit-builder")
    # Check image exists
    r = subprocess.run(["docker", "image", "inspect", image],
                       capture_output=True, text=True)
    if r.returncode != 0:
        CONSOLE.print(
            f"[bold red]Docker image '[yellow]{image}[/]' not found.[/]\n"
            f"Build it first:\n"
            f"  [bold]docker build -t {image} .[/bold]  (from project root)"
        )
        return False
    return True


def _check_executor(preset: dict, meta: dict) -> bool:
    """Return True if the executor for this preset is available and ready."""
    ex = preset["executor"]
    if ex == "native":
        if preset["host"] == "Windows" and _HOST != "Windows":
            CONSOLE.print("[red]Windows presets can only run on Windows.[/]")
            return False
        return True
    if ex == "wsl":
        return _check_wsl()
    if ex == "docker":
        return _check_docker(meta)
    return True


# ---------------------------------------------------------------------------
# Core actions
# ---------------------------------------------------------------------------

def action_select_preset(state: dict, meta: dict) -> None:
    CONSOLE.print()
    table = Table(title="All Presets", box=box.ROUNDED, header_style="bold cyan")
    table.add_column("#", justify="right", style="bold yellow")
    table.add_column("Preset Name", style="white")
    table.add_column("Description", style="dim")
    table.add_column("Arch", justify="center")
    table.add_column("Type", justify="center")
    table.add_column("Runner", justify="center")
    table.add_column("Ready", justify="center")

    available_indices: list[int] = []
    for idx, p in enumerate(PRESETS, start=1):
        avail = preset_is_available(p)
        marker = "[bold green]→[/]" if p["name"] == state["preset"] else "  "
        ready_str = "[green]✓[/]" if avail else "[red]✗[/]"
        if avail:
            available_indices.append(idx)
        table.add_row(
            f"{marker}{idx}",
            p["name"],
            p["display"],
            p["arch"],
            p["build_type"],
            executor_label(p),
            ready_str,
        )
    CONSOLE.print(table)

    if not available_indices:
        CONSOLE.print("[red]No presets available on this host.[/]")
        return

    CONSOLE.print(
        "[dim]✓ = ready  |  ✗ = unavailable (WSL/Docker not found or Windows-only)[/]"
    )
    choice = Prompt.ask(
        "Select preset number",
        choices=[str(i) for i in range(1, len(PRESETS) + 1)],
        default=str(available_indices[0]),
    )
    chosen = PRESETS[int(choice) - 1]
    if not preset_is_available(chosen):
        CONSOLE.print(f"[bold yellow]⚠ Preset '{chosen['name']}' is marked as unavailable — "
                      f"proceeding anyway.[/]")
    state["preset"] = chosen["name"]
    CONSOLE.print(f"[bold green]✓[/] Preset set to [bold]{state['preset']}[/]"
                  f" (executor: {chosen['executor']})")


def _get_preset(name: str) -> dict:
    for p in PRESETS:
        if p["name"] == name:
            return p
    raise KeyError(name)


def action_configure(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Configure — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False
    base = [cmake, "--preset", state["preset"]]
    # For WSL/Docker we invoke cmake from inside the environment; drop Windows cmake path
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--preset", state["preset"]]
    cmd = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Configure succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Configure failed (exit {code})[/]")
    return code == 0


def action_build(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Build — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False
    base = [cmake, "--build", "--preset", state["preset"]]
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--build", "--preset", state["preset"]]
    cmd = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Build succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Build failed (exit {code})[/]")
    return code == 0


def action_clean(state: dict) -> None:
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print(f"[yellow]Build directory not found: {build_dir}[/]")
        return
    if Confirm.ask(f"Delete [bold]{build_dir}[/]?", default=False):
        shutil.rmtree(build_dir)
        CONSOLE.print(f"[bold green]✓[/] Deleted {build_dir}")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


def action_clean_all() -> None:
    build_root = PROJECT_ROOT / "build"
    if not build_root.exists() or not any(build_root.iterdir()):
        CONSOLE.print("[yellow]Build directory is already empty.[/]")
        return
    subdirs = [d for d in build_root.iterdir() if d.is_dir()]
    for d in subdirs:
        CONSOLE.print(f"  [dim]{d}[/]")
    if Confirm.ask("Delete ALL build directories?", default=False):
        shutil.rmtree(build_root)
        CONSOLE.print("[bold green]✓[/] All build directories deleted.")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


def action_run_tests(state: dict, ctest: str, meta: dict) -> None:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Tests — {state['preset']} [{preset['executor']}]"))
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print("[red]Build directory not found. Configure + Build first.[/]")
        return
    if not _check_executor(preset, meta):
        return

    junit_xml = build_dir / "test_results.xml"
    html_report = build_dir / "test_results.html"

    base = ["ctest", "--preset", state["preset"],
            "--output-on-failure", "--output-junit", str(junit_xml)]
    if preset["executor"] == "native":
        base[0] = ctest
        # For native, use the Windows-style junit path as-is
    elif preset["executor"] == "wsl":
        # Inside WSL, paths must be WSL-style
        wsl_junit = _win_to_wsl_path(junit_xml) if _HOST == "Windows" else str(junit_xml)
        base = ["ctest", "--preset", state["preset"],
                "--output-on-failure", "--output-junit", wsl_junit]
    elif preset["executor"] == "docker":
        docker_junit = f"/workspace/build/{state['preset']}/test_results.xml"
        base = ["ctest", "--preset", state["preset"],
                "--output-on-failure", "--output-junit", docker_junit]

    cmd = build_cmd(preset, base, meta)
    pname = state["preset"] if preset["executor"] == "native" else None
    code, output = run_capture(cmd, preset_name=pname)
    CONSOLE.print(output)

    _generate_html(html_report, output, junit_xml, state["preset"], meta)
    CONSOLE.print(f"\n[bold]HTML report:[/] {html_report}")
    if Confirm.ask("Open HTML report in browser?", default=True):
        webbrowser.open(html_report.as_uri())

    if code == 0:
        CONSOLE.print("[bold green]✓ All tests passed[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Some tests failed (exit {code})[/]")


_MINGW_WINPTHREAD = [
    "C:/eng_apps/msys64/mingw64/bin/libwinpthread-1.dll",
    "C:/eng_apps/msys64/mingw32/bin/libwinpthread-1.dll",
]


def _copy_mingw_runtime(preset_name: str, bin_dir: Path) -> None:
    if _HOST != "Windows":
        return
    src: Optional[Path] = None
    for c in _MINGW_WINPTHREAD:
        p = Path(c)
        if not p.exists():
            continue
        if "mingw32" in preset_name and "mingw32" in str(p):
            src = p; break
        if "mingw64" in preset_name and "mingw64" in str(p):
            src = p; break
    if src is None:
        for c in _MINGW_WINPTHREAD:
            p = Path(c)
            if p.exists():
                src = p; break
    if src is None:
        CONSOLE.print("[yellow]  libwinpthread-1.dll not found — add it manually to bin/ if needed.[/]")
        return
    dst = bin_dir / "libwinpthread-1.dll"
    shutil.copy2(src, dst)
    CONSOLE.print(f"  [dim]Bundled:[/] {dst.name} ← {src}")


def _deploy_docker(preset_meta: dict, meta: dict, prefix: Path) -> int:
    """
    Run cmake --install inside Docker, then copy the staged output to prefix.

    Inside the container cmake --install writes to /workspace/deploy/<preset>.
    On Windows the volume-mounted workspace exposes that as
    PROJECT_ROOT/deploy/<preset>, which we then copy to the final prefix.
    """
    image = meta.get("docker_image", "lib-fmu-toolkit-builder")
    preset_name = preset_meta["name"]
    # Linux path inside the container for the install prefix
    docker_prefix = f"/workspace/deploy/{preset_name}"
    volume = _win_to_docker_volume(PROJECT_ROOT) if _HOST == "Windows" else str(PROJECT_ROOT)

    cmd = [
        "docker", "run", "--rm",
        "-v", f"{volume}:/workspace",
        image,
        "cmake", "--install", f"/workspace/build/{preset_name}",
        "--prefix", docker_prefix,
    ]
    CONSOLE.print(f"  [dim]docker cmake --install → {docker_prefix}[/]")
    code = run(cmd)
    if code != 0:
        return code

    # Copy the staged install from PROJECT_ROOT/deploy/<preset> → prefix
    stage = PROJECT_ROOT / "deploy" / preset_name
    if not stage.exists():
        CONSOLE.print(f"[red]Staged install not found at {stage}[/]")
        return 1

    if prefix.exists():
        shutil.rmtree(prefix)
    shutil.copytree(stage, prefix)
    CONSOLE.print(f"  [dim]Copied {stage} → {prefix}[/]")
    return 0


def action_deploy(state: dict, cmake: str, meta: dict) -> None:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Deploy — {state['preset']}"))
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print("[red]Build directory not found. Configure + Build first.[/]")
        return

    deploy_base = Path(meta.get("deploy_folder", "")).expanduser()
    if not deploy_base:
        CONSOLE.print("[red]'deploy_folder' not set in project.json[/]")
        return

    prefix = deploy_base / meta["name"] / meta["version"] / state["preset"]
    CONSOLE.print(f"  Build dir : [dim]{build_dir}[/]")
    CONSOLE.print(f"  Deploy to : [bold]{prefix}[/]")

    if not Confirm.ask("Proceed with deploy?", default=True):
        CONSOLE.print("[dim]Cancelled.[/]")
        return

    prefix.mkdir(parents=True, exist_ok=True)

    executor = preset.get("executor", "native")
    if executor == "docker":
        code = _deploy_docker(preset, meta, prefix)
    else:
        # native / wsl: cmake --install runs on the host directly
        pname = state["preset"] if executor == "native" else None
        code = run([cmake, "--install", str(build_dir), "--prefix", str(prefix)],
                   preset_name=pname)

    if code == 0:
        bin_dir = prefix / "bin"
        if bin_dir.exists() and "windows" in state["preset"]:
            _copy_mingw_runtime(state["preset"], bin_dir)
        CONSOLE.print(f"\n[bold green]✓ Deployed to:[/] {prefix}")
        _print_tree(prefix)
    else:
        CONSOLE.print(f"[bold red]✗ Deploy failed (exit {code})[/]")


def action_docker_build_image(meta: dict) -> None:
    """Build the Docker image from the project Dockerfile."""
    CONSOLE.print(Rule("[bold cyan]Build Docker image"))
    image = meta.get("docker_image", "lib-fmu-toolkit-builder")
    dockerfile = PROJECT_ROOT / "Dockerfile"
    if not dockerfile.exists():
        CONSOLE.print("[red]Dockerfile not found at project root.[/]")
        return
    CONSOLE.print(f"Building image [bold]{image}[/] from {dockerfile} ...")
    code = run(["docker", "build", "-t", image, str(PROJECT_ROOT)])
    if code == 0:
        CONSOLE.print(f"[bold green]✓ Image '{image}' built.[/]")
    else:
        CONSOLE.print(f"[bold red]✗ docker build failed (exit {code})[/]")


def _print_tree(prefix: Path) -> None:
    table = Table(box=box.SIMPLE, header_style="bold magenta", show_header=True)
    table.add_column("Path", style="white")
    table.add_column("Size", justify="right", style="dim")
    for p in sorted(prefix.rglob("*")):
        if p.is_file():
            rel = p.relative_to(prefix)
            sz = p.stat().st_size
            table.add_row(str(rel), f"{sz/1024:.1f} KB" if sz >= 1024 else f"{sz} B")
    CONSOLE.print(table)


# ---------------------------------------------------------------------------
# Menu / header
# ---------------------------------------------------------------------------

def _executor_for_current(name: str) -> str:
    try:
        p = _get_preset(name)
        if p["executor"] == "native":
            return "native"
        if p["executor"] == "wsl":
            return "WSL" if _HOST == "Windows" else "native"
        return p["executor"]
    except KeyError:
        return "?"


def print_header(state: dict, meta: dict) -> None:
    ex = _executor_for_current(state["preset"])
    CONSOLE.print()
    CONSOLE.print(Panel(
        f"[bold white]{meta['name']}[/]  [dim]v{meta['version']}[/]\n"
        f"{meta.get('description', '')}\n\n"
        f"Preset   : [bold yellow]{state['preset']}[/]  "
        f"([italic]{ex}[/italic])\n"
        f"Build    : [dim]{PROJECT_ROOT / 'build' / state['preset']}[/]\n"
        f"Deploy → : [dim]{Path(meta.get('deploy_folder', 'N/A')) / meta['name'] / meta['version'] / state['preset']}[/]",
        title="[bold cyan]lib-fmu-toolkit  Project Manager[/]",
        border_style="cyan",
        expand=False,
    ))


def print_menu() -> None:
    txt = (
        "[bold cyan]Preset[/]\n"
        "  [bold yellow][1][/] Select Preset  (all platforms)\n\n"
        "[bold cyan]Build[/]\n"
        "  [bold yellow][2][/] Configure\n"
        "  [bold yellow][3][/] Build\n"
        "  [bold yellow][4][/] Clean current preset\n"
        "  [bold yellow][5][/] Clean ALL presets\n\n"
        "[bold cyan]Test[/]\n"
        "  [bold yellow][6][/] Run Tests + HTML report\n\n"
        "[bold cyan]Deploy[/]\n"
        "  [bold yellow][7][/] Deploy to shared_lib folder\n\n"
        "[bold cyan]Docker[/]\n"
        "  [bold yellow][8][/] Build Docker image (lib-fmu-toolkit-builder)\n\n"
        "[bold cyan]Exit[/]\n"
        "  [bold yellow][0][/] Quit"
    )
    CONSOLE.print(Panel(txt, title="Menu", border_style="dim white", expand=False))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    meta = load_metadata()
    cmake = find_cmake()
    ctest = find_ctest()

    if not cmake:
        CONSOLE.print(
            "[bold red]cmake not found.[/]\n"
            "Add CMake to PATH or install at C:/eng_apps/cmake/cmake-4.2.0/"
        )
        sys.exit(1)

    CONSOLE.print(f"[dim]cmake  : {cmake}[/]")
    CONSOLE.print(f"[dim]ctest  : {ctest or 'NOT FOUND'}[/]")
    CONSOLE.print(f"[dim]host   : {_HOST}[/]")
    docker_ok = shutil.which("docker") is not None
    CONSOLE.print(f"[dim]docker : {'available' if docker_ok else 'not found'}[/]")
    wsl_ok = shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    CONSOLE.print(f"[dim]wsl    : {'available' if wsl_ok or _HOST=='Linux' else 'not found'}[/]")

    # Default to mingw64-debug on Windows, linux-mingw-debug on Linux
    if _HOST == "Windows":
        default_preset = "windows-mingw64-debug"
    else:
        default_preset = "linux-mingw-debug"

    state: dict = {"preset": default_preset}

    while True:
        print_header(state, meta)
        print_menu()
        choice = Prompt.ask(
            "[bold]Choice",
            choices=["0", "1", "2", "3", "4", "5", "6", "7", "8"],
            default="0",
        )
        if choice == "0":
            CONSOLE.print("[bold]Bye![/]")
            break
        elif choice == "1":
            action_select_preset(state, meta)
        elif choice == "2":
            action_configure(state, cmake, meta)
        elif choice == "3":
            action_build(state, cmake, meta)
        elif choice == "4":
            action_clean(state)
        elif choice == "5":
            action_clean_all()
        elif choice == "6":
            if not ctest and _get_preset(state["preset"])["executor"] == "native":
                CONSOLE.print("[red]ctest not found.[/]")
            else:
                action_run_tests(state, ctest or "ctest", meta)
        elif choice == "7":
            action_deploy(state, cmake, meta)
        elif choice == "8":
            action_docker_build_image(meta)

        CONSOLE.print()
        input("Press Enter to continue...")


if __name__ == "__main__":
    main()
