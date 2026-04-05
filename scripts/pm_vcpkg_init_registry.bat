@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "CONFIG_FILE=%SCRIPT_DIR%vcpkg_registry_config.bat"
if exist "%CONFIG_FILE%" call "%CONFIG_FILE%"

call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" name PORT_NAME
if errorlevel 1 exit /b 1

set "REGISTRY_REPO=%~1"
if "%REGISTRY_REPO%"=="" set "REGISTRY_REPO=%VCPKG_REGISTRY_REPO%"
if "%REGISTRY_REPO%"=="" (
    echo Missing registry repository path.
    echo Pass it as the first argument or configure VCPKG_REGISTRY_REPO.
    exit /b 1
)

set "VERSIONS_DIR=%REGISTRY_REPO%\versions"
set "PORTS_DIR=%REGISTRY_REPO%\ports"
set "PORT_VERSION_FILE=%VERSIONS_DIR%\%PORT_NAME:~0,1%-\%PORT_NAME%.json"

if not exist "%REGISTRY_REPO%" mkdir "%REGISTRY_REPO%"
if not exist "%PORTS_DIR%" mkdir "%PORTS_DIR%"
if not exist "%VERSIONS_DIR%" mkdir "%VERSIONS_DIR%"
if not exist "%VERSIONS_DIR%\%PORT_NAME:~0,1%-" mkdir "%VERSIONS_DIR%\%PORT_NAME:~0,1%-"

if not exist "%VERSIONS_DIR%\baseline.json" (
    >"%VERSIONS_DIR%\baseline.json" echo {
    >>"%VERSIONS_DIR%\baseline.json" echo   "default": {}
    >>"%VERSIONS_DIR%\baseline.json" echo }
)

if not exist "%PORT_VERSION_FILE%" (
    >"%PORT_VERSION_FILE%" echo {
    >>"%PORT_VERSION_FILE%" echo   "versions": []
    >>"%PORT_VERSION_FILE%" echo }
)

echo Registry layout ready: %REGISTRY_REPO%
exit /b 0

:read_manifest_value
set "MANIFEST_FILE=%~1"
set "JSON_KEY=%~2"
set "TARGET_VAR=%~3"
set "RAW_VALUE="
for /f "usebackq delims=" %%A in (`powershell -NoProfile -Command "$json = Get-Content -Raw '%MANIFEST_FILE%' | ConvertFrom-Json; $value = $json.PSObject.Properties['%JSON_KEY%'].Value; if ($null -ne $value) { [Console]::Write($value) }"`) do set "RAW_VALUE=%%A"
if not defined RAW_VALUE exit /b 1
set "%TARGET_VAR%=%RAW_VALUE%"
exit /b 0
