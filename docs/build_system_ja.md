<!--
AI Generated
-->

# GLCE ビルドシステム

## 1. 目的と対象範囲

この文書では、GLCEのビルドシステムについて以下を説明する。

- ビルドシステム全体の構成と責務分離
- `build.sh`を中心とした公開インターフェース
- Debug / Release / Testの各ビルドモード
- macOS / Linux / FreeBSDのOS別設定
- Coverage / Sanitizer / Valgrindの解析ワークフロー
- VS Codeとの連携方針
- ビルド生成物
- 運用上の制約

個別OSのセットアップ手順や、開発環境構築の履歴はこの文書の対象外とする。

GLCEのビルドおよび解析ワークフローの正式な入口は、リポジトリルートの`build.sh`である。

通常の利用では、`make/*.mk`や`scripts/*.sh`を直接実行せず、`build.sh`を経由する。

対応OSは次のとおりである。

| OS | 状態 |
|---|---|
| macOS | 対応 |
| Linux | 対応 |
| FreeBSD | 対応 |
| Windows native | 未対応 |

---

## 2. 全体構成

### 2.1 責務分離

ビルドシステムは、次の構成を採用する。

```text
build.sh
    ユーザー向けの公開インターフェース
    コマンド解釈
    ビルドモード選択
    OS判定
    Makefile / workflow選択

make/common.mk
    OS非依存のcompile / link処理
    source探索
    dependency生成
    build mode別compiler option
    clean

make/macos.mk
make/linux.mk
make/freebsd.mk
    OS固有設定
    compiler
    platform define
    include path
    library path
    link library

scripts/coverage.sh
scripts/sanitizer.sh
scripts/valgrind.sh
    専門的な解析ワークフロー

.vscode/
    VS Codeからbuild.shを利用するための設定
```

基本的な処理関係は次のとおりである。

```text
User / VS Code
      │
      ▼
   build.sh
      │
      ├─ command
      ├─ build mode
      └─ OS
          │
          ├───────────────┐
          ▼               ▼
     make/*.mk        scripts/*.sh
          │               │
          └───────┬───────┘
                  ▼
            make/common.mk
                  │
                  ▼
          compile / link / analysis
```

### 2.2 `build.sh`の責務

`build.sh`はフロントコントローラとして機能する。

主な責務は次のとおりである。

- 公開コマンドを解釈する
- `debug` / `release` / `test`を内部ビルドモードへ変換する
- `uname -s`でOSを判定する
- 使用するMakeコマンドを選択する
- OS別Makefileを選択する
- 必要に応じて専門workflowを起動する
- workflowへ実行コンテキストを渡す

個々の`.c`ファイルのcompile規則や、OS固有libraryの詳細は`build.sh`に置かない。

### 2.3 POSIX `sh`

`build.sh`および`scripts/*.sh`はPOSIX `sh`で記述する。

```sh
#!/bin/sh
```

Bash固有構文への依存を避け、macOS / Linux / FreeBSDで同じshell scriptを使用する。

---

## 3. 公開インターフェースとビルドモード

### 3.1 公開コマンド

ユーザーが使用するコマンドは次のとおりである。

| コマンド | 内容 |
|---|---|
| `./build.sh build debug` | Debugビルド |
| `./build.sh build release` | Releaseビルド |
| `./build.sh build test` | Testビルド |
| `./build.sh clean` | 生成物の削除 |
| `./build.sh coverage` | LLVM Coverage |
| `./build.sh sanitize` | Sanitizer実行 |
| `./build.sh valgrind` | Valgrind Memcheck |

### 3.2 ビルドモード

GLCEには3つのビルドモードがある。

| 公開名 | 内部値 | 主なcompiler option | プリプロセッサ定義 |
|---|---|---|---|
| `debug` | `DEBUG_BUILD` | `-g -O0` | `DEBUG_BUILD` |
| `release` | `RELEASE_BUILD` | `-O3` | `RELEASE_BUILD` |
| `test` | `TEST_BUILD` | `-g -O0` | `TEST_BUILD` |

#### Debug

開発およびデバッグ用である。

```text
-g
-O0
-DDEBUG_BUILD
```

#### Release

最適化した実行ファイルを生成する。

```text
-O3
-DRELEASE_BUILD
```

#### Test

テストコードを含めてビルドする。

```text
-g
-O0
-DTEST_BUILD
```

`TEST_BUILD`では、通常のsourceに加えて`test/src/`をビルド対象へ追加し、`test/include/`をinclude pathへ追加する。

CoverageおよびSanitizerも`TEST_BUILD`を使用する。

---

## 4. Make構成

### 4.1 ファイル構成

```text
make/
├── common.mk
├── macos.mk
├── linux.mk
└── freebsd.mk
```

OS別Makefileは最初に`make/common.mk`をincludeし、共通設定へOS固有設定を追加する。

### 4.2 `make/common.mk`

`make/common.mk`はOSに依存しないビルド処理を定義する。

主要設定は次のとおりである。

```make
TARGET := gl_choco_engine
BUILD_DIR := bin
OBJ_DIR := obj

SRC_ROOT_DIRS := engine application
TEST_SRC_DIR := test/src
```

通常ビルドのsource rootは次のとおりである。

```text
engine/
application/
```

`TEST_BUILD`では次を追加する。

```text
test/src/
```

共通include pathは次のとおりである。

```text
-I.
-Iinclude
```

`TEST_BUILD`では次を追加する。

```text
-Itest/include
```

### 4.3 共通compiler option

共通compiler optionには、C11指定、warning、dependency生成などを含む。

主な分類は次のとおりである。

- C11
- 一般warning
- conversion / format / prototype / shadow等の追加warning
- Clang固有warning
- project方針上無効化しているwarning
- `.d` dependency file生成

dependency生成には次を使用する。

```text
-MMD
-MP
```

生成された`.d`ファイルは`common.mk`からincludeし、header変更時の再コンパイル判定に使用する。

### 4.4 sourceとobjectの対応

source treeの構造を保持したまま、`obj/`以下にobjectを生成する。

例:

```text
engine/core/example.c
    ↓
obj/engine/core/example.o
obj/engine/core/example.d
```

### 4.5 OS別設定

#### macOS

```text
Make command:
    make

Makefile:
    make/macos.mk

Compiler:
    Homebrew LLVM Clang

Platform define:
    PLATFORM_MACOS
```

GLEW / GLFW / LLVMのprefixはHomebrewから取得する。

OpenGL関連では主に次をリンクする。

```text
glfw
glew
OpenGL.framework
IOKit.framework
Cocoa.framework
```

#### Linux

```text
Make command:
    make

Makefile:
    make/linux.mk

Compiler:
    clang

Platform define:
    PLATFORM_LINUX
```

POSIX API用に次を定義する。

```text
_POSIX_C_SOURCE=200809L
```

主なlink libraryは次のとおりである。

```text
-lm
-lGL
-lglfw
-lGLEW
```

#### FreeBSD

```text
Make command:
    gmake

Makefile:
    make/freebsd.mk

Compiler:
    clang

Platform define:
    PLATFORM_FREEBSD
```

FreeBSD標準の`make`はBSD makeであるため、GLCEではGNU Makeを`gmake`として使用する。

`pkg`で導入したthird-party libraryの標準配置に合わせ、次を追加する。

```text
-I/usr/local/include
-L/usr/local/lib
```

主なlink libraryはLinuxと同様に次を使用する。

```text
-lm
-lGL
-lglfw
-lGLEW
```

---

## 5. 解析ワークフロー

解析ワークフローは`scripts/`以下へ分離する。

```text
scripts/
├── coverage.sh
├── sanitizer.sh
└── valgrind.sh
```

これらは`build.sh`から起動される。

`build.sh`はworkflowへ次の実行コンテキストを環境変数として渡す。

| 変数 | 内容 |
|---|---|
| `GLCE_DIR` | GLCEリポジトリの絶対パス |
| `OS_NAME` | OS名 |
| `MAKE_COMMAND` | `make`または`gmake` |
| `MAKEFILE` | 選択済みOS別Makefile |

必要なコンテキストが存在しない場合、workflowはエラー終了する。

### 5.1 Coverage

`scripts/coverage.sh`はLLVM Source-based Code Coverageを実行する。

処理概要:

```text
clean
  ↓
Coverage flag付きTEST_BUILD
  ↓
bin/gl_choco_engine実行
  ↓
llvm-profdata
  ↓
llvm-cov
  ↓
cov/index.html
```

LLVM toolの取得方法はOSによって異なる。

| OS | `llvm-profdata` / `llvm-cov` |
|---|---|
| macOS | Homebrew LLVM配下 |
| Linux | PATHから取得 |
| FreeBSD | PATHから取得 |

### 5.2 Sanitizer

`scripts/sanitizer.sh`はAddressSanitizerとUndefinedBehaviorSanitizerを有効にする。

主な設定は次のとおりである。

```text
-fsanitize=address,undefined
-fno-sanitize-recover=all
-fno-omit-frame-pointer
-fsanitize-address-use-after-scope
-O1
```

処理概要:

```text
clean
  ↓
Sanitizer flag付きTEST_BUILD
  ↓
ASAN_OPTIONS / UBSAN_OPTIONS設定
  ↓
bin/gl_choco_engine実行
```

LeakSanitizerの扱いは次のとおりである。

| OS | Leak detection |
|---|---|
| macOS | 有効 |
| Linux | 有効 |
| FreeBSD | 無効 |

### 5.3 Valgrind

`scripts/valgrind.sh`はValgrind Memcheckを実行する。

対応OS:

```text
Linux
FreeBSD
```

処理概要:

```text
valgrind存在確認
  ↓
clean
  ↓
DEBUG_BUILD
  ↓
Valgrind Memcheck
  ↓
bin/gl_choco_engine
```

macOSではValgrind workflowを使用しない。

---

## 6. VS Code連携

### 6.1 基本方針

VS Code固有設定は`.vscode/`以下に置く。

```text
.vscode/
├── tasks.json
├── launch.json
├── c_cpp_properties.json
└── settings.json
```

ビルドロジック自体はVS Codeへ持たせない。

VS Codeのtaskは`build.sh`を呼び出すだけとし、OS判定やMakefile選択は`build.sh`に任せる。

```text
VS Code task
    ↓
build.sh
    ↓
OS判定
    ↓
make / gmake
```

この構造により、`tasks.json`へOS固有ロジックを重複させない。

### 6.2 workspace構成

現在の`.vscode`設定は次の構成を前提とする。

```text
<workspace root>/
├── .vscode/
└── glce/
    ├── build.sh
    ├── engine/
    ├── application/
    └── ...
```

### 6.3 `tasks.json`

次のtaskを定義する。

```text
build_debug
build_release
build_test
clean
sanitize
coverage
valgrind
```

各taskは`build.sh`の公開コマンドへ対応する。

compiler診断を伴うtaskではproblem matcherを使用し、GLCEルートを基準にsource pathを解決する。

### 6.4 IntelliSense

`c_cpp_properties.json`はmacOSとLinux向けのIntelliSense設定を持つ。

共通のproject include pathは次のとおりである。

```text
${workspaceFolder}/glce
${workspaceFolder}/glce/include
${workspaceFolder}/glce/test/include
```

macOS:

```text
PLATFORM_MACOS
macos-clang-arm64
```

Linux:

```text
PLATFORM_LINUX
_POSIX_C_SOURCE=200809L
linux-clang-x64
```

FreeBSD専用の`c_cpp_properties.json`構成は持たない。

FreeBSDではVS Codeのbuild / analysis taskを利用し、FreeBSD固有のIntelliSense統合はビルドシステムの正式対応範囲外とする。

### 6.5 デバッグ

`launch.json`によるIDE統合デバッグは、利用するVS Code版およびdebug adapterが対応する環境で使用する。

ビルドシステム自体は特定のdebug adapterへ依存しない。

`preLaunchTask`は設定せず、デバッグ前にユーザーが明示的に必要なbuild modeを選択する。

FreeBSDではVS Code統合デバッガを前提としない。

GUIデバッグが必要な場合は、Qt Creator等のGDB対応フロントエンドから、Debugビルド済みの次のbinaryを使用する。

```text
bin/gl_choco_engine
```

IDE固有のbuild systemへGLCEを移行せず、ビルドは引き続き`build.sh`を使用する。

---

## 7. 生成物と依存ツール

### 7.1 生成物

最終実行ファイル:

```text
bin/gl_choco_engine
```

object / dependency:

```text
obj/**/*.o
obj/**/*.d
```

Coverage report:

```text
cov/index.html
```

Debug / Release / Testは同じ`bin/`と`obj/`を共有する。

### 7.2 共通依存

主な依存は次のとおりである。

- POSIX `sh`
- Clang
- Make環境
- OpenGL
- GLFW
- GLEW

### 7.3 macOS固有依存

- Homebrew
- Homebrew LLVM
- Homebrew GLFW
- Homebrew GLEW

### 7.4 Linux固有依存

環境に応じて次が必要である。

- Clang
- GNU Make
- OpenGL development package
- GLFW development package
- GLEW development package
- LLVM Coverage tools
- Valgrind

package名はdistributionによって異なる。

### 7.5 FreeBSD固有依存

主なpackageは次のとおりである。

```text
gmake
glew
glfw
```

解析機能を使用する場合は必要に応じて次を利用する。

```text
valgrind
llvm-profdata
llvm-cov
```

FreeBSDではthird-party packageのheader / libraryが主に`/usr/local`以下へ配置される。

---

## 8. 運用上の制約

### 8.1 ビルドモード変更時はcleanする

Debug / Release / Testは同じ`obj/`を共有する。

Makeはcompiler flagの変更そのものをdependencyとして追跡しないため、ビルドモードを切り替える場合は先にcleanする。

```sh
./build.sh clean
./build.sh build release
```

同一ビルドモードでの通常の増分ビルドでは、毎回cleanする必要はない。

### 8.2 複数モードの成果物を同時保持しない

現在は次のようなモード別出力ディレクトリを使用していない。

```text
bin/debug/
bin/release/
obj/debug/
obj/release/
```

最後にビルドしたモードの成果物だけが`bin/`と`obj/`に保持される。

### 8.3 実行時のcurrent directory

GLCEの一部処理はassets等を相対パスで参照する。

そのため、実行時は原則としてGLCEルートをcurrent directoryとする。

```sh
cd <path-to-glce>
./bin/gl_choco_engine
```

assets pathの設計そのものは、ビルドシステムとは別の責務として扱う。

### 8.4 専門workflowを直接実行しない

`scripts/*.sh`は`build.sh`から渡される実行コンテキストを前提とする。

通常利用では次の形式を使用する。

```text
build.sh
    ↓
scripts/*.sh
```

`scripts/*.sh`をユーザー向け公開インターフェースとして扱わない。
