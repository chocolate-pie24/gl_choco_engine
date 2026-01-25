TARGET = gl_choco_engine

SRC_DIR = src
BUILD_DIR = bin
OBJ_DIR = obj

SAN ?= 0

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
# サニタイザ有効時はログをきれいにするためカバレッジを無効化
		ifneq ($(SAN), 1)
			COMPILER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
			LINKER_FLAGS += -fprofile-instr-generate -fcoverage-mapping
		endif
	endif
endif

ifeq ($(SAN), 1)
  # まずは ASan + UBSan が最優先（Leak は ASan に内包されることが多い）
  SAN_CFLAGS  += -fsanitize=address,undefined -fno-sanitize-recover=all
  SAN_CFLAGS  += -fno-omit-frame-pointer
  # 追加で有効にすると刺さりやすいことがある（Clang向け）
  SAN_CFLAGS  += -fsanitize-address-use-after-scope

  SAN_LDFLAGS += -fsanitize=address,undefined

  # DEBUG/TEST でも -O0 固定だと再現性が落ちることがあるので、SAN時だけ -O1 推奨
  # （あなたの設計上、DEBUG_BUILD/TEST_BUILD で -O0 を入れているため上書きの意図）
  SAN_CFLAGS  += -O1
endif

COMPILER_FLAGS += $(SAN_CFLAGS)
LINKER_FLAGS += $(SAN_LDFLAGS)

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
