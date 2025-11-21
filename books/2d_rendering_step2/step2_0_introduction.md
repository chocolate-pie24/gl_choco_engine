---
title: "step2_0: プラットフォームレイヤーの構築とウィンドウ生成処理"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook2に対応しています。

前回の[Book](https://zenn.dev/chocolate_pie24/books/2d_rendering_step1)では、

- ビルドシステムの構築
- メモリトラッキング機能を有するメモリシステムの構築
- リニアアロケータの構築

を行いました。今回は、それらのシステムを用いながら、

- プラットフォーム基盤の作成
- ウィンドウの生成

を行なっていきます。

## ChangeLog

- 2025-11-21 step2_6における関数名の誤記修正(application_runtime -> application_run)

## Step2実装解説

### メモリシステムの仕様変更

いきなり仕様変更になってしまい恐縮なのですが、前回作成した仕様では、使用の際に不便であったため、仕様を変更することにします。前回の仕様では、メモリのアロケーション、解放のたびにメモリシステムモジュールを引数で渡す必要がありました。

そのため、メモリシステムの機能を使用する際には、メモリシステム構造体のインスタンスを持っておく必要があります。これではこれから開発していく各サブシステム全てにインスタンスを用意する必要があり、非常に不便です。なので、先ずはここを修正していきます。

[メモリシステムの仕様変更](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_1_change_memory_system)

### リニアアロケータの仕様変更

前回作成したリニアアロケータは、自身のメモリ確保のためにmallocを使用していました。ただ、せっかくメモリシステムを構築したので、リニアアロケータ自身のメモリ確保もメモリシステムを使用することにします。これにより、全てのメモリ確保、解放処理をメモリシステム経由で行います。

[リニアアロケータの仕様変更](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_2_change_linear_allocator)

### OpenGL環境の整備とLinuxサポート

以上が前回作成した部分の仕様変更で、ここからは今回新しく作成していく内容になります。

先ずは、OpenGL、GLFWを使用していくために、makefileを修正します。同時にLinux環境用のmakefileも用意し、Linux環境をサポートします。

[OpenGL環境の整備とLinuxサポート](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_3_add_linux_support)

### containers/stringの追加

ここからはGLFW APIを使用してウィンドウを生成する準備をしていきます。先ずは、文字列処理や、文字列リソース管理を行うコンテナモジュールとしてstringモジュールを作成します。

[containers/stringの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_4_add_container_string)

### プラットフォーム基盤の作成

今回、ウィンドウの生成にはGLFW APIを使用しますが、X Window SystemやWin32へ機能拡張が可能なプラットフォーム基盤を作成していきます。そのために、オブジェクト指向のデザインパターンであるStrategyをC言語で実装します。

[プラットフォーム基盤の作成](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_5_add_platform_layer)

### GLFWを使用したウィンドウ生成

GLFW APIを使用してウィンドウを生成する準備が整いましたので、ウィンドウを生成する処理を作成します。

[GLFWを使用したウィンドウ生成](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2/viewer/step2_6_add_glfw_window)

### まとめ

以上で「Step2: プラットフォームレイヤーの構築とウィンドウ生成処理」は完成となります。記事の中ではdoxygenのコメントやテストコードは省いています。全てのコードは、[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)に公開しています。ここまでの成果物には、v0.1.0-step2のタグをつけています。

次回はキーボード、マウス等のイベントを取得し、コールバックを用いたイベント処理システムを構築していきます。
