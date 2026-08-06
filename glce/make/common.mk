TARGET := gl_choco_engine
BUILD_DIR := bin
OBJ_DIR := obj

SRC_ROOT_DIRS := engine application
TEST_SRC_DIR := test/src

# workflow scriptsから与えられる
SAN_CFLAGS ?=
SAN_LDFLAGS ?=
COV_FLAGS ?=

# ビルドモード指定なしでmakeを実行した場合、cleanビルド以外はエラーにする
# MAKECMDGOALS: コマンドラインで指定されたターゲットを表すMakeの組み込み変数
VALID_BUILD_MODES := RELEASE_BUILD DEBUG_BUILD TEST_BUILD
ifneq ($(MAKECMDGOALS),clean)
    ifeq ($(filter $(BUILD_MODE),$(VALID_BUILD_MODES)),)
        $(error BUILD_MODE must be one of: $(VALID_BUILD_MODES))
    endif
endif

SRC_FILES := $(shell find $(SRC_ROOT_DIRS) -name '*.c')
SRC_DIRS := $(shell find $(SRC_ROOT_DIRS) -type d)
ifeq ($(BUILD_MODE),TEST_BUILD)
    SRC_FILES += $(shell find $(TEST_SRC_DIR) -name '*.c')
    SRC_DIRS += $(shell find $(TEST_SRC_DIR) -type d)
endif

OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

# includeパス
INCLUDE_FLAGS := -I.
INCLUDE_FLAGS += -Iinclude
ifeq ($(BUILD_MODE),TEST_BUILD)
    INCLUDE_FLAGS += -Itest/include
endif

# 共通コンパイル設定
COMPILER_FLAGS := -Wall -Wextra -std=c11
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
COMPILER_FLAGS += -Wcomma
COMPILER_FLAGS += -Wfloat-equal
COMPILER_FLAGS += -Wno-declaration-after-statement
COMPILER_FLAGS += -MMD -MP

# 厳しすぎるワーニングの抑制
COMPILER_FLAGS += -Wno-unsafe-buffer-usage
COMPILER_FLAGS += -Wno-padded
COMPILER_FLAGS += -Wno-switch-default
COMPILER_FLAGS += -Wno-pre-c11-compat
COMPILER_FLAGS += -Wno-covered-switch-default
COMPILER_FLAGS += -Wno-double-promotion

# 外部ライブラリのヘッダに対するワーニングの抑制
COMPILER_FLAGS += -Wno-documentation-unknown-command
COMPILER_FLAGS += -Wno-documentation
COMPILER_FLAGS += -Wno-reserved-identifier

# ビルドフラグ設定
ifeq ($(BUILD_MODE),RELEASE_BUILD)
    COMPILER_FLAGS += -O3 -DRELEASE_BUILD
else
    ifeq ($(BUILD_MODE),DEBUG_BUILD)
        COMPILER_FLAGS += -g -O0 -DDEBUG_BUILD
    endif

    ifeq ($(BUILD_MODE),TEST_BUILD)
        COMPILER_FLAGS += -g -O0 -DTEST_BUILD
        COMPILER_FLAGS += $(COV_FLAGS)
        LINKER_FLAGS += $(COV_FLAGS)
    endif
endif

# workflow scriptsから与えられたフラグ設定追加
COMPILER_FLAGS += $(SAN_CFLAGS)
LINKER_FLAGS += $(SAN_LDFLAGS)

.PHONY: all
all: link

.PHONY: scaffold
scaffold:
	@echo --- scaffolding folder structure... ---
	@echo Create directories into obj/
	@mkdir -p $(addprefix $(OBJ_DIR)/,$(SRC_DIRS))
	@echo Create bin directory.
	@mkdir -p $(BUILD_DIR)
	@echo Done.
	@echo --- compiling source files... ---
	@echo build mode - $(BUILD_MODE)

.PHONY: link
link: scaffold $(OBJ_FILES)
	@echo --- linking $(TARGET)... ---
	$(CC) $(OBJ_FILES) -o $(BUILD_DIR)/$(TARGET) $(LINKER_FLAGS)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJ_DIR)
	@rm -rf cov

$(OBJ_DIR)/%.o: %.c
	@echo compiling $<...
	$(CC) $< $(COMPILER_FLAGS) -c -o $@ $(INCLUDE_FLAGS)

# 依存ファイルの取り込み（存在するときのみ）
-include $(DEP_FILES)
