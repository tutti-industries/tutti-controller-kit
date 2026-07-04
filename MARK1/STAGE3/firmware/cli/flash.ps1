# UIAPduino Pro Micro CH32V003 (MARK1 STAGE3) 書き込みスクリプト
# 基板をブートモードでUSB接続してから実行してください。

$ErrorActionPreference = "Stop"

$BoardUrl  = "https://github.com/YuukiUmeta-UIAP/board_manager_files/raw/main/package_uiap.jp_index.json"
$Fqbn      = "UIAP:ch32v:CH32V00x_EVT:pnum=CH32V003V1DOT4,clock=48MHz_HSI,upload_method=minichlink"
$SketchDir = Join-Path $PSScriptRoot "UIAPduino_LED_test"

function Find-ArduinoCli {
    $cmd = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "$env:ProgramFiles\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe",
        "${env:ProgramFiles(x86)}\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe",
        "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) { return $path }
    }
    return $null
}

$ArduinoCli = Find-ArduinoCli
if (-not $ArduinoCli) {
    Write-Host "arduino-cli が見つかりませんでした。" -ForegroundColor Red
    Write-Host "Arduino IDE (https://www.arduino.cc/en/software) をインストールしてから、もう一度実行してください。"
    exit 1
}
Write-Host "arduino-cli: $ArduinoCli"

Write-Host "UIAPduinoボードパッケージを確認/インストールしています..."
& $ArduinoCli core install "UIAP:ch32v" --additional-urls $BoardUrl
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "基板をブートモードでUSB接続してください。" -ForegroundColor Yellow
Read-Host "接続できたらEnterキーを押してください"

Write-Host "コンパイルしています..."
& $ArduinoCli compile --fqbn $Fqbn --additional-urls $BoardUrl "$SketchDir"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "書き込んでいます..."
& $ArduinoCli upload --fqbn $Fqbn --additional-urls $BoardUrl "$SketchDir"
exit $LASTEXITCODE
