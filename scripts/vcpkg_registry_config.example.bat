@echo off
REM Copy this file to scripts\vcpkg_registry_config.bat and customize the values.

set "VCPKG_REGISTRY_REPO=D:\path\to\your-vcpkg-registry"
set "VCPKG_SOURCE_REPOSITORY=https://github.com/<org>/<library-repo>.git"
set "VCPKG_SOURCE_REF=v0.1.0"
set "VCPKG_PORT_HOMEPAGE=https://github.com/<org>/<library-repo>"
set "VCPKG_REGISTRY_COMMIT_MESSAGE=Publish your-library 0.1.0"
