# gl_choco_engineレイヤーツリー

- [gl\_choco\_engineレイヤーツリー](#gl_choco_engineレイヤーツリー)
  - [Public(API) Dependencies](#publicapi-dependencies)
  - [Implementation(Private) Dependencies](#implementationprivate-dependencies)
  - [各レイヤー詳細](#各レイヤー詳細)
    - [application](#application)
    - [base](#base)
    - [core](#core)
    - [containers](#containers)
    - [interfaces](#interfaces)
    - [platform\_concretes](#platform_concretes)
    - [platform\_context](#platform_context)

## Public(API) Dependencies

凡例: A->BはAがBに依存を表す

```mermaid
graph TD
  subgraph CORE[core]
    direction TB

    subgraph MEMORY[memory]
      direction TB
      LINEAR_ALLOCATOR[linear_allocator]
    end

    subgraph PLATFORM[platform]
      direction TB
      PLATFORM_UTILS[platform_utils]
    end

    subgraph EVENT[event]
      direction TB
      KEYBOARD_EVENT[keyboard_event]
      MOUSE_EVENT[mouse_event]
      WINDOW_EVENT[window_event]
    end
  end

  subgraph INTERFACES[interfaces]
    direction TB
    PLATFORM_INTERFACE[platform_interface]
  end

  subgraph PLATFORM_CONCRETES[platform_concretes]
    direction TB
    PLATFORM_GLFW[platform_glfw]
  end

  PLATFORM_CONTEXT[platform_context]

  PLATFORM_INTERFACE --> PLATFORM_UTILS
  PLATFORM_INTERFACE --> KEYBOARD_EVENT
  PLATFORM_INTERFACE --> MOUSE_EVENT
  PLATFORM_INTERFACE --> WINDOW_EVENT

  PLATFORM_GLFW --> PLATFORM_UTILS
  PLATFORM_GLFW --> PLATFORM_INTERFACE

  PLATFORM_CONTEXT --> PLATFORM_UTILS
  PLATFORM_CONTEXT --> LINEAR_ALLOCATOR
  PLATFORM_CONTEXT --> KEYBOARD_EVENT
  PLATFORM_CONTEXT --> MOUSE_EVENT
  PLATFORM_CONTEXT --> WINDOW_EVENT
```

## Implementation(Private) Dependencies

凡例: A->BはAがBに依存を表す

baseレイヤーについては複雑になりすぎるため省略(全域で使用されるため図からは省略)

```mermaid
graph TD
  APPLICATION[application]
  PLATFORM_CONTEXT[platform_context]

  subgraph CONTAINERS[containers]
    direction TB
    CHOCO_STRING[choco_string]
    RING_QUEUE[ring_queue]
  end

  subgraph CORE[core]
    direction TB
    subgraph MEMORY[memory]
      direction TB
      CHOCO_MEMORY[choco_memory]
      LINEAR_ALLOCATOR[linear_allocator]
    end

    subgraph PLATFORM[platform]
      direction TB
      PLATFORM_UTILS[platform_utils]
    end

    subgraph EVENT[event]
      direction TB
      KEYBOARD_EVENT[keyboard_event]
      MOUSE_EVENT[mouse_event]
      WINDOW_EVENT[window_event]
    end
  end

  subgraph INTERFACES[interfaces]
    direction TB
    PLATFORM_INTERFACE[platform_interface]
  end

  subgraph PLATFORM_CONCRETES[platform_concretes]
    direction TB
    PLATFORM_GLFW[platform_glfw]
  end

  APPLICATION --> CHOCO_MEMORY
  APPLICATION --> LINEAR_ALLOCATOR
  APPLICATION --> PLATFORM_UTILS
  APPLICATION --> KEYBOARD_EVENT
  APPLICATION --> MOUSE_EVENT
  APPLICATION --> WINDOW_EVENT
  APPLICATION --> PLATFORM_CONTEXT
  APPLICATION --> RING_QUEUE

  PLATFORM_INTERFACE --> PLATFORM_UTILS
  PLATFORM_INTERFACE --> WINDOW_EVENT
  PLATFORM_INTERFACE --> MOUSE_EVENT
  PLATFORM_INTERFACE --> KEYBOARD_EVENT

  PLATFORM_GLFW --> PLATFORM_INTERFACE
  PLATFORM_GLFW --> PLATFORM_UTILS
  PLATFORM_GLFW --> WINDOW_EVENT
  PLATFORM_GLFW --> MOUSE_EVENT
  PLATFORM_GLFW --> KEYBOARD_EVENT
  PLATFORM_GLFW --> CHOCO_STRING

  PLATFORM_CONTEXT --> LINEAR_ALLOCATOR
  PLATFORM_CONTEXT --> PLATFORM_UTILS
  PLATFORM_CONTEXT --> KEYBOARD_EVENT
  PLATFORM_CONTEXT --> MOUSE_EVENT
  PLATFORM_CONTEXT --> WINDOW_EVENT
  PLATFORM_CONTEXT --> PLATFORM_GLFW
  PLATFORM_CONTEXT --> PLATFORM_INTERFACE
```

## 各レイヤー詳細

### application

- 目的: プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行う。以下の機能を提供する
  - 全サブシステムの起動、終了処理
  - アプリケーションメインループ
- 性質: システムの起動時から終了時まで常駐

### base

- 目的: 最下層の“横断ユーティリティ”。全レイヤーが使える小道具を提供
- 性質: 外部ライブラリ/OS依存なし(標準Cのみ)。初期化不要でシステム起動直後から使用可能
- 保有機能:
  - ***choco_message***: stdout, stderrへの色付きメッセージ出力
  - ***choco_macros***: 共通マクロ定義

### core

- 目的: プロジェクト全体から使用される機能/APIを提供する(メモリアロケータ/数学ライブラリ等)
- 保有機能:
  - ***choco_memory***: 不定期に発生するメモリ確保、解放に対応するメモリアロケータモジュールで、メモリトラッキング機能も有する
  - ***linear_allocator***: サブシステム等、ライフサイクルが固定で、個別のメモリ開放が不要なメモリ確保に対応するリニアアロケータモジュール
  - ***platform/platform_utils***: プラットフォームシステムで共通に使用されるデータ型を提供する
  - ***event/keyboard_event***: キーボードイベントに使用されるデータ型を提供する
  - ***event/mouse_event***: マウスイベントに使用されるデータ型を提供する
  - ***event/window_event***: ウィンドウイベントに使用されるデータ型を提供する

### containers

- 目的: リソースの管理責務を有する各種データを格納可能なコンテナを提供する
- 保有機能:
  - ***choco_string***: 文字列を格納するコンテナモジュールで、文字列比較や文字列連結等の文字列処理機能も提供する
  - ***ring_queue***: ジェネリック型のリングキューモジュールで、push, pop, empty, create, destroyを提供する

### interfaces

- 目的: StrategyパターンのInterfaceオブジェクトに相当するモジュールを格納する
- 保有機能:
  - ***platform_interface***: プラットフォームシステムのInterface構造体を提供する

### platform_concretes

- 目的: プラットフォームシステムStrategyパターンのConcreteオブジェクトに相当するモジュールを格納する
- 保有機能:
  - ***platform_glfw***: GLFW APIで実装されたプラットフォームシステムAPIを提供する

### platform_context

- 目的: プラットフォームシステムのStrategy Contextモジュールを提供する
