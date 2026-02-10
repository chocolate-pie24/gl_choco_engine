# GLCE Architecture (Layered Architecture)

This document describes the high-level layering and module dependencies of GL CHOCO ENGINE (GLCE).

- [GLCE Architecture (Layered Architecture)](#glce-architecture-layered-architecture)
  - [Dependency rules](#dependency-rules)
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

- Dependencies must not point upward (from lower layers to higher layers).
- Circular dependencies are not allowed.

## Engine overview

This diagram shows the module dependencies at the engine level.

```mermaid
graph TD
  APPLICATION[application]

  subgraph ENGINE[engine]
    subgraph SYSTEMS[systems]
      PLATFORM[platform]
      RENDERER[renderer]
    end

    subgraph IO_UTILS[io_utils]
      FS_UTILS[fs_utils]
    end

    subgraph CONTAINERS[containers]
      direction TB
      CHOCO_STRING[choco_string]
      RING_QUEUE[ring_queue]
    end

    CORE[core]
    BASE[base]
  end

  APPLICATION --> PLATFORM
  APPLICATION --> RENDERER
  APPLICATION --> IO_UTILS
  APPLICATION --> CORE
  APPLICATION --> CONTAINERS

  PLATFORM --> CONTAINERS
  PLATFORM --> CORE
  RENDERER --> CORE

  IO_UTILS --> CONTAINERS
  IO_UTILS --> CORE

  CONTAINERS --> CORE

  CORE --> BASE
```

## Detailed view: Platform

```mermaid
graph TD
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
  end

  subgraph CONTAINERS[containers]
    direction TB
    CHOCO_STRING[choco_string]
  end

  subgraph PLATFORM[platform]
    direction TB

    PLATFORM_CONTEXT[platform_context]

    subgraph PLATFORM_CORE[platform_core]
      direction TB
      PLATFORM_ERR_UTILS[platform_err_utils]
      PLATFORM_TYPES[platform_types]
    end

    PLATFORM_INTERFACE[platform_interface]

    subgraph PLATFORM_CONCRETES[platform_concretes]
      direction TB
      PLATFORM_GLFW[platform_glfw]
    end
  end

  PLATFORM_CONTEXT --> PLATFORM_CORE
  PLATFORM_CONTEXT --> PLATFORM_GLFW
  PLATFORM_CONTEXT --> PLATFORM_INTERFACE
  PLATFORM_CONTEXT --> EVENT
  PLATFORM_CONTEXT --> |init parameter| LINEAR_ALLOCATOR

  PLATFORM_GLFW --> PLATFORM_CORE
  PLATFORM_GLFW --> CHOCO_STRING
  PLATFORM_GLFW --> PLATFORM_INTERFACE
  PLATFORM_GLFW --> EVENT

  PLATFORM_INTERFACE --> PLATFORM_TYPES
  PLATFORM_INTERFACE --> EVENT

  PLATFORM_ERR_UTILS -->|error_code type only| CHOCO_STRING
  PLATFORM_ERR_UTILS -->|error code type only| LINEAR_ALLOCATOR

  CHOCO_STRING --> CHOCO_MEMORY
```

## Detailed view: Renderer

The renderer frontend has not been implemented yet, so the application currently uses some backend modules directly; this will be removed once the frontend is introduced.

```mermaid
graph TD
  subgraph CORE[core]
    direction TB
    subgraph MEMORY[memory]
      direction TB
      LINEAR_ALLOCATOR[linear_allocator]
      CHOCO_MEMORY[choco_memory]
    end
  end
  subgraph RENDERER_CORE[renderer_core]
    direction TB
    RENDERER_ERR_UTILS[renderer_err_utils]
    RENDERER_MEMORY[renderer_memory]
  end

  subgraph RENDERER_BACKEND[renderer_backend]
    direction TB
    RENDERER_BACKEND_CONTEXT[renderer_backend_context]

    subgraph RENDERER_BACKEND_INTERFACE[renderer_backend_interface]
      direction TB
      SHADER[shader]
      VERTEX_ARRAY_OBJECT[vertex_array_object]
      VERTEX_BUFFER_OBJECT[vertex_buffer_object]
    end

    subgraph RENDERER_BACKEND_CONCRETES[renderer_backend_concretes]
      subgraph GL33[gl33]
        direction TB
        GL33_SHADER[gl33_shader]
        GL33_VAO[gl33_vao]
        GL33_VBO[gl33_vbo]
      end
    end
  end

  RENDERER_BACKEND_CONTEXT --> RENDERER_ERR_UTILS
  RENDERER_BACKEND_CONTEXT --> RENDERER_BACKEND_INTERFACE
  RENDERER_BACKEND_CONTEXT --> GL33
  RENDERER_BACKEND_CONTEXT --> |init parameter| LINEAR_ALLOCATOR

  GL33 --> RENDERER_BACKEND_INTERFACE
  GL33 --> RENDERER_MEMORY

  GL33 --> RENDERER_ERR_UTILS

  RENDERER_MEMORY --> CHOCO_MEMORY

  RENDERER_ERR_UTILS -->|error_code type only| CHOCO_MEMORY
  RENDERER_ERR_UTILS -->|error_code type only| LINEAR_ALLOCATOR
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
- Characteristics: No module-specific initialization, but requires core memory system to be initialized.
- Modules:
  - ring_queue: Generic ring queue (ring buffer) container module.
  - choco_string: String container module with basic string operations.

### engine/io_utils

- Purpose: Provides higher-level I/O utilities that go beyond the standard C library by building on other GLCE modules.
- Characteristics: No module-specific initialization, but requires core memory system to be initialized.
- Modules:
  - fs_utils: Higher-level file I/O utilities on top of **filesystem**, such as loading an entire text file.

### engine/platform

- Purpose: GLCE currently uses GLFW as its primary platform backend, but the platform subsystem is designed to avoid hard-coding a GLFW dependency. To keep room for future non-GLFW implementations,
GLCE abstracts the platform subsystem using a Strategy-style interface (function table) and swappable backend implementations.
- Characteristics: Requires explicit initialization. Once initialized, the platform subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - platform_core/platform_types: Common data types used across the platform subsystem.
  - platform_core/platform_err_utils: Provides utilities to translate lower-layer error codes into platform subsystem error codes, and to convert platform subsystem error codes into human-readable strings.
  - platform_interface: Defines the platform interface as a function table (vtable-like) shared by all platform backends.
  - platform_concretes/platform_glfw: GLFW-based backend implementation that provides the concrete function table for the platform interface.
  - platform_context: Strategy context and public entry point for the platform subsystem, responsible for initialization, backend selection, lifecycle management, and dispatching API calls through the interface.

### engine/renderer

*Note*: A renderer frontend has not been implemented yet, so the application currently uses some backend modules directly.
This will be removed once the frontend is introduced.

- Purpose: Provides the rendering subsystem. GLCE currently targets an OpenGL 3.3-based implementation, but the renderer is structured to accommodate additional backends in the future (e.g., other OpenGL versions or Vulkan). The long-term design separates a frontend (API-agnostic layer) from backend implementations (graphics-API-specific layers).
- Characteristics: Requires explicit initialization. Once initialized, the renderer subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - renderer_backend/renderer_backend_context/renderer_backend_context: The primary entry point and orchestration layer for the renderer backend. It wires the selected backend implementation and dispatches calls from higher layers through the backend interfaces, while exposing a small set of facade headers (shader/VAO/VBO contexts) as the public API surface.
  - renderer_backend/renderer_backend_context/renderer_backend_shader_context: Provides a thin facade (public API surface) for shader-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_context/renderer_backend_vao_context: Provides a thin facade (public API surface) for VAO-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_context/renderer_backend_vbo_context: Provides a thin facade (public API surface) for VBO-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_concretes/gl33/gl33_shader: OpenGL 3.3 shader program utilities (compile, link, and program use/bind).
  - renderer_backend/renderer_backend_concretes/gl33/gl33_vao: OpenGL 3.3 VAO utilities (bind/unbind and vertex attribute configuration).
  - renderer_backend/renderer_backend_concretes/gl33/gl33_vbo: OpenGL 3.3 VBO utilities (bind/unbind and uploading data to the GPU).
  - renderer_backend/renderer_backend_interface/shader: Defines the shader interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_interface/vertex_array_object: Defines the VAO interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_interface/vertex_buffer_object: Defines the VBO interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_types: Defines common data types shared across the entire renderer backend layer (shader, VAO, and VBO modules).
  - renderer_core/renderer_err_utils: Provides utilities to translate lower-layer error codes into renderer subsystem error codes, and to convert renderer subsystem error codes into human-readable strings.
  - renderer_core/renderer_memory: Wrapper APIs over **engine/core/choco_memory** tailored for the renderer layer (renderer-specific result codes and automatic memory-tag assignment) to simplify allocation/free within the renderer.
  - renderer_core/renderer_types: Common data types shared across the renderer subsystem.
