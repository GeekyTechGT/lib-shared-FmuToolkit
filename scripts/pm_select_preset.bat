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
echo   [1]  dev-debug
echo   [2]  dev-release
echo   [3]  vcpkg-debug
echo   [4]  vcpkg-release
echo   [5]  linux-gcc-debug
echo   [6]  linux-gcc-release
echo   [7]  linux-clang-debug
echo   [8]  linux-clang-release
echo   [9]  windows-msvc-debug
echo   [10] windows-msvc-release
echo   [11] windows-msvc-vcpkg-debug
echo   [12] windows-msvc-vcpkg-release
echo   [13] windows-mingw-debug
echo   [14] windows-mingw-release
echo   [15] windows-mingw-vcpkg-debug
echo   [16] windows-mingw-vcpkg-release
echo   [17] windows-clang-debug
echo   [18] windows-clang-release
echo   [19] windows-clang-vcpkg-debug
echo   [20] windows-clang-vcpkg-release
echo   [21] Enter preset manually
echo   [0]  Back
echo.
set /p "preset_choice=  Select a preset [0-21]: "

if "%preset_choice%"=="1"  set "SELECTED_PRESET=dev-debug" & goto apply_selection
if "%preset_choice%"=="2"  set "SELECTED_PRESET=dev-release" & goto apply_selection
if "%preset_choice%"=="3"  set "SELECTED_PRESET=vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="4"  set "SELECTED_PRESET=vcpkg-release" & goto apply_selection
if "%preset_choice%"=="5"  set "SELECTED_PRESET=linux-gcc-debug" & goto apply_selection
if "%preset_choice%"=="6"  set "SELECTED_PRESET=linux-gcc-release" & goto apply_selection
if "%preset_choice%"=="7"  set "SELECTED_PRESET=linux-clang-debug" & goto apply_selection
if "%preset_choice%"=="8"  set "SELECTED_PRESET=linux-clang-release" & goto apply_selection
if "%preset_choice%"=="9"  set "SELECTED_PRESET=windows-msvc-debug" & goto apply_selection
if "%preset_choice%"=="10" set "SELECTED_PRESET=windows-msvc-release" & goto apply_selection
if "%preset_choice%"=="11" set "SELECTED_PRESET=windows-msvc-vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="12" set "SELECTED_PRESET=windows-msvc-vcpkg-release" & goto apply_selection
if "%preset_choice%"=="13" set "SELECTED_PRESET=windows-mingw-debug" & goto apply_selection
if "%preset_choice%"=="14" set "SELECTED_PRESET=windows-mingw-release" & goto apply_selection
if "%preset_choice%"=="15" set "SELECTED_PRESET=windows-mingw-vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="16" set "SELECTED_PRESET=windows-mingw-vcpkg-release" & goto apply_selection
if "%preset_choice%"=="17" set "SELECTED_PRESET=windows-clang-debug" & goto apply_selection
if "%preset_choice%"=="18" set "SELECTED_PRESET=windows-clang-release" & goto apply_selection
if "%preset_choice%"=="19" set "SELECTED_PRESET=windows-clang-vcpkg-debug" & goto apply_selection
if "%preset_choice%"=="20" set "SELECTED_PRESET=windows-clang-vcpkg-release" & goto apply_selection
if "%preset_choice%"=="21" goto manual_preset
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
