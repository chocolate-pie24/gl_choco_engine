# gl_choco_engineレイヤー構成

## レイヤツリー

```mermaid
graph TD
  A[application]
  C[core/message]

  A --> C
```

## 各レイヤー詳細

### application

- 目的: 最上位のオーケストレーション。サブシステム初期化、メインループ駆動、終了処理。
- 依存:
  - core

### core

- 目的: 最下層のユーティリティ(ログ、メモリアロケータ等)。外部依存なし(標準Cのみ)。
- 保有機能:
  - core/message: 色付き出力/レベル/STDOUT/STDERR
- 依存: C標準ライブラリのみ
