# TUTTI Controller Kit

ツッチーコントローラーキットの取扱説明書・コード保存用リポジトリです。

## フォルダ構成

バージョン（MARK）ごとにフォルダを分けて管理します。

```
MARK1/
  common/         回路図・部品表・共通の取扱説明（STAGE共通）
  STAGE1/         はんだ付けの練習
  STAGE2/         オームの法則の理解
  STAGE3/         半導体（トランジスタ）の理解
    firmware/
      arduino_ide/  Arduino IDEで書き込む用のコード
      cli/          コマンドプロンプト（arduino-cli等）で書き込む用のコード
  STAGE4/         センサー（プッシュスイッチ・ジョイスティック）の理解
    firmware/
      arduino_ide/
      cli/
MARK2/ (今後追加予定、同様の構成)
```

各STAGEフォルダには手順書PDF（`manual.pdf`）とそのSTAGEに必要なコードをまとめてあり、フォルダごとダウンロードすればすぐ作業を始められます。

新しいバージョンが登場したら `MARK2/`, `MARK3/`... と同じ構成でフォルダを追加してください。
