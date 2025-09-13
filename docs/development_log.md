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
