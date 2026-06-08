@echo off
REM SINC ROBOTICS - Braille Embosser Desktop Application Launcher
REM Double-click this file to run the application!

cd /d "%~dp0"

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo.
    echo ============================================================
    echo ERROR: Python is not installed or not in PATH
    echo ============================================================
    echo.
    echo Please install Python from: https://www.python.org/
    echo.
    echo During installation, make sure to check:
    echo "Add Python to PATH"
    echo.
    echo ============================================================
    echo.
    pause
    exit /b 1
)

REM Run the Python application
echo.
echo ============================================================
echo Starting SINC ROBOTICS Braille Embosser...
echo ============================================================
echo.

python run_app.py

if errorlevel 1 (
    echo.
    echo ============================================================
    echo ERROR: Application failed to start
    echo ============================================================
    echo.
    echo Common fixes:
    echo 1. Make sure Python is installed and added to PATH
    echo 2. Ensure all files are in the same directory
    echo 3. Check that port 5000 is not in use
    echo.
    echo ============================================================
    echo.
    pause
)

exit /b 0
