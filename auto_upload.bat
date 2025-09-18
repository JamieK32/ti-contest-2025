@echo off

cd /d "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025\2025K-Code\mspm0g3507\project"
if exist "remove_temporary.bat" call remove_temporary.bat

cd /d "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025"

echo [%date% %time%] Starting auto push...

git add .
if %errorlevel% neq 0 (
    echo Error: git add failed
    pause
    exit /b 1
)

git commit -m "Daily auto commit - %date% %time%"
git push origin main
if %errorlevel% neq 0 (
    echo Error: git push failed
    pause
    exit /b 1
)

echo [%date% %time%] Auto push completed.