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


