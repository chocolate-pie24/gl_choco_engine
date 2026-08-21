<!--
AI Generated
-->

# GLCE Build System

## 1. Purpose and Scope

This document describes the following aspects of the GLCE build system:

- Overall build system structure and separation of responsibilities
- Public interface centered on `build.sh`
- Debug / Release / Test build modes
- OS-specific configuration for macOS / Linux / FreeBSD
- Coverage / Sanitizer / Valgrind analysis workflows
- VS Code integration policy
- Build artifacts
- Operational constraints

Setup procedures for individual operating systems and the history of development environment setup are outside the scope of this document.

The official entry point for GLCE build and analysis workflows is `build.sh` at the repository root.

For normal use, do not invoke `make/*.mk` or `scripts/*.sh` directly. Use `build.sh` instead.

The supported operating systems are as follows.

| OS | Status |
|---|---|
| macOS | Supported |
| Linux | Supported |
| FreeBSD | Supported |
| Windows native | Not supported |

---

## 2. Overall Structure

### 2.1 Separation of Responsibilities

The build system uses the following structure.

```text
build.sh
    Public user-facing interface
    Command parsing
    Build mode selection
    OS detection
    Makefile / workflow selection

make/common.mk
    OS-independent compile / link processing
    Source discovery
    Dependency generation
    Compiler options by build mode
    clean

make/macos.mk
make/linux.mk
make/freebsd.mk
    OS-specific configuration
    compiler
    platform define
    include path
    library path
    link library

scripts/coverage.sh
scripts/sanitizer.sh
scripts/valgrind.sh
    Specialized analysis workflows

.vscode/
    Configuration for using build.sh from VS Code
```

The basic processing flow is as follows.

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

### 2.2 Responsibilities of `build.sh`

`build.sh` acts as the front controller.

Its main responsibilities are as follows.

- Parse public commands
- Convert `debug` / `release` / `test` into internal build modes
- Detect the OS using `uname -s`
- Select the Make command to use
- Select the OS-specific Makefile
- Launch specialized workflows when necessary
- Pass execution context to workflows

Compilation rules for individual `.c` files and details of OS-specific libraries are not placed in `build.sh`.

### 2.3 POSIX `sh`

`build.sh` and `scripts/*.sh` are written in POSIX `sh`.

```sh
#!/bin/sh
```

Bash-specific syntax is avoided so that the same shell scripts can be used on macOS / Linux / FreeBSD.

---

## 3. Public Interface and Build Modes

### 3.1 Public Commands

The commands available to users are as follows.

| Command | Description |
|---|---|
| `./build.sh build debug` | Debug build |
| `./build.sh build release` | Release build |
| `./build.sh build test` | Test build |
| `./build.sh clean` | Remove generated artifacts |
| `./build.sh coverage` | LLVM Coverage |
| `./build.sh sanitize` | Run Sanitizers |
| `./build.sh valgrind` | Valgrind Memcheck |

### 3.2 Build Modes

GLCE has three build modes.

| Public name | Internal value | Main compiler options | Preprocessor definition |
|---|---|---|---|
| `debug` | `DEBUG_BUILD` | `-g -O0` | `DEBUG_BUILD` |
| `release` | `RELEASE_BUILD` | `-O3` | `RELEASE_BUILD` |
| `test` | `TEST_BUILD` | `-g -O0` | `TEST_BUILD` |

#### Debug

Used for development and debugging.

```text
-g
-O0
-DDEBUG_BUILD
```

#### Release

Produces an optimized executable.

```text
-O3
-DRELEASE_BUILD
```

#### Test

Builds with test code included.

```text
-g
-O0
-DTEST_BUILD
```

Under `TEST_BUILD`, `test/src/` is added to the build targets in addition to the normal sources, and `test/include/` is added to the include path.

Coverage and Sanitizer workflows also use `TEST_BUILD`.

---

## 4. Make Structure

### 4.1 File Structure

```text
make/
├── common.mk
├── macos.mk
├── linux.mk
└── freebsd.mk
```

Each OS-specific Makefile first includes `make/common.mk` and then adds OS-specific settings to the common configuration.

### 4.2 `make/common.mk`

`make/common.mk` defines OS-independent build processing.

The main settings are as follows.

```make
TARGET := gl_choco_engine
BUILD_DIR := bin
OBJ_DIR := obj

SRC_ROOT_DIRS := engine application
TEST_SRC_DIR := test/src
```

The source roots for normal builds are as follows.

```text
engine/
application/
```

Under `TEST_BUILD`, the following is added.

```text
test/src/
```

The common include paths are as follows.

```text
-I.
-Iinclude
```

Under `TEST_BUILD`, the following is added.

```text
-Itest/include
```

### 4.3 Common Compiler Options

The common compiler options include C11 selection, warnings, and dependency generation.

The main categories are as follows.

- C11
- General warnings
- Additional warnings for conversion / format / prototype / shadow, etc.
- Clang-specific warnings
- Warnings disabled by project policy
- `.d` dependency file generation

The following options are used for dependency generation.

```text
-MMD
-MP
```

The generated `.d` files are included from `common.mk` and are used to determine whether recompilation is required when headers change.

### 4.4 Source-to-Object Mapping

Objects are generated under `obj/` while preserving the structure of the source tree.

Example:

```text
engine/core/example.c
    ↓
obj/engine/core/example.o
obj/engine/core/example.d
```

### 4.5 OS-Specific Configuration

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

The GLEW / GLFW / LLVM prefixes are obtained from Homebrew.

The primary OpenGL-related libraries and frameworks linked are as follows.

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

The following is defined for POSIX APIs.

```text
_POSIX_C_SOURCE=200809L
```

The main link libraries are as follows.

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

Because the standard `make` on FreeBSD is BSD make, GLCE uses GNU Make as `gmake`.

To match the standard locations of third-party libraries installed through `pkg`, the following paths are added.

```text
-I/usr/local/include
-L/usr/local/lib
```

As on Linux, the main link libraries are as follows.

```text
-lm
-lGL
-lglfw
-lGLEW
```

---

## 5. Analysis Workflows

Analysis workflows are separated under `scripts/`.

```text
scripts/
├── coverage.sh
├── sanitizer.sh
└── valgrind.sh
```

These workflows are launched from `build.sh`.

`build.sh` passes the following execution context to workflows as environment variables.

| Variable | Description |
|---|---|
| `GLCE_DIR` | Absolute path to the GLCE repository |
| `OS_NAME` | OS name |
| `MAKE_COMMAND` | `make` or `gmake` |
| `MAKEFILE` | Selected OS-specific Makefile |

If the required context is not available, the workflow exits with an error.

### 5.1 Coverage

`scripts/coverage.sh` runs LLVM Source-based Code Coverage.

Processing flow:

```text
clean
  ↓
TEST_BUILD with Coverage flags
  ↓
run bin/gl_choco_engine
  ↓
llvm-profdata
  ↓
llvm-cov
  ↓
cov/index.html
```

The method used to locate the LLVM tools differs by OS.

| OS | `llvm-profdata` / `llvm-cov` |
|---|---|
| macOS | From Homebrew LLVM |
| Linux | From PATH |
| FreeBSD | From PATH |

### 5.2 Sanitizer

`scripts/sanitizer.sh` enables AddressSanitizer and UndefinedBehaviorSanitizer.

The main settings are as follows.

```text
-fsanitize=address,undefined
-fno-sanitize-recover=all
-fno-omit-frame-pointer
-fsanitize-address-use-after-scope
-O1
```

Processing flow:

```text
clean
  ↓
TEST_BUILD with Sanitizer flags
  ↓
set ASAN_OPTIONS / UBSAN_OPTIONS
  ↓
run bin/gl_choco_engine
```

LeakSanitizer is handled as follows.

| OS | Leak detection |
|---|---|
| macOS | Enabled |
| Linux | Enabled |
| FreeBSD | Disabled |

### 5.3 Valgrind

`scripts/valgrind.sh` runs Valgrind Memcheck.

Supported operating systems:

```text
Linux
FreeBSD
```

Processing flow:

```text
check for valgrind
  ↓
clean
  ↓
DEBUG_BUILD
  ↓
Valgrind Memcheck
  ↓
bin/gl_choco_engine
```

The Valgrind workflow is not used on macOS.

---

## 6. VS Code Integration

### 6.1 Basic Policy

VS Code-specific settings are placed under `.vscode/`.

```text
.vscode/
├── tasks.json
├── launch.json
├── c_cpp_properties.json
└── settings.json
```

The build logic itself is not implemented in VS Code.

VS Code tasks only invoke `build.sh`; OS detection and Makefile selection are delegated to `build.sh`.

```text
VS Code task
    ↓
build.sh
    ↓
OS detection
    ↓
make / gmake
```

This structure avoids duplicating OS-specific logic in `tasks.json`.

### 6.2 Workspace Structure

The current `.vscode` configuration assumes the following structure.

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

The following tasks are defined.

```text
build_debug
build_release
build_test
clean
sanitize
coverage
valgrind
```

Each task corresponds to a public `build.sh` command.

Tasks that produce compiler diagnostics use a problem matcher and resolve source paths relative to the GLCE root.

### 6.4 IntelliSense

`c_cpp_properties.json` contains IntelliSense configurations for macOS and Linux.

The common project include paths are as follows.

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

There is no FreeBSD-specific `c_cpp_properties.json` configuration.

On FreeBSD, VS Code build / analysis tasks are used, while FreeBSD-specific IntelliSense integration is outside the official scope of the build system.

### 6.5 Debugging

IDE-integrated debugging through `launch.json` is used in environments where the installed VS Code version and debug adapter support it.

The build system itself does not depend on any specific debug adapter.

`preLaunchTask` is not configured; before debugging, the user explicitly selects and builds the required build mode.

VS Code integrated debugging is not assumed on FreeBSD.

When GUI debugging is required, a GDB-compatible front end such as Qt Creator can be used with the following Debug-built binary.

```text
bin/gl_choco_engine
```

GLCE is not migrated to an IDE-specific build system; builds continue to use `build.sh`.

---

## 7. Build Artifacts and Dependencies

### 7.1 Build Artifacts

Final executable:

```text
bin/gl_choco_engine
```

Object / dependency files:

```text
obj/**/*.o
obj/**/*.d
```

Coverage report:

```text
cov/index.html
```

Debug / Release / Test share the same `bin/` and `obj/` directories.

### 7.2 Common Dependencies

The main dependencies are as follows.

- POSIX `sh`
- Clang
- Make environment
- OpenGL
- GLFW
- GLEW

### 7.3 macOS-Specific Dependencies

- Homebrew
- Homebrew LLVM
- Homebrew GLFW
- Homebrew GLEW

### 7.4 Linux-Specific Dependencies

Depending on the environment, the following are required.

- Clang
- GNU Make
- OpenGL development package
- GLFW development package
- GLEW development package
- LLVM Coverage tools
- Valgrind

Package names differ by distribution.

### 7.5 FreeBSD-Specific Dependencies

The main packages are as follows.

```text
gmake
glew
glfw
```

When analysis features are used, the following tools are used as necessary.

```text
valgrind
llvm-profdata
llvm-cov
```

On FreeBSD, headers and libraries from third-party packages are primarily installed under `/usr/local`.

---

## 8. Operational Constraints

### 8.1 Clean When Changing Build Modes

Debug / Release / Test share the same `obj/` directory.

Because Make does not track changes to compiler flags themselves as dependencies, run a clean before switching build modes.

```sh
./build.sh clean
./build.sh build release
```

For normal incremental builds within the same build mode, it is not necessary to clean every time.

### 8.2 Do Not Keep Artifacts from Multiple Modes Simultaneously

Mode-specific output directories such as the following are not currently used.

```text
bin/debug/
bin/release/
obj/debug/
obj/release/
```

Only the artifacts from the most recently built mode are retained in `bin/` and `obj/`.

### 8.3 Current Directory at Runtime

Some GLCE processes reference assets and other resources using relative paths.

Therefore, as a general rule, the GLCE root should be the current directory at runtime.

```sh
cd <path-to-glce>
./bin/gl_choco_engine
```

The design of asset paths themselves is treated as a separate responsibility from the build system.

### 8.4 Do Not Run Specialized Workflows Directly

`scripts/*.sh` assumes the execution context passed from `build.sh`.

For normal use, use the following flow.

```text
build.sh
    ↓
scripts/*.sh
```

`scripts/*.sh` is not treated as a public user-facing interface.
