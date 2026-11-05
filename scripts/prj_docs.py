"""Markdown -> PDF rendering for the documents shipped in every deploy
folder's documents/ (USER_MANUAL.pdf, CHANGELOG.pdf).

Two backends, tried in order:
    1. markdown-pdf  (pure Python, declared in pyproject.toml - no LaTeX,
       no system tools; the default on every machine where `uv sync` ran)
    2. pandoc        (only if markdown-pdf is unavailable and pandoc plus a
       PDF engine happen to be installed)

Both are best-effort: a missing backend is reported, never raised, so a
deploy still succeeds on a machine that cannot render PDFs.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class DocumentSpec:
    """One Markdown source rendered to one PDF under the documents folder."""

    source: Path
    pdf_name: str
    title: str


def project_documents(project_root: Path, display_name: str) -> list[DocumentSpec]:
    """The documents every project (app, tool, lib-shared, lib-static) ships
    in its deploy folder's documents/."""
    return [
        DocumentSpec(
            project_root / "docs" / "user_manual" / "USER_MANUAL.md",
            "USER_MANUAL.pdf",
            f"{display_name} - User Manual",
        ),
        DocumentSpec(project_root / "CHANGELOG.md", "CHANGELOG.pdf", f"{display_name} - Changelog"),
    ]


# markdown-it emits no heading ids, so PyMuPDF's Story cannot resolve in-page
# "#anchor" links and aborts the whole render ("No destination with id=...").
# The PDF already gets a bookmark outline (toc_level), so in-page links are
# flattened to their plain text before rendering.
_INTERNAL_LINK = re.compile(r"\[([^\]]+)\]\(#[^)]*\)")


def _strip_internal_links(text: str) -> str:
    return _INTERNAL_LINK.sub(lambda m: m.group(1), text)


def _render_with_markdown_pdf(source: Path, destination: Path, title: str) -> str | None:
    try:
        from markdown_pdf import MarkdownPdf, Section
    except ImportError:
        return "markdown-pdf is not installed (run `uv sync`)"

    # "js-default" = markdown-it's default preset: CommonMark plus GFM tables
    # and strikethrough, without "gfm-like"'s extra linkify-it dependency.
    pdf = MarkdownPdf(toc_level=2, mode="js-default")
    pdf.meta["title"] = title
    # root= lets relative image links in the Markdown resolve against the
    # source file's own folder.
    pdf.add_section(Section(_strip_internal_links(source.read_text(encoding="utf-8")), root=str(source.parent)))
    pdf.save(str(destination))
    return None


def _render_with_pandoc(source: Path, destination: Path, title: str) -> str | None:
    pandoc = shutil.which("pandoc")
    if pandoc is None:
        return "pandoc is not on PATH"
    result = subprocess.run(
        [pandoc, str(source), "--metadata", f"title={title}", "-o", str(destination)],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return f"pandoc failed: {result.stderr.strip()}"
    return None


def markdown_to_pdf(source: Path, destination: Path, title: str) -> str | None:
    """Renders `source` to `destination`. Returns None on success, or a
    human-readable reason why no backend could render it."""
    if not source.exists():
        return f"{source} does not exist"

    destination.parent.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []
    for backend in (_render_with_markdown_pdf, _render_with_pandoc):
        try:
            reason = backend(source, destination, title)
        except Exception as exc:  # noqa: BLE001 - a broken backend must not abort the deploy
            reason = f"{backend.__name__} raised {exc!r}"
        if reason is None:
            return None
        failures.append(reason)
    return "; ".join(failures)


def render_documents(specs: list[DocumentSpec], output_dir: Path) -> list[tuple[DocumentSpec, str | None]]:
    """Renders every spec into `output_dir`. Returns (spec, failure-or-None)
    pairs so the caller decides how to report them."""
    return [(spec, markdown_to_pdf(spec.source, output_dir / spec.pdf_name, spec.title)) for spec in specs]
