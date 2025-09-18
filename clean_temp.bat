@echo off

echo [%date% %time%] Starting temporary file cleanup...

cd /d "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025\2025K-Code\mspm0g3507\project"
if exist "remove_temporary.bat" call remove_temporary.bat

echo [%date% %time%] Temporary file cleanup completed.