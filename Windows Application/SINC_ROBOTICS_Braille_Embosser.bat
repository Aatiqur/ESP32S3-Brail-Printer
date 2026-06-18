@echo off
REM ============================================================
REM   SINC ROBOTICS - Braille Embosser Desktop App Launcher
REM ============================================================
REM   This script runs the Python app IN THIS SAME window so
REM   any error message stays visible (window won't vanish).
REM ============================================================

setlocal

cd /d "%~dp0"

echo.
echo ============================================================
echo   SINC ROBOTICS - Braille Embosser Desktop Application
echo ============================================================
echo.

REM ---- 1. Check that Python is installed and in PATH ----
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH.
    echo.
    echo Please install Python 3.10+ from:
    echo     https://www.python.org/downloads/
    echo.
    echo IMPORTANT: Check the box "Add Python to PATH" during install.
    echo.
    pause
    exit /b 1
)

echo [OK] Python found:
python --version
echo.

REM ---- 2. Install missing packages if needed ----
echo [INFO] Checking required packages (flask, pyserial)...
python -m pip install --quiet flask pyserial
if errorlevel 1 (
    echo.
    echo [WARNING] pip install failed. Trying again with output visible...
    python -m pip install flask pyserial
    if errorlevel 1 (
        echo.
        echo [ERROR] Could not install required packages.
        echo Try running:  python -m pip install flask pyserial
        echo.
        pause
        exit /b 1
    )
)
echo [OK] Packages ready.
echo.

REM ---- 3. Launch the application (this BLOCKS until app closes) ----
echo [INFO] Starting application...
echo        A browser window will open at http://127.0.0.1:5000
echo        Press Ctrl+C in this window to stop the server.
echo.

python run_app.py

REM ---- 4. Keep the window open after the app exits ----
echo.
echo ============================================================
echo   Application stopped.
echo ============================================================
pause
endlocal
