@echo off
chcp 65001 >nul


echo ðŸš€ Running Demo...
cd examples\Demo
call mvn -q compile exec:java -Dexec.mainClass=fastdisplay.Demo
cd ..\..
pause
