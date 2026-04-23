@echo off
setlocal EnableDelayedExpansion

:: Change to script directory
cd /d "%~dp0"

echo ===========================================
echo FastDisplay JNI Bridge Build Script
echo ===========================================
echo.
echo Running in: %CD%
echo.

:: Check for Java
if not defined JAVA_HOME (
    set "JAVA_HOME=C:\Program Files\Java\jdk-25"
)

if not exist "%JAVA_HOME%\include\jni.h" (
    echo ERROR: Cannot find jni.h in %JAVA_HOME%\include
    echo Please check your Java installation
    exit /b 1
)

:: Use vswhere to find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found!
    echo Visual Studio Installer might be missing.
    echo.
    exit /b 1
)

:: Find VS installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_INSTALL=%%i"
)

if not defined VS_INSTALL (
    echo ERROR: Visual Studio with C++ tools not found!
    echo.
    exit /b 1
)

echo Found Visual Studio at: %VS_INSTALL%

:: Setup VS environment
set "VCVARS=%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat"

echo Setting up Visual Studio environment...
call "%VCVARS%"
if errorlevel 1 (
    echo ERROR: Failed to setup VS environment
    exit /b 1
)

:: Create build directories
if not exist build mkdir build

:: Compile C++ DLL only (Java compilation handled by Maven)
echo.
echo Compiling FastDisplay JNI Bridge...
echo =====================================================
cl /LD /Fe:build\fastdisplay.dll ^
    native\FastDisplay.cpp ^
    user32.lib gdi32.lib shcore.lib advapi32.lib ^
    /I"%JAVA_HOME%\include" ^
    /I"%JAVA_HOME%\include\win32" ^
    /EHsc /std:c++17 /O2 /W3

:: Check result
if %errorlevel% neq 0 (
    echo.
    echo =====================================================
    echo COMPILATION FAILED
    echo =====================================================
    echo Check errors above
    exit /b 1
)

:: Copy to resources
echo.
echo Copying DLL to resources...
copy build\fastdisplay.dll src\main\resources\native\fastdisplay.dll >nul

:: Copy DLL to native folder for demo
copy build\fastdisplay.dll native\fastdisplay.dll >nul

:: Success
echo.
echo =====================================================
echo BUILD SUCCESSFUL!
echo =====================================================
echo.
echo FastDisplay JNI Bridge created with:
echo - Multi-monitor enumeration
echo - Resolution change detection (WM_DISPLAYCHANGE)
echo - DPI scale change detection (WM_DPICHANGED)
echo - Orientation monitoring
echo.
echo DLL: build\fastdisplay.dll
echo.

