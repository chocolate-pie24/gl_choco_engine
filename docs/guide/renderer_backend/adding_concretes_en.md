@page guide_renderer_system_en Renderer System Guide(English)

# Guidelines for Adding a Concrete Module to the Renderer Backend

This page provides guidelines for engine developers to safely add support for additional graphics APIs.
For now, the graphics APIs to be added are assumed to be versions other than OpenGL 3.3. Note that using Vulkan will likely require some design changes.

For an overview of the `Renderer Backend`, refer to [Renderer Backend architecture](../../architecture/renderer_system/renderer_backend/architecture_en.md).

## Guidelines for Adding Support for a New Graphics API

- Add the new graphics API name to `target_graphics_api_t` in `renderer_types.h`.
- Enable the added graphics API in `graphics_api_valid_check()` in `renderer_backend/renderer_backend_context/context.c`.
- Create a directory for the new graphics API under `include/engine/renderer/renderer_backend/renderer_backend_concretes/`.
- Under the added directory, create `concrete_shader.h`, `concrete_vao.h`, and `concrete_vbo.h`.
  (GLCE does not assume mixing multiple graphics APIs. Always create these three as a set.)

In `concrete_shader.h`, `concrete_vao.h`, and `concrete_vbo.h`, define the following APIs (`xxx` is the graphics API identifier):

| Header              | Additional API            | Role                                                       |
| ------------------- | ------------------------- | ---------------------------------------------------------- |
| `concrete_shader.h` | `xxx_shader_vtable_get()` | Provide the virtual function table for shader functionality |
| `concrete_vao.h`    | `xxx_vao_vtable_get()`    | Provide the virtual function table for VAO functionality    |
| `concrete_vbo.h`    | `xxx_vbo_vtable_get()`    | Provide the virtual function table for VBO functionality    |

- Create a directory for the new graphics API under `src/engine/renderer/renderer_backend/renderer_backend_concretes/`.
- Under the added directory, create `concrete_shader.c`, `concrete_vao.c`, and `concrete_vbo.c`.

### Steps to Create concrete_shader.c

In `concrete_shader.c`, define the `renderer_backend_shader_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the linked OpenGL shader program
- A handle to the compiled vertex shader object
- A handle to the compiled fragment shader object

In the shader concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_shader_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `render_mem_allocate()` and `render_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category          | Function Name            | Role                                                                                         |
| ---------------- | ------------------------ | -------------------------------------------------------------------------------------------- |
| Lifecycle         | `xxx_shader_create()`    | Allocate memory for a `renderer_backend_shader_t` instance and zero-initialize its fields    |
| Lifecycle         | `xxx_shader_destroy()`   | Notify the graphics API to stop the shader program and free the `renderer_backend_shader_t` instance |
| Shader operations | `xxx_shader_compile()`   | Compile the shader object                                                                   |
| Shader operations | `xxx_shader_link()`      | Link compiled shader objects into a shader program.                                         |
| Shader operations | `xxx_shader_use()`       | Begin using the linked shader program                                                        |

- Add the vtable for the new graphics API to `shader_vtable_get()` in `renderer_backend/renderer_backend_context/context.c`.

### Steps to Create concrete_vao.c

In `concrete_vao.c`, define the `renderer_backend_vao_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the created VAO

In the VAO concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_vao_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `render_mem_allocate()` and `render_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category        | Function Name              | Role                                                                                         |
| -------------- | -------------------------- | -------------------------------------------------------------------------------------------- |
| Lifecycle       | `xxx_vao_create()`         | Allocate memory for a `renderer_backend_vao_t` instance and zero-initialize its fields       |
| Lifecycle       | `xxx_vao_destroy()`        | Delete the VAO and free the `renderer_backend_vao_t` instance                                |
| VAO operations  | `xxx_vao_bind()`           | Bind the VAO                                                                                 |
| VAO operations  | `xxx_vao_unbind()`         | Unbind the VAO                                                                               |
| VAO operations  | `xxx_vao_attribute_set()`  | Configure VBO attribute data                                                                 |

- Add the vtable for the new graphics API to `vao_vtable_get()` in `renderer_backend/renderer_backend_context/context.c`.

### Steps to Create concrete_vbo.c

In `concrete_vbo.c`, define the `renderer_backend_vbo_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the created VBO

In the VBO concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_vbo_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `render_mem_allocate()` and `render_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category        | Function Name             | Role                                                                                          |
| -------------- | ------------------------- | --------------------------------------------------------------------------------------------- |
| Lifecycle       | `xxx_vbo_create()`        | Allocate memory for a `renderer_backend_vbo_t` instance and zero-initialize its fields        |
| Lifecycle       | `xxx_vbo_destroy()`       | Delete the VBO and free the `renderer_backend_vbo_t` instance                                 |
| VBO operations  | `xxx_vbo_bind()`          | Bind the VBO                                                                                  |
| VBO operations  | `xxx_vbo_unbind()`        | Unbind the VBO                                                                                |
| VBO operations  | `xxx_vbo_vertex_load()`   | Upload vertex data to the GPU                                                                 |

- Add the vtable for the new graphics API to `vbo_vtable_get()` in `renderer_backend/renderer_backend_context/context.c`.

### Specifying the Newly Added Graphics API

To use a newly added graphics API, specify the graphics API type in `renderer_backend_initialize()` inside `application_create()` (*1).

*1: In the future, this will be changed so that the graphics API is specified via a build option.