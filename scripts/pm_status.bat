@echo off
setlocal

set "CURRENT_PRESET=%~1"
set "BUILD_DIR=%~2"
set "DEPLOY_DIR=%~3"

if "%CURRENT_PRESET%"=="" set "CURRENT_PRESET=windows-mingw-debug"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build\%CURRENT_PRESET%"
if "%DEPLOY_DIR%"=="" set "DEPLOY_DIR=deploy\%CURRENT_PRESET%"

echo.
echo ======================================================
echo                    Preset Status
echo ======================================================
echo.
echo   Current preset : %CURRENT_PRESET%
echo   Build dir      : %BUILD_DIR%
if exist "%BUILD_DIR%" (
    echo   Build state    : present
) else (
    echo   Build state    : missing
)
echo   Deploy dir     : %DEPLOY_DIR%
if exist "%DEPLOY_DIR%" (
    echo   Deploy state   : present
) else (
    echo   Deploy state   : missing
)
echo.
exit /b 0
