@echo off
setlocal

set "TARGET_DIR=%~1"
if "%TARGET_DIR%"=="" (
    echo Missing target directory.
    exit /b 1
)

if not exist "%TARGET_DIR%" exit /b 0

rmdir /s /q "%TARGET_DIR%"
exit /b %errorlevel%
