@echo off

cd /d "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025"

echo [%date% %time%] Starting auto push...

git add .
if %errorlevel% neq 0 (
    echo Error: git add failed
    pause
    exit /b 1
)

git diff --cached --quiet
if %errorlevel% equ 0 (
    echo No changes to commit, skipping...
    goto :end
)

git commit -m "Daily auto commit - %date% %time%"
if %errorlevel% neq 0 (
    echo Error: git commit failed
    pause
    exit /b 1
)

git push origin main
if %errorlevel% neq 0 (
    echo Error: git push failed
    pause
    exit /b 1
)

:end
echo [%date% %time%] Auto push completed.