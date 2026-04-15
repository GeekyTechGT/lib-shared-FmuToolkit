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
echo   --- Linux GCC ---
echo   [1] linux-gcc-debug
echo   [2] linux-gcc-release
echo.
echo   --- Linux Clang ---
echo   [3] linux-clang-debug
echo   [4] linux-clang-release
echo.
echo   --- Windows MinGW (MSYS2 / Qt-ready) ---
echo   [5] windows-mingw-debug
echo   [6] windows-mingw-release
echo.
echo   --- Windows Clang (MSYS2) ---
echo   [7] windows-clang-debug
echo   [8] windows-clang-release
echo.
echo   [9] Enter preset manually
echo   [0] Back
echo.
set /p "preset_choice=  Select a preset [0-9]: "

if "%preset_choice%"=="1" set "SELECTED_PRESET=linux-gcc-debug"     & goto apply_selection
if "%preset_choice%"=="2" set "SELECTED_PRESET=linux-gcc-release"   & goto apply_selection
if "%preset_choice%"=="3" set "SELECTED_PRESET=linux-clang-debug"   & goto apply_selection
if "%preset_choice%"=="4" set "SELECTED_PRESET=linux-clang-release" & goto apply_selection
if "%preset_choice%"=="5" set "SELECTED_PRESET=windows-mingw-debug" & goto apply_selection
if "%preset_choice%"=="6" set "SELECTED_PRESET=windows-mingw-release" & goto apply_selection
if "%preset_choice%"=="7" set "SELECTED_PRESET=windows-clang-debug" & goto apply_selection
if "%preset_choice%"=="8" set "SELECTED_PRESET=windows-clang-release" & goto apply_selection
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
