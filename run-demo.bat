@echo off
chcp 65001 >nul
cd /d "%~dp0"
setlocal

echo ⚡ Building Project...
call mvn clean install -DskipTests -q
if %ERRORLEVEL% NEQ 0 ( echo ❌ Build failed. & pause & exit /b %ERRORLEVEL% )

echo 🚀 Running Demo...
cd examples\Demo
call mvn clean compile exec:java -Dexec.mainClass=fastdisplay.Demo -q
if %ERRORLEVEL% NEQ 0 ( echo ❌ Demo failed. & pause & exit /b %ERRORLEVEL% )

cd ..
endlocal
