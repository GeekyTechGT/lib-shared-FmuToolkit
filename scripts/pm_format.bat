@echo off
setlocal

where clang-format >nul 2>nul
if errorlevel 1 (
    echo clang-format not found 1^>^&2
    exit /b 1
)

for /r include %%f in (*.hpp *.cpp) do clang-format -i "%%f"
for /r src %%f in (*.hpp *.cpp) do clang-format -i "%%f"
for /r tests %%f in (*.hpp *.cpp) do clang-format -i "%%f"
for /r examples %%f in (*.hpp *.cpp) do clang-format -i "%%f"

exit /b %errorlevel%
