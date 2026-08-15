@echo off
setlocal

rem Loads the MSVC dev environment (cl.exe, Ninja) and runs the requested
rem CMake preset. Needed because plain PowerShell/cmd sessions don't have
rem cl.exe/Ninja on PATH the way a Developer Command Prompt does.
rem
rem Usage: windows-build.bat [release|debug]

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=release

set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (
    echo Could not find vswhere.exe - is Visual Studio / Build Tools installed?
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if "%VSPATH%"=="" (
    echo Could not find a Visual Studio install with the C++ build tools component.
    exit /b 1
)

call "%VSPATH%\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

cd /d "%~dp0"
cmake --preset windows-%CONFIG%
if errorlevel 1 exit /b 1
cmake --build --preset windows-%CONFIG%
