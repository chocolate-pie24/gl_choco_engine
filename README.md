<p align="center">
  <img src="assets/logo/choco_engine_banner_768x256.png" alt="GL CHOCO ENGINE" width="640">
</p>

- [GL CHOCO ENGINE](#gl-choco-engine)
  - [Non-goals](#non-goals)
  - [Who It Is For](#who-it-is-for)
  - [Documentation](#documentation)
  - [Contributing](#contributing)
  - [Directory Layout](#directory-layout)
  - [Setup](#setup)
    - [macOS](#macos)
    - [Linux](#linux)
    - [FreeBSD](#freebsd)
  - [Build](#build)
  - [Run](#run)
  - [License](#license)
  - [Author](#author)

# GL CHOCO ENGINE

GL CHOCO ENGINE (GLCE) is a lightweight C/OpenGL rendering and application framework primarily aimed at visualization and debugging tools for robotics, industrial systems, and embedded-adjacent environments.

It provides the core functionality needed to build graphics applications while leaving out heavyweight rendering features, large-scale physics simulation, integrated editors, and other large subsystems.

By keeping the feature set focused, GLCE aims to remain small enough for developers to understand the structure and behavior of the entire engine and maintain it over the long term.

GLCE also keeps external library dependencies to a minimum so that setup and maintenance remain straightforward.

GLCE is implemented in C.

C++ provides many useful abstractions and design approaches, but maintaining a consistent codebase over a long period typically requires strict and continuously enforced conventions.

For a small, long-lived project that prioritizes readability and predictable low-level behavior, C's relatively limited language surface helps keep the codebase consistent and easier to review.

## Non-goals

GLCE does not aim to provide the following:

- Heavyweight rendering features such as advanced post-processing or high-end lighting
- Large-scale physics simulation
- A full-featured GUI editor
- A large asset pipeline
- All-in-one frameworks with large dependency surfaces
- Unicode / multibyte text support (ASCII only)

Leaving these areas out allows GLCE to provide the basic functionality needed for graphics applications while keeping the overall engine small enough to understand as a whole.

## Who It Is For

- Engineers looking for a small and readable C/OpenGL rendering framework
- Developers who need lightweight visualization for robotics, industrial systems, or embedded-adjacent environments
- Developers who want sufficient rendering functionality without introducing Unity, Unreal Engine, ROS2, or similarly large systems
- Developers who prefer to understand and control the full system rather than hide it behind middleware

## Documentation

- [Build System](docs/build_system.md)
- [API Reference](https://chocolate-pie24.github.io/gl_choco_engine/)

The Build System document describes the internal build architecture, including `build.sh`, Make configuration, build modes, OS-specific settings, analysis workflows, and VS Code integration.

The API Reference is generated from Japanese Doxygen comments.

## Contributing

GL CHOCO ENGINE is currently developed and maintained by a single developer.

Pull requests are not accepted at this time.

If you find a bug, have a question, or would like to suggest an improvement, please open an Issue.
Feedback from users is welcome.

Forks are welcome for your own experiments and development.

## Directory Layout

GLCE separates the Engine itself from the Application code that uses it.

The Application uses the public API exposed by the Engine.
The Engine does not depend on the Application.

```text
Application
    │
    │ Public API
    ▼
include/engine/
    │
    ▼
Engine
```

The main repository layout is:

```text
.
├── application/
├── assets/
├── docs/
├── engine/
├── include/
│   └── engine/
├── make/
│   ├── common.mk
│   ├── linux.mk
│   ├── macos.mk
│   └── freebsd.mk
├── scripts/
│   ├── coverage.sh
│   ├── sanitizer.sh
│   └── valgrind.sh
├── test/
│   ├── include/
│   └── src/
└── build.sh
```

The responsibilities of the main directories are:

| Directory | Responsibility |
|---|---|
| `application/` | Application-side source code that uses the GLCE Engine |
| `include/engine/` | Public API exposed by the Engine |
| `engine/` | Engine source code and internal headers |
| `assets/` | Runtime resources such as shaders and textures |
| `test/` | Test source files and headers |
| `make/` | Common and OS-specific Make configuration |
| `scripts/` | Development workflows such as Coverage, Sanitizer, and Valgrind |
| `docs/` | Design and development documentation |

Directories such as `bin/`, `obj/`, and `cov/` are generated as needed during builds or analysis workflows.

## Setup

GLCE currently supports:

- macOS
- Linux
- FreeBSD

The supported entry point for building GLCE is `build.sh`.

OS-specific compiler, include path, library path, and linker settings are selected automatically by `build.sh` and the files under `make/`.

### macOS

Install LLVM and the OpenGL-related dependencies with Homebrew:

```bash
brew install llvm
brew install glfw
brew install glew
```

### Linux

Install Clang and the OpenGL development packages.

Example for Ubuntu:

```bash
sudo apt install clang lldb lld
sudo apt install libglew-dev
sudo apt install libglfw3-dev
```

Package names may differ between Linux distributions.

### FreeBSD

Install GNU Make and the OpenGL-related dependencies as root:

```bash
pkg install gmake glew glfw
```

GLCE uses the Clang compiler provided by the FreeBSD base system.

Because FreeBSD's standard `make` is BSD make, GLCE uses GNU Make as `gmake`.
When `build.sh` detects FreeBSD, it selects `gmake` automatically.

## Build

If necessary, make the shell scripts executable:

```bash
chmod +x build.sh
chmod +x scripts/*.sh
```

The same `build.sh` interface is used on macOS, Linux, and FreeBSD.

Debug build:

```bash
./build.sh build debug
```

Release build:

```bash
./build.sh build release
```

Test build:

```bash
./build.sh build test
```

Remove generated files:

```bash
./build.sh clean
```

Debug, Release, and Test builds currently share the same `bin/` and `obj/` directories.

Run `clean` before switching build modes.

For details, see [Build System](docs/build_system.md).

## Run

After building GLCE, run it from the repository root:

```bash
./bin/gl_choco_engine
```

The executable is generated at:

```text
bin/gl_choco_engine
```

Some runtime resources are currently resolved using relative paths, so the GLCE repository root should be used as the current working directory.

## License

This project is released under the MIT License.

See [LICENSE](LICENSE) for details.

## Author

GitHub: https://github.com/chocolate-pie24
