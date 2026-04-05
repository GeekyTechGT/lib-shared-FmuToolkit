@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=dev-debug"

ctest --preset "%PRESET%"
exit /b %errorlevel%
