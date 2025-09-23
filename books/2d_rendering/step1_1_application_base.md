---
title: "Step1_1: アプリケーション土台作り"
free: true
---

## このステップでやること

このステップで作っていくのは、

- VSCodeのコマンドパレットでビルド実行
- ビルドにはRELEASE用、DEBUG用、TEST用のビルドモードを設ける
- ビルドモードおよび実行プラットフォームをプログラム側で識別可能にする
- VSCodeのコマンドパレットでクリーンビルド実行
- VSCode上でF5キーを押すことでビルドされたバイナリを実行

これらを実現できる最小限の環境を整えていくことです。今回はmacOSのみをターゲットとしていますが、将来的にはLinux等にも対応していく予定ですので、拡張性を持たせた構造にしていきます。

1. プロジェクトディレクトリ構造の検討
2. makefileの作成
3. アプリケーションエントリーポイントの作成
4. ビルドスクリプトの作成
5. VSCodeワークスペース設定

の順で作成していきます。

## プロジェクトディレクトリ構造の検討

makefileを作るためには、ソースファイルの場所、ヘッダファイルの場所、生成されたバイナリを配置する場所を決める必要があります。
今回は、下記のような構造を取ることにします。

```console
.
├── bin
├── include
├── obj
├── src
└── test
    └── include
```

- **src,include**
  ソースファイルとヘッダファイルを同一フォルダに置く運用でも良いですが、Linux等のOSのディレクトリ構造とも整合性が取れるため、この構造を使用しています。一応、内部実装を見えなくするというメリットもあるのですが、今回のように私一人で開発し、開発したAPIをこのプロジェクト以外で使用しない場合には特に気にする必要はないかと思っています。
- **test/include**
  テスト時のみに使用する関数は、通常ビルド時には使用せず、余計なコードを入れないために別ディレクトリとしています。
- **obj,bin**
  コンパイル時に生成される各ソースファイルの.oファイルは、ソースディレクトリに生成されるようにすると、ビルドのたびにディレクトリが汚れるため、専用のディレクトリを設けました。最終生成物の実行ファイルについても同様です。

## makefileの作成

検討したプロジェクトディレクトリに従って、makefileを作成します。makefileはビルドプラットフォームごとに用意します。上位のビルドスクリプト内で実行プラットフォームを識別し、呼び出すmakefileを切り替える手法を取ります。なお、当面はmacOSを使用して開発していきますので、makefileはproject_root/makefile_macos.makとして作成していきます。

以下が作成したmakefileになりますが、私自身、makefileについてはあまり細かい文法を覚えているわけではなく、得意ではないためネットで拾ったものを寄せ集めて使いまわしています。このため、プロジェクト固有の重要な部分のみを説明していきます。

```make
TARGET = gl_choco_engine

SRC_DIR = src
BUILD_DIR = bin
OBJ_DIR = obj

SRC_FILES = $(shell find src -name '*.c')
DIRECTORIES = $(shell find $(SRC_DIR) -type d)
OBJ_FILES = $(SRC_FILES:%=$(OBJ_DIR)/%.o)

INCLUDE_FLAGS = -Iinclude
INCLUDE_FLAGS += -Itest/include
CC = /opt/homebrew/opt/llvm/bin/clang

COMPILER_FLAGS = -Wall -Wextra -std=c11
COMPILER_FLAGS += -Wconversion -Wsign-conversion
COMPILER_FLAGS += -Wformat=2 -Wformat-security
COMPILER_FLAGS += -Wstrict-prototypes -Wold-style-definition
COMPILER_FLAGS += -Wmissing-prototypes -Wmissing-declarations
COMPILER_FLAGS += -Wshadow
COMPILER_FLAGS += -Wcast-qual -Wcast-align
COMPILER_FLAGS += -Wpointer-arith
COMPILER_FLAGS += -Wundef
COMPILER_FLAGS += -Wswitch-enum
COMPILER_FLAGS += -Wvla
COMPILER_FLAGS += -pedantic-errors
COMPILER_FLAGS += -Weverything
COMPILER_FLAGS += -Wshorten-64-to-32
COMPILER_FLAGS += -Wdouble-promotion
COMPILER_FLAGS += -Wcomma
COMPILER_FLAGS += -Wfloat-equal
COMPILER_FLAGS += -Wno-declaration-after-statement

ifeq ($(BUILD_MODE), RELEASE_BUILD)
	COMPILER_FLAGS += -O3 -DRELEASE_BUILD -DPLATFORM_MACOS
else
	ifeq ($(BUILD_MODE), DEBUG_BUILD)
		COMPILER_FLAGS += -g -O0 -DDEBUG_BUILD -DPLATFORM_MACOS
	endif
	ifeq ($(BUILD_MODE), TEST_BUILD)
		COMPILER_FLAGS += -g -O0 -DTEST_BUILD -DPLATFORM_MACOS
		COMPILER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
		LINKER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
	endif
endif

.PHONY: all
all: scaffold link

.PHONY: scaffold
scaffold:
	@echo --- scaffolding folder structure... ---
	@echo Create directories into obj/
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(DIRECTORIES))
	@echo Create bin directory.
	@mkdir -p $(BUILD_DIR)
	@echo Done.
	@echo --- compiling source files... ---
	@echo build mode - $(BUILD_MODE)

$(OBJ_DIR)/%.c.o: %.c
	@echo compiling $<...
	$(CC) $< $(COMPILER_FLAGS) -c -o $@ $(INCLUDE_FLAGS)

.PHONY: link
link: scaffold $(OBJ_FILES)
	@echo --- linking $(TARGET)... ---
	@$(CC) $(OBJ_FILES) -o $(BUILD_DIR)/$(TARGET) $(LINKER_FLAGS)

.PHONY: clean
clean:
	@rm -f $(TARGET)
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJ_DIR)
	@rm -rf cov
```

### 実行バイナリファイル名、ディレクトリ、コンパイラーパス設定

```make
TARGET = gl_choco_engine

SRC_DIR = src
BUILD_DIR = bin
OBJ_DIR = obj

SRC_FILES = $(shell find src -name '*.c')
DIRECTORIES = $(shell find $(SRC_DIR) -type d)
OBJ_FILES = $(SRC_FILES:%=$(OBJ_DIR)/%.o)

INCLUDE_FLAGS = -Iinclude
INCLUDE_FLAGS += -Itest/include
CC = /opt/homebrew/opt/llvm/bin/clang
```

コンパイラーパスについては実行環境に応じて変更が必要です。

### ワーニング出力設定

ワーニングはかなり厳しく出力するよう設定されていますが、今後、使用しながら不要なワーニングを省いていく運用にします。なお、今回はコンパイラにclangを用いる前提です。gcc等を使用する場合には一部設定に変更が必要です。

```make
COMPILER_FLAGS = -Wall -Wextra -std=c11
COMPILER_FLAGS += -Wconversion -Wsign-conversion
COMPILER_FLAGS += -Wformat=2 -Wformat-security
COMPILER_FLAGS += -Wstrict-prototypes -Wold-style-definition
COMPILER_FLAGS += -Wmissing-prototypes -Wmissing-declarations
COMPILER_FLAGS += -Wshadow
COMPILER_FLAGS += -Wcast-qual -Wcast-align
COMPILER_FLAGS += -Wpointer-arith
COMPILER_FLAGS += -Wundef
COMPILER_FLAGS += -Wswitch-enum
COMPILER_FLAGS += -Wvla
COMPILER_FLAGS += -pedantic-errors
COMPILER_FLAGS += -Weverything
COMPILER_FLAGS += -Wshorten-64-to-32
COMPILER_FLAGS += -Wdouble-promotion
COMPILER_FLAGS += -Wcomma
COMPILER_FLAGS += -Wfloat-equal
COMPILER_FLAGS += -Wno-declaration-after-statement
```

### ビルドモードに応じた設定

このmakefileでは、makeコマンド実行時の引数でBUILD_MODEを指定して実行します。
BUILD_MODEの値に応じて、設定を行います。各ビルドモードごとの設定内容は、

**リリースビルド**

- 最適化はO3
- プログラム側で#ifdef RELEASE_BUILDでビルドモード識別可能
- プログラム側で#ifdef PLATFORM_MACOSでプラットフォーム識別可能

**デバッグビルド**

- 最適化はO0
- プログラム側で#ifdef DEBUG_BUILDでビルドモード識別可能
- プログラム側で#ifdef PLATFORM_MACOSでプラットフォーム識別可能

**テストビルド**

- 最適化はO0
- プログラム側で#ifdef DEBUG_BUILDでビルドモード識別可能
- プログラム側で#ifdef PLATFORM_MACOSでプラットフォーム識別可能
- カバレッジ計測のためのデータを出力するよう設定(clangコンパイラ使用前提)

```make
ifeq ($(BUILD_MODE), RELEASE_BUILD)
	COMPILER_FLAGS += -O3 -DRELEASE_BUILD -DPLATFORM_MACOS
else
	ifeq ($(BUILD_MODE), DEBUG_BUILD)
		COMPILER_FLAGS += -g -O0 -DDEBUG_BUILD -DPLATFORM_MACOS
	endif
	ifeq ($(BUILD_MODE), TEST_BUILD)
		COMPILER_FLAGS += -g -O0 -DTEST_BUILD -DPLATFORM_MACOS
		COMPILER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
		LINKER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
	endif
endif
```

## アプリケーションエントリーポイントの作成

makefileができたのでアプリケーションのエントリーポイントを作成し、ビルド確認を行います。

src/entry.c

```c
#include <stdio.h>

int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    fprintf(stdout, "Build mode: RELEASE.\n");
#endif
#ifdef DEBUG_BUILD
    fprintf(stdout, "Build mode: DEBUG.\n");
#endif
#ifdef TEST_BUILD
    fprintf(stdout, "Build mode: TEST.\n");
#endif

    return 0;
}
```

動作確認として以下を実行し、DEBUGビルドとして認識されていればOKです。

```bash
make -f makefile_macos.mak all BUILD_MODE=DEBUG_BUILD
./bin/gl_choco_engine
```

さらに、

```bash
make -f makefile_macos.mak clean
```

としてobj、binディレクトリが削除されていればOKです。

## ビルドスクリプトの作成

ビルドスクリプトに求められる要件は、

- プラットフォームの識別
- プラットフォームごとのmakefileの呼び出し
- makefile実行時にビルドモードとビルドアクション(all/clean)を指定

です。ビルドモードおよびビルドアクションの指定は、上位のVSCodeワークスペース設定によって行います。以下がビルドスクリプトになっており、project_root/build.shとして保存し、実行権限を与えておきます。当面はmacOSのみのサポートとなっていますが、今後、Linux等のOSにも対応していく予定です。

```bash
#!/bin/bash

ACTION=$1 # all / clean
BUILD_MODE=${2:-DEBUG_BUILD} # RELEASE_BUILD / DEBUG_BUILD / TEST_BUILD(Default: DEBUG_BUILD)

if [ "$(uname)" == 'Darwin' ]; then
    /usr/bin/make -f makefile_macos.mak $ACTION BUILD_MODE=$BUILD_MODE
else
    echo "Your platform ($(uname -a)) is not supported."
    exit 1
fi
```

実行権限を与えたbuild.shに対して、

```bash
./build.sh all RELEASE_BUILD
./bin/gl_choco_engine
```

として、RELEASE_BUILDとして認識されていればOKです。もし、正しく表示されなかった場合は、

```bash
./build.sh clean
```

をしてから再度実行してみてください。

## VSCodeワークスペース設定

以上でビルドができるようになったので、後はビルドモードに応じてVSCodeのワークスペースを設定するだけです。
.vscode/tasks.jsonを作成し、下記を記載すればコマンドパレットからビルドモードに応じたビルドを行うことができます。

```console
{
    "version": "2.0.0",
    "tasks": [
        {
            "type": "shell",
            "label": "build_debug",
            "command": "${workspaceFolder}/build.sh all DEBUG_BUILD",
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build"
        },
        {
            "type": "shell",
            "label": "build_release",
            "command": "${workspaceFolder}/build.sh all RELEASE_BUILD",
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build"
        },
        {
            "type": "shell",
            "label": "build_test",
            "command": "${workspaceFolder}/build.sh all TEST_BUILD",
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build"
        },
        {
            "type": "shell",
            "label": "clean",
            "command": "${workspaceFolder}/build.sh clean",
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build"
        }
    ]
}
```

最後に、VSCodeでF5キーを押すと実行ファイルを実行できるよう、.vscode/launch.jsonを作成します。

```console
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch gl_choco_engine",
            "type": "lldb",
            "request": "launch",
            "cwd": "${workspaceFolder}",
            "program": "${workspaceFolder}/bin/gl_choco_engine"
        }
    ]
}
```
