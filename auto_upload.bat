@echo off
chcp 65001 >nul
echo Auto uploading to GitHub...
echo.

echo Step 0: Changing to repository directory...
cd /d "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025"
if errorlevel 1 (
    echo Failed to change to repository directory!
    pause
    exit /b 1
)
echo Changed to repository directory successfully
echo.

echo Step 1: Running cleanup script...
call "C:\Users\kjmsd\Documents\GitHub\ti-contest-2025\2025K-Code\mspm0g3507\project\remove_temporary.bat"
if errorlevel 1 (
    echo Cleanup script failed!
    pause
    exit /b 1
)
echo Temporary files cleaned successfully
echo.

echo Step 2: Adding all changes to Git...
git add .
if errorlevel 1 (
    echo Git add failed!
    pause
    exit /b 1
)
echo Git add completed
echo.

echo Step 3: Committing changes...
set /p commit_msg=Enter commit message (press Enter for default):
if "%commit_msg%"=="" set commit_msg=Auto commit update
git commit -m "%commit_msg%"
if errorlevel 1 (
    echo Git commit failed or no changes to commit!
    pause
    exit /b 1
)
echo Git commit completed
echo.

echo Step 4: Pushing to GitHub...
git push origin main
if errorlevel 1 (
    echo Git push failed!
    pause
    exit /b 1
)
echo Git push completed
echo.

echo All steps completed! Auto upload successful!
pause