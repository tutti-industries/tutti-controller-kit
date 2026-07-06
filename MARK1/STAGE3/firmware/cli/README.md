# Makefile用コード（ch32fun生実装）

Arduino IDEもarduino-cliも使わず、[ch32fun](https://github.com/cnlohr/ch32v003fun)の生API + `make` + `minichlink`だけで
ビルド・書き込みするバージョンです。Arduino標準API版（`pinMode`/`digitalWrite`/`analogWrite`）は
[`../arduino_ide`](../arduino_ide) にあります。

このフォルダは自己完結型で、ビルドに必要なch32funコア・libgcc・minichlink（書き込みツール）を
同梱しています。

## 必要なもの

- RISC-V向けCコンパイラ（`riscv-none-elf-gcc`など）と`make`がPATH上にあること
- USBケーブル

## 使い方

### ビルド

```
make build
```

`main.bin`（実行イメージ）が生成されます。ソースを変更しなければ再実行不要です。

### 書き込み

1. 基板を**ブートモード**でPCにUSB接続する
2. `flash.bat`をダブルクリック、またはターミナルで以下を実行

   ```
   minichlink\minichlink.exe -w main.bin flash
   ```

3. `Image written.`と表示されれば書き込み完了です

ブートモードはUSB接続後ごく短時間しか有効ではないため、認識されない場合はケーブルを
一度抜き差ししてから間を置かずに書き込みコマンドを実行してください。

## 補足

- `minichlink/minichlink.exe`は本家[ch32v003fun](https://github.com/cnlohr/ch32v003fun)のビルドではなく、
  UIAPduinoのArduinoボードパッケージに同梱されているビルドを使用しています（本家ビルドではこの基板の
  オンボードデバッガを認識できませんでした）。
- ピン割り当て: LED1=PD2（デジタルIO）, LED2=PC3（TIM1_CH3 PWM）, LED3=PC0（TIM2_CH3 PWM）。
  PC0はこの基板のオンボードデバッガ回路と関係があり、操作するとデバッガがリセットされる場合があります
  （実害は確認されていませんが、書き込みに失敗するようになった場合は要注意です）。
- `ch32fun/`・`extralibs/`・`misc/libgcc.a`は[ch32v003fun](https://github.com/cnlohr/ch32v003fun)（MITライセンス、
  同梱の`ch32fun/LICENSE`参照）からの引用です。
