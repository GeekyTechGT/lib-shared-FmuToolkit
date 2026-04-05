@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "CONFIG_FILE=%SCRIPT_DIR%vcpkg_registry_config.bat"
if exist "%CONFIG_FILE%" call "%CONFIG_FILE%"

call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" name PORT_NAME
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" version-string PORT_VERSION
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" description PORT_DESCRIPTION
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" license PORT_LICENSE

if not defined VCPKG_PORT_HOMEPAGE set "VCPKG_PORT_HOMEPAGE=%VCPKG_SOURCE_REPOSITORY%"

echo.
echo ======================================================
echo                 Vcpkg Registry Status
echo ======================================================
echo.
echo   Port name          : %PORT_NAME%
echo   Port version       : %PORT_VERSION%
echo   Port description   : %PORT_DESCRIPTION%
echo   Port license       : %PORT_LICENSE%
echo   Source repository  : %VCPKG_SOURCE_REPOSITORY%
echo   Source ref         : %VCPKG_SOURCE_REF%
echo   Homepage           : %VCPKG_PORT_HOMEPAGE%
echo   Registry repo      : %VCPKG_REGISTRY_REPO%
if defined VCPKG_REGISTRY_REPO if exist "%VCPKG_REGISTRY_REPO%\versions\baseline.json" (
    echo   Registry layout    : ready
) else (
    echo   Registry layout    : missing or not initialized
)
if not defined VCPKG_REGISTRY_REPO echo   Registry layout    : missing or not initialized
where git >nul 2>nul
if errorlevel 1 (
    echo   Git                : missing
) else (
    echo   Git                : available
)
call :resolve_vcpkg VCPKG_EXE
if defined VCPKG_EXE (
    echo   vcpkg              : %VCPKG_EXE%
) else (
    echo   vcpkg              : missing
)
echo.
exit /b 0

:resolve_vcpkg
set "%~1="
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\vcpkg.exe" set "%~1=%VCPKG_ROOT%\vcpkg.exe"
if defined %~1 exit /b 0
where vcpkg >nul 2>nul
if not errorlevel 1 set "%~1=vcpkg"
exit /b 0

:read_manifest_value
set "MANIFEST_FILE=%~1"
set "JSON_KEY=%~2"
set "TARGET_VAR=%~3"
set "RAW_VALUE="
for /f "usebackq delims=" %%A in (`powershell -NoProfile -Command "$json = Get-Content -Raw '%MANIFEST_FILE%' | ConvertFrom-Json; $value = $json.PSObject.Properties['%JSON_KEY%'].Value; if ($null -ne $value) { [Console]::Write($value) }"`) do set "RAW_VALUE=%%A"
if not defined RAW_VALUE exit /b 0
set "%TARGET_VAR%=%RAW_VALUE%"
exit /b 0
