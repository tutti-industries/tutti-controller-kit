# HID キーボード・マウス + 待機LED演出

UIAPduino Pro Micro CH32V003 V1.4 用の統合ファームです。

- 起動後、USB HIDとしてPCに認識されながらLED演出を繰り返します。
- ボタンを押す、またはジョイスティックをデッドゾーン外へ倒すと、待機演出を直ちに停止します。
- 操作を検出した同じループでHID入力を送るため、最初のキー操作は捨てません。
- 以降は通常のキーボード・マウス動作と、キー種別に応じたLED表示になります。
- ACアダプタ給電時も待機演出は動作します。PCは不要です。

## ビルド

このフォルダで次を実行します。

```text
make clean && make build
```

## 書き込み

```text
../../../STAGE4/firmware/cli/ch32v003fun/minichlink/minichlink.exe -c 0x1209b803 -w demo_hid_keyboard_mouse_led.bin flash -b
```

USBを挿し直した直後に実行してください。

## 編集箇所

キーマップ、行列ピン、LEDピン、ジョイスティック感度は
`demo_hid_keyboard_mouse_led.cpp` 冒頭の定義を編集します。

このプロジェクトは `MARK1/STAGE4/firmware/cli` にある実績済みの
USB/HIDスタックを参照してビルドします。そのため、STAGE4フォルダと
このフォルダは同じリポジトリ内の現在の相対配置で使用してください。
