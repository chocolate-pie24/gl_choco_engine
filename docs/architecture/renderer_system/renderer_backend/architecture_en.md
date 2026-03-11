@page arch_renderer_system_en Renderer Backend Architecture(English)

# Renderer Backend architecture

## Purpose and positioning

`Renderer Backend` is a subsystem that provides a swappable, graphics-API-agnostic interface so that application developers can build graphics applications without being aware of the underlying graphics API.

## Renderer Backend concept

To achieve its goal, `Renderer Backend` applies the object-oriented design pattern *Strategy*.

The functionality provided by `Renderer Backend` is broadly categorized into three groups: VAO, VBO, and Shader. For each group, an Interface is defined to absorb differences between graphics APIs.
On the other hand, the entry point for upper layers (e.g., the application layer) to use `Renderer Backend` is kept simple, so only one Context is used.

VAO, VBO, and Shader are assumed to use the same graphics API and are selected as a set (mixing different APIs is not assumed).

Based on these assumptions, the Strategy pattern has the following structure.

![strategy](./strategy.png)

The correspondence between the Strategy objects and GLCE modules is as follows:

| Strategy Object | GLCE Module                                     | Role                                                                                                                                      |
| --------------- | ----------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| Context(*1)     | renderer_backend_context/context                | Provides the API entry point for initializing and shutting down the `Renderer Backend` among the features owned by `Renderer Backend`.    |
|                 | renderer_backend_context/context_shader         | Provides the API entry point for `Shader`-related functionality among the features owned by `Renderer Backend`.                           |
|                 | renderer_backend_context/context_vao            | Provides the API entry point for `VAO`-related functionality among the features owned by `Renderer Backend`.                              |
|                 | renderer_backend_context/context_vbo            | Provides the API entry point for `VBO`-related functionality among the features owned by `Renderer Backend`.                              |
| ShaderInterface | renderer_backend_interface/interface_shader     | Provides a graphics-API-swappable virtual function table for Shader-related functionality (an API that abstracts Shader features) to Context. |
| VAOInterface    | renderer_backend_interface/interface_vao        | Provides a graphics-API-swappable virtual function table for VAO-related functionality (an API that abstracts VAO features) to Context.      |
| VBOInterface    | renderer_backend_interface/interface_vbo        | Provides a graphics-API-swappable virtual function table for VBO-related functionality (an API that abstracts VBO features) to Context.      |
| ShaderConcrete1 | renderer_backend_concretes/gl33/concrete_shader | Provides an OpenGL 3.3 implementation vtable and its internal implementation for the Interface.                                           |
| VAOConcrete1    | renderer_backend_concretes/gl33/concrete_vao    | Provides an OpenGL 3.3 implementation vtable and its internal implementation for the Interface.                                           |
| VBOConcrete1    | renderer_backend_concretes/gl33/concrete_vbo    | Provides an OpenGL 3.3 implementation vtable and its internal implementation for the Interface.                                           |
| ShaderConcrete2 | Not implemented                                 | Will be added when support for additional graphics APIs is introduced.                                                                    |
| VAOConcrete2    | Not implemented                                 | Will be added when support for additional graphics APIs is introduced.                                                                    |
| VBOConcrete2    | Not implemented                                 | Will be added when support for additional graphics APIs is introduced.                                                                    |

*1: The Context is split into `context`, `context_shader`, `context_vao`, and `context_vbo` header files to improve readability of the public API definitions, but all implementations are located in `renderer_backend_context/context.c`.

### Selecting Concretes (Shader, VAO, VBO) (Current)

Currently, the graphics API to be used is selected by specifying it when creating an instance of the `Renderer Backend modules` .

### Selecting Concretes (Shader, VAO, VBO) (Future)

With the current design, it would be required that the internal implementations for all supported graphics APIs can be built, which is difficult to achieve in practice. In the future, the selection will be moved to build-time, by specifying the target graphics API via build options.

## Current Unsupported Items

At present, the following are not supported. They may be implemented as needed as GLCE evolves.

- Providing thread-safe APIs
- Switching the graphics API at runtime
- Mixing multiple graphics APIs

## Configuration

There are no configuration options at this time.

## References

To add support for a new graphics API, see [Renderer System Guide](../../../guide/renderer/adding_concretes_en.md).
