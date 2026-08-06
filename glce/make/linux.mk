include make/common.mk

CC := clang

# Linux platform settings
COMPILER_FLAGS += -DPLATFORM_LINUX

# flockfile / funlockfileを使用するために必要なPOSIX機能定義
COMPILER_FLAGS += -D_POSIX_C_SOURCE=200809L

# Linux libraries
LINKER_FLAGS += -lm
LINKER_FLAGS += -lGL
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lGLEW
