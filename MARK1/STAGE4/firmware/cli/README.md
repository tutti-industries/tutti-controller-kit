# コマンドプロンプト用コード（CLIビルド）

UIAPduino Pro Micro CH32V003 V1.4 用の HIDキーボード＋マウス ファームウェアです。
Arduino IDE は使わず、`make`（ch32v003fun）+ `minichlink` で書き込みます。

ベース: [nak435/UIAPduino-demo_hid_keyboard_mouse](https://github.com/nak435/UIAPduino-demo_hid_keyboard_mouse)（MIT License、`LICENSE.txt` 参照）

## 機能

- 3列×4行 キーマトリクス（列: PD0/PD5/PD6、行: PC4/PC5/PC6/PC7）
- ジョイスティック（PA2=X, PA1=Y）→ マウス移動（起動時センターキャリブレーション）
- マウスボタン・ホイールのキー割り当て対応
- LED×3（PD2/PC3/PC0）: キー種別ごとの点灯 + 起動演出

## 依存ライブラリ（別途クローンして同じ階層に置く）

| フォルダ | 取得元 |
|---|---|
| `ch32v003fun/` | https://github.com/cnlohr/ch32v003fun |
| `rv003usb/` `lib/` | https://github.com/YuukiUmeta-UIAP/rv003usb |

配置例:

```
cli/
├─ ch32v003fun/            ← クローン
├─ rv003usb/               ← クローン（rv003usb本体）
├─ lib/                    ← 同リポジトリの lib（tinyusb_hid.h など）
└─ demo_hid_keyboard_mouse/ ← このリポジトリのソース。ここでビルド
```

環境構築の詳細（RISC-V ツールチェーン等）はベースリポジトリの README を参照してください。

## ビルド & 書き込み

```
cd demo_hid_keyboard_mouse
make clean && make build
minichlink -C b803boot -w demo_hid_keyboard_mouse.bin flash -b
```
