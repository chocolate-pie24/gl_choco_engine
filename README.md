<p align="center">
  <img src="assets/logo/choco_engine_banner_768x256.png" alt="GL CHOCO ENGINE" width="640">
</p>

- [GL CHOCO ENGINE](#gl-choco-engine)
  - [Motivation / Positioning](#motivation--positioning)
    - [Non-goals](#non-goals)
    - [Who it’s for](#who-its-for)
  - [Inspired by](#inspired-by)
  - [Documentation](#documentation)
    - [Architecture / Layering](#architecture--layering)
      - [Renderer System](#renderer-system)
      - [Renderer Backend](#renderer-backend)
      - [Platform System](#platform-system)
    - [Guide](#guide)
    - [API Reference](#api-reference)
  - [Contributing](#contributing)
  - [Directory layout](#directory-layout)
  - [Setup](#setup)
    - [macOS](#macos)
    - [Linux](#linux)
  - [Build](#build)
  - [Run](#run)
  - [License](#license)
  - [Author](#author)

# GL CHOCO ENGINE

GL CHOCO ENGINE (GLCE) is a C/OpenGL engine focused on clarity, controllability,
and long-term maintainability.

It uses GLFW to keep the platform subsystem portable across desktop environments, while keeping dependencies intentionally small.

The codebase is written with a quality-first mindset—explicit architecture, predictable low-level behavior,
and a preference for testable, well-documented modules—so engineers outside the graphics domain can still understand, modify, and extend it with confidence.

GLCE is written in C by design. While C++ offers many valid styles and abstractions, maintaining a consistent, uniform codebase over a long period typically requires strict and continuously enforced conventions. For a small, long-lived project that prioritizes readability and predictable low-level behavior, C’s narrower surface area helps keep the code coherent and reviewable.

The goal is a practical baseline you can own end-to-end, rather than a full-featured alternative to Unity or Unreal.

## Motivation / Positioning

GL CHOCO ENGINE is designed as a lightweight, dependency-minimal C/OpenGL engine that you can understand and own end-to-end.

In addition to “game engine” use cases, the project targets practical visualization and tooling scenarios often seen in robotics, industrial systems, and embedded-adjacent environments—where you may not want (or cannot afford) large middleware stacks or heavyweight engines.

Typical use cases include:

- Robotics / industrial visualization (lightweight 2D/3D viewers)
- Sensor overlays (camera HUD / annotation rendering)
- Point cloud viewers (e.g., LiDAR data inspection)
- Lightweight tooling and debug viewers for embedded-adjacent systems

Lower-power devices such as Raspberry Pi-class boards are a longer-term target, so the feature set is intentionally scoped.

### Non-goals

- Heavy visual effects and advanced rendering (e.g., complex post-processing, high-end lighting)
- Large-scale physics simulation
- A full editor ecosystem or massive asset pipeline
- “All-in-one” frameworks with large dependency surfaces
- Unicode / multibyte text support (ASCII-only)

### Who it’s for

- Engineers who want a small, readable rendering/application baseline in C/OpenGL
- Teams who need “just enough” 2D / lightweight 3D visualization without Unity/Unreal/ROS2
- Developers who want to learn and control the full stack rather than rely on middleware

## Inspired by

This project was originally inspired by Kohi Game Engine and Travis Vroman’s work.
I’m grateful for the motivation and the educational value of seeing an engine built from the ground up.
GL CHOCO ENGINE is an independent codebase with its own architecture and design decisions, evolving according to its goals and constraints.

## Documentation

### Architecture / Layering

- [Overview](docs/layer.md)

#### Renderer System

- [Renderer System(English)](docs/architecture/renderer_system/renderer_system_en.md)
- [Renderer System(Japanese)](docs/architecture/renderer_system/renderer_system_ja.md)

#### Renderer Backend

- [Renderer Backend(English)](docs/architecture/renderer_system/renderer_backend/architecture_en.md)
- [Renderer Backend(Japanese)](docs/architecture/renderer_system/renderer_backend/architecture_ja.md)

#### Platform System

- [Platform System(English)](docs/architecture/platform_system/architecture_en.md)
- [Platform System(Japanese)](docs/architecture/platform_system/architecture_ja.md)

### Guide

To help engine developers operate the event system safely.

- [Event System Guide(English)](docs/guide/event_system/event_en.md)
- [Event System Guide(Japanese)](docs/guide/event_system/event_ja.md)

Provides guidelines for engine developers to safely add support for new platforms.

- [Platform System Guide(English)](docs/guide/platform_system/adding_concretes_en.md)
- [Platform System Guide(Japanese)](docs/guide/platform_system/adding_concretes_ja.md)

Provides guidelines for engine developers to safely add support for new renderer backends.

- [Renderer Backend Guide(English)](docs/guide/renderer_backend/adding_concretes_en.md)
- [Renderer Backend Guide(Japanese)](docs/guide/renderer_backend/adding_concretes_ja.md)

### API Reference

- [API reference (generated from Japanese Doxygen comments)](https://chocolate-pie24.github.io/gl_choco_engine/)

## Contributing

GL CHOCO ENGINE is currently maintained by a single developer,
and I’m not accepting pull requests at this time.

If you find a bug, have a question, or want to suggest an improvement, please open an Issue.
Feedback from users is very welcome.

Forks are welcome for your own experiments and use.

## Directory layout

<details>
<summary>Show directory tree</summary>

```console
.
├── assets
│   ├── shaders
│   │   └── test_shader
├── docs
│   ├── architecture
│   │   ├── platform_system
│   │   └── renderer_system
│   ├── guide
│   │   ├── event_system
│   │   ├── platform_system
│   │   └── renderer_system
│   └── layer.md
├── include
│   ├── application
│   └── engine
│       ├── base
│       ├── containers
│       ├── core
│       ├── io_utils
│       ├── platform
│       └── renderer
├── makefile_linux.mak
├── makefile_macos.mak
└── src
    ├── application
    ├── engine
    │   ├── base
    │   ├── containers
    │   ├── core
    │   ├── io_utils
    │   ├── platform
    │   └── renderer
    └── entry.c
```

</details>

## Setup

### macOS

Tested on

```bash
% sw_vers
ProductName:		macOS
ProductVersion:		15.5
BuildVersion:		24F74

% /opt/homebrew/opt/llvm/bin/clang --version
Homebrew clang version 20.1.8
Target: arm64-apple-darwin24.5.0
Thread model: posix
InstalledDir: /opt/homebrew/Cellar/llvm/20.1.8/bin
Configuration file: /opt/homebrew/etc/clang/arm64-apple-darwin24.cfg
```

Install Compiler

```bash
brew install llvm
echo 'export PATH="$(brew --prefix llvm)/bin:$PATH"' >> ~/.zshrc
exec $SHELL -l
```

Install Dependencies

```bash
brew install glfw
brew install glew
```

### Linux

Tested on

```bash
$ uname -a
Linux chocolate-pie24 6.14.0-33-generic #33~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 19 17:02:30 UTC 2 x86_64 x86_64 x86_64 GNU/Linux

$ clang --version
Ubuntu clang version 18.1.3 (1ubuntu1)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin
```

Install Compiler

```bash
sudo apt install clang lldb lld
```

Install Dependencies

```bash
sudo apt install libglew-dev
sudo apt install libglfw3-dev
```

## Build

```bash
chmod +x ./build.sh
./build.sh all DEBUG_BUILD    # Debug build
./build.sh all RELEASE_BUILD  # Release build
./build.sh all TEST_BUILD     # Test build
./build.sh clean              # Clean
```

## Run

```bash
./bin/gl_choco_engine
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Author

GitHub: https://github.com/chocolate-pie24
