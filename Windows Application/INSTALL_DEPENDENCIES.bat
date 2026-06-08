@echo off
REM Install required Python packages for SINC ROBOTICS Braille Embosser

echo.
echo ============================================================
echo Installing SINC ROBOTICS Dependencies
echo ============================================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    echo Please install Python from: https://www.python.org/
    echo Make sure to check "Add Python to PATH" during installation.
    echo.
    pause
    exit /b 1
)

echo ✓ Python found
echo.
echo Installing packages: flask, pyserial
echo.

REM Install packages
pip install flask pyserial

if errorlevel 1 (
    echo.
    echo ERROR: Failed to install packages
    echo Please try running this file as Administrator
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo ✓ Installation Complete!
echo ============================================================
echo.
echo You can now run: SINC_ROBOTICS_Braille_Embosser.bat
echo.
pause
