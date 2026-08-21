include make/common.mk

CC := clang

# FreeBSD platform settings
COMPILER_FLAGS += -DPLATFORM_FREEBSD

# FreeBSDではpkgで導入したheader/libraryが/usr/local以下に配置される
INCLUDE_FLAGS += -I/usr/local/include
LINKER_FLAGS += -L/usr/local/lib

# FreeBSD libraries
LINKER_FLAGS += -lm
LINKER_FLAGS += -lGL
LINKER_FLAGS += -lglfw
LINKER_FLAGS += -lGLEW
