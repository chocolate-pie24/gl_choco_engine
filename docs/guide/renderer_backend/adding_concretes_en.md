@page guide_renderer_backend_en Renderer System Guide(English)

# Guidelines for Adding Concrete Modules to the Renderer Backend

This page provides guidelines for engine developers to safely add support for additional graphics APIs.
For the time being, additional graphics APIs are assumed to be versions other than OpenGL 3.3. Note that using Vulkan is expected to require some design changes.

For an overview of the `Renderer Backend`, refer to [Renderer Backend architecture](../../architecture/renderer_system/renderer_backend/architecture_en.md).

## Guidelines for Adding Support for a New Graphics API

- Add the new graphics API name to `target_graphics_api_t` in `renderer_types.h`.
- Enable the added graphics API in `graphics_api_valid_check()` in `renderer_backend/renderer_backend_context/renderer_backend_context.c`.
- Create a directory for the new graphics API under `include/engine/systems/renderer/renderer_backend/renderer_backend_concretes/`.
- Under the added directory, create `concrete_shader.h`, `concrete_texture.h`, `concrete_vao.h`, and `concrete_vbo.h`.
  (GLCE does not assume mixing multiple graphics APIs. Always create these four as a set.)

In `concrete_shader.h`, `concrete_texture.h`, `concrete_vao.h`, and `concrete_vbo.h`, define the following APIs (`xxx` is the graphics API identifier):

| Header               | Additional API             | Role                                                            |
| -------------------- | -------------------------- | --------------------------------------------------------------- |
| `concrete_shader.h`  | `xxx_shader_vtable_get()`  | Provides the virtual function table for shader functionality    |
| `concrete_texture.h` | `xxx_texture_vtable_get()` | Provides the virtual function table for texture functionality   |
| `concrete_vao.h`     | `xxx_vao_vtable_get()`     | Provides the virtual function table for VAO functionality       |
| `concrete_vbo.h`     | `xxx_vbo_vtable_get()`     | Provides the virtual function table for VBO functionality       |

- Create a directory for the new graphics API under `src/engine/systems/renderer/renderer_backend/renderer_backend_concretes/`.
- Under the added directory, create `concrete_shader.c`, `concrete_texture.c`, `concrete_vao.c`, and `concrete_vbo.c`.

### Steps to Create concrete_shader.c

In `concrete_shader.c`, define the `renderer_backend_shader_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the linked OpenGL shader program
- A handle to the compiled vertex shader object
- A handle to the compiled fragment shader object

In the shader concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_shader_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `renderer_mem_allocate()` and `renderer_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category          | Function Name                         | Role                                                                                              |
| ----------------- | ------------------------------------- | ------------------------------------------------------------------------------------------------- |
| Lifecycle         | `xxx_shader_create()`                 | Allocates memory for a `renderer_backend_shader_t` instance and zero-initializes its fields       |
| Lifecycle         | `xxx_shader_destroy()`                | Notifies the graphics API to stop using the shader program and frees the `renderer_backend_shader_t` instance |
| Shader operations | `xxx_shader_compile()`                | Compiles a shader object                                                                          |
| Shader operations | `xxx_shader_link()`                   | Links compiled shader objects                                                                     |
| Shader operations | `xxx_shader_use()`                    | Begins using the linked shader program                                                            |
| Shader operations | `xxx_shader_uniform_location_get()`   | Gets the location of a uniform variable in the shader program                                      |
| Shader operations | `xxx_shader_mat4f_uniform_set()`      | Sends a `mat4f` uniform value to the shader program                                                |

- Add the vtable for the new graphics API to `shader_vtable_get()` in `renderer_backend/renderer_backend_context/renderer_backend_context.c`.

### Steps to Create concrete_texture.c

In `concrete_texture.c`, define the `renderer_backend_texture_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the GPU-side texture resource
- The texture slot number referenced by the shader (a zero-based texture unit index used as `GL_TEXTURE0 + unit_num_`, not an enum value such as `GL_TEXTURE0`)
- Pixel interpolation settings used when reducing the texture
- Pixel interpolation settings used when enlarging the texture
- Pixel settings for texture wrapping on the s-axis
- Pixel settings for texture wrapping on the t-axis

In the texture concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_texture_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `renderer_mem_allocate()` and `renderer_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category           | Function Name                  | Role                                                                                         |
| ------------------ | ------------------------------ | -------------------------------------------------------------------------------------------- |
| Lifecycle          | `xxx_texture_create()`         | Allocates memory for the GPU-side texture resource structure, applies texture settings, and initializes it |
| Lifecycle          | `xxx_texture_destroy()`        | Releases the resources held by the GPU texture resource structure and frees its own memory    |
| Texture operations | `xxx_texture_bind()`           | Activates and binds the texture                                                              |
| Texture operations | `xxx_texture_unbind()`         | Activates and unbinds the texture                                                            |
| Texture operations | `xxx_texture_pixel_upload()`   | Transfers pixel data to the currently active and bound 2D texture target on the GPU           |

- Add the vtable for the new graphics API to `texture_vtable_get()` in `renderer_backend/renderer_backend_context/renderer_backend_context.c`.

### Steps to Create concrete_vao.c

In `concrete_vao.c`, define the `renderer_backend_vao_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the created VAO

In the VAO concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_vao_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `renderer_mem_allocate()` and `renderer_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category       | Function Name             | Role                                                                                         |
| -------------- | ------------------------- | -------------------------------------------------------------------------------------------- |
| Lifecycle      | `xxx_vao_create()`        | Allocates memory for a `renderer_backend_vao_t` instance, creates the API-specific VAO resource, and initializes it |
| Lifecycle      | `xxx_vao_destroy()`       | Deletes the VAO and frees the `renderer_backend_vao_t` instance                              |
| VAO operations | `xxx_vao_bind()`          | Binds the VAO                                                                                 |
| VAO operations | `xxx_vao_unbind()`        | Unbinds the VAO                                                                               |
| VAO operations | `xxx_vao_attribute_set()` | Configures VBO attribute data                                                                 |

- Add the vtable for the new graphics API to `vao_vtable_get()` in `renderer_backend/renderer_backend_context/renderer_backend_context.c`.

### Steps to Create concrete_vbo.c

In `concrete_vbo.c`, define the `renderer_backend_vbo_t` struct.
Its fields are specific to the graphics API being added, but for example, the OpenGL 3.3 backend stores the following:

- A handle to the created VBO

In the VBO concrete module implementation file, implement the following functionality as the `Renderer Backend Interface` virtual function table (`renderer_vbo_vtable_t`) (`xxx` is the name of the graphics API).

When implementing each function, keep the following in mind:

- If memory allocation is required, use `renderer_mem_allocate()` and `renderer_mem_free()` provided by `renderer_core/renderer_memory`.
- For converting result codes to strings and translating lower-layer result codes, use the APIs provided by `renderer_core/renderer_err_utils`.

| Category       | Function Name                  | Role                                                                                         |
| -------------- | ------------------------------ | -------------------------------------------------------------------------------------------- |
| Lifecycle      | `xxx_vbo_create()`             | Allocates memory for a `renderer_backend_vbo_t` instance, creates the API-specific VBO resource, and initializes it |
| Lifecycle      | `xxx_vbo_destroy()`            | Deletes the VBO and frees the `renderer_backend_vbo_t` instance                              |
| VBO operations | `xxx_vbo_bind()`               | Binds the VBO                                                                                 |
| VBO operations | `xxx_vbo_unbind()`             | Unbinds the VBO                                                                               |
| VBO operations | `xxx_vbo_vertex_load()`        | Transfers vertex data to the GPU                                                             |
| VBO operations | `xxx_vbo_vertex_subload()`     | Transfers vertex data to an already-created GPU-side vertex data storage area at the specified offset |

- Add the vtable for the new graphics API to `vbo_vtable_get()` in `renderer_backend/renderer_backend_context/renderer_backend_context.c`.

### Specifying the Newly Added Graphics API

To use a newly added graphics API, specify the graphics API type in `renderer_backend_initialize()` inside `application_create()` (*1).

*1: In the future, this will be changed so that the graphics API is specified via a build option.
