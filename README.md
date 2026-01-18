<p align="center">
  <img src="assets/logo/choco_engine_banner_768x256.png" alt="GL CHOCO ENGINE" width="640">
</p>

## GL CHOCO ENGINE

GL CHOCO ENGINE (GLCE) is a C/OpenGL engine focused on clarity, controllability,
and long-term maintainability.

It uses GLFW to keep the platform subsystem portable across desktop environments, while keeping dependencies intentionally small.

The codebase is written with a quality-first mindset—explicit architecture, predictable low-level behavior,
and a preference for testable, well-documented modules—so engineers outside the graphics domain can still understand, modify, and extend it with confidence.

The goal is a practical baseline you can own end-to-end, rather than a full-featured alternative to Unity or Unreal.

## Motivation / Positioning

GL CHOCO ENGINE is designed as a lightweight, dependency-minimal C/OpenGL engine that you can understand and own end-to-end.

In addition to “game engine” use cases, the project targets practical visualization and tooling scenarios often seen in robotics, industrial systems, and embedded-adjacent environments—where you may not want (or cannot afford) large middleware stacks or heavyweight engines. Lower-power devices such as Raspberry Pi-class boards are a longer-term target, so the feature set is intentionally scoped.

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

For the full background and the development log (Japanese), see the Zenn series linked in the Documentation section.

## Inspired by

This project was originally inspired by Kohi Game Engine and Travis Vroman’s work.
I’m grateful for the motivation and the educational value of seeing an engine built from the ground up.
GL CHOCO ENGINE is an independent codebase with its own architecture and design decisions, evolving according to its goals and constraints.

## Documentation

- [Architecture / Layering](docs/layer.md)
- [API reference (generated from Japanese Doxygen comments)](https://chocolate-pie24.github.io/gl_choco_engine/)
- [Development log (Zenn article series; Japanese)](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)

## Contributing

GL CHOCO ENGINE is currently maintained by a single developer,
and I’m not accepting pull requests at this time.

If you find a bug, have a question, or want to suggest an improvement, please open an Issue.
Feedback from users is very welcome.

Forks are welcome for your own experiments and use.

## Directory layout

<details>
<summary>Show directory tree</summary

```console
.
├── assets
│   └── shaders
│   │   └── test_shader
│   │       ├── fragment_shader.frag
│   │       └── vertex_shader.vert
│   └── test
│       └── filesystem
│           ├── test_file.txt
│           └── test_file_w.txt
├── build.sh
├── Doxyfile
├── include
│   ├── application
│   │   └── application.h
│   └── engine
│       ├── base
│       │   ├── choco_macros.h
│       │   └── choco_message.h
│       ├── containers
│       │   ├── choco_string.h
│       │   └── ring_queue.h
│       ├── core
│       │   ├── event
│       │   │   ├── keyboard_event.h
│       │   │   ├── mouse_event.h
│       │   │   └── window_event.h
│       │   ├── filesystem
│       │   │   └── filesystem.h
│       │   └── memory
│       │       ├── choco_memory.h
│       │       └── linear_allocator.h
│       ├── io_utils
│       │   └── fs_utils
│       │       └── fs_utils.h
│       ├── platform
│       │   ├── platform_concretes
│       │   │   └── platform_glfw.h
│       │   ├── platform_context.h
│       │   ├── platform_core
│       │   │   └── platform_types.h
│       │   └── platform_interface.h
│       └── renderer
│           ├── renderer_backend
│           │   └── gl33
│           │       ├── gl33_shader.h
│           │       ├── vertex_array_object.h
│           │       └── vertex_buffer_object.h
│           └── renderer_core
│               ├── renderer_err_utils.h
│               ├── renderer_memory.h
│               └── renderer_types.h
├── LICENSE
├── makefile_linux.mak
├── makefile_macos.mak
├── README.md
├── src
│   ├── application
│   │   └── application.c
│   ├── engine
│   │   ├── base
│   │   │   └── choco_message.c
│   │   ├── containers
│   │   │   ├── choco_string.c
│   │   │   └── ring_queue.c
│   │   ├── core
│   │   │   ├── filesystem
│   │   │   │   └── filesystem.c
│   │   │   └── memory
│   │   │       ├── choco_memory.c
│   │   │       └── linear_allocator.c
│   │   ├── io_utils
│   │   │   └── fs_utils
│   │   │       └── fs_utils.c
│   │   ├── platform
│   │   │   ├── platform_concretes
│   │   │   │   └── platform_glfw.c
│   │   │   └── platform_context.c
│   │   └── renderer
│   │       ├── renderer_backend
│   │       │   └── gl33
│   │       │       ├── gl33_shader.c
│   │       │       ├── vertex_array_object.c
│   │       │       └── vertex_buffer_object.c
│   │       └── renderer_core
│   │           ├── renderer_err_utils.c
│   │           └── renderer_memory.c
│   └── entry.c
└── test
    └── include
        ├── renderer
        │   ├── test_gl33_shader.h
        │   ├── test_renderer_err_utils.h
        │   ├── test_renderer_memory.h
        │   ├── test_vertex_array_object.h
        │   └── test_vertex_buffer_object.h
        ├── test_choco_string.h
        ├── test_filesystem.h
        ├── test_fs_utils.h
        ├── test_linear_allocator.h
        ├── test_memory_system.h
        ├── test_platform_context.h
        ├── test_platform_glfw.h
        └── test_ring_queue.h
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
