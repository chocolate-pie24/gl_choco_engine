---
title: "step4_1: Platformレイヤーの構成変更と機能追加"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

# Platformレイヤーの構成変更と機能追加

## レイヤー構成変更

現在のレイヤー構成は下図のようになっています。

```mermaid
graph TD
  APPLICATION[application]
  PLATFORM_CONTEXT[platform_context]

  subgraph CONTAINERS[containers]
    direction TB
    CHOCO_STRING[choco_string]
    RING_QUEUE[ring_queue]
  end
  style RING_QUEUE fill:#8BC34A

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
    style KEYBOARD_EVENT fill:#8BC34A
    style MOUSE_EVENT fill:#8BC34A
    style WINDOW_EVENT fill:#8BC34A
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

ここに今回、Rendererレイヤーが新設されます。PlatformについてもRendererと同層に位置するべきだと考え、Platformレイヤーします。
Platformレイヤーには、現状のPlatform関連モジュール(platform_context, platform_concretes, interfaces)を格納します。
さらに、Renderer開発時に採った構成をPlatformにも適用し、統一感を保つため、以下の変更を行います。

- core/platform_utilsをplatform_typesに名称変更し、platform/platform_coreに移動
- platform_core/platform_err_utilsを追加し、Platform関連の実行結果コード定義とモジュール間実行結果コード処理を集約

これらの変更を行った新しいレイヤー構成は下図のようになります。

```mermaid
graph TD
  APPLICATION[application]
  subgraph CORE[core]
    direction TB
    subgraph EVENT[event]
      direction TB
      KEYBOARD_EVENT[keyboard_event]
      MOUSE_EVENT[mouse_event]
      WINDOW_EVENT[window_event]
    end
    subgraph MEMORY[memory]
      direction TB
      CHOCO_MEMORY[choco_memory]
      LINEAR_ALLOCATOR[linear_allocator]
    end
  end

  subgraph CONTAINERS[containers]
    direction TB
    CHOCO_STRING[choco_string]
    RING_QUEUE[ring_queue]
  end

  subgraph PLATFORM[platform]
    direction TB

    PLATFORM_CONTEXT[platform_context]

    subgraph PLATFORM_CORE[platform_core]
      direction TB
      PLATFORM_ERR_UTILS[platform_err_utils]
      PLATFORM_TYPES[platform_types]
    end

    PLATFORM_INTERFACE[platform_interface]

    subgraph PLATFORM_CONCRETES[platform_concretes]
      direction TB
      PLATFORM_GLFW[platform_glfw]
    end
  end

  APPLICATION --> LINEAR_ALLOCATOR
  APPLICATION --> CHOCO_MEMORY
  APPLICATION --> PLATFORM_CONTEXT
  APPLICATION --> RING_QUEUE
  APPLICATION --> EVENT

  PLATFORM_CONTEXT --> PLATFORM_CORE
  PLATFORM_CONTEXT --> PLATFORM_GLFW
  PLATFORM_CONTEXT --> PLATFORM_INTERFACE
  PLATFORM_CONTEXT --> EVENT
  PLATFORM_CONTEXT --> |init parameter| LINEAR_ALLOCATOR

  PLATFORM_GLFW --> PLATFORM_CORE
  PLATFORM_GLFW --> CHOCO_STRING
  PLATFORM_GLFW --> PLATFORM_INTERFACE
  PLATFORM_GLFW --> EVENT

  PLATFORM_INTERFACE --> PLATFORM_TYPES
  PLATFORM_INTERFACE --> EVENT

  PLATFORM_ERR_UTILS -->|error_code type only| CHOCO_STRING
  PLATFORM_ERR_UTILS -->|error code type only| LINEAR_ALLOCATOR

  CHOCO_STRING --> CHOCO_MEMORY
```

## Platformシステムへの機能追加

次は、Platformシステムへの機能追加です。描画を行う際には、一般にダブルバッファリングを用い、
描画を表示中画面とは別のサーフェイスに行い、表示する際に入れ替えるという手法を採ります。

このダブルバッファリング機能なのですが、画面に関する機能とも言えるし、描画に関する機能とも言えます。
前者の場合はPlatformレイヤーに属する機能となり、後者の場合はRendererレイヤーに属する機能となります。
どちらにするか悩んだのですが、今回はダブルバッファリング処理に ***glfwSwapBuffers*** というGLFW依存のAPIを使用するため、
Platformレイヤーに配置することにしました。ここに関しては今後、別のPlatformを使用することになった場合、設計変更が必要となるかもしれません。

追加はplatform_contextに ***platform_swap_buffers()*** を追加し、それに伴い ***platform_interface*** と ***platform_concretes*** にもAPIを追加しました。
処理自体は内部で ***glfwSwapBuffers()*** を呼び出すだけの機能です。

## PlatformシステムAPIの仕様変更

今回、三角形の描画処理を追加した際、Linux環境とmacOS環境で以下のように実行結果が異なる現象が発生しました。

macOS環境

![macOS](/images/macOS_triangle.png)

Linux環境

![Linux](/images/first_triangle.png)

原因は、描画範囲の指定にピクセルサイズではなく、ウィンドウサイズを指定していたことが原因でした。具体的に言うと、
Linuxの場合はウィンドウサイズと画面のピクセルサイズがイコールであるのに対し、
macOSでRetina Displayを使用している場合はイコール(上の例では幅が1/2、高さが1/2で1/4の三角形になっている)とならず、描画範囲がLinuxとmacOSで異なる範囲になっていました。
描画範囲の指定にピクセルサイズを指定することにより、不具合は解消されました。

以上の理由により、Platformレイヤーから上位レイヤーへ画面のピクセルサイズを渡すことができるよう既存APIが仕様変更が必要です。
今回は、 ***platform_glfw_window_create()*** APIの引数に出力変数としてフレームバッファサイズを追加することで対応しました。
なお、画面のピクセルサイズはGLFW環境では、 ***glfwGetFramebufferSize*** によって取得することができます。
