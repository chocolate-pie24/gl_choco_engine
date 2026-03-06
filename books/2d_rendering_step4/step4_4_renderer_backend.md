---
title: "step4_4: API差し替え可能なRenderer Backendの枠組み"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

実装コードについては、リポジトリのタグv0.1.0-step4を参照してください。

# API差し替え可能なRenderer Backendの枠組み

ここから追加していくモジュールは、VAO / VBO / シェーダーです。これらはRenderer Backendの中に配置されることになります。なので、先ずはRenderer Backendの枠組みを決定し、その後に各モジュールを配置していくことにします。

Renderer Backendに配置するモジュールは以下です。

| モジュール  | 役割                                          | 性質                                                                  |
| --------- | --------------------------------------------- | -------------------------------------------------------------------- |
| shader    | シェーダープログラム、シェーダーモジュールの情報を格納 | シェーダープログラムごとにインスタンスが作られる                              |
| vao       | VAO機能を提供する                               | インスタンスの生成単位は未定(シェーダープログラム単位 or 形状データ単位かで悩み中) |
| vbo       | VBO機能を提供する                               | インスタンスの生成単位は未定(シェーダープログラム単位 or 形状データ単位かで悩み中) |

ここで、シェーダープログラムなのですが、現状では三角形描画用のテスト用シェーダーのみですが、今後、描画対象の性質(2D? 3D? マテリアル情報あり? なし?)によって複数のシェーダープログラムが作られる予定です。

また、vao / vboに似たモジュールとして今後、ebo(Index Buffer Object)が追加されます。vao / vboはセットで存在しますが、eboについては形状データの性質によっては作られません。よって、vao / vbo / eboについては、それぞれ独立してインスタンスを管理することにします。

以上を踏まえ、これらをAPI差し替え可能なBackendにするために、Strategyパターンを使用して下記の構成を作ります。

```mermaid
classDiagram
    BackendContext : + context_method()

    ShaderInterface : + strategy_method()
    ShaderConcrete1 : + strategy_method()
    ShaderConcrete2 : + strategy_method()

    VAOInterface : + strategy_method()
    VAOConcrete1 : + strategy_method()
    VAOConcrete2 : + strategy_method()

    VBOInterface : + strategy_method()
    VBOConcrete1 : + strategy_method()
    VBOConcrete2 : + strategy_method()

    BackendContext --> ShaderInterface
    ShaderInterface <|-- ShaderConcrete1
    ShaderInterface <|-- ShaderConcrete2

    BackendContext --> VAOInterface
    VAOInterface <|-- VAOConcrete1
    VAOInterface <|-- VAOConcrete2

    BackendContext --> VBOInterface
    VBOInterface <|-- VBOConcrete1
    VBOInterface <|-- VBOConcrete2
```

上記の構成をC言語で実装するために、Rendererレイヤーは下記の構成を取ることにします。

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

なお、Renderer Frontendについては、このタイミングで作成するよりは、エンジンの機能が充実し、描画内容がリッチになってきてから作成した方が設計の手戻りが少ないと考え、まだ用意しません。
