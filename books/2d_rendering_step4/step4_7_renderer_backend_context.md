---
title: "step4_7: renderer_backend_contextの追加"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

実装コードについては、リポジトリのタグv0.1.0-step4を参照してください。

# renderer_backend_contextの追加

Renderer Backendの最後は、renderer_backend_contextです。
renderer_backend_contextの役割は2つあります。

- Renderer Backendの状態管理
- renderer_backend_interfaceが保有する機能を上位層から使用するための窓口

以下、それぞれの役割について説明していきます。

## Renderer Backendの状態管理

Renderer Backend自体は、エンジン全体で一つだけ存在するもので、内部状態管理構造体では現状、下記のパラメータを保持します。

現在仕様中のプログラム識別子や、現在バインド中のVAO, VBO識別子を保持することで、
現在バインド中のVBOを再バインドする場合には何もしない、といった判定を可能にし、オーバーヘッドを減らすようにしました。

```c
/**
 * @brief RendererBackend内部状態管理構造体
 *
 */
struct renderer_backend_context {
    target_graphics_api_t target_api;               /**< 使用グラフィックスAPI */

    const renderer_shader_vtable_t* shader_vtable;  /**< シェーダー機能提供vtable */
    const renderer_vao_vtable_t* vao_vtable;        /**< VAO機能提供vtable */
    const renderer_vbo_vtable_t* vbo_vtable;        /**< VBO機能提供vtable */

    uint32_t current_program_id;                    /**< 現在使用中のリンクされたシェーダープログラムID */
    uint32_t current_bound_vao;                     /**< 現在バインド中のVAO識別子 */
    uint32_t current_bound_vbo;                     /**< 現在バインド中のVBO識別子 */
};
```

renderer_backend_contextが提供する「Renderer Backendの状態管理」機能として、この構造体の生成と終了時の処理を行うAPIを用意しました。詳細は後述します。

なお、この構造体はエンジン全体で一つのみ存在するものであり、また、エンジン起動時から終了時まで存在し続ける性質があります。
このため、 ***core/choco_memory*** ではなく、 ***core/linear_allocator*** によるリソース管理とします。

## renderer_backend_interfaceが保有する機能を上位層から使用するための窓口

この役割を実現するためには、shader / VAO / VBO操作のための仮想関数テーブルが保持する全ての関数の窓口が必要となります。
そのため、外部公開APIの数が非常に多くなり、使いにくいものになってしまいます。
このため、今回は外部公開APIを以下のヘッダに分けて定義するようにしました。

| ヘッダ名                                | 外部公開API種別                              |
| -------------------------------------- | ------------------------------------------ |
| renderer_backend_context/context.h     | renderer_backend_contextの初期化、終了処理API |
| renderer_backend_context/shader.h      | shader操作API                              |
| renderer_backend_context/context_vao.h | VAO操作API                                  |
| renderer_backend_context/context_vbo.h | VBO操作API                                  |

ただ、それぞれのAPIは内部で ***renderer_backend_context*** 構造体インスタンスを必要とします。
***renderer_backend_context*** は ***context.c*** にて内部構造を隠蔽しているため、実装側も分離することができません。

そこで、ヘッダのみを分割し、全体を把握しやすいようにしておき、実装の方はshader / VAO / VBO操作APIを全て ***context.c*** に記述するようにしました。

それぞれのヘッダが公開するAPIの詳細は以下の表にようにしました。

***renderer_backend_context/context.h***

| 外部公開API                                  | 役割                                                            |
| ------------------------------------------- | -------------------------------------------------------------- |
| renderer_backend_initialize                 | レンダラーバックエンドのメモリを確保し、初期化を行う                   |
| renderer_backend_destroy                    | レンダラーバックエンドの終了処理を行う                               |

***renderer_backend_context/shader.h***

| 外部公開API                                  | 役割                                                            |
| ------------------------------------------- | -------------------------------------------------------------- |
| renderer_backend_shader_create              | シェーダーハンドル構造体インスタンスのメモリを確保し0で初期化する        |
| renderer_backend_shader_destroy             | シェーダーハンドル構造体インスタンスを破棄する                        |
| renderer_backend_shader_compile             | シェーダーソースをコンパイルし、シェーダーオブジェクトハンドルを初期化する |
| renderer_backend_shader_link                | コンパイル済みのシェーダーオブジェクトをリンクする                     |
| renderer_backend_shader_use                 | シェーダープログラムの使用開始をグラフィックスAPIに伝える               |

***renderer_backend_context/context_vao.h***

| 外部公開API                                  | 役割                                                            |
| ------------------------------------------- | -------------------------------------------------------------- |
| renderer_backend_vertex_array_create        | VAO内部状態管理構造体インスタンスのメモリを確保する                    |
| renderer_backend_vertex_array_destroy       | VAO内部状態管理構造体インスタンスを破棄する                           |
| renderer_backend_vertex_array_bind          | VAOをbindする                                                   |
| renderer_backend_vertex_array_unbind        | VAOをunbindする                                                 |
| renderer_backend_vertex_array_attribute_set | 頂点情報のレイアウトをGPUに通知する                                 |

***renderer_backend_context/context_vbo.h***

| 外部公開API                                  | 役割                                                            |
| ------------------------------------------- | -------------------------------------------------------------- |
| renderer_backend_vertex_buffer_create       | VBO内部状態管理構造体インスタンスのメモリを確保する                    |
| renderer_backend_vertex_buffer_destroy      | VBO内部状態管理構造体インスタンスを破棄する                           |
| renderer_backend_vertex_buffer_bind         | VBOをbindする                                                   |
| renderer_backend_vertex_buffer_unbind       | VBOをunbindする                                                 |
| renderer_backend_vertex_buffer_vertex_load  | GPUの頂点情報格納バッファに頂点情報を転送する                         |
