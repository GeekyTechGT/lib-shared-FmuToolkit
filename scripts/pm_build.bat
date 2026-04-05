@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=dev-debug"

cmake --build --preset "%PRESET%"
exit /b %errorlevel%
