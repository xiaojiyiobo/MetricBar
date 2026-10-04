@echo off
setlocal

net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Run install.bat as Administrator.
    pause
    exit /b 1
)

if not exist "%~dp0VpsTraySpeed.dll" (
    echo ERROR: VpsTraySpeed.dll was not found beside this script.
    pause
    exit /b 2
)

"%SystemRoot%\System32\regsvr32.exe" /s "%~dp0VpsTraySpeed.dll"
if errorlevel 1 (
    echo ERROR: DLL registration failed.
    pause
    exit /b 3
)

echo MetricBar registered successfully.
echo If it is not listed yet, restart Explorer or sign out and back in.
echo Then right-click the taskbar and choose Toolbars ^> MetricBar.
pause
exit /b 0
