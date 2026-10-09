@echo off
REM ============================================================
REM CALWF One-Click Build Script
REM ============================================================
REM Usage:
REM   Double-click this file, or run .\build.bat in PowerShell
REM ============================================================

set PATH=D:\Programm\QT\Tools\mingw1310_64\bin;D:\Programm\QT\Tools\CMake_64\bin;%PATH%

cd /d D:\CALWF

echo.
echo ==================================================
echo  Step 1: Configure CMake
echo ==================================================
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 goto :error

echo.
echo ==================================================
echo  Step 2: Build
echo ==================================================
cmake --build build
if errorlevel 1 goto :error

echo.
echo ==================================================
echo  BUILD SUCCESSFUL
echo ==================================================
echo.
echo  Executable: D:\CALWF\build\bin\calwf_cli.exe
echo.
echo  Run with: .\build\bin\calwf_cli.exe
echo.
goto :end

:error
echo.
echo ==================================================
echo  BUILD FAILED - see errors above
echo ==================================================
exit /b 1

:end