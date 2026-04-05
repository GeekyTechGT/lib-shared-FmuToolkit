@echo off
setlocal

set "PRESET=%~1"
set "BUILD_DIR=%~2"

if "%PRESET%"=="" set "PRESET=dev-debug"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build\%PRESET%"

call "%~dp0pm_clean.bat" "%BUILD_DIR%"
if errorlevel 1 exit /b %errorlevel%

call "%~dp0pm_configure.bat" "%PRESET%"
if errorlevel 1 exit /b %errorlevel%

call "%~dp0pm_build.bat" "%PRESET%"
exit /b %errorlevel%
