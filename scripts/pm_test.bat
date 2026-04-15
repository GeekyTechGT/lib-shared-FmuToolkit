@echo off
setlocal

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=windows-mingw-debug"

set "CTEST_EXE="
where ctest >nul 2>nul
if not errorlevel 1 set "CTEST_EXE=ctest"
if not defined CTEST_EXE if exist "C:\eng_apps\cmake\cmake-4.2.0\bin\ctest.exe" set "CTEST_EXE=C:\eng_apps\cmake\cmake-4.2.0\bin\ctest.exe"
if not defined CTEST_EXE (
    echo ctest not found 1^>^&2
    exit /b 1
)

"%CTEST_EXE%" --preset "%PRESET%" --output-on-failure
exit /b %errorlevel%
