@page arch_renderer_system_en Renderer System Architecture(English)

# Renderer System Architecture

`Renderer System` is a subsystem that provides rendering functionality to higher layers, and is composed of the following sublayers.

- `Renderer Backend`, which provides graphics-API-independent interface APIs and allows the graphics API backend implementation to be replaced
- `Renderer Core`, which provides modules used throughout the entire `Renderer System`
- `Renderer Resources`, which provides renderer-layer rendering resource modules intended for use by higher layers

In addition, `Renderer Frontend` is also planned to be added, but it is currently not implemented.

![Layer Diagram](layer.png)

## Sublayer Details

### Renderer Backend

[Renderer Backend Architecture](renderer_backend/architecture_en.md)

### Renderer Core

`Renderer Core` provides the following modules.

| Module Name        | Role |
| ------------------ | ---- |
| renderer_err_utils | Used throughout the entire `Renderer System`, and provides functionality for converting lower-layer result codes and converting result codes into strings |
| renderer_memory    | Provides wrapper APIs over `core/choco_memory` to prevent misuse of memory tags within the `Renderer System` and reduce unnecessary result-code conversion handling |
| renderer_types     | Provides graphics-API-independent data types used within the `Renderer System` |

### Renderer Resources

`Renderer Resources` currently provides the following module.

| Module Name | Role |
| ----------- | ---- |
| ui_shader   | Provides creation, usage, and related uniform operations for UI rendering shader resources |
