@echo off
setlocal
cd /d "%~dp0"

set "BIN=prebuilt\demo_hid_keyboard_mouse_led.bin"
if not "%~1"=="" set "BIN=%~1"

if not exist "tools\windows\minichlink.exe" (
    echo [NG] tools\windows\minichlink.exe was not found.
    pause
    exit /b 1
)

if not exist "%BIN%" (
    echo [NG] %BIN% was not found.
    echo Run build.bat first.
    pause
    exit /b 1
)

echo ============================================
echo  tutti controller kit - firmware writer
echo ============================================
echo.
echo Firmware: %BIN%
echo.
echo Unplug the controller USB cable, then plug it back in.
echo The writer will automatically catch bootloader mode.
echo Press Ctrl+C to cancel.
echo.
echo Waiting for controller...

set /a RETRIES=0
:retry
tools\windows\minichlink.exe -c 0x1209b803 -w "%BIN%" flash -b >nul 2>&1
if not errorlevel 1 goto ok
set /a RETRIES+=1
if %RETRIES% geq 300 goto fail
<nul set /p=.
goto retry

:ok
echo.
echo [OK] Firmware written successfully.
pause
exit /b 0

:fail
echo.
echo [NG] Controller bootloader was not found.
echo Unplug and reconnect USB, then run flash.bat again.
pause
exit /b 1
