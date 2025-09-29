---
title: "Step1_6: doxygen環境の整備"
free: true
---

※本記事は [全体イントロダクション](../../articles/introduction.md)のBook1に対応しています。

## このステップでやること

ここまでで、これからゲームエンジンを構成する様々なサブシステムを開発し、起動させていく土台が完成しました。ゲームエンジン開発は長期に渡る開発になります。このため、過去に作成したコードの仕様を忘れてしまうということが起こります。

現状の機能が一つの区切りであるということと、それなりにコード量も多くなってきたので、ドキュメントを整備していきます。

## doxygen環境整備

ドキュメントはdoxygenを使用してコード上のコメントから自動生成するようにします。doxygenのインストール方法については、「doxygen インストール」等で検索すれば多くのページが出てくるので、ご使用の環境に応じてインストールしてください。

なお、doxygenがなくてもエンジンの動作自体には全く影響がないため、必須の作業ではありません。

doxygenは、プロジェクトに応じた設定ファイルを作成し、起動時に読み込むことで設定に応じた仕様書を生成してくれます。まずは、設定ファイルを作成していきましょう。

今回、設定ファイルはプロジェクトのルートディレクトリに置くことにし、ファイル名はDoxyfileとしています。以下のコマンドでデフォルトのDoxyfileを生成することができます。

```bash
doxygen -g Doxyfile
```

これでDoxyfileが生成されたと思います。次は、ドキュメントを格納するディレクトリの構成について考えます。

doxygenで生成されるドキュメントはhtml形式で出力することが可能です。他にもLaTex形式での出力も可能なのですが、今回のプロジェクトはブラウザ上で確認できれば十分なので、html形式のみを使用します。プロジェクトルートのdocs/doxygen/htmlにドキュメントを出力するようにしていきます。

```console
.
├── build.sh
├── cov.sh
├── docs
│   └── doxygen
│       └── html
├── Doxyfile
├── include
│   ├── application
│   │   └── application.h
│   └── engine
│       ├── base
│       │   ├── choco_macros.h
│       │   └── choco_message.h
│       └── core
│           └── memory
│               ├── choco_memory.h
│               └── linear_allocator.h
├── LICENSE
├── makefile_macos.mak
├── README.md
├── src
│   ├── application
│   │   └── application.c
│   ├── engine
│   │   ├── base
│   │   │   └── choco_message.c
│   │   └── core
│   │       └── memory
│   │           ├── choco_memory.c
│   │           └── linear_allocator.c
│   └── entry.c
└── test
    └── include
        ├── test_linear_allocator.h
        └── test_memory_system.h
```

## モジュール構成の設定

次にレイヤー構成のドキュメントを生成するようにしていきます。docs/doxygen/groups.doxというファイルを作成し、次のように記載します。今後、レイヤーが増えるに従って、こちらのファイルもメンテナンスをしていきます。

```console
/** @defgroup application Application
    @brief エンジンが保有する各サブシステムのオーケストレーション。サブシステム初期化、メインループ駆動、終了処理
*/

/** @defgroup engine Engine
    @brief エンジンの最上位レイヤー。ゲームアプリケーション側に描画/リソース管理/イベント処理等の機能を提供する
*/

/** @defgroup base Base
    @ingroup engine
    @brief 最下層の“横断ユーティリティ”。全レイヤーから安心して使える小道具を提供する
*/

/** @defgroup core Core
    @ingroup engine
    @brief プロジェクト全体から使用される機能/APIを提供する(メモリアロケータ/数学ライブラリ等)
*/

/** @defgroup core_memory Core Memory
    @ingroup core
    @brief メモリアロケータ/メモリトラッキング
*/
```

このファイルを作成し、各モジュールファイルの先頭でaddtogroupでgroupに追加することでモジュールごとにドキュメントを読むことができるようになります。

このように使用します(choco_message.hの例)。まずファイルの先頭にaddtogroupで次のように書いておくと、baseにchoco_messageが追加されます。

```c
/** @addtogroup base
 * @{
 *
 * @file choco_message.h
 * @author chocolate-pie24
 * @brief メッセージ標準出力、標準エラー出力処理
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
```

次に、ファイルの最後に、

```c
/* @} */
```

と書くことで、ファイル内の@{...@}に書かれている情報がbaseレイヤーの資料として生成されます。

## 設定ファイルの編集

これでディレクトリ構成が決まりましたので、実際に設定を行っていきます。

doxygenの設定ファイルは設定値が非常に多く、全体を把握することが難しいです。私は基本的には設定ファイルは使い回しで、たまに気が向いた時にメンテナンスをしています。

今回は記事を書くということで折角なので新規に作成しています。おすすめの設定値をChatGPTに聞いて、得られたものをベースに作っていきました。

doxygenの使用法については、そこまで強いこだわりがあるわけではないのと、それなりに良いものができたのでこれで良いかなと思っています。今回設定を変更した点について、設定値の意味と今回設定した値を解説してきます。

- PROJECT_NAME = "gl_choco_engine"
生成物に表示するプロジェクト名

- EXTRACT_ALL = YES
YESにするとコメントが無くても全シンボルをドキュメント化対象にする(＝未記載も出す)。
典型: 初期整備期はYESにして「未記載の発見」を促し、安定後はNOにして「書いたものだけ」を公開。

- EXTRACT_STATIC = YES
static関数/変数をドキュメント化するか。内部実装まで可視化したいならYES、公開APIだけならNO。当面はYESを選択。

- WARN_IF_UNDOCUMENTED = YES
コメントが無いエンティティがあれば警告する。品質担保に有効。YES推奨(CIで見落とし防止)。

- WARN_NO_PARAMDOC = YES
@param など引数の説明が欠けている場合に警告。関数APIの質を上げたいならYES推奨。

- INPUT = include src docs/doxygen README.md
解析対象のパスやファイル(スペース区切りで複数可)。
例: ./core/include ./core/src README.md

- RECURSIVE = YES
INPUT以下を再帰的に辿るか。通常YES。

- FILE_PATTERNS = *.h *.c *.dox *.md
対象にする拡張子パターン。
例: *.h *.c *.md(Cプロジェクト＋Markdown)

- USE_MDFILE_AS_MAINPAGE = README.md
ここで指定した Markdown をトップページ(Main Page)として使う。

- EXCLUDE = docs/doxygen/html
除外したいパス(絶対/相対)。生成物/サードパーティなどを外すのに使う。
例: build/ third_party/

- EXCLUDE_PATTERNS = docs/doxygen/html/**
除外パターン(ワイルドカード)。
例: */tests/* */.cache/*

- OUTPUT_DIRECTORY = docs/doxygen
生成物の出力ディレクトリ。
例: docs/build(HTMLやXMLなどのルート出力先)

- GENERATE_HTML = YES
HTMLドキュメントを出すか。通常YES。

- HTML_OUTPUT = html
HTML のサブディレクトリ名。
例: html(最終的に docs/build/html などになる)

- GENERATE_TREEVIEW = YES
画面左側にツリーナビを出す(古めのテーマ想定)。見通し改善にYESが多い。

- GENERATE_XML = NO
XML出力を生成。Sphinx+Breathe連携や外部ツール処理に必要ならYES。不要ならNO。

- GENERATE_LATEX = NO
LaTeX(PDF化前提)を出すか。PDFを配布したいときのみYES。通常はNO。

- GENERATE_MAN = NO
manページを出すか。UNIX系でCLI/ライブラリのmanを配布したいならYES、通常はNO。

設定項目のINPUTにdocs/doxygenを追加しているのは先ほどのgroups.doxを読み込ませるためです。また、このままだとdocs/doxygen/htmlも入力対象になってしまうため、EXCLUDEで除外しています。

以上でdoxygenの環境が整備されたので、プロジェクトルートで、

```bash
doxygen ./Doxyfile
```

を実行することでドキュメントが生成されます。生成されたドキュメントはdocs/doxygen/html/index.htmlを開くと見ることができます。なお、実際に各APIに記載したdoxygenコメントについては、GitHubのコードを参照してください。

## Step1まとめ

以上、だいぶ長くなってしまいましたが、"Step1: 実行基盤とメモリ管理レイヤの初期化(イントロダクション)"の解説は以上になります。ここまでのコードは[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)にv0.1.0-step1タグをつけています。

まだグラフィックプログラミング特有の話題が一切出てきていませんが、こういった基盤作りはゲームエンジン開発では非常に重要だと考えています。土台がしっかりしていれば、後に作成する機能の開発がスムーズになりますし、不要な手戻りも少なくなります。

ただ、このような記事を書くのが初めてであることと、何分ドキュメント作成が苦手なこともあり、読みづらい点も多かったと思います。不明な点、より詳しい解説をして欲しい点がありましたら、是非コメント等でお知らせください。

次回以降はいよいよウィンドウ作成、三角形の描画、イベント処理に入っていきますので、Step1よりはグラフィックプログラミングらしい作業になると思います。ご興味のある方はそちらも是非、読んでいただけると幸いです。
