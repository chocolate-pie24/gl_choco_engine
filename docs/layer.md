# GLCE Architecture (Layered Architecture)

This document describes the high-level layering and module dependencies of GL CHOCO ENGINE (GLCE).

- [GLCE Architecture (Layered Architecture)](#glce-architecture-layered-architecture)
  - [Dependency rules](#dependency-rules)
  - [Engine foundation](#engine-foundation)
  - [Engine overview](#engine-overview)
  - [Detailed view: Platform](#detailed-view-platform)
  - [Detailed view: Renderer](#detailed-view-renderer)
  - [Layer Reference](#layer-reference)
    - [application](#application)
    - [engine/base](#enginebase)
    - [engine/core](#enginecore)
    - [engine/containers](#enginecontainers)
    - [engine/io\_utils](#engineio_utils)
    - [engine/platform](#engineplatform)
    - [engine/renderer](#enginerenderer)

## Dependency rules

- Dependencies are allowed only from higher layers to lower layers.
- Circular dependencies are not allowed.

## Engine foundation

The foundation layer provides modules that are available across the entire engine.
This section corresponds to engine/base and engine/core.

```mermaid
graph TD
  subgraph BASE[base]
    direction TB
    CHOCO_MESSAGE[choco_message]
    CHOCO_MACROS[choco_macros]
  end
  CHOCO_MACROS --> CHOCO_MESSAGE

  subgraph CORE[core]
    direction TB
    subgraph EVENT[event]
      direction TB
      KEYBOARD_EVENT[keyboard_event]
      MOUSE_EVENT[mouse_event]
      WINDOW_EVENT[window_event]
    end

    subgraph MEMORY[memory]
      direction TB
      CHOCO_MEMORY[choco_memory]
      LINEAR_ALLOCATOR[linear_allocator]
    end

    FILESYSTEM[filesystem]
  end
  FILESYSTEM --> CHOCO_MACROS
  FILESYSTEM --> CHOCO_MESSAGE
  FILESYSTEM --> CHOCO_MEMORY
  CHOCO_MEMORY --> CHOCO_MACROS
  CHOCO_MEMORY --> CHOCO_MESSAGE
  LINEAR_ALLOCATOR --> CHOCO_MACROS
  LINEAR_ALLOCATOR --> CHOCO_MESSAGE
```

## Engine overview

This diagram shows the module dependencies at the engine level.

In this overview, dependencies on the base and core layers are omitted to keep the diagram readable,
since all engine modules may depend on them.

The renderer frontend has not been implemented yet, so the application currently uses some backend modules directly; this will be removed once the frontend is introduced.

```mermaid
graph TD
  APPLICATION[application]
  APPLICATION --> RING_QUEUE
  APPLICATION --> FS_UTILS
  APPLICATION --> PLATFORM_CONTEXT
  APPLICATION --> RENDERER_TYPES
  APPLICATION --> GL33_VBO
  APPLICATION --> GL33_VAO
  APPLICATION --> GL33_SHADER
  APPLICATION --> PLATFORM_TYPES

  subgraph ENGINE[engine]
    direction TB

    subgraph CONTAINERS[containers]
      direction TB
      CHOCO_STRING[choco_string]
      RING_QUEUE[ring_queue]
    end

    subgraph IO_UTILS[io_utils]
      direction TB
      FS_UTILS[fs_utils]
    end
    FS_UTILS --> CHOCO_STRING

    subgraph PLATFORM[platform]
      PLATFORM_CONTEXT[platform_context]
      PLATFORM_INTERFACE[platform_interface]

      direction TB
      subgraph PLATFORM_CORE[platform_core]
        direction TB
        PLATFORM_TYPES[platform_types]
      end
      subgraph PLATFORM_CONCRETES[platform_concretes]
        direction TB
        PLATFORM_GLFW[platform_glfw]
      end
    end
    PLATFORM_INTERFACE --> PLATFORM_TYPES
    PLATFORM_GLFW --> PLATFORM_INTERFACE
    PLATFORM_GLFW --> CHOCO_STRING
    PLATFORM_GLFW --> PLATFORM_TYPES
    PLATFORM_CONTEXT --> PLATFORM_INTERFACE
    PLATFORM_CONTEXT --> PLATFORM_GLFW
    PLATFORM_CONTEXT --> PLATFORM_TYPES

    subgraph RENDERER[renderer]
      direction TB
      subgraph RENDERER_BACKEND[renderer_backend]
        direction TB
        subgraph GL33[gl33]
          direction TB
          GL33_SHADER[gl33_shader]
          GL33_VAO[gl33_vao]
          GL33_VBO[gl33_vbo]
        end
      end
      subgraph RENDERER_CORE[renderer_core]
        direction TB
        RENDERER_ERR_UTILS[renderer_err_utils]
        RENDERER_MEMORY[renderer_memory]
        RENDERER_TYPES[renderer_types]
      end
    end
    GL33_SHADER --> RENDERER_TYPES
    GL33_SHADER --> RENDERER_MEMORY
    GL33_SHADER --> RENDERER_ERR_UTILS

    GL33_VAO --> RENDERER_TYPES
    GL33_VAO --> RENDERER_ERR_UTILS
    GL33_VAO --> RENDERER_MEMORY
    GL33_VBO --> RENDERER_TYPES
    GL33_VBO --> RENDERER_ERR_UTILS
    GL33_VBO --> RENDERER_MEMORY
    RENDERER_ERR_UTILS --> RENDERER_TYPES
    RENDERER_MEMORY --> RENDERER_TYPES
  end
```

## Detailed view: Platform

```mermaid
graph TD
  subgraph CONTAINERS[containers]
    direction TB
    CHOCO_STRING[choco_string]
    RING_QUEUE[ring_queue]
  end

  subgraph PLATFORM[platform]
    PLATFORM_CONTEXT[platform_context]
    PLATFORM_INTERFACE[platform_interface]
    direction TB
    subgraph PLATFORM_CORE[platform_core]
      direction TB
      PLATFORM_TYPES[platform_types]
    end
    subgraph PLATFORM_CONCRETES[platform_concretes]
      direction TB
      PLATFORM_GLFW[platform_glfw]
    end
  end
  PLATFORM_INTERFACE --> PLATFORM_TYPES
  PLATFORM_GLFW --> PLATFORM_INTERFACE
  PLATFORM_GLFW --> CHOCO_STRING
  PLATFORM_GLFW --> PLATFORM_TYPES
  PLATFORM_CONTEXT --> PLATFORM_INTERFACE
  PLATFORM_CONTEXT --> PLATFORM_GLFW
  PLATFORM_CONTEXT --> PLATFORM_TYPES
```

## Detailed view: Renderer

```mermaid
graph TD
  subgraph RENDERER[renderer]
    direction TB
    subgraph RENDERER_BACKEND[renderer_backend]
      direction TB
      subgraph GL33[gl33]
        direction TB
        GL33_SHADER[gl33_shader]
        GL33_VAO[gl33_vao]
        GL33_VBO[gl33_vbo]
      end
    end
    subgraph RENDERER_CORE[renderer_core]
      direction TB
      RENDERER_ERR_UTILS[renderer_err_utils]
      RENDERER_MEMORY[renderer_memory]
      RENDERER_TYPES[renderer_types]
    end
  end
  GL33_SHADER --> RENDERER_TYPES
  GL33_SHADER --> RENDERER_MEMORY
  GL33_SHADER --> RENDERER_ERR_UTILS

  GL33_VAO --> RENDERER_TYPES
  GL33_VAO --> RENDERER_ERR_UTILS
  GL33_VAO --> RENDERER_MEMORY
  GL33_VBO --> RENDERER_TYPES
  GL33_VBO --> RENDERER_ERR_UTILS
  GL33_VBO --> RENDERER_MEMORY
  RENDERER_ERR_UTILS --> RENDERER_TYPES
  RENDERER_MEMORY --> RENDERER_TYPES
```

## Layer Reference

### application

- Purpose: The top-level layer of the project, responsible for orchestrating all subsystems. It provides:
  - Initialization and shutdown of all subsystems
  - The application main loop
- Characteristics: Runs for the full lifetime of the application (startup to shutdown).

### engine/base

- Purpose: Engine-wide, project-agnostic utilities reusable beyond GLCE.
- Characteristics: Initialization-free; usable immediately at startup.
- Modules:
  - choco_macros: Common macro definitions.
  - choco_message: Colored logging/output helpers for stdout/stderr.

### engine/core

- Purpose: GLCE-specific engine foundations used across the whole engine.
- Characteristics: Some modules require explicit initialization.
- Modules:
  - keyboard_event / mouse_event / window_event: Event-related data types.
  - choco_memory: Allocation/free with memory tracking.
  - linear_allocator: Linear allocator for fixed-lifecycle allocations.
  - filesystem: Basic file I/O (open/close, byte reads).

### engine/containers

- Purpose: Provides container modules that encapsulate resource ownership and management.
- Characteristics: Initialization-free; usable immediately at startup.
- Modules:
  - ring_queue: Generic ring queue (ring buffer) container module.
  - choco_string: String container module with basic string operations.

### engine/io_utils

- Purpose: Provides higher-level I/O utilities that go beyond the standard C library by building on other GLCE modules.
- Characteristics: Initialization-free; usable immediately at startup.
- Modules:
  - fs_utils: Higher-level file I/O utilities on top of **filesystem**, such as loading an entire text file.

### engine/platform

- Purpose: GLCE currently uses GLFW as its primary platform backend, but the platform subsystem is designed to avoid hard-coding a GLFW dependency.
To keep room for future non-GLFW implementations,
GLCE abstracts the platform subsystem using a Strategy-style interface (function table) and swappable backend implementations.
- Characteristics: Requires explicit initialization. Once initialized, the platform subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - platform_core/platform_types: Common data types used across the platform subsystem.
  - platform_interface: Defines the platform interface as a function table (vtable-like) shared by all platform backends.
  - platform_concretes/platform_glfw: GLFW-based backend implementation that provides the concrete function table for the platform interface.
  - platform_context: Strategy context and public entry point for the platform subsystem, responsible for initialization, backend selection, lifecycle management, and dispatching API calls through the interface.

### engine/renderer

*Note*: A renderer frontend has not been implemented yet, so the application currently uses some backend modules directly.
This will be removed once the frontend is introduced.

- Purpose: Provides the rendering subsystem. GLCE currently targets an OpenGL 3.3-based implementation, but the renderer is structured to accommodate additional backends in the future (e.g., other OpenGL versions or Vulkan). The long-term design separates a frontend (API-agnostic layer) from backend implementations (graphics-API-specific layers).
- Characteristics: Requires explicit initialization. Once initialized, the renderer subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - renderer_backend/gl33/gl33_shader: OpenGL 3.3 shader program utilities (compile, link, and program use/bind).
  - renderer_backend/gl33/gl33_vao: OpenGL 3.3 VAO utilities (bind/unbind and vertex attribute configuration).
  - renderer_backend/gl33/gl33_vbo: OpenGL 3.3 VBO utilities (bind/unbind and uploading data to the GPU).
  - renderer_core/renderer_err_utils: Utilities for converting renderer result/error codes to human-readable strings.
  - renderer_core/renderer_memory: Wrapper APIs over **engine/core/choco_memory** tailored for the renderer layer (renderer-specific result codes and automatic memory-tag assignment) to simplify allocation/free within the renderer.
  - renderer_core/renderer_types: Common data types shared across the renderer subsystem.
