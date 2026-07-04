# TUTTI Controller Kit

ツッチーコントローラーキットの取扱説明書・コード保存用リポジトリです。

## フォルダ構成

バージョン（MARK）ごとにフォルダを分けて管理します。

```
MARK1/
  firmware/
    arduino_ide/  Arduino IDEで書き込む用のコード
    cli/          コマンドプロンプト（arduino-cli等）で書き込む用のコード
  hardware/       回路図・部品表・基板データなど
  docs/           取扱説明書（README.mdがそのまま取説）
MARK2/ (今後追加予定)
  firmware/
    arduino_ide/
    cli/
  hardware/
  docs/
```

新しいバージョンが登場したら `MARK2/`, `MARK3/`... と同じ構成でフォルダを追加してください。
