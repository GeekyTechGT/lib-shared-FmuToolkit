@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=dev-debug"

cmake --preset "%PRESET%"
exit /b %errorlevel%
