@echo off
setlocal

set "BUILD_DIR=%~1"
set "DEPLOY_DIR=%~2"

if "%BUILD_DIR%"=="" (
    echo Missing build directory.
    exit /b 1
)

if "%DEPLOY_DIR%"=="" (
    echo Missing deploy directory.
    exit /b 1
)

if not exist "%BUILD_DIR%" (
    echo Build directory not found: %BUILD_DIR%
    exit /b 1
)

if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
cmake --install "%BUILD_DIR%" --prefix "%DEPLOY_DIR%"
exit /b %errorlevel%
