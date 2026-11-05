"""Renders installer/app_setup.iss.in and (optionally) compiles it with Inno
Setup 6 into a Windows installer. Only applicable to app/tool/catalog.
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
import uuid
from pathlib import Path

from prj_metadata import (
    PROJECT_ROOT,
    deploy_directory,
    find_inno_setup_compiler,
    load_metadata,
    main_executable_name,
    run_cli_main,
    stale_build_version,
)
from prj_models import ProjectModel, save_project_model

INSTALLER_CATEGORIES = {"app", "tool", "catalog", "app-qml"}
DEFAULT_PUBLISHER = "Cristian Tesconi"


def executable_path(bin_dir: Path, executable_name: str) -> Path | None:
    candidate = bin_dir / executable_name
    return candidate if candidate.exists() else None


def _guid_fingerprint(guid: str, version: str) -> str:
    return hashlib.sha256(f"{guid}:{version}".encode("utf-8")).hexdigest()


def ensure_guid(project_root: Path) -> str | None:
    """Mints a fresh installer GUID (AppId) the first time it's needed, and
    refuses to proceed if project.json's version was bumped without a fresh
    GUID: reusing the same AppId across versions makes Windows treat a new
    version as an in-place upgrade of the previous one, silently orphaning
    the previous version's (still on-disk, side-by-side) install directory.
    """
    project_json = project_root / "project.json"
    project = load_metadata(project_root)

    guid = project.guid or str(uuid.uuid4())

    if project.guid_version is None:
        # Either just minted above, or assigned by the generator at
        # `prj_init.py` time but never fingerprinted for an installer build
        # yet - either way this is a first use, not a stale one: fingerprint
        # it for the current version and proceed.
        data = project.model_dump(exclude_unset=True)
        data["guid"] = guid
        data["guid_version"] = project.version
        data["guid_fingerprint"] = _guid_fingerprint(guid, project.version)
        save_project_model(project_json, ProjectModel.model_validate(data))
        return guid

    if project.guid_version != project.version:
        raise RuntimeError(
            "project.json's version changed since the installer GUID was last generated "
            f"(guid_version={project.guid_version!r}, current version={project.version!r}). "
            "Set \"guid\": null in project.json and re-run to mint a fresh GUID for this version."
        )

    expected_fingerprint = _guid_fingerprint(project.guid, project.version)
    if project.guid_fingerprint != expected_fingerprint:
        raise RuntimeError(
            "project.json's guid_fingerprint does not match its guid/version "
            "(guid_version or guid_fingerprint may have been hand-edited). "
            "Set \"guid\": null in project.json and re-run to mint a fresh GUID."
        )

    return project.guid


def render_iss_script(project: ProjectModel, guid: str, preset: str, source_dir: Path, output_dir: Path) -> str:
    template_path = PROJECT_ROOT / "installer" / "app_setup.iss.in"
    text = template_path.read_text(encoding="utf-8")

    icon_path = PROJECT_ROOT / "resources" / "img" / "app_icon.ico"
    icon_directive = f'SetupIconFile="{icon_path}"' if icon_path.exists() else ""

    tokens = {
        "__APP_NAME__": project.application_name or project.name,
        "__APP_REGISTRY_NAME__": project.name,
        "__APP_VERSION__": f"{project.version}.{project.build}",
        "__APP_VERSION_BASE__": project.version,
        "__APP_PUBLISHER__": DEFAULT_PUBLISHER,
        # Relative to {app}: the deploy layout keeps executables under bin\.
        "__APP_EXE_NAME__": "bin\\" + main_executable_name(project),
        # Bare filename (no directory) - what taskkill's /IM expects.
        "__APP_EXE_BASENAME__": main_executable_name(project),
        "__APP_GUID__": guid,
        "__OUTPUT_DIR__": str(output_dir),
        "__OUTPUT_BASE_FILENAME__": f"{project.name}-setup-{project.version}",
        "__SOURCE_DIR__": str(source_dir),
        "__SETUP_ICON_FILE_DIRECTIVE__": icon_directive,
    }

    for token, value in tokens.items():
        text = text.replace(token, value)
    return text


def build_installer(project_root: Path, preset: str, iss_only: bool = False) -> int:
    project = load_metadata(project_root)

    if project.category not in INSTALLER_CATEGORIES:
        print(f"Installer not applicable to category '{project.category}'.")
        return 0

    template_path = project_root / "installer" / "app_setup.iss.in"
    if not template_path.exists():
        print(f"No installer template found at {template_path}; skipping.")
        return 0

    if sys.platform != "win32":
        print("The installer can only be built on Windows (Inno Setup).")
        return 1

    try:
        guid = ensure_guid(project_root)
    except RuntimeError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    stale = stale_build_version(project_root, preset, project)
    if stale:
        print(
            f"Error: the build under preset '{preset}' was built for version {stale}, "
            f"but project.json now says {project.version}. Rebuild first.",
            file=sys.stderr,
        )
        return 1

    source_dir = deploy_directory(project_root, preset, project)
    if not source_dir.exists():
        print(f"Error: deploy directory not found at {source_dir}. Deploy first.", file=sys.stderr)
        return 1

    executable_name = main_executable_name(project)
    if executable_path(source_dir / "bin", executable_name) is None:
        print(f"Error: no executable '{executable_name}' found under {source_dir / 'bin'}.", file=sys.stderr)
        return 1

    installer_dir = project_root / "installer" / project.version
    installer_dir.mkdir(parents=True, exist_ok=True)

    script_text = render_iss_script(project, guid, preset, source_dir, installer_dir)
    iss_path = installer_dir / "Setup.iss"
    iss_path.write_text(script_text, encoding="utf-8")

    if iss_only:
        print(f"Wrote {iss_path} (--iss-only, not compiled).")
        return 0

    compiler = find_inno_setup_compiler()
    if compiler is None:
        print("Error: Inno Setup 6 (ISCC.exe) was not found.", file=sys.stderr)
        return 1

    result = subprocess.run([compiler, str(iss_path)])
    return result.returncode


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the Windows installer.")
    parser.add_argument("--preset", required=True)
    parser.add_argument("--iss-only", action="store_true", help="Render Setup.iss without compiling it.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    sys.exit(build_installer(PROJECT_ROOT, args.preset, iss_only=args.iss_only))


if __name__ == "__main__":
    run_cli_main(main)
