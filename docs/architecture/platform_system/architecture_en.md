# Platform System Architecture

## Purpose and Scope

`Platform System` is a subsystem that provides a swappable, platform-agnostic interface API so that application developers can build graphics applications without being aware of the underlying platform.

## Platform System Concept

To achieve this goal, `Platform System` applies the object-oriented design pattern **Strategy**.
The Strategy pattern typically has the following structure:

```mermaid
classDiagram
    Context
    Interface
    Concrete1
    Concrete2

    Context : + context_method()

    Interface : + strategy_method()
    Concrete1 : + strategy_method()
    Concrete2 : + strategy_method()

    Context --> Interface
    Interface <|-- Concrete1
    Interface <|-- Concrete2
```

The correspondence between the Strategy objects and GLCE modules is as follows.

| Strategy Object | GLCE Module        | Role                                                                                                                |
| --------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------- |
| Context         | platform_context   | Provides API entry points for the functionality owned by `Platform System` to upper layers                          |
| Interface       | platform_interface | Provides a per-platform swappable virtual function table (vtable) to the Context (holding abstracted platform APIs) |
| Concrete1       | platform_glfw      | Provides the GLFW-based vtable implementation for the Interface and its internal implementation                     |
| Concrete2       | Not implemented    | Added when supported platforms increase                                                                             |

In addition, `Platform System` provides the following modules that support the Strategy-based design.

| Module             | Role                                                                                                                                                                                        |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| platform_err_utils | For all `Platform System` modules, provides (1) conversion from lower-layer result codes to `Platform System` result codes, and (2) conversion of `Platform System` result codes to strings |
| platform_types     | Provides common data types shared across all `Platform System` modules                                                                                                                      |

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

### Platform Concrete Selection (Current)

Currently, the platform to use is selected by specifying it when creating an instance of `Platform System`.

### Platform Concrete Selection (Future)

With the current specification, all platform concrete implementations must be buildable, which is difficult to achieve. In the future, the system will migrate to a build-option-based approach where the platform is selected via build options.

## Design / Internal Structure / Usage Flow

`Platform Context` exposes `platform_context_t` to upper layers as the structure that manages the internal state of the system (only the type name is exposed; the internal structure is not public).
An instance of `platform_context_t` is held by the application-layer internal state structure `app_state_t` under the instance name `platform_context`.
The application layer is also responsible for allocating and releasing the resources of the `platform_context` instance.

`Platform System` is brought up at system startup and stays resident until system shutdown.
Therefore, since it does not release resources during execution, the memory resources for `Platform System` use the Linear Allocator.
The Linear Allocator held by `app_state_t` as `linear_alloc` serves as the subsystem allocator, and `Platform System` uses it.

The usage flow is as follows.

| Phase               | Operation                                 | Method                                                                                                                       |
| ------------------- | ----------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| Initialization      | Allocate resources for `platform_context` | Call `platform_initialize()` in `application_create()`                                                                       |
| Initialization      | Create the rendering window               | Call `platform_window_create()` in `application_create()` (planned to be moved after `renderer_frontend` is created)         |
| Shutdown            | Release resources for `platform_context`  | Call `platform_destroy()` in `application_destroy()`                                                                         |
| Runtime (per-frame) | Pump events from the Platform layer       | Call `platform_pump_messages()` in `application_run()` (per-frame)                                                           |
| Runtime (per-frame) | Swap buffers via double buffering         | Call `platform_swap_buffers()` in `application_run()` (per-frame) (planned to be moved after `renderer_frontend` is created) |

For event pumping via `platform_pump_messages()`, refer to the [Event System Guide](../../guide/event_system/event_en.md).

## Currently Unsupported Items

The following items are not supported at this time. They may be supported as GLCE evolves, as needed.

- Thread-safe API support
- Runtime switching of the active platform

## Configuration

No configuration options are available at this time.

## References

When adding a supported platform, refer to the [Platform System Guide](../../guide/platform/adding_concretes_en.md).
