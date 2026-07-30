# MARK1 共通ファームウェア

UIAPduino Pro Micro CH32V003 V1.4 を搭載したコントローラーへ、
LED待機演出付きのキーボード・マウスファームを書き込むための
自己完結フォルダです。STAGE1〜4配下のfirmwareは必要ありません。

## いちばん簡単な書き込み方法（Windows）

1. GitHubからリポジトリのZIPをダウンロードして展開します。
2. `MARK1/common/firmware` フォルダを開きます。
3. `flash.bat` をダブルクリックします。
4. 画面に案内が出たら、コントローラーのUSBを抜いて挿し直します。
5. `[OK] Firmware written successfully.` と表示されたら完了です。

コンパイラ、Arduino IDE、追加ドライバのインストールは不要です。
書き込みには `prebuilt` 内の実機確認用ファームを使用します。

## コマンドプロンプトから書き込む

```bat
cd /d <ダウンロード先>\tutti-controller-kit\MARK1\common\firmware
flash.bat
```

## コードを変更してビルドする

GNU MakeとRISC-V GCC（`riscv-none-elf-gcc` など）がPATHに必要です。
準備後、このフォルダで次を実行します。

```bat
build.bat
```

ビルドに成功すると、次の2ファイルが更新されます。

- `UIAPduino_HID_keyboard_mouse_LED/demo_hid_keyboard_mouse_led.bin`
- `prebuilt/demo_hid_keyboard_mouse_led.bin`

続けて `flash.bat` を実行すると、新しいファームを書き込めます。

## フォルダ構成

```text
firmware/
├─ README.md
├─ flash.bat                         書き込み（通常ユーザー向け）
├─ build.bat                         再ビルド（開発者向け）
├─ prebuilt/                         ビルド済みファーム
├─ UIAPduino_HID_keyboard_mouse_LED/ メインコード
├─ shared/hid/                       共通HID実装
├─ vendor/ch32v003fun/               CH32V003ビルド環境
├─ vendor/rv003usb/                  ソフトウェアUSB
├─ vendor/lib/                       USB関連ヘッダー
└─ tools/windows/                    Windows書き込みツール
```

## macOS / Linux

ファーム自体は標準USB HIDとして使用できます。ただし、同梱の
`minichlink.exe` はWindows用です。macOS / Linuxから書き込む場合は、
各OS向けにビルドしたminichlinkとlibusbが必要です。
