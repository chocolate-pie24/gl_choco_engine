---
title: "step4_5: renderer_coreの追加"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

# renderer_coreの追加

このステップでは、Rendererの構成のうち、renderer_coreを作っていきます。Rendererレイヤーの構成をもう一度貼ります。

```mermaid
graph TD
  subgraph RENDERER_CORE[renderer_core]
    direction TB
    RENDERER_ERR_UTILS[renderer_err_utils]
    RENDERER_MEMORY[renderer_memory]
    RENDERER_TYPES[renderer_types]
  end

  subgraph RENDERER_BACKEND[renderer_backend]
    direction TB
    RENDERER_BACKEND_TYPES[renderer_backend_types]

    subgraph RENDERER_BACKEND_INTERFACE[renderer_backend_interface]
      direction TB
      INTERFACE_SHADER[interface_shader]
      INTERFACE_VAO[interface_vao]
      INTERFACE_VBO[interface_vbo]
    end

    subgraph RENDERER_BACKEND_CONCRETES[renderer_backend_concretes]
      direction TB
      subgraph GL33
        direction TB
        CONCRETE_SHADER[concrete_shader]
        CONCRETE_VAO[concrete_vao]
        CONCRETE_VBO[concrete_vbo]
      end
    end

    subgraph RENDERER_BACKEND_CONTEXT[renderer_backend_context]
      direction TB
      CONTEXT[context]
    end
  end

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

renderer_coreの役割ですが、Renderer Frontend / Renderer Backend全体から使用されるモジュールを配置します。
配置するモジュールと役割は下表の通りです。

| モジュール           | 役割                                                                                             |
| ------------------ | ------------------------------------------------------------------------------------------------ |
| renderer_err_utils | 下位レイヤーに属するモジュールの実行結果コード変換機能を提供                                               |
| renderer_memory    | choco_memoryのallocate / freeのラッパーAPIで、Renderer専用の実行結果コード出力、メモリータグ指定を可能にする |
| renderer_types     | Renderer全体で使用される型を提供する。現状では以下を提供する                                              |
|                    | - renderer_result_t: Renderer実行結果コード定義                                                     |
|                    | - buffer_usage_t: GPU側バッファの使用方法(STATIC / DYNAMIC)を定義                                     |
|                    | - renderer_type_t: グラフィックスAPIに依存しないデータ型を定義                                          |
|                    | - shader_type_t: シェーダー種別を定義                                                                |
