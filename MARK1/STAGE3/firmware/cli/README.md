# コマンドプロンプト用コード

Arduino IDE の画面を使わず、ターミナル（PowerShell）だけで UIAPduino Pro Micro CH32V003 に
LEDテストファームウェアを書き込むためのフォルダです。

## 必要なもの

- [Arduino IDE 2.x](https://www.arduino.cc/en/software) がインストールされていること
  （内蔵の `arduino-cli` を利用します。単体で `arduino-cli` をインストールして PATH に
  通している場合はそちらが優先されます）
- USBケーブル

## 使い方

1. 基板を **ブートモード** でPCにUSB接続する
2. `flash.bat` をダブルクリックする（またはターミナルで `flash.ps1` を実行する）
3. 初回はUIAPduinoのボードパッケージが自動でインストールされます
4. `Image written.` と表示されれば書き込み完了です

## 手動で行う場合

```
arduino-cli core install UIAP:ch32v --additional-urls https://github.com/YuukiUmeta-UIAP/board_manager_files/raw/main/package_uiap.jp_index.json

arduino-cli upload --fqbn UIAP:ch32v:CH32V00x_EVT:pnum=CH32V003V1DOT4,clock=48MHz_HSI,upload_method=minichlink --additional-urls https://github.com/YuukiUmeta-UIAP/board_manager_files/raw/main/package_uiap.jp_index.json ./UIAPduino_LED_test
```

ポート指定は不要です（`upload_method=minichlink` はUSB経由で基板を直接検出します）。

同じソースは Arduino IDE で開く用に [`../arduino_ide/UIAPduino_LED_test`](../arduino_ide/UIAPduino_LED_test) にも置いてあります。
