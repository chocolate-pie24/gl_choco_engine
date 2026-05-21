@page arch_renderer_backend_en Renderer Backend Architecture(English)

# Renderer Backend Architecture

## Purpose and positioning

`Renderer Backend` is a subsystem that provides a swappable, graphics-API-independent interface layer so that application developers can build graphics applications without depending directly on a specific graphics API.

## Renderer Backend concept

To achieve this goal, `Renderer Backend` uses the Strategy pattern from object-oriented design.

The functionality provided by `Renderer Backend` is broadly categorized into four groups: Shader, Texture, VAO, and VBO. For each group, an interface is defined to absorb differences between graphics APIs.
On the other hand, the upper-layer entry point for using `Renderer Backend` is kept simple, so a single conceptual Context is used.
Shader, Texture, VAO, and VBO are assumed to use the same graphics API and are selected as a set. Mixing different graphics APIs is not assumed.

Based on these assumptions, the Strategy pattern has the following structure.

![strategy](./strategy.png)

The correspondence between the Strategy objects and GLCE modules is as follows. Conceptually, there is only one Context. However, the public API headers are split by functionality to improve readability.

| Strategy Object  | GLCE Module                                                   | Role                                                                                                                        |
| ---------------- | ------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| Context(*1)      | renderer_backend_context/renderer_backend_context             | Provides the API entry point for initializing and shutting down `Renderer Backend`.                                          |
|                  | renderer_backend_context/context_shader                       | Provides the upper-layer API entry point for `Shader`-related functionality.                                                 |
|                  | renderer_backend_context/context_texture                      | Provides the upper-layer API entry point for `Texture`-related functionality.                                                |
|                  | renderer_backend_context/context_vao                          | Provides the upper-layer API entry point for `VAO`-related functionality.                                                    |
|                  | renderer_backend_context/context_vbo                          | Provides the upper-layer API entry point for `VBO`-related functionality.                                                    |
| ShaderInterface  | renderer_backend_interface/interface_shader                   | Defines the Shader vtable used by Context to dispatch to the selected graphics API implementation.                           |
| TextureInterface | renderer_backend_interface/interface_texture                  | Defines the Texture vtable used by Context to dispatch to the selected graphics API implementation.                          |
| VAOInterface     | renderer_backend_interface/interface_vao                      | Defines the VAO vtable used by Context to dispatch to the selected graphics API implementation.                              |
| VBOInterface     | renderer_backend_interface/interface_vbo                      | Defines the VBO vtable used by Context to dispatch to the selected graphics API implementation.                              |
| ShaderConcrete1  | renderer_backend_concretes/gl33/concrete_shader               | Provides the OpenGL 3.3 Shader vtable and its internal implementation.                                                       |
| TextureConcrete1 | renderer_backend_concretes/gl33/concrete_texture              | Provides the OpenGL 3.3 Texture vtable and its internal implementation.                                                      |
| VAOConcrete1     | renderer_backend_concretes/gl33/concrete_vao                  | Provides the OpenGL 3.3 VAO vtable and its internal implementation.                                                          |
| VBOConcrete1     | renderer_backend_concretes/gl33/concrete_vbo                  | Provides the OpenGL 3.3 VBO vtable and its internal implementation.                                                          |
| ShaderConcrete2  | Not implemented                                               | Will be added when support for additional graphics APIs is introduced.                                                       |
| TextureConcrete2 | Not implemented                                               | Will be added when support for additional graphics APIs is introduced.                                                       |
| VAOConcrete2     | Not implemented                                               | Will be added when support for additional graphics APIs is introduced.                                                       |
| VBOConcrete2     | Not implemented                                               | Will be added when support for additional graphics APIs is introduced.                                                       |

*1: The Context public API definitions are split into `renderer_backend_context`, `context_shader`, `context_texture`, `context_vao`, and `context_vbo` header files to improve readability, but all implementations are located in `renderer_backend_context/renderer_backend_context.c`.

### Selecting Concretes (Shader, Texture, VAO, VBO) (Current)

Currently, the graphics API is selected when the `Renderer Backend` context is initialized. The selected target API determines the Shader, Texture, VAO, and VBO vtables stored in the context.

### Selecting Concretes (Shader, Texture, VAO, VBO) (Future)

With the current design, all supported graphics API concrete implementations must be buildable so that context initialization can select the appropriate vtables at runtime. This is difficult to achieve in practice. In the future, the selection will be moved to build time by specifying the target graphics API via build options.

## Current unsupported items

At present, the following items are not supported. They may be implemented as needed as GLCE evolves.

- Providing thread-safe APIs
- Switching the graphics API at runtime
- Mixing multiple graphics APIs

## Configuration

There are no configuration options at this time.

## References

To add support for a new graphics API, see [Renderer System Guide](../../../guide/renderer_backend/adding_concretes_en.md).
