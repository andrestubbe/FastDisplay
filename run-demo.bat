@echo off


echo ðŸš€ Running Demo...
cd examples\Demo
call mvn compile exec:java -Dexec.mainClass=fastdisplay.Demo
cd ..\..
pause
