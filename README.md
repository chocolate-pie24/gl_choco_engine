<p align="center">
  <img src="assets/logo/choco_engine_banner_768x256.png" alt="GL CHOCO ENGINE" width="640">
</p>

- [GL CHOCO ENGINE](#gl-choco-engine)
  - [Non-goals](#non-goals)
  - [Who It Is For](#who-it-is-for)
  - [Contributing](#contributing)
  - [Directory Layout](#directory-layout)
  - [Documentation](#documentation)
    - [AI Context Generation](#ai-context-generation)
    - [Planned project-wide policies](#planned-project-wide-policies)
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
glce/application/
    │
    │ Public API
    ▼
glce/include/engine/
    │
    ▼
glce/engine/
```

The main repository layout is:

```text
.
├── assets/
├── docs/
├── glce/
│   ├── application/
│   ├── engine/
│   ├── include/
│   │   └── engine/
│   ├── make/
│   ├── scripts/
│   ├── test/
│   └── build.sh
├── generate_ai_context.sh
├── LICENSE
└── README.md
```

The responsibilities of the main directories are:

| Directory | Responsibility |
|---|---|
| `glce/application/` | Application-side source code that uses the GLCE Engine |
| `glce/include/engine/` | Public API exposed by the Engine |
| `glce/engine/` | Engine source code and internal headers |
| `glce/test/` | Test source files and headers |
| `glce/make/` | Common and OS-specific Make configuration |
| `glce/scripts/` | Development workflows such as Coverage, Sanitizer, and Valgrind |
| `assets/` | Runtime resources such as shaders and textures |
| `docs/` | Design and development documentation |

Directories such as `glce/bin/`, `glce/obj/`, and `glce/cov/` are generated as needed during builds or analysis workflows.

## Documentation

GLCE's design documents are intended to be used by both humans and AI tools.

They are intentionally detailed so that design intent, rationale, constraints, and historical context can be recovered later. Reading every document from beginning to end is not required; using AI-assisted summarization and question answering is an intended way to access the documentation.

AI is also used extensively to create and maintain these documents. When design decisions or project-wide policies change, new sections are often generated through AI-assisted design discussions and added to the existing documents. As a result, some concepts may be described repeatedly from different perspectives or at different stages of the project's development.

This redundancy is partly intentional, but the documents are periodically reviewed to remove unnecessary duplication, resolve contradictions, and retire outdated assumptions.

The design documents are currently written primarily in Japanese. Because they are intended to be used with AI-assisted summarization, translation, and question answering, non-Japanese readers can use AI tools to access their contents.

- [Design Decisions](docs/design/design_decisions.md)
  Accepted architectural and design decisions, including rationale,
  rejected alternatives, consequences, and revisit conditions.

- [Validation Policy](docs/design/validation_policy.md)
  Project-wide rules for validation responsibility, validation depth,
  failure classification, and related contracts.

- [Boundary Model](docs/design/boundary_model.md)
  The model used to reason about trust, authority, ownership,
  and integrity boundaries.

- [Memory Model](docs/design/memory_model.md)
  The current CPU-side memory architecture and allocation ownership model.

- [Coding Style](docs/design/coding_style.md)
  Project-wide naming, API semantics, module structure, and coding conventions.

### AI Context Generation

GLCE provides generate_ai_context.sh to make the source code and design documentation easy to provide to AI tools for code review, design discussion, investigation, and refactoring.

Run the script from the repository root:

```bash
./generate_ai_context.sh
```

The script generates two files in the repository root:

glce_source_context.txt
Contains the repository tree and the contents of all .c and .h files under glce/.

glce_design_context.txt
Contains the contents of all Markdown documents under docs/design/, concatenated into a single file.

Each generated file includes an explanation of its contents and file boundaries so that it can be provided directly to an AI tool.

For project-wide code review, the two files can be provided together so that the implementation can be reviewed against GLCE-specific design policies and design decisions.

The generated context files are not sources of truth and are not tracked by Git. The original source files and documents in the repository remain authoritative.

### Planned project-wide policies

- Message Policy
- Doxygen Documentation Style Policy
- Test Style Policy (long-term)

Until these policies are established, the corresponding implementation and documentation may continue to change substantially.

## Setup

GLCE currently supports:

- macOS
- Linux
- FreeBSD

The supported entry point for building GLCE is `glce/build.sh`.

OS-specific compiler, include path, library path, and linker settings are selected automatically by `glce/build.sh` and the files under `glce/make/`.

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
When `glce/build.sh` detects FreeBSD, it selects `gmake` automatically.

## Build

From the repository root, move to the GLCE directory:

```bash
cd glce
```

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

Debug, Release, and Test builds currently share the same `glce/bin/` and `glce/obj/` directories.

Run `clean` before switching build modes.

For details, see the Build System documentation:

- [Japanese](docs/build_system_ja.md)
- [English](docs/build_system_en.md)

## Run

After building GLCE, run:

```bash
./bin/gl_choco_engine
```

## License

GLCE source code and project documentation are released under the MIT License.

See [LICENSE](LICENSE) for details.

## Author

GitHub: https://github.com/chocolate-pie24
