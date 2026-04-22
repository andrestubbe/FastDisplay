@echo off
setlocal EnableDelayedExpansion

:: Change to script directory
cd /d "%~dp0"

echo ===========================================
echo FastTheme JNI Bridge Build Script
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
    pause
    exit /b 1
)

:: Use vswhere to find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found!
    echo Visual Studio Installer might be missing.
    echo.
    pause
    exit /b 1
)

:: Find VS installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_INSTALL=%%i"
)

if not defined VS_INSTALL (
    echo ERROR: Visual Studio with C++ tools not found!
    echo.
    pause
    exit /b 1
)

echo Found Visual Studio at: %VS_INSTALL%

:: Setup VS environment
set "VCVARS=%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat"

echo Setting up Visual Studio environment...
call "%VCVARS%"
if errorlevel 1 (
    echo ERROR: Failed to setup VS environment
    pause
    exit /b 1
)

:: Create build directories
if not exist build mkdir build
if not exist build\classes mkdir build\classes

:: Compile Java classes first
echo.
echo Compiling Java classes...
"%JAVA_HOME%\bin\javac" -d build\classes src\main\java\fasttheme\*.java

:: Generate JNI header
echo Generating JNI header...
"%JAVA_HOME%\bin\javac" -h native -d build\classes src\main\java\fasttheme\FastTheme.java

:: Compile
echo.
echo Compiling FastTheme JNI Bridge...
echo =====================================================
cl /LD /Fe:build\fasttheme.dll ^
    native\FastTheme.cpp ^
    user32.lib gdi32.lib shcore.lib advapi32.lib dwmapi.lib ^
    /I"%JAVA_HOME%\include" ^
    /I"%JAVA_HOME%\include\win32" ^
    /EHsc /std:c++17 /O2 /W3 ^
    /link /DEF:native\FastTheme.def

:: Check result
if %errorlevel% neq 0 (
    echo.
    echo =====================================================
    echo COMPILATION FAILED
    echo =====================================================
    echo Check errors above
    pause
    exit /b 1
)

:: Copy to resources
echo.
echo Copying DLL to resources...
copy build\fasttheme.dll src\main\resources\native\fasttheme.dll

:: Create manifest file
echo Manifest-Version: 1.0 > build\manifest.txt
echo Main-Class: fasttheme.Demo >> build\manifest.txt

:: Create fat jar
echo Creating FastTheme.jar...
cd build\classes
"%JAVA_HOME%\bin\jar" cfm ..\FastTheme.jar ..\manifest.txt fasttheme\*.class
cd ..\..

:: Copy DLL to same directory as jar for easy testing
copy build\fasttheme.dll FastTheme.dll

:: Success
echo.
echo =====================================================
echo BUILD SUCCESSFUL!
echo =====================================================
echo.
echo FastTheme JNI Bridge created with:
echo - Resolution change detection (WM_DISPLAYCHANGE)
echo - DPI scale change detection (WM_DPICHANGED)
echo - Windows display settings monitoring
echo.
echo Run demo with: java -jar FastTheme.jar
echo.
pause
