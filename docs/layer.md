# gl_choco_engineレイヤー構成

## レイヤツリー

### ベーシック
| 色名 | Hex |
|---|---|
| White | #FFFFFF |
| Near-white | #F5F5F5 |
| Light Gray | #E0E0E0 |
| Gray | #9E9E9E |
| Dark Gray | #424242 |
| Black | #000000 |

### ブルー系
| 色名 | Hex |
|---|---|
| Blue | #2196F3 |
| Light Blue | #03A9F4 |
| Indigo | #3F51B5 |
| Navy | #000080 |
| Blue Grey | #607D8B |

### シアン／ティール
| 色名 | Hex |
|---|---|
| Cyan | #00BCD4 |
| Teal | #009688 |

### グリーン系
| 色名 | Hex |
|---|---|
| Green | #4CAF50 |
| Light Green | #8BC34A |
| Dark Green | #2E7D32 |
| Lime | #CDDC39 |

### イエロー／オレンジ
| 色名 | Hex |
|---|---|
| Yellow | #FFEB3B |
| Amber | #FFC107 |
| Orange | #FF9800 |
| Deep Orange | #FF5722 |

### レッド／ピンク／パープル
| 色名 | Hex |
|---|---|
| Red | #F44336 |
| Crimson | #DC143C |
| Pink | #E91E63 |
| Purple | #9C27B0 |
| Deep Purple | #673AB7 |
| Magenta | #FF00FF |


### Public(API) Dependencies

```mermaid
graph TD
  A[application]
  B[base/message]
  CML[core/memory/linear_allocator]
  CMM[core/memory/memory_system]

  A --> B
  A --> CML
  A --> CMM
  CMM --> B
  CML --> B
```

### Implementation(Private) Dependencies

TODO: applicationがplatform_registryに依存している。横方向依存。要修正

```mermaid
graph TD
  %% subgraph BASE[base]
  %%   direction TB
  %%   MACROS[macros]
  %%   MESSAGE[message]
  %% end
  %% style BASE fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core/containers
  subgraph CORE_CONTAINERS[containers]
    direction TB
    STRING[string]
    RING_QUEUE[ring_queue]
  end
  style CORE_CONTAINERS fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core/event
  subgraph CORE_EVENT[event]
    direction TB
    KEYBOARD_EVENT[keyboard_event]
    MOUSE_EVENT[mouse_event]
    WINDOW_EVENT[window_event]
  end
  style CORE_EVENT fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core/memory
  subgraph CORE_MEMORY[memory]
    direction TB
    MEMORY[memory]
    LINEAR_ALLOCATOR[linear_allocator]
  end
  style CORE_MEMORY fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core/platform
  subgraph CORE_PLATFORM[platform]
    direction TB
    PLATFORM_UTILS[platform_utils]
  end
  style CORE_PLATFORM fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core
  subgraph CORE[core]
    direction TB
    CORE_EVENT[event]
    CORE_MEMORY[memory]
    CORE_PLATFORM[platform]
  end
  style CORE fill:#E0E0E0,stroke:#1565C0,stroke-width:2px

  %% platform
  subgraph PLATFORM[platform]
    direction TB
    PLATFORM_GLFW[platform_glfw]
  end
  style PLATFORM fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% interfaces
  subgraph INTERFACES[interfaces]
    direction TB
    PLATFORM_INTERFACE[platform_interface]
  end
  style PLATFORM fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% application
  subgraph APPLICATION_LAYER[application_layer]
    direction TB
    APPLICATION[application]
    PLATFORM_REGISTRY[platform_registry]
  end
  style APPLICATION_LAYER fill:#9E9E9E,stroke:#1565C0,stroke-width:2px


  STRING --> MEMORY
  %% STRING --> MACROS
  %% STRING --> MESSAGE

  RING_QUEUE --> MEMORY
  %% RING_QUEUE --> MACROS
  %% RING_QUEUE --> MESSAGE

  PLATFORM_INTERFACE --> PLATFORM_UTILS
  PLATFORM_INTERFACE --> KEYBOARD_EVENT
  PLATFORM_INTERFACE --> MOUSE_EVENT
  PLATFORM_INTERFACE --> WINDOW_EVENT

  %% PLATFORM_GLFW --> MACROS
  %% PLATFORM_GLFW --> MESSAGE
  PLATFORM_GLFW --> PLATFORM_UTILS
  PLATFORM_GLFW --> KEYBOARD_EVENT
  PLATFORM_GLFW --> MOUSE_EVENT
  PLATFORM_GLFW --> WINDOW_EVENT
  PLATFORM_GLFW --> STRING
  PLATFORM_GLFW --> PLATFORM_INTERFACE

  PLATFORM_REGISTRY --> PLATFORM_INTERFACE
  PLATFORM_REGISTRY --> PLATFORM_GLFW
  PLATFORM_REGISTRY --> PLATFORM_UTILS

  APPLICATION --> PLATFORM_REGISTRY
  %% APPLICATION --> MACROS
  %% APPLICATION --> MESSAGE
  APPLICATION --> MEMORY
  APPLICATION --> LINEAR_ALLOCATOR
  APPLICATION --> KEYBOARD_EVENT
  APPLICATION --> MOUSE_EVENT
  APPLICATION --> WINDOW_EVENT
  APPLICATION --> PLATFORM_UTILS
  APPLICATION --> RING_QUEUE
  APPLICATION --> PLATFORM_INTERFACE
```

#### Platform System

- event/keyboard_event, event/mouse_event, event/window_eventはeventに集約
- baseレイヤーは省略

```mermaid
graph TD

  %% engine/platform/platform_context
  PLATFORM_CONTEXT[platform/platform_context]
  style PLATFORM_CONTEXT fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% engine/platform_concretes/platform_glfw
  PLATFORM_GLFW[platform_concretes/platform_glfw]
  style PLATFORM_GLFW fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% engine/interfaces/platform_interface
  PLATFORM_INTERFACE[interfaces/platform_interface]
  style PLATFORM_INTERFACE fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% engine/containers/choco_string
  CHOCO_STRING[containers/choco_string]
  style PLATFORM_INTERFACE fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  %% core
  subgraph CORE[core]
    direction TB
    PLATFORM_PLATFORM_UTILS[platform/platform_utils]
    MEMORY_LINEAR_ALLOCATOR[memory/linear_allocator]
    MEMORY_CHOCO_MEMORY[memory/choco_memory]
    EVENT[event]
  end
  style CORE fill:#9E9E9E,stroke:#1565C0,stroke-width:2px

  PLATFORM_CONTEXT --> PLATFORM_GLFW
  PLATFORM_CONTEXT --> PLATFORM_INTERFACE
  PLATFORM_CONTEXT --> PLATFORM_PLATFORM_UTILS
  PLATFORM_CONTEXT --> MEMORY_LINEAR_ALLOCATOR
  PLATFORM_CONTEXT --> EVENT

  PLATFORM_GLFW --> PLATFORM_INTERFACE
  PLATFORM_GLFW --> PLATFORM_PLATFORM_UTILS
  PLATFORM_GLFW --> MEMORY_LINEAR_ALLOCATOR
  PLATFORM_GLFW --> EVENT
  PLATFORM_GLFW --> CHOCO_STRING

  CHOCO_STRING --> MEMORY_CHOCO_MEMORY

  PLATFORM_INTERFACE --> PLATFORM_PLATFORM_UTILS
  PLATFORM_INTERFACE --> EVENT
```

## 各レイヤー詳細

### application

- 目的: 最上位のオーケストレーション。サブシステム初期化、メインループ駆動、終了処理。
- 依存:
  - base

### base

- 目的: 最下層の“横断ユーティリティ”。全レイヤーから安心して使える小道具を提供する。
- 性質: 外部ライブラリ/OS依存なし(標準Cのみ)。原則として初期化不要・状態最小。
- 保有機能:
  - `base/message`: stdout, stderrへの色付きメッセージ出力
  - `base/macros`: 共通マクロ
- 依存: C標準ライブラリのみ
- 入れないもの: メモリアロケータ、数学/コンテナなど“機能モジュール”(＝coreへ)

### core

- 目的: プロジェクト全体から使用される機能/APIを提供する(メモリアロケータ/数学ライブラリ等)。
- 保有機能:
  - `core/memory/linear_allocator`: リニアアロケータ
  - `core/memory/choco_memory`: メモリトラッキング+mallocラッパー(将来的にはFreeListに入れ替え)
- 依存:
  - base
