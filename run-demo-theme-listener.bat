@echo off
cd /d "%~dp0"
setlocal

echo [*] Building FastDisplay...
call mvn clean install -DskipTests -q
if %ERRORLEVEL% NEQ 0 (
    echo [!] FastDisplay build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo [*] Running FastDisplay Theme Listener Demo...
cd examples\Demo
call mvn clean compile exec:java -DmainClass=fastdisplay.ThemeListenerDemo -q
if %ERRORLEVEL% NEQ 0 (
    echo [!] Demo failed.
    pause
    exit /b %ERRORLEVEL%
)

cd ..\..
endlocal