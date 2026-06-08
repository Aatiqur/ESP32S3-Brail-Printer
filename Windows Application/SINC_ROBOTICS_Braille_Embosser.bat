@echo off
REM Quick Launcher - Minimizes console window
setlocal enabledelayedexpansion

cd /d "%~dp0"

REM Create a VBS script to run Python silently and open browser
set VBS_SCRIPT=%TEMP%\sinc_launch.vbs

(
echo Set objShell = CreateObject("WScript.Shell"^)
echo strPath = "%CD%"
echo objShell.CurrentDirectory = strPath
echo.
echo REM Check if Python is available
echo Dim shell, exec, output
echo Set shell = CreateObject("WScript.Shell"^)
echo On Error Resume Next
echo Set exec = shell.Exec("python --version"^)
echo output = exec.StdOut.ReadAll(^)
echo If Err.Number ^<^> 0 Then
echo    MsgBox "Python not found! Please install Python first." ^& vbCrLf ^& "https://www.python.org/", vbCritical, "SINC ROBOTICS"
echo    WScript.Quit
echo End If
echo On Error GoTo 0
echo.
echo REM Run the application
echo shell.Run "python run_app.py", 1, False
) > "!VBS_SCRIPT!"

cscript.exe "!VBS_SCRIPT!"
del "!VBS_SCRIPT!" 2>nul

endlocal
