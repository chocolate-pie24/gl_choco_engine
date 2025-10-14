# development_log.md

そのうちやる
- [] doxygen GitHub pages
- [] doxygenページをREADME.mdにリンク追加
- [] clang-tidyセットアップ
- [] サニタイザセットアップ

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
  - 将来的にはメモリシステムコンフィグレーションファイルを用意し、システム起動
- ブランチ名称: feat/core-memory

### Docs step1_1-wrapup

- 実装内容:
  - 本ドキュメントを含む各ドキュメントの整備
  - README.mdへのビルド方法等追加
  - doxygenドキュメント
  - todo.md作成

### Refactor step1_1-cleanup

- 実装内容
  - [x] 無駄に厳しいビルドワーニング修正
  - [x] エラーチェックマクロをbase/macros.hへ移動し共通化
  - [x] README.mdにレイヤー構成資料へのリンクを追加
  - [x] message_outputのバッファオーバーフローの危険性バグを修正
  - [x] memory_system_preinitの引数にconst付加
  - [x] application_createの未使用変数削除
  - [x] application_createの不適切なエラーコードの集約を修正
  - [x] const付加

## 2D-Rendering Step2: ウィンドウの生成とイベント、インプットシステムの作成

目的: プラットフォーム(X-window / Win32 / GLFW...)に依存しないウィンドウ制御とマウス、キーボードイベント処理を実現する
追加レイヤー: platform
追加モジュール: core/input_system, core/event_system
ブランチ: feat/2d-rendering-step2

step2 TODO:
 - [x] platform/platform_glfwを作ってウィンドウ初期化
   - [x] 初期化コード / ウィンドウ生成コード作成
   - [x] makefile修正
   - [x] 実行確認
 - [x] platform層をstrategy化
   - [x] strategy化
   - [x] ChatGPTレビュー実施
 - [x] containers/choco_string
 - [x] core/event_system
 - [x] core/input_system
 - [] テスト
 - [] doxygenコメント追加
 - [] doxygen(groups.doxメンテナンス)
 - [] README.mdのtree修正
 - [] layer.mdメンテナンス
 - [] memory_system_allocateで確保されるメモリがmax_align_tである旨を明記する
 - [] books執筆
   - [] articleに更新履歴を追加
   - [] 前回、今回やるといった内容との整合性がとれているか確認
   - [] booksのディレクトリ整理
   - [] development_logのタイトル整理(2d_rendering/step1が前回まで)
   - [] articleメンテナンス
   - [] book執筆

メモ:
 - [] book/chapter1: glfw window
 - [] book/chapter2: strategy
 - [] book/chapter3: choco_string
 - [] book/chapter4: memory_system仕様変更
 - [] book/chapter5: sleep -> application run変更
 - [] book/chapter6: input system
 - [] book/chapter7: event system

### platform/platform_glfwを作ってウィンドウ初期化

実装内容: GLFW実行環境の整備, GLFWを使用したウィンドウ初期化 / 生成処理
ブランチ: feat/2d-rendering-step2 -> feat/glfw-window-initialize

### platformへのstrategyパターンの適用

実装内容: ウィンドウ生成処理をstrategyパターンを使用するように変更
目的:
 - プラットフォームを抽象化することにより、将来的なx-window, win32への拡張性を持たせる
 - makefileに-DUSE_GLFWを追加し、ビルド時に使用プラットフォームを選択できるようにする
 - 選択したプラットフォーム以外のライブラリ等が存在しなくてもビルドできるように#ifdef USE_XXXを入れる
ブランチ: feat/2d-rendering-step2 -> feat/apply-strategy-to-platform

### containers/choco_stringの追加

実装内容: platform_stateのwindow_labelをリソース管理機能を持つchoco_stringに変更する
ブランチ: feat/2d-rendering-step2 -> feat/choco-string

### fix/memory-system, refactor/linear-allocator, refactor/application-create refactor/choco-string

改善理由:
memory_systemの実態をアプリケーション側に持たせるのはまずい。memory_system_allocateでいちいちmemory_systemを渡す必要がある
特に、choco_stringのように様々なところから呼ばれる関数の場合、choco_string_xxxにすべてmemory_systemを渡す必要があり煩わしい
memory_systemの改善に合わせ、linear_allocatorについても仕様を変更する

- [x] test_malloc操作の外部公開APIを追加
- [x] linear_allocatorのtest_mallocを除去
- [x] fix/memory_system          : 先にfix/memory_systemでmemory_systemの仕様変更を行う
- [x] refactor/linear-allocator  : linear_allocatorのメモリをmemory_systemで確保するように仕様変更
- [x] refactor/application-create : 上記に合わせ、サブシステム用メモリ総容量を事前に計算してlinear_allocatorでメモリ確保するようapplication変更
- [x] refactor/memory-system      : destroyの際にtotal_allocatedが0でない場合にワーニングを出力
- [x] test/memory-refactoring     : 上記仕様変更に合わせて全てのテストコードを追加、修正、テスト実施
  - [x] linear_allocatorテスト
  - [x] memory_systemテスト
    - [x] memory_system_create
    - [x] memory_system_destroy
    - [x] memory_system_allocate
    - [x] memory_system_free
    - [x] memory_system_report
- [x] docs/choco-memory           : choco_memory.h, .cのdoxygenコメント修正, memory_system_allocate_aligneをtodoに追加(FreeListの後)
    - [x] memory_system_create
    - [x] memory_system_destroy
    - [x] memory_system_allocate
    - [x] memory_system_free
    - [x] memory_system_report
- [x] docs/linear-allocator       : linear_allocator.h, .cのdoxygenコメント修正

### feat/event-system

実装内容:
- キーボード、マウスイベントを取得する処理を追加する
- application_runのループにsleepを追加する
- escapeを押下でapplication_runを抜けるようにする

- [x] callbackはapplication層に配置する
- [x] core/event/event_utils
- [x] core/input/mouse_event -> mouse_event_t
- [x] core/input/keyboard_event -> keyboard_event_t
- [x] containers/ring_queue -> 所有はapplication_state
  - [x] mouse_event_queue
  - [x] keyboard_event_queue
- [x] platform/platform_glfw -> メッセージをキューにpush, applicatio層がpop
- [] layer.md整理
- [x] memory_system_allocateの契約にmax_align_tにアラインされたメモリを返す、を追記
- [x] application.c エラーコード変換(application以外も全部 refactor/error-message)
- [x] clang-tidy include過不足チェック
- [] そのうちやる(application.cのエラー文字列周りを別ファイルに移す)
- [x] linux support
- [x] README.mdへのコンパイラ、ライブラリセットアップ追記
- [x] macOS makefile修正(m1チップとintelチップでのhomebrewのインストールパスの違いを吸収)
