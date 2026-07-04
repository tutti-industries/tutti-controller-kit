# コマンドプロンプト用コード

Arduino IDE を使わず、コマンドプロンプト（arduino-cli など）で書き込むためのコードを置くフォルダです。

## 使い方例

```
arduino-cli compile --fqbn <ボード名> .
arduino-cli upload -p <ポート名> --fqbn <ボード名> .
```

※ 具体的なボード名・ポート名は環境に合わせて置き換えてください。
