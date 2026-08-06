include make/common.mk

GLEW_PREFIX := $(shell brew --prefix glew)
GLFW_PREFIX := $(shell brew --prefix glfw)

# common.mkですでにINCLUDE_FLAGSには値が入っているので、+=にする
INCLUDE_FLAGS += -I$(GLEW_PREFIX)/include
INCLUDE_FLAGS += -I$(GLFW_PREFIX)/include

LLVM_PREFIX := $(shell brew --prefix llvm)
CC := $(LLVM_PREFIX)/bin/clang

# プラットフォーム設定追加
COMPILER_FLAGS += -DPLATFORM_MACOS

LINKER_FLAGS += -L$(GLEW_PREFIX)/lib
LINKER_FLAGS += -L$(GLFW_PREFIX)/lib
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lglew
LINKER_FLAGS += -framework OpenGL
LINKER_FLAGS += -framework IOKit
LINKER_FLAGS += -framework Cocoa
