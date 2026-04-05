@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "CONFIG_FILE=%SCRIPT_DIR%vcpkg_registry_config.bat"
if exist "%CONFIG_FILE%" call "%CONFIG_FILE%"

call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" name PORT_NAME
if errorlevel 1 exit /b 1
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" version-string PORT_VERSION
if errorlevel 1 exit /b 1
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" description PORT_DESCRIPTION
call :read_manifest_value "%PROJECT_ROOT%\vcpkg.json" license PORT_LICENSE

set "REGISTRY_REPO=%~1"
if "%REGISTRY_REPO%"=="" set "REGISTRY_REPO=%VCPKG_REGISTRY_REPO%"
set "SOURCE_REPOSITORY=%~2"
if "%SOURCE_REPOSITORY%"=="" set "SOURCE_REPOSITORY=%VCPKG_SOURCE_REPOSITORY%"
set "SOURCE_REF=%~3"
if "%SOURCE_REF%"=="" set "SOURCE_REF=%VCPKG_SOURCE_REF%"
set "COMMIT_MESSAGE=%~4"
if "%COMMIT_MESSAGE%"=="" set "COMMIT_MESSAGE=%VCPKG_REGISTRY_COMMIT_MESSAGE%"
if not defined VCPKG_PORT_HOMEPAGE set "VCPKG_PORT_HOMEPAGE=%SOURCE_REPOSITORY%"
if not defined PORT_LICENSE set "PORT_LICENSE=MIT"
if not defined PORT_DESCRIPTION set "PORT_DESCRIPTION=Professional C++23 library scaffold with PIMPL install export support and tests"

if "%REGISTRY_REPO%"=="" (
    echo Missing registry repository path.
    exit /b 1
)
if "%SOURCE_REPOSITORY%"=="" (
    echo Missing source repository URL.
    exit /b 1
)
if "%SOURCE_REF%"=="" set "SOURCE_REF=v%PORT_VERSION%"

where git >nul 2>nul
if errorlevel 1 (
    echo Git is required to publish to a Git registry.
    exit /b 1
)

git -C "%REGISTRY_REPO%" rev-parse --is-inside-work-tree >nul 2>nul
if errorlevel 1 (
    echo Registry path is not a Git repository: %REGISTRY_REPO%
    exit /b 1
)

call :resolve_vcpkg VCPKG_EXE
if not defined VCPKG_EXE (
    echo vcpkg executable not found. Set VCPKG_ROOT or add vcpkg to PATH.
    exit /b 1
)

call "%SCRIPT_DIR%pm_vcpkg_init_registry.bat" "%REGISTRY_REPO%"
if errorlevel 1 exit /b 1

set "PORT_DIR=%REGISTRY_REPO%\ports\%PORT_NAME%"
if exist "%PORT_DIR%" rmdir /s /q "%PORT_DIR%"
mkdir "%PORT_DIR%"

call :render_template "%PROJECT_ROOT%\packaging\vcpkg\portfile.cmake.in" "%PORT_DIR%\portfile.cmake"
if errorlevel 1 exit /b 1
call :render_template "%PROJECT_ROOT%\packaging\vcpkg\vcpkg.json.in" "%PORT_DIR%\vcpkg.json"
if errorlevel 1 exit /b 1
call :render_template "%PROJECT_ROOT%\packaging\vcpkg\usage.in" "%PORT_DIR%\usage"
if errorlevel 1 exit /b 1

pushd "%REGISTRY_REPO%"
"%VCPKG_EXE%" x-add-version "%PORT_NAME%" --overwrite-version --x-builtin-ports-root="%REGISTRY_REPO%\ports" --x-builtin-registry-versions-dir="%REGISTRY_REPO%\versions"
if errorlevel 1 (
    popd
    exit /b 1
)
if not "%COMMIT_MESSAGE%"=="" (
    git add "ports/%PORT_NAME%" versions
    git commit -m "%COMMIT_MESSAGE%"
    if errorlevel 1 (
        popd
        exit /b 1
    )
)
popd

echo Published %PORT_NAME% %PORT_VERSION% to %REGISTRY_REPO%
exit /b 0

:resolve_vcpkg
set "%~1="
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\vcpkg.exe" set "%~1=%VCPKG_ROOT%\vcpkg.exe"
if defined %~1 exit /b 0
where vcpkg >nul 2>nul
if not errorlevel 1 set "%~1=vcpkg"
exit /b 0

:render_template
set "TEMPLATE_FILE=%~1"
set "OUTPUT_FILE=%~2"
if not exist "%TEMPLATE_FILE%" exit /b 1
break > "%OUTPUT_FILE%"
for /f "usebackq delims=" %%L in ("%TEMPLATE_FILE%") do (
    set "LINE=%%L"
    set "LINE=!LINE:@PORT_NAME@=%PORT_NAME%!"
    set "LINE=!LINE:@PORT_VERSION@=%PORT_VERSION%!"
    set "LINE=!LINE:@PORT_DESCRIPTION@=%PORT_DESCRIPTION%!"
    set "LINE=!LINE:@PORT_LICENSE@=%PORT_LICENSE%!"
    set "LINE=!LINE:@PORT_HOMEPAGE@=%VCPKG_PORT_HOMEPAGE%!"
    set "LINE=!LINE:@SOURCE_REPOSITORY@=%SOURCE_REPOSITORY%!"
    set "LINE=!LINE:@SOURCE_REF@=%SOURCE_REF%!"
    >>"%OUTPUT_FILE%" echo(!LINE!
)
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
