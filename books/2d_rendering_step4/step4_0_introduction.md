---
title: "step4_0: レンダラーの構築とシンプルな三角形の描画"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

前回の[Book](https://zenn.dev/chocolate_pie24/books/2d_rendering_step2)では、イベントシステムを構築し、キーボード、マウス、ウィンドウに関するイベントを処理できるようになりました。

今回は、描画処理に入っていきます。描画するのはシンプルな三角形です。三角形の描画は新しい言語を学ぶ際のHello World!!に相当し、最もシンプルなグラフィックスアプリケーションです。

なお、今回追加していくレイヤー、モジュールの作成に伴い、これまでに作成してきたモジュールに対して多くの変更を行いました。
モジュールの役割については変更していないのですが、戻り値の実行結果コードや、テスト関数に変更が入っています。
これら変更点について解説していくと、Book4の本題がぼやけてしまうため、これら変更点の解説については省略させていただきます。

また、今までは各APIの内部実装についても解説していましたが、このように細部に変更があった際に整合性が取れなくなってしまうため、
今後はこちらについても省略させていただこうと思います。ただ、C言語でのStrategyパターンの実装や、ジェネリック型のコンテナモジュールの実装等、
実装方法の解説が重要になる部分もあると思いますので、そういったテーマについてはBookの付録や、別途記事にして出すことを考えています。

今後も進めながら方針を変えさせていただくことがあろうかと思いますが、よろしくお願いします。

さて、本題の三角形描画についてですが、アプローチとして、レイヤー構成やモジュールの分割といったことは考えず、
とりあえず最速で三角形を描画できるようにしていきます。その後、作成した各処理をモジュール化して外部に掃き出していく手法を取ります。
このやり方を取る理由は、「なぜこのモジュールが必要なのか？」ということが分かりやすいのと、
とりあえず早く結果が見えた方がモチベーションも上がりやすいためです。

## Step4実装解説

今回追加されるモジュールを示します。
coreレイヤーへも追加されるモジュールがありますが、図が複雑になりすぎるため省いてあります。

また、applicationがダイレクトにレンダラーバックエンドに依存していますが、
これはレンダラーフロントエンドの作成後、依存が解消されます。

```mermaid
graph TD
  APPLICATION[application]
  APPLICATION --> RING_QUEUE
  APPLICATION --> FS_UTILS
  APPLICATION --> PLATFORM_CONTEXT
  APPLICATION --> RENDERER_TYPES
  APPLICATION --> VERTEX_BUFFER_OBJECT
  APPLICATION --> VERTEX_ARRAY_OBJECT
  APPLICATION --> GL33_SHADER

  subgraph ENGINE[engine]
    direction TB

    subgraph CONTAINERS[containers]
      direction TB
      CHOCO_STRING[choco_string]
      RING_QUEUE[ring_queue]
    end

    subgraph IO_UTILS[io_utils]
      direction TB
      FS_UTILS[fs_utils]
    end
    style FS_UTILS fill:#8BC34A
    FS_UTILS --> CHOCO_STRING

    subgraph PLATFORM[platform]
      PLATFORM_CONTEXT[platform_context]
      direction TB
      subgraph INTERFACES[interfaces]
        direction TB
        PLATFORM_INTERFACE[platform_interface]
      end
      subgraph PLATFORM_CONCRETES[platform_concretes]
        direction TB
        PLATFORM_GLFW[platform_glfw]
      end
    end
    PLATFORM_GLFW --> PLATFORM_INTERFACE
    PLATFORM_GLFW --> CHOCO_STRING
    PLATFORM_CONTEXT --> PLATFORM_INTERFACE
    PLATFORM_CONTEXT --> PLATFORM_GLFW

    subgraph RENDERER[renderer]
      direction TB
      subgraph RENDERER_BACKEND[renderer_backend]
        direction TB
        subgraph GL33[gl33]
          direction TB
          GL33_SHADER[gl33_shader]
          VERTEX_ARRAY_OBJECT[vertex_array_object]
          VERTEX_BUFFER_OBJECT[vertex_buffer_object]
        end
        style GL33_SHADER fill:#8BC34A
        style VERTEX_ARRAY_OBJECT fill:#8BC34A
        style VERTEX_BUFFER_OBJECT fill:#8BC34A
      end
      subgraph RENDERER_BASE[renderer_base]
        direction TB
        RENDERER_TYPES[renderer_types]
      end
      style RENDERER_TYPES fill:#8BC34A
      subgraph RENDERER_CORE[renderer_core]
        direction TB
        RENDERER_ERR_UTILS[renderer_err_utils]
        RENDERER_MEMORY[renderer_memory]
      end
      style RENDERER_ERR_UTILS fill:#8BC34A
      style RENDERER_MEMORY fill:#8BC34A
    end
    GL33_SHADER --> RENDERER_TYPES
    GL33_SHADER --> RENDERER_MEMORY
    GL33_SHADER --> RENDERER_ERR_UTILS

    VERTEX_ARRAY_OBJECT --> RENDERER_TYPES
    VERTEX_ARRAY_OBJECT --> RENDERER_ERR_UTILS
    VERTEX_ARRAY_OBJECT --> RENDERER_MEMORY
    VERTEX_BUFFER_OBJECT --> RENDERER_TYPES
    VERTEX_BUFFER_OBJECT --> RENDERER_ERR_UTILS
    VERTEX_BUFFER_OBJECT --> RENDERER_MEMORY
    RENDERER_ERR_UTILS --> RENDERER_TYPES
    RENDERER_MEMORY --> RENDERER_TYPES
  end
```

### Step4-1: PlatformレイヤーへのAPI追加

最速で三角形を描画するためには、application_runで描画するようにしてしまうのが最も速いです。
描画するためにはOpenGLのウィンドウインスタンスが必要なのですが、現状ではplatform_glfwモジュールが保有しています。
よって、準備として、Platformレイヤーからウィンドウインスタンスを取得するAPIを追加します。

[PlatformレイヤーへのAPI追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_1_window_instance)

### Step4-1: とりあえず三角形を出す

準備が整ったので、三角形を出す処理を追加します。
今回は、[OpenGL Tutorial](https://www.opengl-tutorial.org/jp/beginners-tutorials/tutorial-2-the-first-triangle/)
のコードを使用して三角形を表示します。

[とりあえず三角形を出す](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_2_hello_triangle)

なお、画面に描画するためにはOpenGLの座標系の知識が必要です。
座標系の解説をメモとして[付録1: OpenGLで使われる座標系メモ](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/appendix1_coordinates)
に記しました。

### Step4-2: レンダラーレイヤーの追加とVAO、VBOモジュールの追加

追加した三角形描画処理から、VAO、VBOに関連する処理をモジュール化していきます。
このため、新規にレンダラーレイヤーを追加し、モジュールを追加します。

なお、VAO、VBOについても付録にメモを追加しました。
[付録2: OpenGL VAO、VBO解説](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/appendix2_vao_vbo)

### Step4-3: filesystemモジュールの追加

次はシェーダープログラムを外部のファイルに出していきます。よって、読み込み処理が必要となります。
coreレイヤーにfilesystemモジュールを追加していきます。

[filesystemモジュールの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_3_core_filesystem)

### Step4-4: choco_stringモジュールへのAPI追加

シェーダープログラムの読み込みの準備として、
追加したfilesystemモジュールが提供するバイト単位での読み込みAPIを使用して取得した文字列を連結するAPIをchoco_stringに追加します。

[choco_stringモジュールへのAPI追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_4_string_cat)

### Step4-5: fs_utilモジュールの追加

filesystemモジュール、choco_stringモジュールを使用して、シェーダープログラムを読み込む機能を提供するfs_utilモジュールを追加します。

[fs_utilモジュールの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_5_fs_util)

### Step4-6: シェーダーモジュールの追加

以上でシェーダープログラムの読み込みが可能になったので、シェーダーモジュールを追加していきます。

[シェーダーモジュールの追加](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/step4_5_shader)

### 付録1: OpenGLで使われる座標系メモ

[付録1: OpenGLで使われる座標系メモ](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/appendix1_coordinates)

### 付録2: VAO, VBO解説メモ

[付録2: OpenGL VAO、VBO解説](https://zenn.dev/chocolate_pie24/books/2d_rendering_step4/viewer/appendix2_vao_vbo)
