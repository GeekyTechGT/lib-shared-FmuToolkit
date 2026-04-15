@echo off
REM ============================================================
REM  Project Manager - lib-fmu-toolkit
REM ============================================================

setlocal enabledelayedexpansion

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "SCRIPTS_DIR=%PROJECT_ROOT%\scripts"
set "CURRENT_PRESET=windows-mingw-debug"
set "BUILD_ROOT=%PROJECT_ROOT%\build"
set "DEPLOY_ROOT=%PROJECT_ROOT%\deploy"

for %%F in (
    pm_select_preset.bat
    pm_configure.bat
    pm_build.bat
    pm_rebuild.bat
    pm_clean.bat
    pm_test.bat
    pm_pipeline.bat
    pm_format.bat
    pm_lint.bat
    pm_deploy.bat
    pm_status.bat
    pm_vcpkg_status.bat
    pm_vcpkg_init_registry.bat
    pm_vcpkg_publish_registry.bat
) do (
    if not exist "%SCRIPTS_DIR%\%%F" (
        echo.
        echo !RED![ERROR]!RESET! Missing script: scripts\%%F
        echo.
        pause
        exit /b 1
    )
)

:main_menu
call :update_paths
cls
echo.
echo !CYAN!======================================================!RESET!
echo !CYAN!         lib-fmu-toolkit - Project Manager!RESET!
echo !CYAN!======================================================!RESET!
echo.
echo   !BLUE!Current Context!RESET!
echo   Preset     : !YELLOW!!CURRENT_PRESET!!RESET!
echo   Build Dir  : !YELLOW!!CURRENT_BUILD_DIR!!RESET!
echo   Deploy Dir : !YELLOW!!CURRENT_DEPLOY_DIR!!RESET!
echo.
echo   !BLUE!Preset Management!RESET!
echo   !GREEN![1]!RESET! Select Preset
echo.
echo   !BLUE!Build Operations!RESET!
echo   !GREEN![2]!RESET! Configure
echo   !GREEN![3]!RESET! Build
echo   !GREEN![4]!RESET! Rebuild Current Preset
echo   !GREEN![5]!RESET! Clean Current Build
echo   !GREEN![6]!RESET! Clean All Builds
echo.
echo   !BLUE!Quality Operations!RESET!
echo   !GREEN![7]!RESET! Run Tests
echo   !GREEN![8]!RESET! Run Configure + Build + Test
echo   !GREEN![9]!RESET! Run clang-format
echo   !GREEN![10]!RESET! Run clang-tidy
echo.
echo   !BLUE!Deployment!RESET!
echo   !GREEN![11]!RESET! Deploy (cmake --install)
echo   !GREEN![12]!RESET! Clean Deploy Folder
echo.
echo   !BLUE!Vcpkg Registry!RESET!
echo   !GREEN![13]!RESET! Show Vcpkg Registry Status
echo   !GREEN![14]!RESET! Init Vcpkg Registry Layout
echo   !GREEN![15]!RESET! Publish Port To Vcpkg Registry
echo.
echo   !BLUE!Other!RESET!
echo   !GREEN![16]!RESET! Show Preset Status
echo   !GREEN![0]!RESET! Exit
echo.
echo !CYAN!======================================================!RESET!
echo.

set /p "choice=  Select an option [0-16]: "

if "%choice%"=="1"  goto preset_menu
if "%choice%"=="2"  goto configure_project
if "%choice%"=="3"  goto build_project
if "%choice%"=="4"  goto rebuild_project
if "%choice%"=="5"  goto clean_build
if "%choice%"=="6"  goto clean_all_builds
if "%choice%"=="7"  goto run_tests
if "%choice%"=="8"  goto run_pipeline
if "%choice%"=="9"  goto run_format
if "%choice%"=="10" goto run_lint
if "%choice%"=="11" goto deploy_project
if "%choice%"=="12" goto clean_deploy
if "%choice%"=="13" goto show_vcpkg_status
if "%choice%"=="14" goto init_vcpkg_registry
if "%choice%"=="15" goto publish_vcpkg_registry
if "%choice%"=="16" goto show_status
if "%choice%"=="0"  goto exit_script

echo.
echo !RED![ERROR]!RESET! Invalid option. Please try again.
timeout /t 2 >nul
goto main_menu

:preset_menu
call "%SCRIPTS_DIR%\pm_select_preset.bat" CURRENT_PRESET "%CURRENT_PRESET%"
goto main_menu

:configure_project
cls
echo.
echo !BLUE![CONFIGURE]!RESET! Running preset !YELLOW!!CURRENT_PRESET!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_configure.bat" "%CURRENT_PRESET%"
call :handle_result "Configure"
goto main_menu

:build_project
cls
echo.
echo !BLUE![BUILD]!RESET! Running preset !YELLOW!!CURRENT_PRESET!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_build.bat" "%CURRENT_PRESET%"
call :handle_result "Build"
goto main_menu

:rebuild_project
cls
echo.
echo !BLUE![REBUILD]!RESET! Cleaning and rebuilding !YELLOW!!CURRENT_PRESET!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_rebuild.bat" "%CURRENT_PRESET%" "%CURRENT_BUILD_DIR%"
call :handle_result "Rebuild"
goto main_menu

:clean_build
cls
echo.
echo !BLUE![CLEAN]!RESET! Removing !YELLOW!!CURRENT_BUILD_DIR!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_clean.bat" "%CURRENT_BUILD_DIR%"
call :handle_result "Clean current build"
goto main_menu

:clean_all_builds
cls
echo.
echo !BLUE![CLEAN]!RESET! Removing !YELLOW!!BUILD_ROOT!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_clean.bat" "%BUILD_ROOT%"
call :handle_result "Clean all builds"
goto main_menu

:run_tests
cls
echo.
echo !BLUE![TEST]!RESET! Running preset !YELLOW!!CURRENT_PRESET!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_test.bat" "%CURRENT_PRESET%"
call :handle_result "Tests"
goto main_menu

:run_pipeline
cls
echo.
echo !BLUE![PIPELINE]!RESET! Configure + Build + Test for !YELLOW!!CURRENT_PRESET!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_pipeline.bat" "%CURRENT_PRESET%"
call :handle_result "Pipeline"
goto main_menu

:run_format
cls
echo.
echo !BLUE![FORMAT]!RESET! Running clang-format
echo.
call "%SCRIPTS_DIR%\pm_format.bat"
call :handle_result "Format"
goto main_menu

:run_lint
cls
echo.
echo !BLUE![LINT]!RESET! Using build dir !YELLOW!!CURRENT_BUILD_DIR!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_lint.bat" "%CURRENT_BUILD_DIR%"
call :handle_result "Lint"
goto main_menu

:deploy_project
cls
echo.
echo !BLUE![DEPLOY]!RESET! Installing !YELLOW!!CURRENT_PRESET!!RESET! into !YELLOW!!CURRENT_DEPLOY_DIR!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_deploy.bat" "%CURRENT_BUILD_DIR%" "%CURRENT_DEPLOY_DIR%"
call :handle_result "Deploy"
goto main_menu

:clean_deploy
cls
echo.
echo !BLUE![CLEAN]!RESET! Removing !YELLOW!!CURRENT_DEPLOY_DIR!!RESET!
echo.
call "%SCRIPTS_DIR%\pm_clean.bat" "%CURRENT_DEPLOY_DIR%"
call :handle_result "Clean deploy"
goto main_menu

:show_vcpkg_status
cls
call "%SCRIPTS_DIR%\pm_vcpkg_status.bat"
echo.
pause
goto main_menu

:init_vcpkg_registry
cls
echo.
echo !BLUE![VCPKG]!RESET! Initializing registry layout
echo.
call "%SCRIPTS_DIR%\pm_vcpkg_init_registry.bat"
call :handle_result "Init Vcpkg registry"
goto main_menu

:publish_vcpkg_registry
cls
echo.
echo !BLUE![VCPKG]!RESET! Publishing port to Git registry
echo.
call "%SCRIPTS_DIR%\pm_vcpkg_publish_registry.bat"
call :handle_result "Publish Vcpkg port"
goto main_menu

:show_status
cls
call "%SCRIPTS_DIR%\pm_status.bat" "%CURRENT_PRESET%" "%CURRENT_BUILD_DIR%" "%CURRENT_DEPLOY_DIR%"
echo.
pause
goto main_menu

:update_paths
set "CURRENT_BUILD_DIR=%BUILD_ROOT%\%CURRENT_PRESET%"
set "CURRENT_DEPLOY_DIR=%DEPLOY_ROOT%\%CURRENT_PRESET%"
exit /b 0

:handle_result
set "STEP_NAME=%~1"
echo.
if errorlevel 1 (
    echo !RED![ERROR]!RESET! %STEP_NAME% failed.
    echo.
    pause
    exit /b 1
)
echo !GREEN![OK]!RESET! %STEP_NAME% completed successfully.
echo.
pause
exit /b 0

:exit_script
cls
echo.
echo !GREEN!  Goodbye!!RESET!
echo.
exit /b 0
