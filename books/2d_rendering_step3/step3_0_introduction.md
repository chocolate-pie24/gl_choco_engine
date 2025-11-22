---
title: "step3_0: イベントシステムの構築"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook3に対応しています。なお、GL Choco Engineの立ち位置について追記しています。

前回の[Book](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2)では、X Window Systemやwin32への拡張を考慮したプラットフォーム基盤を作成し、ウィンドウの生成までを行いました。

今回は、作成したプラットフォーム基盤にイベントシステムを追加していきます。このステップによって、キーボードイベント、マウスイベント、ウィンドウイベントを取得し、アプリケーション側でイベントに応じた処理を行うことが可能になります。

今回実装するモジュールを全て実装すると、レイヤー構成図は以下のようになります。大分複雑になっていますが、追加されるモジュールは緑のブロックのみです。その他の変更はAPIの追加、変更となります。

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

## Step3実装解説

### 今回作成するイベントシステムの概要とイベント構造体の定義

今回作成していくイベントは、

- キーボードキー押下イベント
- マウスボタンクリックイベント
- ウィンドウイベント

の3種類です。ゆくゆくはゲームパッド等のイベントも扱えるようにしていくかもしれませんが、当面はこれで行きます。これらのイベントをどのように取得し、アプリケーション側へ伝えるのか、その概要について先ずは説明します。概要の説明の後、簡易的にウィンドウイベントを取得できるようにし、イベントシステムの構造をより具体的に理解できるようにします。

[イベントシステムの概要とイベント構造体の定義](https://zenn.dev/chocolate_pie24/books/2d_rendering_step3/viewer/step3_1_event_system_abstract)

### イベント格納用リングキューの追加

前回は、イベントデータを格納する仕組みがなかったため、発生したイベントは全て即対応する必要がありました。このため、連続したイベントがあった場合、最後のイベントのみに対応すれば良いところを全て対応するという非効率な処理が発生します。今回は、発生したイベントを一時的に格納するキューを作成することで、この問題に対処します。

[イベント格納用リングキューの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step3/viewer/step3_2_ring_queue)

### イベントポンプ処理のリファクタリング

ここまで、イベントシステムの全体を説明するため、イベントポンプ処理を多少雑なやり方で行っていました。具体的にはイベントポンプ処理に様々な処理が詰め込まれています。今回は処理を分割し、関数の責務を明確にしていくリファクタリングを行います。

[イベントポンプ処理のリファクタリング](https://zenn.dev/chocolate_pie24/books/2d_rendering_step3/viewer/step3_3_event_pump_refactoring)

### マウスイベントの主機能機能の追加

以上でイベントシステムの構造は完成です。次はマウスイベントを処理できるようにしていきます。

[マウスイベントの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step3/viewer/step3_4_mouse_event)

### キーボードイベントの取得機能の追加

最後にキーボードイベントを追加します。

[キーボードイベントの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step3/viewer/step3_5_keyboard_event)

### まとめ

以上で「Step3: イベントシステムの構築」は完成となります。記事の中ではdoxygenのコメントやテストコードは省いています。全てのコードは、[リポジトリ](https://github.com/chocolate-pie24/gl_choco_engine)に公開しています。ここまでの成果物には、v0.1.0-step3のタグをつけています。

次回はようやく描画処理に入ります。シェーダーを作成し、三角形の描画処理までを行います。
