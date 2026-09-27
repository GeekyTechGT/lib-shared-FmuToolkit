"""Pydantic schema for this project's own project.json.

Runtime copy: shipped inside every generated CMake project and operates on
that project's own project.json. Schema must stay in sync with
scripts/prj_models.py in the generator repository - see CLAUDE.md there.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Literal

from pydantic import AliasChoices, BaseModel, ConfigDict, Field, ValidationError, field_validator


class TestReportsModel(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    enabled: bool
    directory: str
    save_text: bool
    save_html: bool
    console_tail_characters: int = Field(ge=0)
    artifact_globs: list[str] = Field(default_factory=list)
    artifact_max_bytes: int = Field(ge=0)
    notes: str


class SubmoduleModel(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    name: str
    url: str
    branch: str | None = None
    tag: str | None = None


class DependencyCopyMappingModel(BaseModel):
    """One explicit copy step for a dependency's deployed artifacts.

    `source` is relative to the dependency's own deployed root
    (`<deploy_folder>/<name>/<version>/<preset>/`); `destination` is relative
    to the consuming project's deploy prefix. Either side may name a file or
    a directory - resolved at copy time, not declared here.
    """

    model_config = ConfigDict(extra="forbid", frozen=True)

    source: str
    destination: str

    @field_validator("source", "destination")
    @classmethod
    def _must_not_be_blank(cls, value: str) -> str:
        cleaned = value.strip()
        if not cleaned:
            raise ValueError("value must not be blank")
        return cleaned


class DependencyModel(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    # Folder name of the dependency under deploy_folder - by convention the
    # producing library's artifact_name, e.g. "lib-shared-QtCustomWidgets".
    name: str
    version: str
    # Library base name override (lib<library>.dll / lib<library>.a). Empty =
    # derived from `name` by cmake/Dependencies/Dependencies.cmake: category
    # prefix stripped, separators removed, lower-cased ("qtcustomwidgets").
    library: str | None = None
    # Empty = legacy default: copy files directly under the dependency's own
    # bin/ into the consumer's bin/. Non-empty replaces that default entirely
    # with explicit source -> destination copies. "deploy" is accepted as the
    # key's former name.
    copy_to: list[DependencyCopyMappingModel] = Field(
        default_factory=list, validation_alias=AliasChoices("copy_to", "deploy")
    )


class RuntimeModel(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    dlls: list[str] = Field(default_factory=list)
    source_path: str


class ProjectModel(BaseModel):
    model_config = ConfigDict(extra="allow", frozen=True)

    name: str
    version: str
    build: int = Field(ge=0)
    description: str
    application_name: str | None = None
    license: str
    environment: Literal["dev", "test", "production"]
    deploy_folder: str
    docker_image: str | None = None
    category: (
        Literal[
            "app", "tool", "lib-shared", "lib-static", "catalog", "webapp", "svc-python",
            "svc-cpp", "app-qml", "lib-wasm", "desktop-web", "workspace", "container",
        ]
        | None
    ) = None
    library_type: Literal["static", "shared"] | None = None
    # lib-shared/lib-static only: folder name under deploy_folder (and the name
    # consumers declare as a dependency), e.g. "lib-shared-QtCustomWidgets".
    # Falls back to `name` when absent.
    artifact_name: str | None = None
    guid: str | None = None
    guid_version: str | None = None
    guid_fingerprint: str | None = None
    splash_screen_enabled: bool = True
    tool_variant: Literal["cli", "gui", "both"] = "both"
    webapp_variant: Literal["static", "frontend", "fullstack"] | None = None
    language: Literal["c", "cpp"] = "cpp"
    test_reports: TestReportsModel | None = None
    submodules: list[SubmoduleModel] = Field(default_factory=list)
    dependencies: list[DependencyModel] = Field(default_factory=list)
    runtimes: dict[str, RuntimeModel] = Field(default_factory=dict)

    @field_validator("name", "version", "description", "license", "deploy_folder")
    @classmethod
    def _must_not_be_blank(cls, value: str) -> str:
        cleaned = value.strip()
        if not cleaned:
            raise ValueError("value must not be blank")
        return cleaned


class ProjectModelError(RuntimeError):
    """Raised when project.json cannot be read, parsed, or validated."""


def write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=4) + "\n", encoding="utf-8")


def load_project_model(project_json: Path) -> ProjectModel:
    try:
        raw = project_json.read_text(encoding="utf-8")
    except OSError as exc:
        raise ProjectModelError(f"cannot read {project_json}: {exc}") from exc

    try:
        return ProjectModel.model_validate_json(raw)
    except (json.JSONDecodeError, ValidationError) as exc:
        raise ProjectModelError(f"invalid project.json at {project_json}: {exc}") from exc


def save_project_model(project_json: Path, project_model: ProjectModel) -> None:
    write_json(project_json, project_model.model_dump(exclude_unset=True))
