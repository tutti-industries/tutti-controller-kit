# コマンドプロンプト用コード

**Arduino IDE 不要**。ターミナルだけで UIAPduino Pro Micro CH32V003 に
LEDテストファームウェアを書き込むためのフォルダです。

## 必要なもの

- [`arduino-cli`](https://arduino.github.io/arduino-cli/latest/installation/) 単体
  （Arduino IDEは不要です。`winget install ArduinoSA.CLI` でインストールできます）
- USBケーブル

## 使い方

1. 基板を **ブートモード** でPCにUSB接続する
2. `flash.bat` をダブルクリックする（またはターミナルで `flash.ps1` を実行する）
3. 初回はUIAPduinoのボードパッケージ（コンパイラ・minichlinkを含む）が自動でインストールされます
4. `Image written.` と表示されれば書き込み完了です

`flash.ps1` はまず PATH 上の `arduino-cli` を探し、無ければ Arduino IDE 同梱のものに
フォールバックします（IDEが入っていれば流用するだけで、必須ではありません）。

## 手動で行う場合

```
arduino-cli core install UIAP:ch32v --additional-urls https://github.com/YuukiUmeta-UIAP/board_manager_files/raw/main/package_uiap.jp_index.json

arduino-cli compile --fqbn UIAP:ch32v:CH32V00x_EVT:pnum=CH32V003V1DOT4,clock=48MHz_HSI,upload_method=minichlink ./UIAPduino_LED_test

arduino-cli upload --fqbn UIAP:ch32v:CH32V00x_EVT:pnum=CH32V003V1DOT4,clock=48MHz_HSI,upload_method=minichlink ./UIAPduino_LED_test
```

ポート指定は不要です（`upload_method=minichlink` はUSB経由で基板を直接検出します）。

同じソースは Arduino IDE で開く用に [`../arduino_ide/UIAPduino_LED_test`](../arduino_ide/UIAPduino_LED_test) にも置いてあります。
