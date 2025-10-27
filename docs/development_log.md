# development_log.md

## Step1: 実行基盤とメモリ管理レイヤの初期化

[Book1](https://zenn.dev/chocolate_pie24/books/2d_rendering)

## Step2: プラットフォームレイヤーの構築とウィンドウ生成

- [x] ソフトfix
- [x] ブランチ: release/2d-rendering-step2
- [x] Step1タグをcloneしてそれをベースにソフト追加(記事にはエラー処理を省く)
- [] articlesの構成変更
- [] booksのディレクトリ構成変更
- [] step2構成
  - [] step2_0_introduction.md            : イントロダクション
  - [] step2_1_change_memory_system.md    : メモリーシステム仕様変更
  - [] step2_2_change_linear_allocator.md : リニアアロケータ仕様変更(application.c未対応項目がないか確認)
  - [] step2_3_add_linux_support.md       : Linux対応 / OpenGLライブラリ対応
  - [] step2_4_add_container_string.md    : コンテナ-string
  - [] step2_5_add_platform_layer.md      : プラットフォーム基盤
  - [] step2_6_add_glfw_window.md         : ウィンドウ生成
- [] 最終確認
  - [] development_log.md掃除
  - [] books/step1が公開されているか確認, 図のリンク等、一通り確認
  - [] articleの各種リンク動作確認
  - [] 不要ブランチ整理
  - [] README.md更新
    - [] clang-tidy includeチェック
    - [] layer.md
    - [] groups.dox
    - [] ツリー更新
    - [] ブックリンク、構成更新
    - [] その他全体チェック
  - [] featブランチのdevelopment_log.mdのTodoをtodo.mdに移動
  - [] articleにchangelog追加
  - [] ディレクトリ構成変更により新しいbook1が出るので、古いbook1を削除する
