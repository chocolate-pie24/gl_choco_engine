@page arch_texture_system_ja Texture System Architecture(Japanese)

# Texture System architecture

## 目的と位置づけ

`Texture System` は、CPU側テクスチャリソースとGPU側テクスチャリソースを対応づけて管理し、上位レイヤーがグラフィックスAPIを直接意識せずにテクスチャを登録・取得・破棄できるようにするためのサブシステムである。

`Resource Layer` の `texture` モジュールは、テクスチャ名、幅、高さ、チャンネル数、ピクセルデータなどを保持するCPU側テクスチャリソース `texture_t` を提供する。
また、CPU側テクスチャリソースの生成・破棄、ピクセルデータのロード・解放、ピクセルデータ参照取得、テクスチャサイズ情報取得、テクスチャ名取得などのAPIを提供する。

一方、`Renderer Backend` の Texture API は、GPU側テクスチャリソース `renderer_backend_texture_t` の生成、破棄、bind / unbind、pixel upload などを提供する。

`Texture System` は、この2つのレイヤーを橋渡しする。
具体的には、CPU側テクスチャリソースを `Resource Layer` 経由で生成・ロードし、そのピクセルデータを `Renderer Backend` 経由でGPU側テクスチャリソースへアップロードする。
その後、CPU側リソースとGPU側リソースを同一のテクスチャIDで管理し、上位レイヤーへGPU側リソース取得APIを提供する。

## Texture Systemコンセプト

`Texture System` は、複数のテクスチャリソースを `texture_manager` に集約して管理する。

上位レイヤーは、テクスチャ名とGPU側テクスチャユニット番号を指定してテクスチャを登録する。
登録処理では、`texture_manager` がCPU側テクスチャリソースとGPU側テクスチャリソースを生成し、CPU側にロードしたピクセルデータをGPU側リソースへアップロードする。
登録が成功すると、`texture_manager` はCPU側リソースとGPU側リソースを同じスロットに保持し、そのスロット番号を `texture_id` として上位レイヤーへ返す。

`texture_manager` は、内部に以下の2つの配列を持つ。

- CPU側テクスチャリソース配列: `texture_t**`
- GPU側テクスチャリソース配列: `renderer_backend_texture_t**`

同じインデックスに格納されたCPU側リソースとGPU側リソースは、同じテクスチャを表す。
このインデックスが `texture_id` として上位レイヤーに返される。

つまり、`texture_id` は `texture_manager` 内部のリソース配列インデックスであり、CPU側リソースとGPU側リソースの対応関係を表す識別子である。

管理配列のサイズは、`texture_manager_initialize()` の `max_texture_count_` によって初期化時に決定され、現状の実装では固定長である。
そのため、初期化後に管理可能なテクスチャ数を拡張することはできない。
将来的には、`dynamic_array` などを利用した可変長管理へ変更し、必要に応じて管理可能数を拡張できるようにする可能性がある。

なお、`texture_manager` は登録後も `texture_t*` を保持するが、CPU側ピクセルデータを永続的に保持するわけではない。
現在の実装では、登録処理中に `texture_pixel_load()` で一時的にピクセルデータをロードし、GPU側リソースへアップロードした後、`texture_pixel_unload()` によりCPU側ピクセルデータを解放する。
そのため、登録後に保持されるCPU側リソースは、主にテクスチャ名と登録状態を表すリソースオブジェクトとして機能する。

## モジュール依存関係

`Texture System` は、CPU側リソース操作のために `Resource Layer` に依存し、GPU側リソース操作のために `Renderer Backend` に依存する。
また、管理用メモリの確保に `linear_allocator` を使用し、テクスチャ名比較に `choco_string` を使用する。

![layer](./layer.png)

この図は、`Texture System` の主要な依存関係を示す。
すべての include 関係を厳密に列挙するものではなく、`texture_manager` がどのレイヤーの機能を利用しているかを把握するための概要図である。

## 保有モジュールの役割と性質

`Texture System` が保有するモジュールは、現時点では `texture_manager` のみである。

| モジュール | 役割 | 性質 |
| ---------- | ---- | ---- |
| texture_manager | CPU側テクスチャリソースとGPU側テクスチャリソースを対応づけて管理し、テクスチャの登録・削除・ID取得・GPUリソース取得APIを提供する | `Texture System` の中心モジュール。リニアアロケータで確保した配列にCPU側リソースとGPU側リソースを保持し、同一インデックスを `texture_id` として扱う |

## Texture Systemの責務境界

`Texture System` は、CPU側テクスチャリソースとGPU側テクスチャリソースの対応関係を管理するためのサブシステムである。
そのため、以下を責務に含む。

- テクスチャ管理システムの初期化・終了処理
- 管理可能な最大テクスチャ数に基づくリソース管理配列の確保
- テクスチャ名を指定したCPU側テクスチャリソースの生成
- GPU側テクスチャリソースの生成
- CPU側テクスチャリソースへのピクセルデータロード
- GPU側テクスチャリソースへのピクセルデータアップロード
- GPUアップロード後のCPU側ピクセルデータ解放
- CPU側リソースとGPU側リソースを同一 `texture_id` で対応づけて管理すること
- テクスチャIDまたはテクスチャ名によるGPU側リソース取得
- テクスチャIDまたはテクスチャ名によるテクスチャリソースの登録解除
- Resource Layer / Renderer Backend / Linear Allocator / choco_string の実行結果コードを Texture System の実行結果コードへ変換すること

一方で、以下は `Texture System` の責務ではない。

- BMPファイル形式の詳細解析
- CPU側テクスチャリソース単体の内部表現定義
- GPU側テクスチャリソースのAPI固有実装
- テクスチャのbind / unbindを直接行うこと
- 描画時のuniform設定
- レンダリングコマンドの発行
- テクスチャキャッシュや参照カウントによる寿命管理

BMPファイル解析やCPU側テクスチャ表現は `Resource Layer` の責務である。
GPU側テクスチャリソースの具体的な生成・破棄・アップロード処理は `Renderer Backend` の責務である。
描画時のbind / unbindやuniform設定は、Renderer BackendのTexture APIを利用する上位レイヤー、または将来導入されるRenderer Frontendの責務となる。

## texture_managerモジュール詳細

`texture_manager` は、`Texture System` の中心となるモジュールである。

内部状態として、以下を保持する。

- `max_texture_count`: システムで管理可能なテクスチャ数の上限
- `cpu_resources`: CPU側テクスチャリソース `texture_t*` の配列
- `gpu_resources`: GPU側テクスチャリソース `renderer_backend_texture_t*` の配列

`cpu_resources[i]` と `gpu_resources[i]` は、同じテクスチャを表すペアである。
そのため、片方だけがNULLで、もう片方だけが非NULLの状態はデータ不整合として扱われる。

### 初期化

`texture_manager_initialize()` は、`texture_manager_t` 本体とCPU側リソース配列、GPU側リソース配列をリニアアロケータから確保する。
確保後、全てのリソース配列要素をNULLで初期化する。

`texture_manager` はシステム起動時から終了時まで常駐する前提であり、個別メモリ解放ではなく、リニアアロケータによる一括管理を前提としている。

### 終了処理

`texture_manager_deinitialize()` は、管理中のすべてのCPU側テクスチャリソースとGPU側テクスチャリソースを破棄する。

CPU側リソースは `texture_destroy()` により破棄される。
GPU側リソースは `renderer_backend_texture_destroy()` により破棄される。

ただし、`texture_manager_t` 自身のメモリはリニアアロケータで確保されているため、この関数では解放しない。

### テクスチャ登録

`texture_manager_register()` は、テクスチャ名を指定して、CPU側リソースとGPU側リソースを登録する。

主な処理の流れは以下の通り。

1. 引数と `texture_manager` の内部状態を検証する
2. 登録済みテクスチャ名との重複を確認する
3. 空きスロットを検索する
4. `texture_create()` によりCPU側テクスチャリソースを生成する
5. `renderer_backend_texture_create()` によりGPU側テクスチャリソースを生成する
6. `texture_pixel_load()` によりCPU側テクスチャリソースへピクセルデータをロードする
7. `texture_pixel_get()` と `texture_pixel_size_get()` によりアップロードに必要な情報を取得する
8. `renderer_backend_texture_pixel_upload()` によりGPU側テクスチャリソースへピクセルデータをアップロードする
9. `texture_pixel_unload()` によりCPU側ピクセルデータを解放する
10. CPU側リソースとGPU側リソースを同じスロットに登録する
11. 登録したスロット番号を `texture_id` として返す

登録に失敗した場合、作成途中のCPU側リソースとGPU側リソースは破棄される。
また、成功時にのみ `texture_manager` 内部のリソース配列へコミットされる。

### テクスチャ登録解除

`texture_manager_unregister()` は、`texture_id` を指定して、登録済みのCPU側リソースとGPU側リソースを破棄する。

`texture_manager_unregister_by_name()` は、テクスチャ名から `texture_id` を検索し、その `texture_id` に対応するリソースを登録解除する。

### テクスチャID取得

`texture_manager_texture_id_get()` は、テクスチャ名に対応する `texture_id` を取得する。
テクスチャ名の比較には `choco_string_equal()` を使用する。

### GPU側リソース取得

`texture_manager_gpu_resource_get()` は、`texture_id` を指定してGPU側テクスチャリソースを取得する。

`texture_manager_gpu_resource_get_by_name()` は、テクスチャ名から `texture_id` を検索し、その `texture_id` に対応するGPU側テクスチャリソースを取得する。

取得したGPU側リソースは参照用であり、呼び出し側は取得した `renderer_backend_texture_t*` を破棄してはならない。

## リソース所有権

`Texture System` では、CPU側リソースとGPU側リソースの所有権を `texture_manager` が保持する。

| リソース | 所有者 | 解放タイミング |
| -------- | ------ | -------------- |
| `texture_manager_t` | `Texture System` / 上位レイヤーの初期化処理 | リニアアロケータの寿命に従う。`texture_manager_deinitialize()` では個別解放しない |
| `texture_t*` | `texture_manager` | `texture_manager_unregister()` または `texture_manager_deinitialize()` |
| `renderer_backend_texture_t*` | `texture_manager` | `texture_manager_unregister()` または `texture_manager_deinitialize()` |

`texture_manager_register()` は、登録処理が成功した場合にのみCPU側リソースとGPU側リソースの所有権を `texture_manager` に移す。
失敗した場合は、一時的に生成したリソースを内部で破棄する。

なお、`texture_t*` の所有権は `texture_manager` が保持するが、登録後にCPU側ピクセルデータを保持し続けることを意味しない。
現在の実装では、GPUアップロード後に `texture_pixel_unload()` を呼び出してCPU側ピクセルデータを解放する。
そのため、登録後の `texture_t*` は、主にテクスチャ名や登録状態を保持するCPU側リソースオブジェクトとして扱われる。

## Resource Layerとの関係

`Texture System` は、CPU側テクスチャリソースの生成・ピクセルロード・ピクセル解放のために `Resource Layer` の `texture` APIを利用する。

`Resource Layer` は、外部ファイルやビルトインデータをCPU側リソース表現へ変換する責務を持つ。
`Texture System` は、そのCPU側リソースを複数管理し、GPU側リソースと対応づける責務を持つ。

詳細は [Resource Layer](../../resource/architecture_ja.md) を参照のこと。

## Renderer Backendとの関係

`Texture System` は、GPU側テクスチャリソースの生成・破棄・ピクセルアップロードのために `Renderer Backend` の Texture API を利用する。

具体的には、以下のAPIを利用する。

- `renderer_backend_texture_create()`
- `renderer_backend_texture_destroy()`
- `renderer_backend_texture_pixel_upload()`

`Texture System` はGPU側リソースの具体的な実装には依存しない。
OpenGL 3.3などの具体的なグラフィックスAPI差異は、`Renderer Backend` のConcrete実装によって吸収される。

詳細は [Renderer Backend](../renderer_system/renderer_backend/architecture_ja.md) を参照のこと。

## 現状の非対応項目

現状では以下には対応していない。GLCEの機能拡張に伴い、必要に応じて対応する。

- スレッドセーフなAPIの提供
- テクスチャキャッシュ機構
- 参照カウントによるリソース寿命管理
- テクスチャの非同期ロード
- mipmap生成
- テクスチャフィルタ設定の外部指定
- テクスチャwrap設定の外部指定
- テクスチャファイル検索パスの外部指定
- BMP以外の通常画像ファイル形式のロード
- 複数Renderer Backend Contextへの同時対応

## 設定方法

現状では、`texture_manager_initialize()` の `max_texture_count_` により、管理可能なテクスチャ数の上限を指定する。

また、`texture_manager_register()` の `gpu_unit_num_` により、登録するGPU側テクスチャリソースで使用するテクスチャユニット番号を指定する。

それ以外の設定項目は現時点では固定である。
たとえば、GPU側テクスチャ作成時のfilter / wrap設定は、現在の実装では以下に固定されている。

- min filter: `TEXTURE_MIN_FILTER_CONFIG_NEAREST`
- mag filter: `TEXTURE_MAG_FILTER_CONFIG_NEAREST`
- wrap s: `TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE`
- wrap t: `TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE`

また、通常テクスチャファイルのロードでは、現在の実装上、以下のパス規則を使用する。

- ベースパス: `assets/textures/`
- 拡張子: `.bmp`

## 参照

- CPU側リソース表現については、[Resource Layer](../../resource/architecture_ja.md)を参照のこと。
- GPU側Texture APIについては、[Renderer Backend](../renderer_system/renderer_backend/architecture_ja.md)を参照のこと。
