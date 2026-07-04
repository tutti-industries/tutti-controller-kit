@echo off
chcp 65001 >nul
cd /d "%~dp0"

rem 引数で .bin を指定可能。省略時はビルド済みファームウェアを書き込む
set BIN=prebuilt\demo_hid_keyboard_mouse.bin
if not "%~1"=="" set BIN=%~1

if not exist "%BIN%" (
    echo [NG] %BIN% が見つかりません。
    pause
    exit /b 1
)

echo ============================================
echo  tutti controller kit - ファームウェア書き込み
echo ============================================
echo.
echo 書き込むファイル: %BIN%
echo.
echo コントローラーの USB ケーブルを一度抜いて、挿し直してください。
echo 挿した直後のブートローダー起動中に自動で書き込みます。
echo （中止するには Ctrl+C）
echo.
echo 待機中...

set /a N=0
:retry
ch32v003fun\minichlink\minichlink.exe -c 0x1209b803 -w "%BIN%" flash -b >nul 2>&1
if not errorlevel 1 goto ok
set /a N+=1
if %N% geq 300 goto fail
<nul set /p=.
goto retry

:ok
echo.
echo [OK] 書き込み完了！ コントローラーが再起動します。
pause
exit /b 0

:fail
echo.
echo [NG] 書き込みできませんでした。
echo      USB を挿し直してから、もう一度 flash.bat を実行してください。
echo      解決しない場合は README.md のトラブルシューティングを参照。
pause
exit /b 1
