# development_log.md

## application_scaffolding

- 実装内容: Makefile / ビルドスクリプト / VSCodeワークスペース設定 / main関数の雛形
- ブランチ名称: feat/build-system
- 備考:
  - 目標は「ビルド・実行・クリーンアップできる最小環境」の構築
  - 今回はMacOSをターゲットにしたmakefileを作成するが、将来的な拡張を前提とする(将来的にはMac/Linux/FreeBSD対応を想定)

## Add application_layer

- 実装内容: application_create/run/destroyの公開API設計と最小実装。main() はこの3関数のみを呼び出す構成に整理。
- ブランチ名称: feat/application-layer

## Add core/message

- 実装内容: ERROR_MESSAGE/INFO_MESSAGE/DEBUG_MESSAGE/WARN_MESSAGE
- ブランチ名称: feat/core-message

## Refactor core/message -> base/message

- 実装内容: Move 'core/message' -> 'base/message'
  - 追加予定のcore/memoryがmessageへ依存するため、一方向依存を守る
  - engine/base/message.hへ変更

## Add base/macors

- 実装内容: メモリアロケータ実装の準備として、KIB, MIB, GIB等の共通マクロを用意する
  - KIB, MIB, GIB
  - 今後のテスト関数の用意のため、NO_COVERAGEを追加
- ブランチ名称: feat/base-macros

## Refactor base/message

- 実装内容: base/macrosと同様、名前衝突を回避するためchoco_messageへ変更
- ブランチ名称: refactor/base-message
