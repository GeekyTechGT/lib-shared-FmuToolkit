@echo off
REM Copy this file to scripts\vcpkg_registry_config.bat and customize the values.

set "VCPKG_REGISTRY_REPO=D:\path\to\your-vcpkg-registry"
set "VCPKG_SOURCE_REPOSITORY=https://github.com/<org>/lib-fmu-toolkit.git"
set "VCPKG_SOURCE_REF=v0.1.0"
set "VCPKG_PORT_HOMEPAGE=https://github.com/<org>/lib-fmu-toolkit"
set "VCPKG_REGISTRY_COMMIT_MESSAGE=Publish lib-fmu-toolkit 0.1.0"
