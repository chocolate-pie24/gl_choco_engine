# GLCE Architecture (Layered Architecture)

This document describes the high-level layering and module dependencies of GL CHOCO ENGINE (GLCE).

- [GLCE Architecture (Layered Architecture)](#glce-architecture-layered-architecture)
  - [Dependency rules](#dependency-rules)
  - [Engine overview](#engine-overview)
  - [Detailed view: Platform System](#detailed-view-platform-system)
  - [Detailed view: Renderer System](#detailed-view-renderer-system)
  - [Detailed view: Camera System](#detailed-view-camera-system)
  - [Detailed view: Texture System](#detailed-view-texture-system)
  - [Detailed view: Texture Resource](#detailed-view-texture-resource)
  - [Layer Reference](#layer-reference)
    - [application](#application)
    - [engine/base](#enginebase)
    - [engine/core](#enginecore)
    - [engine/containers](#enginecontainers)
    - [engine/io\_utils](#engineio_utils)
    - [engine/resource](#engineresource)
    - [engine/systems/platform](#enginesystemsplatform)
    - [engine/systems/renderer](#enginesystemsrenderer)
    - [engine/systems/camera\_system](#enginesystemscamera_system)
    - [engine/systems/texture\_system](#enginesystemstexture_system)

## Dependency rules

- Dependencies must not point upward (from lower layers to higher layers).
- Circular dependencies are not allowed.
- This document shows high-level module dependencies. Dependencies on internal submodules are folded into their parent module.

## Engine overview

This diagram shows the module dependencies at the engine level.

![Engine Overview](./engine_overview.png)

## Detailed view: Platform System

[Platform System Architecture](./architecture/systems/platform_system/architecture_en.md)

## Detailed view: Renderer System

[Renderer Backend Architecture](./architecture/systems/renderer_system/renderer_backend/architecture_en.md)

## Detailed view: Camera System

[Camera System Architecture](./architecture/systems/camera_system/architecture_en.md)

## Detailed view: Texture System

[Texture System Architecture](./architecture/systems/texture_system/architecture_en.md)

## Detailed view: Texture Resource

[Texture Resource Architecture](./architecture/resource/texture/architecture_en.md)

## Layer Reference

### application

- Purpose: The top-level composition layer of GLCE. It creates, owns, connects, runs, and shuts down the engine subsystems required by the current executable.
  It is responsible for:
  - Creating and destroying engine-wide runtime state.
  - Initializing and shutting down subsystems in the required order.
  - Owning the application main loop.
  - Receiving platform events and storing them in application-level event queues.
  - Updating application state from queued events.
  - Dispatching updated state to subsystems such as camera, renderer, and texture management.
  - Holding temporary sample-scene logic until the renderer frontend and higher-level scene layer are introduced.

- Characteristics: Runs for the full lifetime of the executable, from startup to shutdown. This layer is allowed to depend on multiple engine subsystems because it acts as the composition root. However, lower engine layers must not depend on the application layer.

- Notes:
  - The application layer currently uses `ring_queue` directly to buffer window, keyboard, and mouse events received from the platform subsystem.
  - The application currently uses some renderer backend and OpenGL/GLFW-related functionality directly as a temporary implementation detail. This dependency should be reduced when the renderer frontend is introduced.

- Modules:
  - application: Public lifecycle API for creating, running, and destroying the application.
  - application_core/application_types: Common application-layer configuration and result types.
  - application_core/application_err_utils: Utilities for converting lower-layer result codes into application-layer result codes and strings.
  - command_interpreter/flight_camera: Converts application-level keyboard input state into flight-camera control commands.

### engine/base

- Purpose: Engine-wide, project-agnostic utilities reusable beyond GLCE.
- Characteristics: Initialization-free; usable immediately at startup.
- Modules:
  - choco_macros: Common macro definitions.
  - choco_message: Colored logging/output helpers for stdout/stderr.
  - choco_math: General-purpose math utilities, including vector and matrix operations.

### engine/core

- Purpose: GLCE-specific engine foundations used across the whole engine.
- Characteristics: Some modules require explicit initialization.
- Modules:
  - keyboard_event / mouse_event / window_event: Event-related data types.
  - choco_memory: Allocation/free with memory tracking.
  - linear_allocator: Linear allocator for fixed-lifecycle allocations.
  - filesystem: Basic file I/O (open/close, byte reads).
  - buffer_utils/buffer_utils: Defines APIs for writing data to densely packed buffers and reading data back from them.
  - geometry_primitive/vertex: Defines basic geometric data structures used to represent shape data.

### engine/containers

- Purpose: Provides general-purpose container modules used by multiple GLCE layers and subsystems.
- Characteristics: No module-specific initialization, but requires core memory system to be initialized.
- Modules:
  - ring_queue: Generic ring queue (ring buffer) container module.
  - choco_string: String container module with basic string operations.

### engine/io_utils

- Purpose: Provides higher-level I/O utilities that go beyond the standard C library by building on other GLCE modules.
- Characteristics: No module-specific initialization, but requires core memory system to be initialized.
- Modules:
  - fs_utils: Higher-level file I/O utilities on top of **filesystem**, such as loading an entire text file.

### engine/resource

- Purpose: Provides modules that convert asset data, such as textures and geometry, into CPU-side resources that can be used by the engine.
- Characteristics: No module-specific initialization, but requires core memory system to be initialized.
- Modules:
  - loaders/bmp_loader: BMP file loader.
  - resource_core/resource_err_utils: Resource-layer error utility module that provides result-code translation between modules and conversion of resource-layer result codes to strings.
  - resource_core/resource_types: Provides common data types used across the resource layer.
  - texture/texture: Provides APIs for operating on CPU-side texture resources.

### engine/systems/platform

- Purpose: GLCE currently uses GLFW as its primary platform backend, but the platform subsystem is designed to avoid hard-coding a GLFW dependency. To keep room for future non-GLFW implementations,
GLCE abstracts the platform subsystem using a Strategy-style interface (function table) and swappable backend implementations.
- Characteristics: Requires explicit initialization. Once initialized, the platform subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - platform_core/platform_types: Common data types used across the platform subsystem.
  - platform_core/platform_err_utils: Provides utilities to translate lower-layer error codes into platform subsystem error codes, and to convert platform subsystem error codes into human-readable strings.
  - platform_interface: Defines the platform interface as a function table (vtable-like) shared by all platform backends.
  - platform_concretes/platform_glfw: GLFW-based backend implementation that provides the concrete function table for the platform interface.
  - platform_context: Strategy context and public entry point for the platform subsystem, responsible for initialization, backend selection, lifecycle management, and dispatching API calls through the interface.

### engine/systems/renderer

*Note*: A renderer frontend has not been implemented yet, so the application currently uses some backend modules directly.
This will be removed once the frontend is introduced.

- Purpose: Provides the rendering subsystem. GLCE currently targets an OpenGL 3.3-based implementation, but the renderer is structured to accommodate additional backends in the future (e.g., other OpenGL versions or Vulkan). The long-term design separates a frontend (API-agnostic layer) from backend implementations (graphics-API-specific layers).
- Characteristics: Requires explicit initialization. Once initialized, the renderer subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - renderer_backend/renderer_backend_context/renderer_backend_context: The primary entry point and orchestration layer for the renderer backend. It wires the selected backend implementation and dispatches calls from higher layers through the backend interfaces, while exposing a small set of facade headers (shader/texture/VAO/VBO contexts) as the public API surface.
  - renderer_backend/renderer_backend_context/context_shader: Provides a thin facade (public API surface) for shader-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_context/context_texture: Provides a thin facade (public API surface) for texture-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_context/context_vao: Provides a thin facade (public API surface) for VAO-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_context/context_vbo: Provides a thin facade (public API surface) for VBO-related backend operations used by higher layers. The implementation is consolidated in renderer_backend_context.c.
  - renderer_backend/renderer_backend_concretes/gl33/concrete_shader: OpenGL 3.3 shader program utilities (compile, link, and program use/bind).
  - renderer_backend/renderer_backend_concretes/gl33/concrete_texture: OpenGL 3.3 texture operation APIs, including bind, unbind, and pixel upload.
  - renderer_backend/renderer_backend_concretes/gl33/concrete_vao: OpenGL 3.3 VAO utilities (bind/unbind and vertex attribute configuration).
  - renderer_backend/renderer_backend_concretes/gl33/concrete_vbo: OpenGL 3.3 VBO utilities (bind/unbind and uploading data to the GPU).
  - renderer_backend/renderer_backend_interface/interface_shader: Defines the shader interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_interface/interface_texture: Defines the texture interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_interface/interface_vao: Defines the VAO interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_interface/interface_vbo: Defines the VBO interface as a function table (vtable-like) shared by all renderer backends.
  - renderer_backend/renderer_backend_types: Defines common data types shared across the entire renderer backend layer (shader, texture, VAO, and VBO modules).
  - renderer_core/renderer_err_utils: Provides utilities to translate lower-layer error codes into renderer subsystem error codes, and to convert renderer subsystem error codes into human-readable strings.
  - renderer_core/renderer_memory: Wrapper APIs over **engine/core/choco_memory** tailored for the renderer layer (renderer-specific result codes and automatic memory-tag assignment) to simplify allocation/free within the renderer.
  - renderer_core/renderer_types: Common data types shared across the renderer subsystem.
  - renderer_resources: Provides renderer-level resource modules used by higher layers. Currently, this layer contains ui_shader, which encapsulates a shader program together with cached uniform locations and shader-specific operations.

### engine/systems/camera_system

- Purpose: The `Camera System` is a subsystem that provides camera state management and control functionality in three-dimensional space.
  It provides upper layers with a unified API for creating, retrieving, and deleting cameras, as well as for handling position, orientation, view matrices, and projection matrices.
  Control functionality for each camera type is also included in the responsibilities of this system.
  As a result, upper layers can use camera functionality without being aware of the details of individual camera implementations or internal memory management.
- Characteristics: Some modules require explicit initialization. In particular, camera_manager must be initialized before camera instances can be centrally registered, retrieved, and managed.
- Modules:
  - camera_controller/flight_camera_controller: Provides control APIs for flight-camera movement and orientation updates.
  - camera_manager: Manages camera instances and provides registration, deletion, and retrieval APIs.
  - camera: Holds camera state such as name, position, orientation, and projection parameters, and provides APIs for retrieving matrices and direction vectors.
  - camera_core/camera_memory: Wrapper APIs over `choco_memory` tailored for the camera layer.
  - camera_core/camera_err_utils: Provides utilities to translate lower-layer error codes into camera-system result codes and to convert them into strings.
  - camera_core/camera_types: Common data types, constants, and result codes used throughout the camera system.

### engine/systems/texture_system

- Purpose: A system for managing CPU-side and GPU-side texture resources. It provides the following functionality:
  - Manages CPU-side texture resources through `engine/resource/texture`.
  - Manages GPU-side texture resources through the Texture API of `renderer_backend`.
  - Provides registration, deletion, and retrieval APIs for CPU/GPU texture resources by texture name and texture ID.
  - Allocates the module's memory resources using a linear allocator during initialization.
- Characteristics: Requires explicit initialization. Once initialized, the texture subsystem remains active for the lifetime of the application (until shutdown).
- Modules:
  - texture_manager: Manages the correspondence between CPU-side texture resources and GPU-side texture resources, and provides registration, deletion, and retrieval APIs by texture name and texture ID.
