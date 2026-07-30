@echo off
setlocal
cd /d "%~dp0\UIAPduino_HID_keyboard_mouse_LED"

where make >nul 2>&1
if errorlevel 1 (
    echo [NG] make was not found in PATH.
    echo Install GNU Make and add it to PATH.
    pause
    exit /b 1
)

set "RISCV_GCC="
for %%G in (riscv64-unknown-elf-gcc riscv-none-elf-gcc riscv64-elf-gcc) do (
    where %%G >nul 2>&1
    if not errorlevel 1 set "RISCV_GCC=%%G"
)
if not defined RISCV_GCC (
    echo [NG] A supported RISC-V GCC compiler was not found in PATH.
    echo Install a RISC-V GCC toolchain and add its bin folder to PATH.
    pause
    exit /b 1
)

echo Building firmware...
make -B build
if errorlevel 1 goto fail

copy /Y "demo_hid_keyboard_mouse_led.bin" "..\prebuilt\demo_hid_keyboard_mouse_led.bin" >nul
if errorlevel 1 goto fail

echo.
echo [OK] Build completed and prebuilt firmware was updated.
pause
exit /b 0

:fail
echo.
echo [NG] Build failed.
pause
exit /b 1
