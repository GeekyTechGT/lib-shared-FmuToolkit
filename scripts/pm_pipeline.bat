@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=dev-debug"

call "%~dp0pm_configure.bat" "%PRESET%"
if errorlevel 1 exit /b %errorlevel%

call "%~dp0pm_build.bat" "%PRESET%"
if errorlevel 1 exit /b %errorlevel%

call "%~dp0pm_test.bat" "%PRESET%"
exit /b %errorlevel%
