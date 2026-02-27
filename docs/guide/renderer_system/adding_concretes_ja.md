@page guide_renderer_system_ja Renderer System Guide(Japanese)

# Renderer SystemへのConcreteモジュールの追加方法ガイドライン

このページでは、エンジン開発者が安全に対応グラフィックスAPIを追加するためのガイドラインを提供する。
なお、追加するグラフィックスAPIは当面はOpenGL3.3以外のバージョンを想定しており、Vulkanを使用する際にはある程度の設計変更が必要があることを前提としている。

なお、`Renderer System`の全体像については、[Renderer System architecture](../../architecture/renderer_system/architecture_ja.md)を参照のこと。

## 対応グラフィックスAPI追加ガイドライン

- `renderer_types.h`の`target_graphics_api_t`に追加グラフィックス名称を追加
- `renderer_backend/renderer_backend_context/context.c`の`graphics_api_valid_check()`で追加したグラフィックスAPIを有効化する
- `include/engine/renderer/renderer_backend/renderer_backend_concretes/`以下に追加グラフィックスAPI用ディレクトリを追加
- 追加したディレクトリ以下に、`concrete_shader.h`, `concrete_vao.h`, `concrete_vbo.h`を追加(なお、GLCEでは複数のグラフィックスAPIの混在は想定していない。必ず3つセットで作成すること)

`concrete_shader.h`, `concrete_vao.h`, `concrete_vbo.h`では以下のAPIを定義する(xxxはグラフィックスAPI識別名称)。

| ヘッダ               | 追加API                    | 役割                                              |
| ------------------- | ------------------------- | ------------------------------------------------ |
| `concrete_shader.h` | `xxx_shader_vtable_get()` | シェーダー機能を提供する仮想関数テーブルを上位層に提供する |
| `concrete_vao.h`    | `xxx_vao_vtable_get()`    | VAO機能を提供する仮想関数テーブルを上位層に提供する       |
| `concrete_vbo.h`    | `xxx_vbo_vtable_get()`    | VBO機能を提供する仮想関数テーブルを上位層に提供する       |

- `src/engine/renderer/renderer_backend/renderer_backend_concretes/`以下に追加グラフィックスAPI用ディレクトリを追加
- 追加したディレクトリ以下に、`concrete_shader.c`, `concrete_vao.c`, `concrete_vbo.c`を追加

### concrete_shader.c作成手順

`concrete_shader.c`で、`renderer_backend_shader_t`構造体を定義する。フィールドは追加するグラフィックスAPI固有であるが、例えばOpenGL3.3用バックエンドでは、以下を保持している

- リンクしたOpenGLシェーダープログラムへのハンドル
- コンパイルしたバーテックスシェーダーオブジェクトへのハンドル
- コンパイルしたフラグメントシェーダーオブジェクトへのハンドル

shader concreteモジュール用実装ファイルには`Renderer Backend Interface`用仮想関数テーブル(`renderer_shader_vtable_t`)の実装として、以下の機能を実装する(xxxはグラフィックスAPIの名称)。

各関数の実装においては下記に留意する

- メモリ確保が必要な場合は、`renderer_core/renderer_memory`が提供する`render_mem_allocate()`,`render_mem_free()`を使用すること
- 実行結果コードの文字列への変換処理、下位レイヤーの実行結果コードの変換処理は`renderer_core/renderer_err_utils`が提供するAPIを使用すること

| カテゴリー       | 関数名称                 | 役割                                                                                                   |
| -------------- | ----------------------- | ----------------------------------------------------------------------------------------------------- |
| Lifecycle      | `xxx_shader_create()`  | `renderer_backend_shader_t`構造体インスタンスのメモリを確保し、構造体フィールドを0で初期化する                   |
| Lifecycle      | `xxx_shader_destroy()` | シェーダープログラムの停止をグラフィックスAPIに伝え、`renderer_backend_shader_t`構造体インスタンスのメモリを開放する |
| シェーダー操作   | `xxx_shader_compile()` | シェーダーオブジェクトをコンパイルする                                                                         |
| シェーダー操作   | `xxx_shader_link()`    | コンパイルしたシェーダーオブジェクトをリンクする                                                                            |
| シェーダー操作   | `xxx_shader_use()`     | リンク済のシェーダープログラムの使用を開始する                                                                |

- `renderer_backend/renderer_backend_context/context.c`の`shader_vtable_get()`に追加したグラフィックスAPI用vtableを追加

### concrete_vao.c作成手順

`concrete_vao.c`で、`renderer_backend_vao_t`構造体を定義する。フィールドは追加するグラフィックスAPI固有であるが、例えばOpenGL3.3用バックエンドでは、以下を保持している

- 生成したVAOのハンドル

VAO concreteモジュール用実装ファイルには`Renderer Backend Interface`用仮想関数テーブル(`renderer_vao_vtable_t`)の実装として、以下の機能を実装する(xxxはグラフィックスAPIの名称)。

各関数の実装においては下記に留意する

- メモリ確保が必要な場合は、`renderer_core/renderer_memory`が提供する`render_mem_allocate()`,`render_mem_free()`を使用すること
- 実行結果コードの文字列への変換処理、下位レイヤーの実行結果コードの変換処理は`renderer_core/renderer_err_utils`が提供するAPIを使用すること

| カテゴリー       | 関数名称                   | 役割                                                                              |
| -------------- | ------------------------- | -------------------------------------------------------------------------------- |
| Lifecycle      | `xxx_vao_create()`        | `renderer_backend_vao_t`構造体インスタンスのメモリを確保し、構造体フィールドを0で初期化する |
| Lifecycle      | `xxx_vao_destroy()`       | VAOを削除し、`renderer_backend_vao_t`構造体インスタンスのメモリを開放する                |
| VAO操作         | `xxx_vao_bind()`          | VAOをbindする                                                                     |
| VAO操作         | `xxx_vao_unbind()`        | VAOをunbindする                                                                   |
| VAO操作         | `xxx_vao_attribute_set()` | VBO属性情報を設定する                                                               |

- `renderer_backend/renderer_backend_context/context.c`の`vao_vtable_get()`に追加したグラフィックスAPI用vtableを追加

### concrete_vbo.c作成手順

`concrete_vbo.c`で、`renderer_backend_vbo_t`構造体を定義する。フィールドは追加するグラフィックスAPI固有であるが、例えばOpenGL3.3用バックエンドでは、以下を保持している

- 生成したVBOのハンドル

VBO concreteモジュール用実装ファイルには`Renderer Backend Interface`用仮想関数テーブル(`renderer_vbo_vtable_t`)の実装として、以下の機能を実装する(xxxはグラフィックスAPIの名称)。

各関数の実装においては下記に留意する

- メモリ確保が必要な場合は、`renderer_core/renderer_memory`が提供する`render_mem_allocate()`,`render_mem_free()`を使用すること
- 実行結果コードの文字列への変換処理、下位レイヤーの実行結果コードの変換処理は`renderer_core/renderer_err_utils`が提供するAPIを使用すること

| カテゴリー       | 関数名称                 | 役割                                                                              |
| -------------- | ----------------------- | -------------------------------------------------------------------------------- |
| Lifecycle      | `xxx_vbo_create()`      | `renderer_backend_vbo_t`構造体インスタンスのメモリを確保し、構造体フィールドを0で初期化する |
| Lifecycle      | `xxx_vbo_destroy()`     | VBOを削除し、`renderer_backend_vbo_t`構造体インスタンスのメモリを開放する                |
| VBO操作         | `xxx_vbo_bind()`        | VBOをbindする                                                                     |
| VBO操作         | `xxx_vbo_unbind()`      | VBOをunbindする                                                                   |
| VBO操作         | `xxx_vbo_vertex_load()` | 頂点情報をGPUに転送する                                                             |

- `renderer_backend/renderer_backend_context/context.c`の`vbo_vtable_get()`に追加したグラフィックスAPI用vtableを追加

### 追加したグラフィックスAPIの指定

新規に追加したグラフィックスAPIを使用する場合は、`application_create()`内の`renderer_backend_initialize()`でグラフィックスAPI種別を指定する(*1)

*1: 将来的にはビルドオプションで指定するように変更予定
