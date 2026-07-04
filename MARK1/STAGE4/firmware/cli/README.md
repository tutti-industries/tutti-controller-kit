# CLI からのファームウェア書き込み

tutti controller kit（UIAPduino Pro Micro CH32V003 V1.4 搭載）に、
コマンドライン（CLI）からファームウェアを書き込むためのフォルダです。

**書き込みに必要なツールはすべて同梱済み**です。インストール作業は不要で、
ダウンロード → コマンド1つ（またはダブルクリック）で書き込みが完了します。

---

## 1. ダウンロード

どちらか好きな方法で。

**A. ZIP でダウンロード（Git 不要）**

1. リポジトリのトップページで `Code` → `Download ZIP`
2. ZIP を展開し、`MARK1/STAGE4/firmware/cli` フォルダを開く

**B. git clone**

```
git clone https://github.com/tutti-industries/tutti-controller-kit.git
cd tutti-controller-kit\MARK1\STAGE4\firmware\cli
```

## 2. 書き込み（3ステップ）

1. `cli` フォルダの **`flash.bat` をダブルクリック**
   （またはコマンドプロンプトで `cd` してから `flash.bat` を実行）
2. 画面の指示に従い、コントローラーの **USB ケーブルを抜いて、挿し直す**
3. `[OK] 書き込み完了！` と出たら終了。そのままキーボード＋マウスとして使えます

> しくみ: コントローラーは USB を挿した直後の数秒だけ「ブートローダーモード」で
> 起動します。`flash.bat` はその瞬間を自動で捕まえて書き込みます。

**バッチファイルを使わず手動で書き込む場合**（`cli` フォルダで実行。USB を挿した直後に）:

```
ch32v003fun\minichlink\minichlink.exe -c 0x1209b803 -w prebuilt\demo_hid_keyboard_mouse.bin flash -b
```

## 3. 動作確認

書き込み後、コントローラーは PC から **HID キーボード＋マウス** として認識されます。

- LED が「ナイトライダー風」に流れる起動演出が出れば正常起動
- キーを押すと文字入力/マウス操作、スティックでマウスカーソルが動きます

---

## キー割り当てを変更したい（ビルドする人向け）

ソースは [`demo_hid_keyboard_mouse/`](demo_hid_keyboard_mouse/) にあります。
キー割り当ては `demo_hid_keyboard_mouse.cpp` の **`KEY_MAP`** を書き換えるだけです。
ASCII 文字（`'a'`, `'A'`, `'!'` など）、`KEY_ESC` などの特殊キー、
`KEY_MOUSE_BTN_LEFT` などのマウス操作を自由に割り当てられます
（大文字・記号は Shift が自動で付きます。同じキーの複数割り当ても可）。

### 必要なツール（ビルド時のみ）

| ツール | 入手先 |
|---|---|
| RISC-V GCC (`riscv-none-elf-gcc`) | [xPack GCC](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases)。Windows は同梱の `ch32v003fun/misc/install_xpack_gcc.ps1` でも導入可 |
| `make` | Windows は [Git for Windows](https://gitforwindows.org/)（Git Bash）推奨 |

### ビルド & 書き込み

```
cd demo_hid_keyboard_mouse
make clean && make build
copy demo_hid_keyboard_mouse.bin ..\prebuilt\
cd ..
flash.bat
```

（`copy` せずに `flash.bat demo_hid_keyboard_mouse\demo_hid_keyboard_mouse.bin` と
引数で直接指定することもできます）

---

## トラブルシューティング

| 症状 | 原因と対処 |
|---|---|
| `VID:0x1209, PID:0xb003` と出て失敗する | アプリモードで認識されています（ブートローダーは `0xb803`）。USB を挿し直した**直後**に再実行してください。`flash.bat` なら自動でリトライします |
| `Could not initialize b803boot programmer` | 同上。挿し直しのタイミングの問題です |
| flash.bat が延々と `....` のまま | USB ケーブルが充電専用（データ線なし）でないか確認。別のポート/ケーブルで試す |
| 書き込み後に動かない | 一度 USB を抜き挿しして再起動。LED の起動演出が出るか確認 |
| ビルド時に `ch32v003_GPIO_branchless.h: No such file or directory` | フォルダの階層が深すぎて Windows のパス長制限（260文字）を超えています。`C:\tutti` など浅い場所に移動してからビルドしてください（書き込みだけなら影響なし） |

## macOS / Linux の場合

同梱の `minichlink.exe` は Windows 用です。macOS / Linux では
[ch32v003fun](https://github.com/cnlohr/ch32v003fun) の `minichlink` を
`make` でビルドして、同じコマンド（`-c 0x1209b803 -w ... flash -b`）を実行してください。

## ライセンス / ベース

- ベースコード: [nak435/UIAPduino-demo_hid_keyboard_mouse](https://github.com/nak435/UIAPduino-demo_hid_keyboard_mouse)（MIT、`LICENSE.txt`）
- 同梱ライブラリ: [ch32v003fun](https://github.com/cnlohr/ch32v003fun)（MIT、`ch32v003fun/LICENSE`）、[rv003usb](https://github.com/YuukiUmeta-UIAP/rv003usb)（MIT）
