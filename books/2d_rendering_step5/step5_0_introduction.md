---
title: "step5_0: MVP行列の導入と、カメラシステムの構築"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook5に対応しています。

前回は、Renderer Backendを追加し、シンプルな三角形を描画できるようにしました。

今回は、キーボード操作によって描画視点を変更できるようにし、三角形を様々な位置から見れるようにしていきます。

## Step5解説

- step5_1: [Model, View, Projection行列の導入](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_1_mvp_matrix)
- step5_2: [Camera Systemアーキテクチャ](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_2_camera_system_architecture)
- step5_3: [Cameraモジュールの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_3_camera_module)
- step5_4: [Camera制御モジュールの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_4_camera_control_module)
- step5_5: [Cameraシステムの構築](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_5_camera_system)
- step5_6: [カメラ制御コマンドとキー操作イベントの連携](https://zenn.dev/chocolate_pie24/books/2d_rendering_step5/viewer/step5_6_control_command_interpreter)

## レイヤー構成図

今回の変更によって、レイヤー構成図は以下のように変化しました。

- 青: 変更あり
- 緑: 新規追加

### GL Choco Engine OverView

```mermaid
graph TD
  subgraph APPLICATION_LAYER[application_layer]
    APPLICATION[application]
    subgraph APPLICATION_CORE[application_core]
        direction TB
        APPLICATION_ERR_UTILS[application_err_utils]
        APPLICATION_TYPES[application_types]
    end
    style APPLICATION_ERR_UTILS fill:#8BC34A
    style APPLICATION_TYPES fill:#8BC34A

    subgraph COMMAND_INTERPRETER[command_interpreter]
        direction TB
        FLIGHT_CAMERA[flight_camera]
    end
    style FLIGHT_CAMERA fill:#8BC34A
  end

  subgraph ENGINE[engine_layer]
    subgraph SYSTEMS[systems]
      PLATFORM[platform]
      RENDERER[renderer]
      CAMERA[camera]
    end
    style CAMERA fill:#8BC34A
    style RENDERER fill:#2196F3

    CONTAINERS[containers]
    IO_UTILS[io_utils]
  end

  APPLICATION --> APPLICATION_ERR_UTILS
  APPLICATION --> APPLICATION_TYPES
  APPLICATION --> FLIGHT_CAMERA
  FLIGHT_CAMERA --> APPLICATION_ERR_UTILS
  FLIGHT_CAMERA --> APPLICATION_TYPES
  FLIGHT_CAMERA --> CAMERA

  APPLICATION --> PLATFORM
  APPLICATION --> RENDERER
  APPLICATION --> CAMERA

  PLATFORM --> CONTAINERS
  RENDERER --> CONTAINERS
  RENDERER --> IO_UTILS
  CAMERA --> CONTAINERS
```

### Renderer System

```mermaid
graph TD
  subgraph RENDERER_RESOURCES[renderer_resources]
    direction TB
    UI_SHADER[ui_shader]
  end
  style UI_SHADER fill:#8BC34A

  subgraph RENDERER_CORE[renderer_core]
    direction TB
    RENDERER_ERR_UTILS[renderer_err_utils]
    RENDERER_MEMORY[renderer_memory]
    RENDERER_TYPES[renderer_types]
  end
  style RENDERER_ERR_UTILS fill:#2196F3

  subgraph RENDERER_BACKEND[renderer_backend]
    direction TB
    RENDERER_BACKEND_TYPES[renderer_backend_types]

    subgraph RENDERER_BACKEND_INTERFACE[renderer_backend_interface]
      direction TB
      INTERFACE_SHADER[interface_shader]
      INTERFACE_VAO[interface_vao]
      INTERFACE_VBO[interface_vbo]
    end
    style INTERFACE_SHADER fill:#2196F3

    subgraph RENDERER_BACKEND_CONCRETES[renderer_backend_concretes]
      direction TB
      subgraph GL33
        direction TB
        CONCRETE_SHADER[concrete_shader]
        CONCRETE_VAO[concrete_vao]
        CONCRETE_VBO[concrete_vbo]
      end
    end
    style CONCRETE_SHADER fill:#2196F3

    subgraph RENDERER_BACKEND_CONTEXT[renderer_backend_context]
      direction TB
      CONTEXT[context]
    end
    style CONTEXT fill:#2196F3
  end

  RENDERER_RESOURCES --> CONTEXT
  RENDERER_RESOURCES --> RENDERER_BACKEND_TYPES
  RENDERER_RESOURCES --> RENDERER_ERR_UTILS
  RENDERER_RESOURCES --> RENDERER_MEMORY

  RENDERER_BACKEND_INTERFACE --> RENDERER_BACKEND_TYPES
  %% RENDERER_BACKEND_INTERFACE --> RENDERER_TYPES

  RENDERER_BACKEND_CONCRETES --> RENDERER_BACKEND_INTERFACE
  RENDERER_BACKEND_CONCRETES --> RENDERER_BACKEND_TYPES
  %% RENDERER_BACKEND_CONCRETES --> RENDERER_CORE

  %% CONTEXT --> RENDERER_CORE
  CONTEXT --> RENDERER_BACKEND_TYPES
  CONTEXT --> GL33
  CONTEXT --> RENDERER_BACKEND_INTERFACE

  RENDERER_BACKEND --> RENDERER_CORE
```

### Camera System

このレイヤーは全モジュールが新規のため色分けはしていません。

```mermaid
graph TD
    subgraph CAMERA_CONTROLLER[camera_controller]
        direction TB
        FLIGHT_CAMERA_CONTROLLER[flight_camera_controller]
    end

    CAMERA_MANAGER[camera_manager]
    CAMERA[camera]

    subgraph CAMERA_CORE[camera_core]
        direction TB
        CAMERA_MEMORY[camera_memory]
        CAMERA_ERR_UTILS[camera_err_utils]
        CAMERA_TYPES[camera_types]
    end

    FLIGHT_CAMERA_CONTROLLER --> CAMERA_TYPES
    FLIGHT_CAMERA_CONTROLLER --> CAMERA
    FLIGHT_CAMERA_CONTROLLER --> CAMERA_ERR_UTILS

    CAMERA_MANAGER --> CAMERA
    CAMERA_MANAGER --> CAMERA_ERR_UTILS
    CAMERA_MANAGER --> CAMERA_TYPES

    CAMERA --> CAMERA_MEMORY
    CAMERA --> CAMERA_TYPES
    CAMERA --> CAMERA_ERR_UTILS

    CAMERA_MEMORY --> CAMERA_TYPES

    CAMERA_ERR_UTILS --> CAMERA_TYPES
```
