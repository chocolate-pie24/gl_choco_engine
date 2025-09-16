# development_log.md

## Step1_1: 実行基盤とメモリ管理レイヤの初期化

### application_scaffolding

- 実装内容: Makefile / ビルドスクリプト / VSCodeワークスペース設定 / main関数の雛形
- ブランチ名称: feat/build-system
- 備考:
  - 目標は「ビルド・実行・クリーンアップできる最小環境」の構築
  - 今回はmacOSをターゲットにしたmakefileを作成するが、将来的な拡張を前提とする(将来的にはMac/Linux/FreeBSD対応を想定)

### Add application_layer

- 実装内容: application_create/run/destroyの公開API設計と最小実装。main() はこの3関数のみを呼び出す構成に整理。
- ブランチ名称: feat/application-layer

### Add core/message

- 実装内容: ERROR_MESSAGE/INFO_MESSAGE/DEBUG_MESSAGE/WARN_MESSAGE
- ブランチ名称: feat/core-message

### Refactor core/message -> base/message

- 実装内容: Move 'core/message' -> 'base/message'
  - 追加予定のcore/memoryがmessageへ依存するため、一方向依存を守る
  - engine/base/message.hへ変更

### Add base/macros

- 実装内容: メモリアロケータ実装の準備として、KIB, MIB, GIB等の共通マクロを用意する
  - KIB, MIB, GIB
  - 今後のテスト関数の用意のため、NO_COVERAGEを追加
- ブランチ名称: feat/base-macros

### Refactor base/message

- 実装内容: base/macrosと同様、名前衝突を回避するためchoco_messageへ変更
- ブランチ名称: refactor/base-message

### Add core/memory/linear_allocator

- 実装内容: リニアアロケータ
  - 個別解放不可(全てまとめて解放のみ)
  - 起動時から終了時まで解放されない各システム用メモリ確保に使用
- ブランチ名称: feat/core-memory

### Add core/memory/memory_system

- 実装内容: mallocラッパー+メモリトラッキング
  - 将来的にはFreeListを実装し、メモリアロケーション
  - 将来的にはメモリーシステムコンフィグレーションファイルを用意し、システム起動
- ブランチ名称: feat/core-memory

### Docs step1-wrapup

- 実装内容:
  - 本ドキュメントを含む各ドキュメントの整備
  - README.mdへのビルド方法等追加
  - doxygenドキュメント
  - todo.md作成

- TODO:
  - [x] このログのセクションを増やす。アプリケーション土台作り(メモリーシステム構築まで)
    - [x] base/macorsの誤記を修正
  - [x] README.mdにビルド方法等を整備
  - [x] memory_system_report で mem_tag_str[i] ?: "unknown" 的な防御（UB回避）。
  - [x] ヘッダガードのユニーク化
  - [] doxygenドキュメント
    - [x] Doxyfile用意
    - [] doxygenコメント追加
    - [] doxygen GitHub pages
    - [] doxygenページをREADME.mdにリンク追加
  - [] 別ブランチ エラーチェックマクロをmacros.hへ移動
  - [] doxygenスクリプト
  - [] エンジンレイヤー構成資料へのリンクをREADME.mdへ追加
