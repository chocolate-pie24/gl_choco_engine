# Guidelines for Adding a Concrete Module to the Platform System

This page provides guidelines for engine developers to safely add support for new platforms.

For an overview of the `Platform System`, see [Platform System architecture](../../architecture/platform_system/architecture_en.md).

## Guidelines for Adding a Supported Platform

- Add the new platform name to `platform_type_t` in `platform_types.h`.
- Add a header file for the new concrete module under `include/platform/platform_concretes/`.
- Add an implementation file for the new concrete module under `src/platform/platform_concretes/`, and define the fields of `platform_backend_t`.

`platform_backend_t` must provide fields that allow upper layers to obtain the following information (upper layers obtain them through the public APIs of `platform_context` and must not directly access the backend struct):

- Mouse position (x, y)
- Window width and height
- Framebuffer size (width, height)
- Mouse button pressed states
- Keyboard key pressed states
- Window title string (stored as `choco_string_t`)

In the concrete module implementation file, implement the following functions as the virtual function table (vtable) for the `Platform Interface`.

| Category       | Function Name                | Responsibility                                                                                           |
| -------------- | ---------------------------- | -------------------------------------------------------------------------------------------------------- |
| Lifecycle      | `platform_preinit`           | Provide the upper layer with the size and alignment requirements of the internal state struct `platform_backend_t`. |
| Lifecycle      | `platform_init`              | Initialize the platform API (if required) and initialize `platform_backend_t`.                           |
| Lifecycle      | `platform_destroy`           | Shut down the platform API (if required) and release resources held by `platform_backend_t`.            |
| Window         | `platform_window_create`     | Create a window.                                                                                         |
| Window         | `platform_swap_buffers`      | Perform buffer swapping for double buffering.                                                            |
| Event pipeline | `platform_snapshot_collect`  | Collect events.                                                                                           |
| Event pipeline | `platform_snapshot_process`  | Process collected events and convert them into a format that upper layers can handle.                    |
| Event pipeline | `platform_pump_messages`     | Deliver events to the callback functions provided by upper layers.                                       |

- Add the new platform vtable to `platform_vtable_get()` in `platform_context.c`.
- Enable the new platform in `platform_type_valid_check()` in `platform_context.c`.
- To use the newly added platform, specify the platform type in `platform_initialize()` within `application_create()` (*1).

*1: This is planned to be changed to a build-option-based selection in the future.
