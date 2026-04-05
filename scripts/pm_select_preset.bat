@echo off

set "TARGET_VAR=%~1"
set "CURRENT_PRESET=%~2"
set "REQUESTED_PRESET=%~3"

if "%TARGET_VAR%"=="" exit /b 1

if not "%REQUESTED_PRESET%"=="" (
    set "SELECTED_PRESET=%REQUESTED_PRESET%"
    goto apply_selection
)

:menu
cls
echo.
echo ======================================================
echo                  Select Preset
echo ======================================================
echo.
echo   Current preset: %CURRENT_PRESET%
echo.
echo   [1] dev-debug
echo   [2] dev-release
echo   [3] vcpkg-debug
echo   [4] vcpkg-release
echo   [5] windows-msvc-debug
echo   [6] windows-msvc-vcpkg-debug
echo   [7] windows-mingw-debug
echo   [8] windows-mingw-vcpkg-debug
echo   [9] Enter preset manually
echo   [0] Back
echo.
set /p "preset_choice=  Select a preset [0-9]: "

if "%preset_choice%"=="1" set "SELECTED_PRESET=dev-debug" & goto apply_selection
if "%preset_choice%"=="2" set "SELECTED_PRESET=dev-release" & goto apply_selection
if "%preset_choice%"=="3" set "SELECTED_PRESET=vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="4" set "SELECTED_PRESET=vcpkg-release" & goto apply_selection
if "%preset_choice%"=="5" set "SELECTED_PRESET=windows-msvc-debug" & goto apply_selection
if "%preset_choice%"=="6" set "SELECTED_PRESET=windows-msvc-vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="7" set "SELECTED_PRESET=windows-mingw-debug" & goto apply_selection
if "%preset_choice%"=="8" set "SELECTED_PRESET=windows-mingw-vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="9" goto manual_preset
if "%preset_choice%"=="0" exit /b 0

echo.
echo [ERROR] Invalid option. Please try again.
timeout /t 2 >nul
goto menu

:manual_preset
echo.
set /p "manual_preset=  Enter preset name: "
if "%manual_preset%"=="" (
    echo [ERROR] Preset cannot be empty.
    timeout /t 2 >nul
    goto menu
)
set "SELECTED_PRESET=%manual_preset%"

:apply_selection
set "%TARGET_VAR%=%SELECTED_PRESET%"
exit /b 0
