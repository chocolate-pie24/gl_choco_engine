---
title: "Step2_2: OpenGL環境整備, Linuxサポート"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook2に対応しています。

## このステップでやること

前回で、メモリシステム、リニアアロケータの仕様変更が完了しました。今回は、Book2のゴールであるGLFWを使用したウィンドウ生成の準備として、
既存のmakefileの修正をしていきます。また、ついでにLinux環境でのビルド用makefileを用意することで、Linux環境のサポートを追加することにします。

なお、今回の内容は、makefileとbuild.shの変更のみとなっており、ソースコードの変更はありません。
また、OpenGLやGLFWの環境構築方法は[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)のREADME.mdに記載してあります。
なので、読み飛ばしていただいても今後の進行に差し支えはありません。

## macOS用makefileの修正

修正版makefileの全体を貼り付けます。

```makefile
TARGET = gl_choco_engine

SRC_DIR = src
BUILD_DIR = bin
OBJ_DIR = obj

SRC_FILES = $(shell find src -name '*.c')
DIRECTORIES = $(shell find $(SRC_DIR) -type d)
OBJ_FILES = $(SRC_FILES:%=$(OBJ_DIR)/%.o)

GLEW_PREFIX := $(shell brew --prefix glew)
GLFW_PREFIX := $(shell brew --prefix glfw)
INCLUDE_FLAGS = -Iinclude
INCLUDE_FLAGS += -I$(GLEW_PREFIX)/include
INCLUDE_FLAGS += -I$(GLFW_PREFIX)/include
ifeq ($(BUILD_MODE), TEST_BUILD)
  INCLUDE_FLAGS += -Itest/include
endif

LLVM_PREFIX := $(shell brew --prefix llvm)
CC = $(LLVM_PREFIX)/bin/clang

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
COMPILER_FLAGS += -MMD -MP

# 厳しすぎるワーニング除去
COMPILER_FLAGS += -Wno-unsafe-buffer-usage
COMPILER_FLAGS += -Wno-padded
COMPILER_FLAGS += -Wno-switch-default
COMPILER_FLAGS += -Wno-pre-c11-compat

# glfw3のワーニング抑制
COMPILER_FLAGS += -Wno-documentation-unknown-command
COMPILER_FLAGS += -Wno-documentation
COMPILER_FLAGS += -Wno-reserved-identifier

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
LINKER_FLAGS += -L$(GLEW_PREFIX)/lib
LINKER_FLAGS += -L$(GLFW_PREFIX)/lib
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lglew
LINKER_FLAGS += -framework OpenGL
LINKER_FLAGS += -framework IOKit
LINKER_FLAGS += -framework Cocoa

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

# 依存ファイルの取り込み(存在するときのみ)
-include $(OBJ_FILES:.o=.d)
```

OpenGLとGLFWを使用するためには、ヘッダファイルのincludeディレクトリとライブラリのリンクのためのディレクトリを指定するのみです。
macOSの場合はこのように設定することで使用可能になります。なお、GLFWのインストール方法については[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)の
README.mdに記載してありますので、そちらを参照してください。

includeディレクトリの追加

```makefile
GLEW_PREFIX := $(shell brew --prefix glew)
GLFW_PREFIX := $(shell brew --prefix glfw)
INCLUDE_FLAGS += -I$(GLEW_PREFIX)/include
INCLUDE_FLAGS += -I$(GLFW_PREFIX)/include
```

ライブラリリンクディレクトリの追加

```makefile
LINKER_FLAGS += -L$(GLEW_PREFIX)/lib
LINKER_FLAGS += -L$(GLFW_PREFIX)/lib
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lglew
LINKER_FLAGS += -framework OpenGL
LINKER_FLAGS += -framework IOKit
LINKER_FLAGS += -framework Cocoa
```

## Linux用makefileの追加とビルドシステムの変更

### Linux用makefileの追加

Linux用のmakefileですが、基本的な構造はmacOSと同じで良いのですが、各種ディレクトリが異なります。
また、src/engine/choco_message.cで使用しているflockfileとfunlockfileがPOSIX関数で標準CのAPIではなく、ビルドができないため追加の設定を行います。

Linux環境でのGLFWのインストール方法についても[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)の
README.mdに記載してありますので、そちらを参照してください。

まずmakefile全体を貼り付けます。

```makefile
TARGET = gl_choco_engine

SRC_DIR = src
BUILD_DIR = bin
OBJ_DIR = obj

SRC_FILES = $(shell find src -name '*.c')
DIRECTORIES = $(shell find $(SRC_DIR) -type d)
OBJ_FILES = $(SRC_FILES:%=$(OBJ_DIR)/%.o)

INCLUDE_FLAGS = -I/usr/include/
INCLUDE_FLAGS += -Iinclude
ifeq ($(BUILD_MODE), TEST_BUILD)
  INCLUDE_FLAGS += -Itest/include
endif

CC = /usr/bin/clang

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
COMPILER_FLAGS += -MMD -MP

# 厳しすぎるワーニング除去
COMPILER_FLAGS += -Wno-unsafe-buffer-usage
COMPILER_FLAGS += -Wno-padded
COMPILER_FLAGS += -Wno-switch-default
# COMPILER_FLAGS += -Wno-pre-c11-compat

# glfw3のワーニング抑制
COMPILER_FLAGS += -Wno-documentation-unknown-command
COMPILER_FLAGS += -Wno-documentation
COMPILER_FLAGS += -Wno-reserved-identifier
# flockfile / funlockfileを使用するために必要(非標準CのPOSIX関数)
COMPILER_FLAGS += -D_POSIX_C_SOURCE=200809L

ifeq ($(BUILD_MODE), RELEASE_BUILD)
	COMPILER_FLAGS += -O3 -DRELEASE_BUILD -DPLATFORM_LINUX
else
	ifeq ($(BUILD_MODE), DEBUG_BUILD)
		COMPILER_FLAGS += -g -O0 -DDEBUG_BUILD -DPLATFORM_LINUX
	endif
	ifeq ($(BUILD_MODE), TEST_BUILD)
		COMPILER_FLAGS += -g -O0 -DTEST_BUILD -DPLATFORM_LINUX
		COMPILER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
		LINKER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
	endif
endif
LINKER_FLAGS = -L/usr/lib/x86_64-linux-gnu/
LINKER_FLAGS += -lm
LINKER_FLAGS += -lGL
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lGLEW

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

# 依存ファイルの取り込み(存在するときのみ)
-include $(OBJ_FILES:.o=.d)
```

基本的な構造はmacOSと同じなので、flockfileとfunlockfileを使用するための設定のみ説明すると、

```makefile
# flockfile / funlockfileを使用するために必要(非標準CのPOSIX関数)
COMPILER_FLAGS += -D_POSIX_C_SOURCE=200809L
```

このように、コンパイラフラグにマクロ定義設定を追加することで使用可能になります。

### ビルドスクリプトの修正

次にビルドスクリプトの修正です。このように修正します。

```bash
#!/bin/bash

ACTION=$1 # all or clean
BUILD_MODE=${2:-DEBUG_BUILD} # RELEASE_BUILD or DEBUG_BUILD(Default: DEBUG_BUILD)

if [ "$(uname)" == 'Darwin' ]; then
  /usr/bin/make -f makefile_macos.mak $ACTION BUILD_MODE=$BUILD_MODE
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
  /usr/bin/make -f makefile_linux.mak $ACTION BUILD_MODE=$BUILD_MODE
else
  echo "Your platform ($(uname -a)) is not supported."
  exit 1
fi
```

以上でmakefileの修正と、Linux用のビルドシステムが完成しました。
次回からは新しい機能の実装に取り掛かることにします。まずはシンプルな文字列コンテナの作成から行っていくことにします。
