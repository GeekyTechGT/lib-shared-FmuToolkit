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

set "CMAKE_EXE="
where cmake >nul 2>nul
if not errorlevel 1 set "CMAKE_EXE=cmake"
if not defined CMAKE_EXE if exist "C:\eng_apps\cmake\cmake-4.2.0\bin\cmake.exe" set "CMAKE_EXE=C:\eng_apps\cmake\cmake-4.2.0\bin\cmake.exe"
if not defined CMAKE_EXE (
    echo cmake not found 1^>^&2
    exit /b 1
)

if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
"%CMAKE_EXE%" --install "%BUILD_DIR%" --prefix "%DEPLOY_DIR%"
exit /b %errorlevel%
