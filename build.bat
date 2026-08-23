@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Visual Studio Build Tools are not installed.
    exit /b 1
)

set "VSPATH="
set "VSPATHFILE=%TEMP%\shareboard_vspath.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%VSPATHFILE%"
if exist "%VSPATHFILE%" set /p VSPATH=<"%VSPATHFILE%"
del "%VSPATHFILE%" 2>nul

if not defined VSPATH (
    echo ERROR: no VS installation with the C++ toolset was found.
    exit /b 1
)

call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul

where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: cl.exe is not on PATH after vcvars64. MSVC setup failed.
    exit /b 1
)

if not exist build mkdir build

cl /nologo /W4 /EHsc /std:c++17 /Zi ^
   src\*.cpp ^
   /Fe:build\shareboard.exe /Fo:build\ /Fd:build\ ^
   /link user32.lib ws2_32.lib

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

echo.
echo Built build\shareboard.exe
