@echo off
setlocal

set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build\windows-mingw-debug"

where clang-tidy >nul 2>nul
if errorlevel 1 (
    echo clang-tidy not found 1^>^&2
    exit /b 1
)

for /r include %%f in (*.hpp *.cpp) do clang-tidy -p "%BUILD_DIR%" "%%f"
for /r src %%f in (*.hpp *.cpp) do clang-tidy -p "%BUILD_DIR%" "%%f"
for /r tests %%f in (*.hpp *.cpp) do clang-tidy -p "%BUILD_DIR%" "%%f"

exit /b %errorlevel%
