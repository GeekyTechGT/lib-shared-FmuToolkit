@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=windows-mingw-debug"

set "CMAKE_EXE="
where cmake >nul 2>nul
if not errorlevel 1 set "CMAKE_EXE=cmake"
if not defined CMAKE_EXE if exist "C:\eng_apps\cmake\cmake-4.2.0\bin\cmake.exe" set "CMAKE_EXE=C:\eng_apps\cmake\cmake-4.2.0\bin\cmake.exe"
if not defined CMAKE_EXE (
    echo cmake not found 1^>^&2
    exit /b 1
)

"%CMAKE_EXE%" --preset "%PRESET%"
exit /b %errorlevel%
