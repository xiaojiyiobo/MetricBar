@echo off
setlocal

net session >nul 2>&1
if errorlevel 1 (
    echo ERROR: Run uninstall.bat as Administrator.
    pause
    exit /b 1
)

if not exist "%~dp0VpsTraySpeed.dll" (
    echo ERROR: VpsTraySpeed.dll was not found beside this script.
    pause
    exit /b 2
)

"%SystemRoot%\System32\regsvr32.exe" /u /s "%~dp0VpsTraySpeed.dll"
if errorlevel 1 (
    echo ERROR: DLL unregistration failed.
    pause
    exit /b 3
)

echo MetricBar unregistered successfully.
echo Restart Explorer or sign out and back in to unload the DLL completely.
pause
exit /b 0
