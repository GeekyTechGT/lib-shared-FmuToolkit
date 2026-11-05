"""Runs CTest for the current project and writes text/HTML reports.

Driven by project.json's "test_reports" block: where to write reports, how
much console output to keep, and which extra artifacts to collect.
"""

from __future__ import annotations

import argparse
import html
import subprocess
import sys
import webbrowser
import xml.etree.ElementTree as ElementTree
from dataclasses import dataclass
from pathlib import Path

from prj_metadata import PROJECT_ROOT, build_directory, load_metadata, run_cli_main, run_command, runtime_environment
from prj_models import ProjectModel, TestReportsModel
from rich.console import Console

console = Console()


@dataclass
class TestCaseResult:
    name: str
    classname: str
    status: str  # "passed" | "failed" | "skipped"
    duration_seconds: float
    message: str = ""


def report_directory(project_root: Path, preset: str, test_reports: TestReportsModel) -> Path:
    relative = test_reports.directory.replace("{preset}", preset)
    return project_root / relative


def load_test_descriptions(project_root: Path) -> dict[str, str]:
    descriptions_path = project_root / "tests" / "descriptions.json"
    if not descriptions_path.exists():
        return {}
    import json

    try:
        return json.loads(descriptions_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def write_text_report(destination: Path, output: str) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(output, encoding="utf-8")


def parse_junit_report(path: Path) -> list[TestCaseResult] | None:
    if not path.exists():
        return None

    try:
        tree = ElementTree.parse(path)
    except ElementTree.ParseError:
        return None

    root = tree.getroot()
    suites = [root] if root.tag == "testsuite" else list(root.findall("testsuite"))

    results: list[TestCaseResult] = []
    for suite in suites:
        suite_name = suite.get("name", "")
        for testcase in suite.findall("testcase"):
            name = testcase.get("name", "")
            classname = testcase.get("classname", suite_name)
            duration = float(testcase.get("time", "0") or "0")

            failure = testcase.find("failure")
            error = testcase.find("error")
            skipped = testcase.find("skipped")

            if failure is not None or error is not None:
                status = "failed"
                node = failure if failure is not None else error
                message = (node.get("message") or "").strip()
            elif skipped is not None:
                status = "skipped"
                message = (skipped.get("message") or "").strip()
            else:
                status = "passed"
                message = ""

            results.append(TestCaseResult(name=name, classname=classname, status=status,
                                           duration_seconds=duration, message=message))
    return results


def _format_duration(seconds: float) -> str:
    if seconds < 60:
        return f"{seconds:.2f}s"
    minutes, remainder = divmod(int(seconds), 60)
    return f"{minutes}m {remainder:02d}s"


def _find_description(test: TestCaseResult, descriptions: dict[str, str]) -> str | None:
    return descriptions.get(test.name) or descriptions.get(test.classname)


def _status_badge(status: str) -> str:
    colors = {"passed": "#1a7f37", "failed": "#cf222e", "skipped": "#9a6700"}
    color = colors.get(status, "#57606a")
    return f'<span style="color:{color};font-weight:600">{html.escape(status.upper())}</span>'


def _render_test_card(test: TestCaseResult, description: str | None) -> str:
    description_html = f"<p>{html.escape(description)}</p>" if description else ""
    message_html = f"<pre>{html.escape(test.message)}</pre>" if test.message else ""
    return f"""
    <div class="test-card">
        <div class="test-card-header">
            <span class="test-name">{html.escape(test.name)}</span>
            {_status_badge(test.status)}
            <span class="test-duration">{_format_duration(test.duration_seconds)}</span>
        </div>
        {description_html}
        {message_html}
    </div>
    """


def _group_by_suite(test_cases: list[TestCaseResult]) -> dict[str, list[TestCaseResult]]:
    groups: dict[str, list[TestCaseResult]] = {}
    for test in test_cases:
        groups.setdefault(test.classname, []).append(test)
    return groups


def _render_suite_group(suite_name: str, tests: list[TestCaseResult], descriptions: dict[str, str]) -> str:
    cards = "".join(_render_test_card(test, _find_description(test, descriptions)) for test in tests)
    return f'<section><h2>{html.escape(suite_name)}</h2>{cards}</section>'


def write_html_report(destination: Path, project: ProjectModel, preset: str,
                       test_cases: list[TestCaseResult], descriptions: dict[str, str],
                       console_tail: str) -> None:
    passed = sum(1 for t in test_cases if t.status == "passed")
    failed = sum(1 for t in test_cases if t.status == "failed")
    skipped = sum(1 for t in test_cases if t.status == "skipped")

    groups = _group_by_suite(test_cases)
    sections = "".join(_render_suite_group(name, tests, descriptions) for name, tests in groups.items())

    document = f"""<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>{html.escape(project.name)} - test report ({html.escape(preset)})</title>
<style>
body {{ font-family: system-ui, sans-serif; margin: 2rem; background: #f6f8fa; color: #1f2328; }}
h1 {{ margin-bottom: 0.25rem; }}
.summary {{ margin-bottom: 1.5rem; }}
.test-card {{ background: #fff; border: 1px solid #d0d7de; border-radius: 6px; padding: 0.75rem 1rem; margin: 0.5rem 0; }}
.test-card-header {{ display: flex; gap: 1rem; align-items: center; }}
.test-name {{ font-weight: 600; flex-grow: 1; }}
.test-duration {{ color: #57606a; font-size: 0.9em; }}
pre {{ background: #f6f8fa; padding: 0.5rem; overflow-x: auto; }}
details {{ margin-top: 2rem; }}
</style>
</head>
<body>
<h1>{html.escape(project.name)} {html.escape(project.version)}</h1>
<div class="summary">Preset: {html.escape(preset)} &middot;
<span style="color:#1a7f37">{passed} passed</span> &middot;
<span style="color:#cf222e">{failed} failed</span> &middot;
<span style="color:#9a6700">{skipped} skipped</span></div>
{sections}
<details>
<summary>Console output (tail)</summary>
<pre>{html.escape(console_tail)}</pre>
</details>
</body>
</html>
"""
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(document, encoding="utf-8")


def open_html_report(report_path: Path) -> None:
    try:
        webbrowser.open(report_path.as_uri())
    except Exception:
        pass


def _collect_artifacts(project_root: Path, destination: Path, globs: list[str], max_bytes: int) -> None:
    if not globs:
        return
    for pattern in globs:
        for match in project_root.glob(pattern):
            if not match.is_file():
                continue
            if match.stat().st_size > max_bytes:
                continue
            relative = match.relative_to(project_root)
            target = destination / "artifacts" / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(match.read_bytes())


def run_tests(preset: str, project_root: Path | None = None, prompt_open_report: bool = False) -> int:
    root = project_root or PROJECT_ROOT
    project = load_metadata(root)

    build_dir = build_directory(root, preset)
    if not build_dir.exists():
        print(f"Build directory not found: {build_dir}. Configure and build first.", file=sys.stderr)
        return 1

    env = runtime_environment(root, preset, project)
    test_reports = project.test_reports
    if test_reports is None or not test_reports.enabled:
        return run_command(["ctest", "--test-dir", str(build_dir), "--output-on-failure"], env=env)

    destination = report_directory(root, preset, test_reports)
    junit_path = destination / "junit.xml"
    destination.mkdir(parents=True, exist_ok=True)

    process = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--output-on-failure", "--output-junit", str(junit_path)],
        capture_output=True, text=True, env=env,
    )
    console_output = process.stdout + process.stderr
    console_tail = console_output[-test_reports.console_tail_characters:]

    if test_reports.save_text:
        write_text_report(destination / "console.txt", console_output)

    test_cases = parse_junit_report(junit_path) or []
    descriptions = load_test_descriptions(root)

    if test_reports.save_html:
        write_html_report(destination / "report.html", project, preset, test_cases, descriptions, console_tail)
        if prompt_open_report:
            open_html_report(destination / "report.html")

    _collect_artifacts(root, destination, test_reports.artifact_globs, test_reports.artifact_max_bytes)

    print(console_tail)

    if test_cases:
        passed = sum(1 for t in test_cases if t.status == "passed")
        failed = sum(1 for t in test_cases if t.status == "failed")
        skipped = sum(1 for t in test_cases if t.status == "skipped")
        color = "bold green" if failed == 0 else "bold red"
        console.print(
            f"[{color}]{passed} passed[/{color}], [bold red]{failed} failed[/bold red], "
            f"[bold yellow]{skipped} skipped[/bold yellow]  (report: {destination / 'report.html'})"
        )
    elif process.returncode == 0:
        console.print("[dim]No tests were found.[/dim]")

    return process.returncode


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run CTest and write text/HTML reports.")
    parser.add_argument("--preset", required=True)
    parser.add_argument("--open-report", action="store_true", help="Open the HTML report in a browser when done.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    sys.exit(run_tests(args.preset, prompt_open_report=args.open_report))


if __name__ == "__main__":
    run_cli_main(main)
