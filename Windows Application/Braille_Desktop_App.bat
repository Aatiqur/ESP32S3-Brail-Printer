@echo off
REM Launch SINC ROBOTICS Braille Embosser Desktop App

echo ========================================
echo SINC ROBOTICS Braille Embosser Desktop
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python from https://www.python.org/downloads/
    pause
    exit /b 1
)

REM Check for required packages
python -c "import PyQt5; import pyttsx3" >nul 2>&1
if errorlevel 1 (
    echo Installing required packages...
    pip install PyQt5==5.15.9 pyttsx3==2.90 pyserial==3.5
    if errorlevel 1 (
        echo ERROR: Failed to install packages
        pause
        exit /b 1
    )
)

echo Starting SINC ROBOTICS Braille Embosser Desktop App...
echo.

REM Start the desktop app
python "%~dp0braille_desktop_app.py"

if errorlevel 1 (
    echo ERROR: Failed to start application
    pause
    exit /b 1
)

pause
