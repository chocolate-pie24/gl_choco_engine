<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Coding Style

## このドキュメントの使い方

このドキュメントは、GLCEの設計方針・設計判断・内部モデルを、人間とAIの双方が参照できる形で保存することを目的としています。

内容は、設計意図を後から復元できることを重視しているため、一般的なプロジェクトドキュメントより詳細かつ冗長に記述されている場合があります。最初から最後まで通読することを必須とはしていません。

GLCEの設計について知りたい場合は、このドキュメントをAIへ読み込ませた上で、知りたい内容を質問したり、必要な範囲を要約させたりする利用方法を推奨します。

このドキュメント自体の作成・更新にもAIを積極的に利用しています。GLCEでは、設計についてAIと議論し、設計判断が追加・変更された際には、その内容を説明するセクションや記述をAIによって生成し、既存ドキュメントへ追記・挿入する運用を行っています。

そのため、異なる時期に追加された複数のセクションが同じ背景や設計原則を別の観点から説明し、内容が部分的に重複する場合があります。この冗長性は、設計変更の経緯や判断理由を失わず、AIが後から設計意図を復元しやすくするため、ある程度意図的に許容しています。

一方で、同じ方針について複数の異なる正本が生まれることや、古い設計と新しい設計が矛盾したまま残ることは避けます。そのため、AIによる横断的なレビューも利用しながら、文書間および文書内の重複、矛盾、古くなった前提、不要になった記述を定期的に見直し、必要に応じて統合・整理します。

このドキュメントはGLCEの現在の設計方針を表しますが、プロジェクトの発展に伴って更新されることがあります。実装経験からより適切なモデルや方針が得られた場合、既存の設計、実装、および文書そのものを大きく変更することがあります。

GLCEではAIを、単なる文章生成手段ではなく、設計議論、設計レビュー、矛盾の探索、既存方針との照合、ドキュメント生成・更新などに広く利用しています。ただし、AIによる提案や生成内容がそのままGLCEの方針になるわけではありません。最終的に採用された内容は、GLCEの目的、制約、実装経験、および設計思想に基づくプロジェクト上の判断です。

このドキュメントに記載されている方針は、一般的なソフトウェア開発に対する普遍的なベストプラクティスを主張するものではありません。GLCEの目的、制約、設計思想に基づいて採用されているプロジェクト固有の方針です。

最終更新: 2026-10-02

## 目的

GLCE全体で、ディレクトリ構成、命名、API設計、include、Doxygen、エラーメッセージの粒度を揃える。

この文書は、既存コードの一括修正だけでなく、今後の新規モジュール作成、レビュー、リファクタリング時の判断基準として使用する。

## 基本方針

- 名前から責務と所属モジュールが分かることを優先する。
- 公開APIは一貫性を優先し、private関数は読みやすさを優先する。
- headerは利用者に必要な最小限を公開する。
- sourceは自分が直接使う依存を自分でincludeする。
- エラーメッセージは、原因調査に必要な情報を過不足なく含める。
- 命名、include、コメント、メッセージの整備では、原則として処理内容やAPI契約を変えない。動作変更が必要な場合は別タスクとして扱う。

## ディレクトリ名 / ファイル名

### ディレクトリ名

- ディレクトリは分類、配置、レイヤーを表す。
- カテゴリディレクトリは複数形を許可する。
- `core`は、その所有領域内の複数モジュールで共通利用する型、結果コード、エラー変換、下位レイヤーAPIのラッパーなどを置く場所とする。
- ファイル名と公開API prefixは、Cシンボル上の公開モジュール名を表す。
- ディレクトリ名を機械的にファイル名や公開API prefixへ転写しない。

### ディレクトリ名とモジュール名の関係

ディレクトリ名は配置上の分類であり、原則としてそのままファイル名や公開API prefixへ含めない。

避けたい例:

```text
resource_pipelines/lit_mesh_geometry.h
resource_pipelines_lit_mesh_geometry_import_from_file()
```

この場合、`resource_pipelines`は配置カテゴリであり、公開API prefixへ転写しない。

ただし、`pipeline`、`registry`、`shader`、`loader`のように、モジュールの役割を表す語はファイル名や公開API prefixへ含めてよい。

許容する例:

```text
resource_pipelines/lit_mesh_geometry_pipeline.h
lit_mesh_geometry_pipeline_import_from_file()
```

この場合、`pipeline`はディレクトリ名の転写ではなく、`lit_mesh_geometry`をimport、upload、registerするモジュールの役割名である。

親ディレクトリ名またはその一部をモジュール名へ含めるのは、次のいずれかに該当する場合とする。

- ファイル単体で見たときに、名前だけでは責務や対象領域が曖昧になる。
- 同名または近い名前のモジュールが別領域にも存在し、衝突や誤読を避けたい。
- 公開API prefix、include guard、Doxygen `@file`など、パス情報なしで外部に露出する名前の基準になる。
- 親ディレクトリ名またはその一部が、単なる分類ではなくモジュール概念の一部になっている。

親ディレクトリ名またはその一部をモジュール名へ含めないのは、次のいずれかに該当する場合とする。

- 親ディレクトリ名が単なるカテゴリであり、子の名前だけで責務が十分に分かる。
- 親名を重ねることで同じ語が連続する、または名前が不自然に長くなる。
- ディレクトリ階層をそのまま関数名やファイル名へ転写しているだけで、意味の追加がない。
- 同一モジュール内のprivate関数やprivate fileで、公開名として外へ出ない。

### 公開範囲と内部実装の配置

ファイルの配置は、最初に公開範囲を決め、次に責務の所有者を決める。

- Applicationを含むEngine利用者へ公開するheaderは`include/engine/`へ置く。
- Engine内部だけで使用するsourceおよびheaderは`engine/`へ置く。
- `engine/`に置かれたheaderは、Engine内部モジュール間の契約であり、Application向け公開APIではない。
- testがEngine内部headerを参照する場合は、white-box testに限定した特権的アクセスとして扱い、公開API化とは区別する。
- ファイルの配置は`.c`または`.h`という拡張子ではなく、そのファイルの責務と所有者によって決める。

#### `internal`ディレクトリ

`internal/`は、親モジュールのprivateな表現を複数の実装ファイルで共有するために使用する。

格納対象は次とする。

- public headerでopaqueにしている型の実体定義
- 複数の実装`.c`が共有するprivate型
- 複数の実装`.c`から使用するprivate関数宣言
- 親モジュール内部だけで成立する状態、不変条件および補助定義

次は`internal/`へ格納しない。

- Application向け公開API
- 独立した責務を持つ内部モジュール
- 特定のConcrete実装だけに属する型または関数
- 一つの`.c`からしか使用しないstatic関数およびprivate型
- テスト専用APIおよび失敗注入設定

`internal/`を、非公開ファイルを無条件に集める汎用ディレクトリとして使用しない。

#### `.c`と`.h`の配置

一つの内部モジュールを構成する`.c`と`.h`は、同じ責務ディレクトリへ置く。

内部モジュールの`.h`は、同じ内部階層にある別の`.c`へ提供する契約を宣言する。Applicationへ公開する必要があるheaderは、対応する`.c`と分離して`include/engine/`へ置く。

headerの宣言が一つの`.c`からしか使用されない場合は、原則としてheaderを作成せず、型および関数をその`.c`へ閉じ込める。

#### `core`ディレクトリ

`core/`は、親ディレクトリが表す所有領域のうち、複数の兄弟モジュールから共通利用される定義または実装を置く。

格納対象は次とする。

- 複数の公開モジュールが共有する値型、result型および共通定数
- 複数の内部モジュールが共有するresult変換、error utilityおよび構築補助
- 親領域全体が所有する下位レイヤーAPIのラッパー

Applicationを含むEngine利用者へ公開する共通型は、`include/engine/`側の対応する`core/`へ置く。Engine内部だけで使用するheaderおよびsourceは、`engine/`側の対応する`core/`へ置く。

```text
include/engine/systems/renderer/renderer_backend/core/renderer_backend_types.h
engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h
engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.c
```

次は`core/`へ格納しない。

- `types.h`、`result_t`または`status_t`という名前だけを理由に移動するファイルまたは型
- 一つのモジュールだけが所有する状態、設定、statusおよびprivate型
- 一つの`.c`からしか使用しない補助関数
- 所有領域を越えて無関係なモジュールを集めた汎用定義

型のsuffixではなく、共有範囲と所有者で配置を決める。例えば、`range_allocation_t`はVBO Managerからも利用するが、Range Allocatorが所有するallocation descriptorであるため、Engine内部のRange Allocatorモジュールへ置く。利用側で完全型が不要な場合は前方宣言を使用する。一方、VBO Managerと将来のEBO Managerが共有するBuffer Manager共通型は、`engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h`へ置く。

`range_allocation_t`はApplication向けpublic APIへ公開するopaque型ではなく、Engine内部で使用するallocation descriptorである。現行設計でprivate化できない依存がある場合も、型またはallocatorへのpointerをpublic contractへ露出させるのではなく、設計側を見直してEngine内部へ閉じる構造へ寄せる。

`core/`を階層ごとに機械的に追加しない。親領域の選択を誤った結果として`core/allocators/core/`のような構造が生じる場合は、先に所有領域を見直す。

#### 責務名ディレクトリ

複数のファイルが共通の概念、契約または実装ファミリーを形成し、それ自体に独立した責務名を付けられる場合は、`internal/`ではなく責務名のディレクトリを設ける。

例:

- `vtables/`: Graphics APIに依存しないStrategy契約
- `renderer_backend_concretes/`: Renderer BackendのConcrete実装を分類するカテゴリ
- `renderer_backend_concretes/gl33/`: OpenGL 3.3によるStrategy契約の実装ファミリー

責務名ディレクトリへ置けるファイル種別を`.c`または`.h`に限定しない。

現在は型と宣言だけを持つためheaderしか存在しないディレクトリでも、将来その責務に属する共通処理を実装する場合は、同じディレクトリへ`.c`を追加してよい。

ただし、契約型を使用しているという理由だけで、その実装を契約側ディレクトリへ置かない。Concrete実装、Context初期化、実装選択などは、それぞれ実際に処理を所有するモジュールへ置く。

Concreteが一種類しか存在しない場合でも、親モジュール直下の契約、選択処理およびConcrete固有実装を区別する必要があるなら、Concreteカテゴリを設けてよい。将来別のBackendを追加する場合は、`renderer_backend_concretes/`の下へ実装ファミリー単位で追加する。

#### 配置判断

新しいファイルの配置は、次の順序で判断する。

1. Engine利用者へ公開する必要があるか。
   - 公開する場合は`include/engine/`へ置く。
   - 公開しない場合は`engine/`へ置く。
2. 親モジュールのprivateな表現か。
   - 複数の実装`.c`で共有する場合は、所有モジュールの`internal/`へ置く。
3. 独立した責務を持つ内部モジュール、契約または実装ファミリーか。
   - 責務名のディレクトリを設けて配置する。
4. 一つの`.c`へ閉じ込められる実装詳細か。
   - headerを新設せず、その`.c`内へ閉じ込める。

#### Rendererへの適用例

Applicationを含むEngine利用者へ公開するRenderer APIは、次のように配置する。

```text
include/engine/systems/renderer/
├── config/
│   └── renderer_config.h
├── core/
│   └── renderer_types.h
├── render_resources/
│   ├── core/
│   │   └── render_resource_types.h
│   ├── line_mesh_render_resource.h
│   ├── lit_mesh_render_resource.h
│   ├── point_mesh_render_resource.h
│   └── ui_mesh_render_resource.h
├── renderer_backend/
│   ├── core/
│   │   └── renderer_backend_types.h
│   ├── renderer_backend_context.h
│   ├── renderer_backend_shader.h
│   ├── renderer_backend_texture.h
│   ├── renderer_backend_vao.h
│   └── renderer_backend_vbo.h
├── resource_pipelines/
│   ├── core/
│   │   └── resource_pipeline_types.h
│   ├── geometries/
│   │   ├── line_mesh_geometry_pipeline.h
│   │   ├── lit_mesh_geometry_pipeline.h
│   │   ├── point_mesh_geometry_pipeline.h
│   │   └── ui_mesh_geometry_pipeline.h
│   └── texture/
│       └── texture_pipeline.h
├── resource_registries/
│   ├── core/
│   │   └── resource_registry_types.h
│   ├── geometries/
│   │   ├── line_mesh_geometry_registry.h
│   │   ├── lit_mesh_geometry_registry.h
│   │   ├── point_mesh_geometry_registry.h
│   │   └── ui_mesh_geometry_registry.h
│   └── texture/
│       └── texture_registry.h
└── resources/
    ├── shaders/
    │   ├── line_mesh_shader.h
    │   ├── lit_mesh_shader.h
    │   ├── point_mesh_shader.h
    │   └── ui_mesh_shader.h
    └── texture/
        ├── texture_gpu_resource_types.h
        └── texture_gpu_resource.h
```

Engine内部のRenderer実装は、次のように配置する。

```text
engine/systems/renderer/
├── config/
│   └── renderer_config.c
├── core/
│   └── renderer_types.c
├── render_resources/
│   ├── core/
│   │   ├── render_resource_err_utils.c
│   │   └── render_resource_err_utils.h
│   ├── line_mesh_render_resource.c
│   ├── lit_mesh_render_resource.c
│   ├── point_mesh_render_resource.c
│   └── ui_mesh_render_resource.c
├── renderer_backend/
│   ├── core/
│   │   ├── renderer_backend_err_utils.c
│   │   └── renderer_backend_err_utils.h
│   ├── internal/
│   │   └── renderer_backend_context_internal.h
│   ├── renderer_backend_concretes/
│   │   └── gl33/
│   │       ├── gl33_shader.c
│   │       ├── gl33_shader.h
│   │       ├── gl33_texture.c
│   │       ├── gl33_texture.h
│   │       ├── gl33_vao.c
│   │       ├── gl33_vao.h
│   │       ├── gl33_vbo.c
│   │       └── gl33_vbo.h
│   ├── vtables/
│   │   ├── renderer_backend_shader_vtable.h
│   │   ├── renderer_backend_texture_vtable.h
│   │   ├── renderer_backend_vao_vtable.h
│   │   └── renderer_backend_vbo_vtable.h
│   ├── renderer_backend_context.c
│   ├── renderer_backend_shader.c
│   ├── renderer_backend_texture.c
│   ├── renderer_backend_vao.c
│   └── renderer_backend_vbo.c
├── resource_pipelines/
│   ├── core/
│   │   ├── resource_pipeline_err_utils.c
│   │   └── resource_pipeline_err_utils.h
│   ├── geometries/
│   │   ├── line_mesh_geometry_pipeline.c
│   │   ├── lit_mesh_geometry_pipeline.c
│   │   ├── point_mesh_geometry_pipeline.c
│   │   └── ui_mesh_geometry_pipeline.c
│   └── texture/
│       └── texture_pipeline.c
├── resource_registries/
│   ├── core/
│   │   ├── resource_registry_err_utils.c
│   │   └── resource_registry_err_utils.h
│   ├── geometries/
│   │   ├── line_mesh_geometry_registry.c
│   │   ├── lit_mesh_geometry_registry.c
│   │   ├── point_mesh_geometry_registry.c
│   │   └── ui_mesh_geometry_registry.c
│   └── texture/
│       └── texture_registry.c
└── resources/
    ├── allocators/
    │   ├── range_allocator.c
    │   └── range_allocator.h
    ├── buffer_managers/
    │   ├── core/
    │   │   ├── buffer_manager_err_utils.c
    │   │   ├── buffer_manager_err_utils.h
    │   │   └── buffer_manager_types.h
    │   ├── vbo_manager.c
    │   └── vbo_manager.h
    ├── shaders/
    │   ├── core/
    │   │   ├── shader_err_utils.c
    │   │   ├── shader_err_utils.h
    │   │   ├── shader_program_builder.c
    │   │   ├── shader_program_builder.h
    │   │   ├── shader_resource_types.c
    │   │   └── shader_resource_types.h
    │   ├── line_mesh_shader.c
    │   ├── lit_mesh_shader.c
    │   ├── point_mesh_shader.c
    │   └── ui_mesh_shader.c
    └── texture/
        ├── texture_gpu_resource_err_utils.c
        ├── texture_gpu_resource_err_utils.h
        └── texture_gpu_resource.c
```

この構成では、`include/engine/`側に公開契約だけを置き、Engine内部の共有表現、補助実装、Strategy契約およびConcrete実装を`engine/`側へ置く。公開側と内部側のディレクトリを機械的に完全一致させず、それぞれの公開範囲と所有者に必要なファイルだけを配置する。

Range Allocator、VBO ManagerおよびShaderの内部共通型はEngine内部の実装契約であり、Application向け公開APIへ置かない。Application向け公開contractから内部allocation descriptorまたはallocatorへのpointerが必要になる場合は、それらを公開するのではなく、上位APIの責務を見直す。

`resources/`直下の`allocators/`、`buffer_managers/`、`shaders/`および`texture/`には依存関係上の高低差があるが、ディレクトリ階層は呼び出し順序ではなく、Rendererが所有するリソース責務の分類を表す。ShaderがBuffer Managerを使用することだけを理由に、両者を同じモジュールへ統合したり、実行時依存をそのままディレクトリ階層へ転写したりしない。

`render_resources/`は、ShaderやRegistryなどの下位resourceをtopology単位のlifetime／ownershipとして束ねる長寿命module群を置く。具体的なRenderer ownership設計と判断理由は`design_decisions.md`を正本とする。

`resource_pipelines/`は複数の下位リソース操作を編成して構築処理を実行し、`resource_registries/`は構築済みリソースの検索および保持を担当する。これらは単一のGPUリソース実装ではないため、`resources/`の外へ独立したRenderer上位カテゴリとして置く。

Renderer Backendでは、公開操作API、親モジュールのprivateな表現、Graphics API非依存のStrategy契約、ConcreteカテゴリおよびGL33固有実装を、それぞれ異なる所有者として表現する。`vtables/`は契約を所有し、`renderer_backend_concretes/gl33/`はそのOpenGL 3.3実装を所有する。

Renderer Backendの現在の契約はOpenGLのVAO/VBO概念を採用しているため、対応するモジュール名には`vao` / `vbo`を用いる。将来のGraphics APIを予測して`vertex_array` / `vertex_buffer`へ一般化せず、実際に異なる抽象化が必要になった時点で契約ごと再設計する。

`vao`はOpenGLのVertex Array Objectを指す場合にのみ使用する。CPU側の頂点配列やそのバイト数には`vertex_array` / `vertex_data`を使用し、機械的に`vao`へ置換しない。同様に、`vbo`はOpenGLのVertex Buffer Objectまたはそれを直接管理するリソースを指す。

`vtables/`にheaderしか存在しないのはheader専用ディレクトリだからではなく、現時点のvtableが型と契約だけを持つためである。将来、vtable共通の検証処理などが必要になり、その処理をvtableが所有する場合は、対応する`.c`を`vtables/`へ置く。

### ファイル名

- ファイル名は、そのディレクトリ内でのモジュール名を表す。
- プロジェクト全体で一意にしたいモジュールは、必要に応じて対象名や役割名を含めた完全モジュール名をファイル名にする。
- 公開API prefix、include guard、Doxygen `@file`は、この完全モジュール名を基準に揃える。
- ファイル名は単数形を基本にする。
- 公開APIのprefixは、原則としてファイル名と一致させる。
- `pipeline`、`registry`、`shader`、`loader`などの役割名は、モジュールを区別するために必要であればファイル名へ含める。
- `types.h`、`err_utils.h`のような共通補助ファイルは慣用名として扱ってよい。

## 単数形 / 複数形

- モジュール名、型名、関数prefixは単数形を基本にする。
- 複数の同種モジュールをまとめるカテゴリディレクトリは複数形でよい。
- `resources`、`registries`、`pipelines`のようなカテゴリ名と、`resource_registry`のようなモジュール名を混同しない。
- 迷った場合は、「その名前が1つのモジュールを指すか、複数モジュールの置き場を指すか」で判断する。

## 公開API命名

公開APIは次の形を基本にする。

- `module_prefix + operation`
- `module_prefix`はファイル名と一致させる。
- ディレクトリ名は、機械的にprefixへ含めない。
- `pipeline`、`registry`、`shader`、`loader`などの役割名は、モジュール名として採用する場合にprefixへ含める。
- 操作対象がmodule prefixから明らかな場合、同じ名詞をoperation側で繰り返さない。

### 例

| 避けたい形 | 方針 |
|---|---|
| `resource_pipelines_lit_mesh_geometry_import_from_file` | ディレクトリ名をprefixへ転写しない |
| `lit_mesh_geometry_import_from_file` | geometry本体APIとpipeline APIが混ざる場合は役割名を含める |
| `lit_mesh_geometry_registry_geometry_register` | `geometry`の重複を避ける |
| `*_valid_check` | boolの意味が分かる名前にする |

## 関数名共通規約

- 関数名は、呼び出し側から見た操作内容を表す。
- 戻り値を持つ関数は、戻り値の意味が名前から推測できるようにする。
- `bool`を返す関数は、`true`が何を意味するか名前から分かるようにする。
- `check`単独のように、成功条件が名前から分からない動詞は避ける。
- 公開APIとprivate関数のどちらも、この規約に従う。

### bool関数の推奨パターン

| 用途 | 推奨例 | 避けたい例 |
|---|---|---|
| 状態判定 | `*_is_initialized` | `*_check_initialized` |
| 存在確認 | `*_exists`, `*_name_exists` | `*_find`を単なる存在確認に使う |
| 所有確認 | `*_has_geometry` | `*_check_geometry` |
| 妥当性判定 | `*_is_valid` | `*_validate`をboolで返す |
| 範囲判定 | `*_is_in_range` | `*_check_range` |

`find`は、対象を探索してID、pointer、indexなどの検索結果を取得する処理に使用する。単に「対象が存在するか」を`bool`で返すだけのAPIには`exists`を使用する。

## 動詞の語彙

| 動詞 | 用途 |
|---|---|
| `create` | 新しいobject identityを生成し、callee側でstorage/addressを取得してcallerへ公開する |
| `initialize` | objectまたは値へ初期状態を与え、利用可能な状態にする |
| `destroy` | objectのlogical lifetimeを終了し、対象自身の独立解放可能なbacking storage/resourceも解放する |
| `deinitialize` | objectの利用可能状態とowned subresourceを終了するが、対象自身のbacking storageは残す |
| `reinitialize` | 利用可能な既存objectを、明示的な要求に基づいて再構築する |
| `register / unregister` | registryやmanagerへ登録・登録解除する |
| `exists` | 対象が存在するかを判定する。基本は`bool`返却 |
| `find` | 対象を探索し、ID、pointer、indexなどの検索結果を取得する |
| `get` | 値や参照の取得。失敗理由が必要ならresultを返す |
| `load` | ファイルなどからCPU側へ読み込む |
| `upload` | CPU側データをGPU側へ転送する |
| `append` | 既存バッファ末尾へ追加する |
| `write` | 指定位置へ書き込む |
| `update` | 既存状態や既存領域を更新する |
| `import` | load、変換、upload、registerなど複数工程を統括する |
| `release` | 確保済み領域や登録済み関連リソースを解放する |

`create`、`initialize`、`destroy`、`deinitialize`は、API同士の言語的な対称性やallocatorの種類ではなく、そのoperation自身が実際に行う処理から独立に決定する。

- external allocatorを受け取ることだけを理由に`initialize`とはしない。calleeがそのallocatorから新しいobject identityのstorageを取得し、callerへ公開するなら`create`になり得る。
- `create`したobjectであっても、対象自身のbacking storageを個別解放できず、利用可能状態とowned subresourceだけを終了する場合は`deinitialize`を使用してよい。したがって`create -> deinitialize`のような非対称な組み合わせを許容する。
- `initialize`はcallerが既に保持するobject/storageをin-placeで利用可能にする操作が代表例である。ただし、軽量なvalue typeを値返却で構築する場合にも、慣例的で意味が明確なら`initialize`を使用してよい。
- 名前をより狭い、または実装詳細に近い動詞へ変更できるという理由だけではrenameしない。現在の名前がcallerから見たoperationを自然かつ正確に表している場合は、その語彙を維持する。

## 引数順序

基本順序は以下。

1. 実行主体または所有者: context、manager、registry、allocatorなど
2. 操作対象インスタンス
3. 入力値
4. in/out値
5. 出力先

既存コードに揺れがある場合は、最終的にプロジェクト全体の規約へ統一する。

## 引数名 / ローカル変数名

引数名およびローカル変数名は、型情報を機械的に名前へ複写するのではなく、そのscope内における値またはobjectのsemantic roleを表すことを基本とする。

型は「その値が具体的に何であるか」を表し、変数名は「その場所でどの意味を持つか」を表す。

変数名は、型が表す具体的な概念から**一段上のsemantic abstraction**を使用することを基本とする。

```c
linear_allocator_t* allocator_;
range_allocator_t* allocator_;
renderer_backend_context_t* backend_context_;
engine_event_view_t* event_view_;
application_flight_camera_t* flight_camera_;
application_renderer_t* renderer_;
```

この原則は、名前を単に短くするための規則ではない。具体型が持つmodule ownership、implementation specificity、または具体的なkindを一段だけ外し、そのscopeで必要なsemantic informationを保持することを目的とする。

「一段上」は、型名から単語を一つ削除するという文字列上の操作ではない。削除対象のqualifierが、具体的な実装・所属・種類を表しているのか、それともobjectのsemantic roleそのものを構成しているのかを判断する。

### semantic roleを構成する語を保持する

対象、関係、用途などを表し、その語を失うと「何をするobjectか」「何に対するobjectか」が曖昧になる場合、その語はsemantic compoundの一部として保持する。

```c
engine_event_view_t* event_view_;               // use
engine_event_view_t* view_;                     // avoid

renderer_backend_context_t* backend_context_;   // use
renderer_backend_context_t* context_;           // avoid

vbo_manager_t* vbo_manager_;                    // use
vbo_manager_t* manager_;                        // avoid

window_event_storage_t* window_event_storage_;  // use
window_event_storage_t* window_storage_;        // avoid
```

例えば`engine_event_view`は、次の概念階層を持つ。

```text
engine_event_view
    ↓
event_view
    ↓
view
```

`event_view_`は直接上位のsemantic abstractionであるため使用できるが、`view_`まで一般化すると「eventを参照するview」という意味を失うため使用しない。

同様に、`VBO Manager`における`VBO`は単なるmanagerの実装種別ではなく、「VBOを管理するobject」という役割を成立させる対象語である。そのため`manager_`まで一般化しない。

```text
vbo_manager
    ↓
manager   // avoid: 何を管理するmanagerかが失われる
```

`window_event_storage`における`event`も、「window eventを保持するstorage」というsemantic roleの一部である。途中の概念だけを削除して`window_storage_`へ変形しない。

型名と変数名が同一であっても、その名称全体がsemantic roleを直接表している場合は、機械的に短縮する必要はない。

### 具体的なkind・所属・実装を表すqualifierは一段抽象化してよい

qualifierを取り除いてもobjectの基本的な役割が保たれる場合は、直接上位のsemantic abstractionを変数名として使用する。

```text
application_renderer
    → renderer

application_flight_camera
    → flight_camera

engine_event_view
    → event_view

renderer_backend_context
    → backend_context

linear_allocator
    → allocator

range_allocator
    → allocator
```

例えば`range_allocator`における`range`はallocatorの具体的な管理方式・kindを表す。`range`を外しても「allocator」というobjectの役割は保持されるため、通常は`allocator_`を使用できる。

一方、同一scope内に複数種類のallocatorが存在し、そのkindを区別すること自体に意味がある場合は、必要なqualifierを保持してよい。

```c
linear_allocator_t* linear_allocator_;
range_allocator_t* range_allocator_;
```

### 一段上の抽象化に対する特例

一段上のsemantic abstractionを基本とするが、scope内で役割が一意であり、さらに抽象化してもsemantic informationを失わない定着したrole nameについては、複数段階を抽象化してよい。

代表例としてconfiguration objectは`config_`を使用してよい。

```c
renderer_config_t* config_;
platform_system_config_t* config_;
event_system_config_t* config_;
line_mesh_shader_config_t* config_;
```

例えば`line_mesh_shader_config_t*`を`shader_config_`で止める必要はない。対象functionまたはmoduleのscope内でconfiguration objectが一意であれば、`config_`だけでsemantic roleが明確であるためである。

ただし、この特例を一般化して、意味のあるdomain conceptまで削除してはならない。`config_`のようにscope内で役割が一意に定まり、追加のqualifierを保持する実益がない場合に限って適用する。

### 型名の機械的な複写を避ける

次のように、型名をそのまま変数名へ複写することは、それ自体を目的として行わない。

```c
renderer_config_t* renderer_config_;
linear_allocator_t* linear_allocator_;
application_renderer_t* application_renderer_;
engine_event_view_t* engine_event_view_;
```

ただし、型名と同じ語がsemantic roleとして必要である場合は、そのまま使用してよい。`vbo_manager_`や`window_event_storage_`はこの例に該当する。

### scope内での識別に必要なqualifier

同種または類似したobjectが同一scope内に複数存在する場合、またはそれぞれの役割の違いがAPI contract上重要な場合は、必要なsemantic qualifierを追加する。

qualifierは、可能な限り型の種類ではなくsemantic roleを表す。

```c
linear_allocator_t* persistent_allocator_;
linear_allocator_t* temporary_allocator_;
```

複数のobjectを区別するために必要なsemantic informationは削除しない。

```c
window_event_storage_t* window_event_storage_;
keyboard_event_storage_t* keyboard_event_storage_;
mouse_event_storage_t* mouse_event_storage_;
```

将来同種objectが追加される可能性だけを理由に、現在不要なqualifierを先回りして追加しない。実際に複数objectを区別する必要が生じた時点で、その役割に応じてrenameする。

この規則は主にfunction parameterおよびlocal variableへ適用する。

struct fieldは、そのstructのscope内で複数の同種概念を永続的に保持することが多いため、field同士を明確に識別するためにmodule名、object名またはrole qualifierを積極的に使用してよい。

```c
typedef struct application_state {
    renderer_config_t renderer_config;
    platform_system_config_t platform_system_config;
    event_system_config_t event_system_config;
} application_state_t;
```

## オブジェクトの構築と公開状態

### 完全コンストラクタ

`create`または`initialize`は、成功時に通常操作へ使用できるstable stateを公開する完全コンストラクタを基本とする。

- 成功時には、そのobjectの通常操作に必要な必須resource、所有関係および依存関係をすべて確立する。
- 構築中だけ必要な部分初期化状態はprivate temporaryへ閉じ込め、callerへ公開しない。
- 一度しか実行しない必須初期化手順を、合理的な理由なく複数のpublic APIへ分割しない。
- 完全コンストラクタは、将来使用する可能性があるoptional resource、遅延生成resourceまたは動的resourceをすべて事前生成することまでは要求しない。
- constructor成功後に許可される通常操作と、objectが満たすstable stateをpublic contractで明示する。
- constructorが所有resourceを取得する場合、可能な範囲でresource取得処理をconstructor本体にまとめ、resourceの取得順序、temporary ownership、ownership commitおよびfailure時のcleanupを一つのlifecycleとして追える構造を優先する。関数を短くすることだけを目的として、resource取得処理をprivate helperへ分散させることは避ける。
- resource取得を開始する前に判定できるinput condition、size calculation、overflow等は、可能な範囲でconstructor冒頭の`Preconditions`で処理する。resource取得開始後に、それ以前から判定可能だったconditionによってfailureする経路を不要に作らない。
- ただし、独立したsemantic operationとして切り出す方が責務や可読性が明確になる場合、複数箇所で共有する必要がある場合、またはconstructor本体へ置くことでかえってlifecycleを追いにくくなる場合はprivate helperを使用してよい。
- 構築途中のtemporary、validationおよびownership commitの具体的な順序は、Validation Policyを正本とする。
- constructor内部のknown partial stateをrollbackするため、public destroyとは別にprivateなunchecked cleanup helperを設けてよい。
- unchecked cleanup helperは、そのconstruction path自身が確立したpartial stateだけを対象とし、public APIのvalidationを迂回する一般的な破棄手段として使用しない。
- unchecked cleanup helperの名前には`destroy_unchecked`など、validationを行わないことが分かる語を使用する。
- `DATA_CORRUPTED`が確定した後は、unchecked cleanup helperを含む通常rollbackを行わず、Validation Policyのfail-stop ruleを優先する。

`filesystem_t`／`fs_stream_t`はこの原則の適用例である。create成功時のnon-NULL objectはopen済みで通常利用可能であり、publicなclosed state、独立open API、再open前提のstateを持たない。

### 完成と公開、非公開化と破棄を分離する

Object lifecycleでは、「完成」と「公開」、「非公開化」と「破棄」を別の状態遷移として扱う。

```text
構築中
    ↓ 完成
利用可能だが未公開
    ↓ 登録 / 公開
検索・新規参照取得可能
    ↓ 登録解除 / 非公開化
新規参照取得不可
    ↓ 既存参照のlifetime終了
破棄可能
```

次を原則とする。

1. **完成するまで検索・参照可能にしない。**
   - constructor途中のpartial objectをRegistry等へ登録しない。
   - Registryへのownership commitや公開は、objectがstable stateへ到達した後に行う。
2. **公開中のobjectを直接破棄しない。**
   - Registry等から検索可能なobjectを破棄する場合は、先に登録解除して新規参照取得を停止する。
   - `unregister`と`destroy`を概念上同一視しない。
3. **登録解除後のexisting reference lifetimeをcontract化する。**
   - unregister後も既存borrow/referenceの存続を許可する場合は、最後の参照が消滅するまで実体を保持する。
   - unregister時点で既存参照が存在しないことをcaller contractで保証する場合は、reference counting等を機械的に導入する必要はない。

この原則は、すべてのRegistry objectへreference countingやdeferred destructionを要求するものではない。必要なのは、公開状態と実体lifetimeを区別し、unregister後に既存参照が存続し得るかどうかをcontractとして明示することである。

### 段階的初期化を公開する条件

段階的初期化をpublic APIとして残すのは、次のいずれかに該当する場合とする。

- 中間状態自体に独立した利用価値があり、その状態で許可される通常操作を定義できる。
- 初期化工程をcallerが選択または反復することが、モジュールの正式な要件である。
- 外部resourceのlifetimeまたは非同期処理によって、単一のtransactionとして完了できない。

段階的初期化を公開する場合は、各stable stateの不変条件、許可操作、禁止操作、遷移条件およびdestroy contractを明示する。構築実装を分割しやすいことだけを理由に、部分初期化状態を公開しない。

### 再構築

scene切り替え、容量変更またはruntime設定変更などによって構築済みobjectの再構築が必要な場合は、初期constructorを分割せず、`reinitialize`などの明示的な別APIとして扱う。

- 初期構築と再構築のcontractを分離する。
- 再構築対象と、再構築によって無効化されるID、handle、descriptorおよびborrowed referenceを明示する。
- 可能な限りtemporaryへ新しい状態を構築し、成功時だけ置き換えるtransactional contractとする。
- 既存状態を維持できない場合は、commit位置、失敗後状態およびrollback不可であることを明示する。

## 所有依存と借用依存

objectがpointerまたはhandleを保持する場合、その参照がownershipかborrowかを明確にする。

- ownerは、所有resourceのlifetime終了と対応するdestroy、deinitializeまたはreleaseに責任を持つ。
- borrowerは参照先をdestroyせず、contractで許可された期間と操作にだけ使用する。
- borrowed pointerの複製はownershipの複製ではない。
- borrowはmutabilityとは独立した概念である。読み取り専用なら`const T*`を使用し、参照先への操作が必要ならborrowed `T*`を使用してよい。
- pointerを保持しない呼び出し中だけの一時参照と、object fieldとして保持する長寿命borrowを区別する。

### System-lifetime dependencyのborrow

Backend Contextのようにsystem初期化後から終了直前まで存在するdependencyは、次をすべて満たす場合に限り、長寿命objectがborrowed pointerとして保持してよい。

- dependencyのlifetimeが、すべてのborrowerのlifetimeを確実に包含する。
- dependencyのaddressが、そのlifetime中に変化しない。
- borrowerが生存中に同じdependencyを継続して使用する。
- 別dependencyへの差し替えまたはmigrationを暗黙に行わない。
- borrowerがdependencyをdestroyしない。
- 上位ownerが、すべてのborrowerをdependencyより先にdestroyする順序を保証する。

これらを満たす場合、dependencyはconstructorで一度だけ関連付け、同じdependencyを通常操作またはdestroyのたびに外部から再指定させない。これにより、生成時と異なるdependencyを後続操作へ誤って渡すAPI経路をなくす。

一方、同じ引数が複数APIに現れることだけを理由に、長寿命borrowを導入しない。dependencyのlifetime、address、差し替え可能性またはdestroy順を保証できない場合は、明示引数または別の所有構造を維持する。

## 文字列表現の使い分け

文字列表現は、validation項目の多さではなく、所有、保持、変更、metadata管理およびsemantic identityの必要性で選択する。

| 表現 | 主な用途 |
|---|---|
| `const char*` | 呼び出し中だけ参照するNUL終端文字列、string literal、外部API由来の文字列 |
| `choco_string_t` | 文字列を所有、保持または変更し、length／capacityなどのmetadataを管理するobject |
| `fs_path_t` | 完成済みpathとしての意味、不変条件および所有権を持つobject |

- textual contentsを呼び出し中だけ読み、保持も変更もしないAPIは`const char*`を基本とする。
- `const char*`のcallerは、呼び出し完了まで読み取り可能でNUL終端された文字列を渡す責任を持つ。
- ownership、呼び出し後の保持、変更またはmetadata管理が必要な場合に`choco_string_t`を使用する。
- calleeがGLCE string objectのmetadataまたはobjectとしての不変条件を必要とする場合は、borrowed `const choco_string_t*`を使用してよい。
- pathとしてのsemantic identityと不変条件を所有させる場合に`fs_path_t`を使用する。完成済みpathの文字列を呼び出し中だけ読む下位APIへ、validationのためだけに`fs_path_t`を要求しない。
- validation項目を増やすことだけを理由に、borrowed `const char*`を`choco_string_t`または`fs_path_t`へ置き換えない。
- object validatorも、渡されたobject pointerが解放済みでないことや、参照可能なmemoryを指していることまでは保証できない。C pointerの基本的なlivenessはcaller contractとして残る。
- owner内部のC文字列をgetterで返す場合、その戻り値はborrowed viewとする。callerは解放せず、ownerの変更または破棄によって無効化された後まで保持しない。

## 事前条件検証

### 基本方針

- public APIは、自身がdereferenceするpointerなどのAPI-use／memory-safety contract、自身が所有するownership／lifecycle contract、および自身がsemantic ownershipを持つ条件を、処理の副作用を開始する前に検証する。
- 下位moduleがsemantic ownershipを持つ条件は上位のforwarding／facade／orchestration layerで重複検証せず、そのmoduleへ委譲する。validation responsibilityの詳細は`validation_policy.md`を正本とする。
- 単純なポインタ存在条件と、値、状態、所有関係または複数引数間の意味的条件を区別する。
- 定型的なNULL検証による記述量を抑えるため、`IF_ARG_NULL_GOTO_CLEANUP`および`IF_ARG_NOT_NULL_GOTO_CLEANUP`は用途を限定して使用してよい。
- 任意のbool式を受け取る`IF_ARG_FALSE_GOTO_CLEANUP`は、新規コードおよび今回の改修範囲では使用しない。既存の定義および呼び出しは、各モジュールを改修する際に通常の`if`文へ段階的に置き換え、最終的に削除する。`REQUIRE`など、任意条件を受け取る同種の汎用マクロへ置き換えない。
- 意味的な事前条件は通常の`if`文で明示し、条件に対応するresult code、処理後状態および具体的なエラーメッセージを呼び出し側に記述する。
- private preconditions helperは例外的な整理手段とし、public APIごとに機械的に作成しない。

### NULL検証マクロの責務

NULL検証マクロは、次のポインタ存在条件だけを表す。

| マクロ | 表す契約 |
|---|---|
| `IF_ARG_NULL_GOTO_CLEANUP` | 指定したポインタはNULLであってはならない |
| `IF_ARG_NOT_NULL_GOTO_CLEANUP` | 指定したポインタはNULLでなければならない |

- マクロ名には`GOTO_CLEANUP`を含め、呼び出し元の制御フローを変更することを明示する。
- マクロは、呼び出し側が指定したresult codeの設定、ポインタ存在条件に対応する診断、および`cleanup`への分岐だけを行う。
- マクロ自身は、失敗を`INVALID_ARGUMENT`、`BAD_OPERATION`または`DATA_CORRUPTED`のいずれに分類するかを推測しない。分類はAPI契約を知る呼び出し側が指定する。
- マクロ自身は、コンポーネント状態を変更せず、`component_result_t.state_after`を決定しない。状態遷移および処理後状態は呼び出し側が明示する。
- 関数名と引数式は、可能な限り`__func__`および`#argument`によって取得し、関数名や引数名の文字列を呼び出し側から重複して渡さない。
- 各実引数は1回だけ評価する。副作用を持つ式を実引数として渡さない。
- マクロ本体は`do { ... } while(0)`形式とし、定義末尾にセミコロンを含めない。呼び出し側が通常の文と同様にセミコロンを付ける。
- マクロ内で、リソースの取得、解放、rollback、所有権移転または暗黙の状態更新を行わない。

GLCEでは、制御フローを変更するマクロを全面的に許容するのではなく、名前と意味が固定された上記2種類のポインタ契約に限定して使用する。

### 意味的な事前条件

値を引数として受け取ること自体は、その値のsemantic validationをそのAPIまたはmoduleが所有することを意味しない。semantic conditionは、その意味を所有するmoduleで検証し、上位のforwarding／facade／orchestration layerでは重複検証しない。semantic ownership、build mode別validation、canonical／shallow／deep validationの詳細は`validation_policy.md`を正本とする。

値の範囲、列挙値、サイズ、alignment、複数引数間の関係、初期化状態、登録状態、所有関係および内部不変条件は、NULL検証マクロの対象にしない。

同じfalse条件でも、API契約上の意味によってresult codeが異なる。

| 条件の意味 | 代表的なresult code |
|---|---|
| 値、範囲または形式が公開APIの引数契約を満たさない | `INVALID_ARGUMENT` |
| 初期化順序、登録状態、所有関係またはライフサイクル契約を満たさない | `BAD_OPERATION` |
| 成立しているはずの内部不変条件または所有関係が破損している | `DATA_CORRUPTED` |

この区別を維持するため、意味的条件は具体的な`if`文として記述する。

```c
if(!size_is_power_of_two(req_align_)) {
    ret.code = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
    ret.state_after = allocator_->state;
    ERROR_MESSAGE(
        "%s(INVALID_ARGUMENT) - Alignment must be a power of two. "
        "req_align=%zu",
        __func__, req_align_);
    goto cleanup;
}
```

- エラーメッセージには、単なる`Argument is not valid`ではなく、満たさなかった条件と診断に必要な値を記録する。
- 呼び出し側が与え得る事前条件違反は、回復可能なAPIエラーとして処理する。
- 内部不変条件の破損は、通常の引数不正と混同しない。対応する重大エラーコード、コンポーネント状態遷移、およびDEBUG / TESTビルドでのassertを検討する。

### private preconditions helper

private helperへ分離するのは、次のいずれかに該当し、検証処理自体が独立した責務になる場合とする。

- 多数の意味的条件によってpublic APIの主要処理が読み取りにくくなる。
- 複数引数間の関係、所有関係または内部構造の走査を検証する。
- 同じ検証規則を複数のpublic APIで共有する。
- 検証結果の算出に複数段階の処理が必要である。

単一または少数の単純な意味的条件しかない関数に、関数ごとのpreconditions helperを機械的に作成しない。helperを使用する場合も、関数名は`*_is_valid`、`*_has_*`または`*_validate_*`など、何を検証し、戻り値が何を意味するか分かる名前にする。

### returnとcleanupの使い分け

- リソース取得および外部副作用の開始前で、cleanupすべき対象が存在しない場合は、引数不正を直接returnしてよい。
- リソース取得後、状態変更後、または複数の失敗経路で共通cleanupが必要な場合は、`goto cleanup`へ集約する。
- cleanup labelは、実際に行う処理が複数ある場合は`cleanup`、対象が明確な場合は`out_unbind_vao`など、解放内容または到達後の処理が分かる名前を検討する。
- NULL検証マクロの`GOTO_CLEANUP`形式を使用する関数では、分岐先labelが常に存在し、初期化前のローカル変数や無効な操作対象をcleanupしない構造にする。
- cleanup開始時点でrecoverable failureであっても、cleanup operationの失敗によって`DATA_CORRUPTED`へ昇格した場合は、その時点で残りの通常cleanupを停止する。
- `DATA_CORRUPTED`確定後のcleanup可否、rollback停止およびresource leak許容の詳細はValidation Policyを正本とする。

## 関数内部の処理フェーズ

処理の流れを明確にする必要がある関数では、次のphase名を使用する。存在しないphaseを表す空コメントは機械的に追加しない。

| Phase | 役割 |
|---|---|
| `Preconditions` | caller／API contractとしてoperation開始前に成立している必要がある条件を検証する |
| `Prepare` | 後続処理に必要な値、address、size、temporary resource等を準備する。operationのsemantic effectはまだ成立させない |
| `Preflight` | operationを実行可能か判断するための探索、resource確認、対象特定等を行う。operationのsemantic effectはまだ成立させない |
| `Commit eligibility` | Commit対象となるcandidate、値、state、resource等がCommit contractを満たし、semantic transitionを開始してよいことを確認する |
| `Commit` | operationのsemantic effectを成立させるstate transitionを行う |
| `Postconditions` | Commit完了後のstable stateがcontractを満たすことを確認する |
| `Output` | semantic transitionを伴わず、成立済みの結果、value、borrowed view、status、ID等をcallerへ伝達する |
| `Cleanup / rollback` | 必要なoperationに限り、失敗時のresource解放または状態復元を行う |

`Commit`はpersistent fieldのmutationだけを意味しない。ownership／lifecycle transition、registration／unregistration、public lifetimeへのobject publication、external side effect等、そのoperationの意味上のeffectを成立させる処理も`Commit`として扱う。

`Output`は`Commit`の別名ではない。すでに成立したoperation resultをcallerへcopyまたはborrowとして伝えるだけで、ownership、lifecycle、module state、external state等のsemantic transitionを発生させない処理に使用する。

このため、operationによって`Commit`だけ、`Output`だけ、または両方を持つことがある。例えば、allocatorが内部allocation stateを`Commit`した後に取得pointerをcallerへ通知する処理は`Output`である。一方、constructorがtemporary objectをcallerへ公開し、その時点でpublic lifetimeやownership transferを成立させる処理は`Commit`である。getterによるvalue copyやborrowed pointerの返却は通常`Output`である。

`Prepare`で取得したconstruction-localなtemporary resourceは、ownershipを移転するsemantic transitionが`Commit`されるまでcurrent operationがownershipとfailure時のcleanup responsibilityを保持する。

### Commit eligibility

- `Commit eligibility`は、Commit対象がsemantic transitionを安全かつcontractどおりに成立させられることをCommit直前に確認するphaseである。
- 対象は複数値の組み合わせに限定しない。Prepareで完成したcandidate representationをpublic lifetimeへ昇格またはownership transferする前にcanonical validationする場合も`Commit eligibility`として扱う。
- `Preconditions`、`Prepare`、`Preflight`等ですでに成立しているconditionを機械的に再検査するためのphaseではない。
- 各helperが生成または検証した複数の値やstateについて、Commit時に初めて必要となる組み合わせ整合性がある場合は`Commit eligibility`で確認する。
- semantic `Commit`を持たないpure `Output` operationには`Commit eligibility`を機械的に設けない。
- Commit前のcandidate validationは`Postconditions`とは呼ばない。`Postconditions`はCommit完了後のstable stateに対する確認に使用する。

### Private mutation helperのvoid化

- `Commit` phaseで使用するprivate helperについて、事前の`Commit eligibility`によってそのhelperを安全に実行できることが保証されている場合は、result codeを返さず`void`化する。
- `void`化のためにrecoverable errorを握りつぶしたり、失敗時にsilent returnする構造へ変更してはならない。
- helper自身で初めて判明するrecoverable failure、resource取得失敗、外部API失敗、探索失敗、capacity不足、arithmetic overflow等が残る場合は`void`化しない。
- `void` helperは、成立済みのinternal contractのもとでは処理を最後まで完了できる構造にする。
- `void` helperが上位処理で確立されたstrong internal contractを信頼して再検査しない場合は、必要に応じて関数直前の通常コメントで`Contract`を明示する。
- `Contract`には、どのphaseまたはhelperの完了後に呼ばれるか、argument／stateについて何が成立しているか、transient stateを扱う場合は何が一時的に許容されるかを書く。
- 実装から明らかなeffectを説明するためだけに`Effect`節を機械的に追加しない。

例:

```c
/*
 * Contract:
 * - xxx_allocate()のCommit eligibility成功後にのみ呼び出す。
 * - target_はCommit対象として確定済みである。
 * - required_size_とtarget_の組み合わせはCommit可能である。
 *
 * このhelperは上記contractを再検証しない。
 * contract成立下では失敗しない。
 */
static void target_commit(...);
```

## 型名規約

### opaque型

- 内部状態、所有リソース、ライフサイクル、dirty flag、cache、backend差分を持つ型はopaqueを基本にする。
- public headerでは構造体を前方宣言し、実体定義を公開しない。
- 実体定義を一つの`.c`だけで使用する場合は、その`.c`へ置く。
- 実体定義を同一モジュール内の複数の`.c`で共有する場合は、所有モジュールの`internal/`にprivate headerを設けて置く。
- opaque型の実体定義を共有するためだけに、Application向け公開headerへ移動しない。
- owned childがopaque型である場合、ownerまたはcallerはchildのprivate representationを推測してrollback、destroy、releaseの可否を判断しない。cleanup可否はpublic contract、ownership情報およびresult codeに基づいて判断する。

### 非opaque型

- enum、result、単純な値型、頂点配列要素のようなデータ搬送型は非opaqueでよい。
- 内部フィールドを直接別APIへ渡す必要がある型は非opaqueを検討する。

### enum

GLCEでrange validationを行う通常のenumは、原則として次の形式で定義する。

- 最初のenumeratorを0とする。
- 有効値を連続した整数値として定義する。
- 最後に有効値ではない`*_MAX` enumeratorを置く。
- `*_MAX`は要素数およびrange validationの上限として使用する。

```c
typedef enum {
    EXAMPLE_A = 0,
    EXAMPLE_B,
    EXAMPLE_C,
    EXAMPLE_MAX
} example_t;
```

range validationは、negative valueと`*_MAX`以上の値を一つの条件で除外できる次の形式を基本とする。

```c
if((unsigned int)value >= (unsigned int)EXAMPLE_MAX) {
    return false;
}
```

- `value < EXAMPLE_A`のように、range checkを先頭enumeratorの具体名へ依存させない。
- 先頭enumeratorの名前が変更されても、0始まりというenum contractとvalidation方法が変わらない構造を優先する。
- bit flag、protocol-defined value、外部ABIとの対応など、明示的に非連続値を持つ必要があるenumはこの規約の対象外とし、そのenum固有のvalidation contractを定義する。

### raw arrayとcollection validator

- raw arrayの各要素を走査するだけの目的で、`xxx_array_is_valid()`のようなcollection-level validatorを新設しない。
- objectがraw arrayを所有する場合、owned elementの検証はそのowner objectのcanonical validatorがownership closureの一部として行う。
- element typeが単純なvalue typeであり、単体の妥当性定義が必要な場合はelement validatorを提供してよい。
- ordering、uniqueness、count/capacity関係、複数field間の対応など、collection自体にpersistent invariantが必要になった場合は、raw array用helperを増やすのではなく、そのcollectionをopaque objectとして設計し、object自身にcanonical validatorを持たせることを検討する。
- automatic validationでraw array全体を走査するかどうかは、このCoding StyleではなくValidation Policyを正本とする。

### result型

- モジュールまたはサブシステムごとに、その責務に応じたresult codeを定義する。
- 状態を持つコンポーネントの実行結果は、baseレイヤーで定義する共通の`component_result_t`にresult codeと処理終了後のコンポーネント状態を格納して返すことを基本とする。
- 状態を持たないUtility、単純な値操作、数学関数、`bool`または値を直接返す照会関数には、`component_result_t`を機械的に適用しない。
- 下位レイヤーのresult codeは、上位レイヤーのresult codeへ変換して返す。
- 実行結果コードを表す名称には`result`を使用し、`rslt`は使用しない。
- result型は`<module>_result_t`を基本とする。
- resultを受け取る引数名は`result_`を基本とする。
- 現在の関数自身が返すresultを保持するlocal variableは`ret`を基本とする。
- 下位moduleまたは別moduleから受け取ったresultを保持する場合は`ret_<module>`を基本とする。
- `<module>`には、そのmoduleで使用している正式なmodule名またはC symbol prefixを使用し、localな都合による独自略称を作らない。既存の正式名称自体に含まれる`fs`等を、命名統一だけを目的として別表現へ展開しない。

```c
application_result_t ret;

memory_system_result_t ret_memory_system;
linear_allocator_result_t ret_linear_allocator;
renderer_backend_result_t ret_renderer_backend;
resource_registry_result_t ret_resource_registry;
```

次のような独自略称は使用しない。

```c
ret_mem_sys;
ret_linear_alloc;
ret_buff_mgr;
ret_registry;
```

同一moduleのresultを複数保持し、module名では役割を区別できない場合は、そのoperation上の役割を表す名称を使用してよい。

```c
shader_result_t ret;
shader_result_t ret_cleanup;
```

`ret_<module>`は「result型であること」を表すためだけの機械的prefixではなく、現在の関数自身のresultと、下位moduleから受け取ったresultを区別するために使用する。

### 共通コンポーネント状態

GLCE全体で、状態を持つコンポーネントが使用する共通状態型をbaseレイヤーに定義する。

```c
typedef enum {
    COMPONENT_STATE_UNINITIALIZED = 0,
    COMPONENT_STATE_READY,
    COMPONENT_STATE_DEGRADED,
    COMPONENT_STATE_FAULTED
} component_state_t;
```

各状態の意味は以下とする。

| 状態 | 意味 |
|---|---|
| `UNINITIALIZED` | 所有権および内部状態が未確立であり、通常操作を開始できない |
| `READY` | 内部不変条件と公開API契約を満たし、全ての通常機能を使用できる |
| `DEGRADED` | 内部整合性は維持されているが、一部機能または性能を利用できない |
| `FAULTED` | 内部整合性または必須機能を失い、通常操作を継続してはならない |

#### 状態の保持単位

- 状態はGLCE全体で1個のグローバル変数として保持せず、独立して利用停止、破棄または再初期化できる障害隔離単位ごとに保持する。
- Application、Subsystem、Backend Context、Manager、Registry、Allocator、長寿命Resourceなど、永続的な可変状態を持ち、障害後の再利用を防止する必要があるコンポーネントを主な適用対象とする。
- 小さな値オブジェクト、状態を持たないUtility、および障害隔離単位にならないモジュールへ状態フィールドを機械的に追加しない。
- 状態の変更権限は、その状態を所有するコンポーネント自身に持たせる。上位レイヤーが下位コンポーネントの状態を任意に書き換えるpublic APIは設けない。
- 外部へ状態を公開する必要がある場合は、読み取り専用のgetterまたは`component_result_t`の`state_after`を使用する。

#### 状態遷移

状態遷移は以下を基本とする。

| 事象 | 遷移 |
|---|---|
| 初期化成功 | `UNINITIALIZED -> READY` |
| 初期化失敗かつrollback成功 | `UNINITIALIZED`を維持 |
| 初期化失敗かつrollback失敗 | `UNINITIALIZED -> FAULTED` |
| 一部機能または性能の継続的な喪失 | `READY -> DEGRADED` |
| 機能回復 | `DEGRADED -> READY` |
| 内部整合性または必須機能の喪失 | `READY / DEGRADED -> FAULTED` |
| 正常なrelease、deinitializeまたはreset | `READY / DEGRADED -> UNINITIALIZED` |
| 成立済み状態の解放失敗 | `READY / DEGRADED -> FAULTED` |

- `NO_MEMORY`、`LIMIT_EXCEEDED`、`INVALID_ARGUMENT`など、処理は失敗してもコンポーネントの内部整合性を維持している場合は、状態を機械的に`DEGRADED`または`FAULTED`へ遷移させない。
- `FAULTED -> READY`への直接遷移は行わない。完全な再初期化によって不変条件、所有権および依存関係を再確立できた場合に限り、通常運転へ復帰してよい。
- `FAULTED`状態から安全に解放または再初期化できない場合は、通常運転への復帰を試みず、Application側のFATAL処置対象とする。
- `FATAL`はコンポーネント状態ではなく、Fault Policyが決定する処置とする。ApplicationがFATAL処置を決定した場合、Application自身を`FAULTED`へ遷移させたうえで終了処理を行う。
- `destroy`によってオブジェクト自身の生存期間が終了する場合、破棄後のオブジェクト状態は存在しない。所有ポインタをNULLにする契約によって破棄済みを表し、`UNINITIALIZED`への状態遷移とは区別する。

### 共通component result

状態を持つコンポーネントの実行結果を共通形式で返すため、baseレイヤーに`component_result_t`を定義する。

```c
typedef struct {
    int32_t code;
    component_state_t state_after;
} component_result_t;
```

#### フィールドの意味

- `code`は今回の関数呼び出し結果を表す。値には、その公開APIを所有するモジュールまたはサブシステムが定義したresult codeを格納する。
- result codeの格納型は、C実装およびABIによる`int`サイズの差異を避けるため、固定幅整数型の`int32_t`を基本とする。
- `state_after`は状態変数そのものではなく、関数呼び出し終了時点における主要操作対象コンポーネントの状態のスナップショットを表す。
- コンポーネントの現在状態は、コンポーネント自身が内部に永続保持する。`component_result_t`の`code`を「最後に発生したエラー」としてコンポーネント内部へ永続保持することは基本としない。
- `component_result_t`を返すAPIは、どのコンポーネントの状態を`state_after`へ格納するかを一意に定める。複数の操作対象が存在して主要対象を定められないAPIには、`component_result_t`を機械的に適用しない。
- `create`または`initialize`がコンポーネントの公開前に失敗した場合、`state_after`には、有効なコンポーネントが生成または初期化されなかったことを表す`UNINITIALIZED`を格納する。部分初期化状態を安全にrollbackできなかった場合は`FAULTED`とする。
- 主要操作対象がNULLで状態を読み取れない引数不正の場合、対象コンポーネントが存在しないことを表す`UNINITIALIZED`を格納する。主要操作対象が有効で、その他の引数だけが不正な場合は、主要操作対象の実際の状態を格納する。

#### result codeと状態の独立性

result codeとコンポーネント状態は独立した情報として扱う。

- 処理が失敗しても、failure atomicまたはrollbackによって内部状態が維持されていれば、エラーコードと`READY`を返してよい。
- 処理が成功しても、既知の機能制限を抱えたまま運転を継続している場合は、`SUCCESS`と`DEGRADED`を返してよい。
- rollback失敗、成立済み所有関係の破損または内部不変条件の喪失を検出した場合は、対応する重大エラーコードと`FAULTED`を返す。
- エラーコードだけから状態を機械的に決定しない。同じresult codeでも、実行した操作、rollback結果、代替機能の有無および障害範囲によって処理後状態は異なり得る。

例:

```c
/* 容量不足だがAllocator自体は正常 */
return (component_result_t){
    .code = LINEAR_ALLOCATOR_NO_MEMORY,
    .state_after = COMPONENT_STATE_READY
};

/* rollbackにも失敗し、以後の通常利用を禁止 */
allocator_->state = COMPONENT_STATE_FAULTED;
return (component_result_t){
    .code = LINEAR_ALLOCATOR_DATA_CORRUPTED,
    .state_after = allocator_->state
};
```

#### レイヤー境界での扱い

- 上位モジュールは、下位`component_result_t`の`code`を、従来どおり上位モジュールの責務と公開APIから見たresult codeへ意味変換する。
- 下位コンポーネントの`state_after`を、上位コンポーネントの`state_after`へ機械的にコピーしない。上位コンポーネントは、障害を隔離できるか、代替機能があるか、必須依存かを判断して自身の状態を決定する。
- 下位コンポーネントが`FAULTED`でも、原因が内部state corruptionではなく、かつ上位がその下位コンポーネントを安全に隔離できる場合は、上位が`DEGRADED`または`READY`を維持できる余地を残す。ただし`DATA_CORRUPTED`は内部canonical stateの信頼喪失を表すため、上位都合で`DEGRADED`へ降格せずFATAL方向へ伝播する。
- Applicationは、報告されたコンポーネント状態に加えて、そのコンポーネントの重要性、代替手段、再初期化可能性および現在の運転モードを考慮し、通常運転継続、DEGRADED運転またはFATAL処置を決定する。
- `component_result_t`を汎用Fault Handlerへ渡し、呼び出し元APIから発生元を判別できない場合は、component domainまたは発生元識別子を別途付加する。`int32_t code`だけをモジュール横断で解釈しない。

### result型の変換

GLCEでは、モジュールまたはサブシステムごとに固有のresult codeを持ち、レイヤー境界では下位result codeを上位モジュールの操作結果へ意味変換する。状態を持つコンポーネントでは、変換後のresult codeと上位コンポーネント自身の処理後状態を共通の`component_result_t`へ格納する。

#### Result utilityの命名

result codeを文字列へ変換するpublic utilityは、次の形式を基本とする。

```text
<module>_result_to_str()
```

例:

```c
application_result_to_str();
renderer_backend_result_to_str();
resource_result_to_str();
```

一つの`.c`内だけで使用するprivate utilityでは、module prefixを省略してよい。

```c
static const char* result_to_str(...);
```

下位moduleのresult codeを上位moduleのresult codeへ変換するpublic utilityは、次の形式を基本とする。

```text
<destination_module>_result_convert_<source_module>()
```

例:

```c
application_result_convert_linear_allocator();
renderer_backend_result_convert_linear_allocator();
resource_result_convert_fs_stream();
```

一つの`.c`内だけで使用するprivate converterでは、destination module prefixを省略する。

```text
result_convert_<source_module>()
```

例:

```c
static filesystem_result_t result_convert_memory_system(...);
static fs_stream_result_t result_convert_filesystem(...);
static fs_stream_result_t result_convert_choco_string(...);
```

source moduleは末尾のqualifierとして表し、`from`等の追加語は使用しない。

```c
result_convert_filesystem();          // use
result_convert_from_filesystem();     // avoid
```

converterの引数名は`result_`を基本とする。

```c
application_result_t
application_result_convert_linear_allocator(
    linear_allocator_result_t result_);
```

result utilityでも`rslt`略称は使用しない。

```c
application_result_to_str();              // use
application_rslt_to_str();                // avoid

application_result_convert_resource();    // use
application_rslt_convert_resource();      // avoid
```

#### 基本方針

- result変換は、下位モジュールのエラーコードを上位resultへ機械的に複製する処理ではない。下位の失敗を、上位モジュールの責務と公開APIから見た意味へ射影する処理とする。
- `SUCCESS`、`INVALID_ARGUMENT`、`NO_MEMORY`、`LIMIT_EXCEEDED`、`BAD_OPERATION`、`DATA_CORRUPTED`、`OVERFLOW`など、レイヤーが変わっても意味が維持されるコードは、対応する上位コードへ変換する。
- 下位モジュール固有の既知の失敗は、上位の公開APIで呼び出し側が判断すべき操作結果へ変換する。上位モジュールがその下位処理を直接担当しないことだけを理由に、`UNDEFINED_ERROR`へ変換してはならない。
- 複数の下位エラーに対して、上位の呼び出し側が行う回復処理、状態遷移または重大度判断が同じ場合は、1つの上位コードへ集約する。下位コードを上位resultへ個別に追加し続けない。
- 上位resultへ新しいコードを追加するのは、そのコードによって呼び出し側の回復処理、再試行、状態遷移または重大度判断が変わる場合に限る。診断情報を増やすことだけを目的としてresultコードを追加しない。
- file open failureのように上位operationを成立させないfailureは対応するsemantic resultへ変換してよい。一方、read完了後のbest-effort close failureのように上位operationの意味を変えない事象は、対称性だけを理由に`FILE_CLOSE_ERROR`等を各layerへ追加しない。
- 下位固有の詳細原因を上位でプログラムから参照する必要が生じた場合は、上位resultを増やし続けるのではなく、詳細エラー情報構造体、error handleまたは同等の診断用経路を別途検討する。

#### `RUNTIME_ERROR`

- `RUNTIME_ERROR`は、引数およびライフサイクル上の事前条件を満たした有効な呼び出しで発生し得る、意味の分かっている実行時失敗を表す。
- 下位モジュール固有の既知の失敗について、上位の呼び出し側が個別の処理を行う必要がなく、かつ上位resultに対応する操作別コードがない場合は、上位の`RUNTIME_ERROR`へ集約する。
- `RUNTIME_ERROR`は、未知のresult値、変換表の不足、内部状態破損またはAPI契約違反を隠すためのfallbackとして使用しない。

#### `UNDEFINED_ERROR`

- `UNDEFINED_ERROR`は、result型に定義されていない数値、変換表の不足、またはその操作の正当な実行経路から返されるはずのないresultを受け取った場合に使用する。
- `UNDEFINED_ERROR`は、既知かつ正当に発生し得る下位エラーを上位モジュールが公開していないという理由では使用しない。
- `UNDEFINED_ERROR`への変換時は、下位モジュール名、元のresult値、実行中の操作および変換先を重大ログへ記録する。DEBUG / TESTビルドではassertの対象を検討する。
- `default`節で未定義値を`UNDEFINED_ERROR`へ変換する場合は、元の数値を失わないよう診断情報へ記録する。
- 内部状態または成立済み所有関係の破損が確認できた場合は、`UNDEFINED_ERROR`ではなく`DATA_CORRUPTED`として扱う。

#### 下位原因の診断

- result変換によって下位固有の意味が初めて失われる境界では、下位モジュール名、下位result、上位result、失敗した上位操作および対象識別情報を診断可能な形で残す。
- shader compiler log、ファイルパス、backend固有情報など、resultコードだけでは表現できない詳細はエラーメッセージまたは専用診断情報として保持する。
- 上位レイヤーは、下位レイヤーですでに十分な詳細が記録されている場合、同じ内容を繰り返さず、上位操作の文脈だけを追加する。

#### Shaderの変換例

- Renderer BackendからShaderへの変換では、Shader自身がコンパイルおよびリンクを担当するため、`RENDERER_BACKEND_SHADER_COMPILE_ERROR`を`SHADER_COMPILE_ERROR`、`RENDERER_BACKEND_SHADER_LINK_ERROR`を`SHADER_LINK_ERROR`へ変換する。
- Shaderより上位のモジュールでは、コンパイル失敗とリンク失敗に対する呼び出し側の処理が同じ場合、両方を上位の初期化失敗、リソース構築失敗または`RUNTIME_ERROR`へ集約する。上位resultへ`COMPILE_ERROR`と`LINK_ERROR`を機械的に追加しない。
- Shaderのコンパイルまたはリンクへ到達しない設計の操作からこれらのコードを受け取った場合は、通常の実行時失敗への変換ではなく、到達不能なresultとして`UNDEFINED_ERROR`および重大診断の対象とする。

## Source File構成

`.c`は、module全体のcontract、公開API、private helper、utility、validatorの責務が視覚的に分かるようにsectionを構成する。すべてのmoduleへ同じsectionを機械的に作成せず、そのfileに存在する責務だけを配置する。

### 基本順序

必要なsectionを、原則として次の順序で配置する。

```c
// SPDX / copyright

#include ...

/*
 * Module Internal Contract
 *
 */

/*
 * Module Validation Policy
 *
 */

// ============================================================
// Private Type Definitions
// ============================================================

// ============================================================
// Private Constants
// ============================================================

// ============================================================
// Private Function Declarations
// ============================================================

// ============================================================
// Public API
// ============================================================

// ============================================================
// <Responsibility> Helpers
// ============================================================

// ============================================================
// Utilities
// ============================================================

// ============================================================
// Validators
// ============================================================
```

### 各sectionの責務

#### Module Internal Contract

- module内部のstable stateで成立すべきinvariantを記述する。
- representation、layout、ownership、state relationなど、implementation内部に属するcontractを含めてよい。
- Doxygenではなく通常のblock commentとして記述する。

#### Module Validation Policy

- そのmoduleでvalidationをどのように適用するかというmodule固有のpolicyを記述する。
- canonical／shallow validatorの役割、public APIごとのvalidation配置、transient stateの扱い等、共通`validation_policy.md`だけでは決まらない事項を書く。
- 共通Validation Policyを全文複製しない。
- Doxygenではなく通常のblock commentとして記述する。

#### Private Type Definitions

- 一つの`.c`だけで使用するprivate struct、enum、typedef等を配置する。
- 複数`.c`で共有するprivate型は、このsectionへ複製せず所有moduleの`internal/`等へ配置する。
- private typeが存在しない場合はsectionを作成しない。

#### Private Constants

- file-localな`static const`、private tableその他の定数を配置する。
- private constantが存在しない場合はsectionを作成しない。

#### Private Function Declarations

- file-local helperのforward declarationを配置する。
- helperが多い場合は、`// Allocation helpers`、`// Free helpers`、`// Utilities`、`// Validators`のように責務単位でsubgroup化する。
- declarationの並びは、後続のdefinition sectionの並びと可能な限り一致させる。

#### Public API

- 対応headerで宣言されるpublic APIのdefinitionをまとめる。
- 各API固有のValidation Policyがある場合は、関数definitionの直前に通常コメントとして記述する。
- public canonical validatorのように機能上はvalidatorであっても、公開APIであればdefinitionは`Public API`へ配置する。

#### `<Responsibility> Helpers`

- private helperは処理責務ごとにsectionを分ける。
- `Allocation Helpers`、`Free Helpers`、`Parsing Helpers`等、実際の責務名を使用する。
- helperをすべて一つの`Private Functions` sectionへ機械的に集約しない。

#### Utilities

- result-to-string変換等、主要operationの一部ではないfile-local utilityを配置する。
- 独立した責務を持つ処理はUtilityへ寄せず、対応するresponsibility sectionを設ける。

#### Validators

- private shallow validator、element／state validator等、validation専用のprivate helperを配置する。
- public validatorのdefinitionは`Public API`へ置き、private validatorだけをこのsectionへ配置する。

### Section commentの階層

file内のコメント階層は次の3段階を基本とする。

1. **file-level section**: banner形式を使用する。

```c
// ============================================================
// Allocation Helpers
// ============================================================
```
2. **section内部のsubgroup**: 短い通常コメントを使用する。

```c
// Allocation helpers
```

3. **関数内部の処理phase**: 末尾にピリオドを付けた短いコメントを使用する。

```c
// Preconditions.
// Prepare.
// Commit eligibility.
// Commit.
// Postconditions.
// Output.
```

- banner形式はfile-level sectionに限定し、関数単位や小さなsubgroupへ乱用しない。
- sectionまたはphaseが存在しない場合は空のcomment blockを機械的に追加しない。

## Private関数命名

- `static`関数はファイル内に閉じるため、公開APIほど長いprefixを付けない。
- モジュール名prefixは原則不要。
- ファイル内で複数対象を扱う場合のみ、短い対象名を付ける。
- private関数も関数名共通規約に従う。

### 推奨パターン

| 用途 | 例 |
|---|---|
| 内部状態検証 | `internal_state_is_valid` |
| ID検証 | `id_is_valid` |
| slot探索 | `find_free_slot` |
| 名前検索 | `find_by_name` |
| 設定値変換 | `resolve_*` |

## Include規約

### Header

- public headerは利用者に必要な型と宣言だけを公開する。
- sourceでしか使わない依存はheaderに置かない。
- 前方宣言で足りる型は前方宣言にする。
- 値渡し、メンバアクセス、`sizeof`、inline関数等で完全型が必要な場合はincludeする。
- `size_t`を使うheaderは`<stddef.h>`を直接includeする。
- `bool`を使うheaderは`<stdbool.h>`を直接includeする。
- 固定幅整数を使うheaderは`<stdint.h>`を直接includeする。

### Source

- sourceは、自分が直接使う型、関数、macroのheaderを自分でincludeする。
- headerでincludeしていても、source側で直接使っているならsource側でもincludeする。
- 偶然の間接includeに依存しない。
- 最初に対応する自モジュールheaderをincludeし、header単体で不足している依存を検出しやすくする。

### Include順序

engine内のheaderは、下位レイヤーから上位レイヤーへ並べる。

1. 対応する自モジュールheader
2. C標準header
3. 外部ライブラリheader
4. `engine/base`
5. `engine/core`
6. `engine/containers`
7. `engine/io_utils`
8. `engine/resource`
9. `engine/systems`
10. application層のheader
11. test専用header

同一レイヤー内では、共通定義から具象モジュールへ並べる。

- `*_types.h`
- `*_err_utils.h`
- context、interface、registryなどの基盤header
- 具象モジュールheader

既存コードに揺れがある場合は、最終的にこの順序へ統一する。

## Include Guard

- include guardは実ファイルパスに追従させる。
- ディレクトリ名・ファイル名を変更したら必ず更新する。
- copy/paste由来の別モジュール名を残さない。
- 単数形/複数形は実パスと一致させる。

## License / Copyright Header

### 基本形

Cのsourceおよびheaderでは、ファイル先頭を次の形とする。

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24
/** @ingroup application
 *
 * @file flight_camera.c
 * @author chocolate-pie24
 * @brief アプリケーションからのイベント情報をもとにフライトカメラを制御するAPIの実装
 *
 * @version 0.1
 * @date 2026-03-25
 *
 */
```

- SPDX license identifierを1行目、copyright noticeを2行目に置き、その後に既存のDoxygen file commentを続ける。
- `SPDX-License-Identifier: MIT`は、このファイルへMIT Licenseを適用することを機械可読な形で示す。
- 完全なlicense textはrepository rootの`LICENSE`を正とする。
- `See LICENSE file in the project root for full license text.`のような補助文は重複になるため、各ファイルへ追加しない。
- author名は既存のproject表記である`chocolate-pie24`へ統一する。

### Copyright year

- 新規ファイルには、そのファイルを作成した年を記載する。
- 既存ファイルのyearは、headerを整備した年へ機械的に更新しない。既存の`2025`はそのまま維持してよい。
- copyright yearとDoxygenの`@date`は異なるmetadataであり、一致を強制しない。
- `@date`には、そのfile commentが表す実際の日付を記載する。copyright yearへ合わせる目的だけで変更しない。
- yearまたは`@date`の誤記を発見した場合は、licenseの有効性とは分けてmetadataの誤りとして修正する。

### 段階的な適用

- 新規のproduction sourceおよびheaderは、このheader形式に従う。
- 既存ファイルは、変更する範囲から段階的に適用する。
- 現時点でSPDX headerがない`test/`配下の既存ファイルは、一括修正の対象外とする。testおよびDoxygen構造の再編時に整備する。
- SPDX追加だけを理由に、既存のcopyright yearまたはDoxygen `@date`を変更しない。

## Doxygen

### Public Header

- headerには、そのmoduleの利用者が知る必要のある外部contractを記述する。
- file/module levelでは、必要に応じてmodule descriptionおよびModule Boundary ContractをDoxygenで記述する。
- Module Boundary Contractには、ownership、lifetime、storage、公開範囲、固定alignment等、module利用者が守る条件または期待できる保証を記述する。
- public APIには、責務、所有権、事前条件、事後条件、戻り値を記述する。
- `component_result_t`を返すAPIには、主要操作対象、`state_after`が表すコンポーネント、および代表的な失敗時の処理後状態を記述する。
- リソースライフサイクルAPIには、取得処理、オブジェクト自身の破棄、識別子またはdescriptorを指定する解除のどれに該当するか、および失敗時に保証する状態を記述する。
- constructorには、成功時に公開されるstable state、通常利用に必要な構成、および失敗時の出力とownershipの状態を記述する。
- resourceを受け取るAPIには、copy、moveまたはborrowのどれで受け取るかを記述する。
- borrowed pointerには、owner、borrowが有効な期間、無効化条件およびdestroy責務を記述する。
- getterが内部pointerを返す場合は、その戻り値がborrowed viewであること、callerが解放してはならないこと、および有効期間を記述する。
- system-lifetime dependencyを保持するobjectには、dependencyをdestroyしないことと、必要なdestroy順を記述する。
- `@file`は実ファイル名と一致させる。
- `@brief`はモジュールの責務を短く説明する。
- opaque型の場合、利用者が知るべきライフサイクルを説明する。

### Source / Private関数

- sourceには、実装者が維持するModule Internal Contract、Module Validation Policy、API-specific Validation Policy、private helper固有のContract、およびimplementation-specific rationaleを記述する。
- Module Internal ContractおよびModule Validation PolicyはDoxygenではなく通常のblock commentとする。
- private関数のDoxygenは必要最小限でよい。
- 名前と実装から明らかな処理に長いコメントを付けない。
- 複雑な不変条件、所有権、失敗時不変、rollback不可、上位処理で成立済みのcontractを信頼して再検査しない理由などはコメントする。
- `.h`はmodule利用者が守る／期待できるcontract、`.c`は実装者が維持するinternal contractとstrategyを記述する場所として役割を分ける。

### AI支援表記

AIを用いて設計・契約に関する文書コメントの草案を作成した場合は、適用範囲が分かる位置に次の形式を使用してよい。

```text
AI支援:
- 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
- 実装コードはプロジェクト作成者が作成した。
```

- `本ドキュメント`のように適用範囲が曖昧な表現を避け、section単位で記述する場合は`本セクション`とする。
- AI支援表記は設計・契約文書コメントへ適用し、通常の実装コメントへ機械的に付加しない。

## エラーメッセージ

### 基本形

エラーメッセージは次の形を基本にする。

- `function_name(RESULT) - Failed to <operation>. reason=<reason>, key=value`

### 粒度

- public APIは、失敗した操作、理由、識別情報を出す。
- private関数は、public API側で十分な文脈が出るなら過剰にERRORを出さない。
- private関数で異常を出す場合は、内部状態破損や原因特定に必要なものに限る。
- destructor/deinitialize系は、無効引数を許容するならWARNINGまたは無出力にする。
- cleanup中の失敗は、握りつぶしてよいものと危険なものを分ける。

### 含める情報

- 対象名: `name`, `query_name`, `resource_name`
- ID: `geometry_id`, `texture_id`, `camera_id`
- サイズ: `bytes`, `elem_count`, `vertex_count`
- 配置: `offset`, `capacity`
- 状態: `initialized`, `registered`, `rollback=not_supported`

### 注意点

- format stringと可変引数の数を必ず一致させる。
- メッセージ内の関数名は実関数名と一致させる。
- 旧関数名、旧モジュール名を残さない。

## 浮動小数点値のNaN / Infチェック

`isfinite()`等によるgenericなNaN / Infチェックは、浮動小数点値を扱うすべてのAPIへ機械的に追加しない。

### 外部trust boundary

通信、外部ファイル、外部device、外部process等から取得した浮動小数点値は、GLCE内部のtrusted dataとして扱う前のacquisition boundaryで、必要に応じてfinite性を検証する。

外部値を一度validated internal representationへ変換した後は、通常の内部処理でgenericなfinite checkを繰り返さない。

### 内部演算

GLCE内部の浮動小数点演算では、genericな`isfinite()`ではなく、その演算固有の成立条件を検証する。

例:

- 除算: divisorが0でない。
- normalize: 対象vectorがzero vectorでない。
- matrix inverse: matrixがsingularでない。
- perspective frustum:
  - `aspect > 0`
  - `0 < fovy < 180`
  - `0 < near < far`

有限な入力同士の演算でもoverflow等によってInf / NaNは生成し得るため、「GLCE内部の浮動小数点状態は常にfiniteでなければならない」という一般契約は設けない。

### Validator

canonical validatorへgenericなfinite checkを追加するのは、finite性そのものがその型の明示的なinvariantである場合に限る。

validatorは実際にその型が保証するstructural / semantic invariantを表現し、防御的理由だけで新しいinvariantを追加しない。

## 失敗時不変

### 基本方針

- 外部入力を受けるAPIは、可能な限り失敗時不変を目指す。
- GPU appendや一部の外部副作用のようにrollbackできないものは、API契約とエラーメッセージに明記する。
- 複数リソースをまたぐ操作では、どの時点でcommitされるかを明確にする。
- semantic `Commit`を行うoperationでは、通常発生し得るfailureを可能な限り`Commit`開始前に処理する。
- `Commit`開始後は、可能な限り通常のfailure pathを持たず、stable stateまで処理を完了できる構造を優先する。
- `Commit`途中のfailureに対応するため複雑なrollbackを追加するより、可能であればfailure条件を`Preconditions`、`Prepare`、`Preflight`、`Commit eligibility`へ移動する。
- `Commit eligibility`は、Commit対象となるcandidateや値／state／resourceについて、Commit開始前に追加のeligibility確認が必要な場合に設ける。pure `Output`だけのoperationには機械的に設けない。

### 所有権取得・解放の対

所有権の取得と解放が対になるAPIは、取得処理、オブジェクト自身の破棄、識別子またはdescriptorを指定する解除に分けて設計する。

#### 取得処理

- `create`、`initialize`、`import`、`allocate`、`register`などの取得処理は、通常の実行条件によって失敗し得る。
- 取得処理は、成功時にのみ有効なオブジェクト、ID、handleまたはdescriptorを公開する。
- recoverableな取得失敗では所有権を呼び出し側へ移転せず、取得途中のリソースを解放し、可能な限り処理開始前の状態へrollbackする。`DATA_CORRUPTED`検出後のcleanupはこの一般則の例外とし、Validation Policyのfail-stop ruleを優先する。
- 取得処理の成功後に後続処理がrecoverable failureで失敗した場合は、取得済みの所有権を対応する解放処理によって解放し、可能な限り処理開始前の状態へrollbackする。後続処理またはrollback途中で`DATA_CORRUPTED`が確定した場合は、Validation Policyのfail-stop ruleを優先し、以後の通常cleanupを行わない。
- rollbackに使用する解放処理自体が、メモリ不足、node不足、slot不足などのリソース不足によって失敗する設計は避ける。
- complete constructor内部のpartial stateをrollbackする場合、public destroyがstable objectだけを対象とするcontractであれば、privateな`destroy_unchecked`等を用いてよい。ただし対象はconstructor自身が確立したknown partial stateに限定し、`DATA_CORRUPTED`確定後には使用しない。

#### オブジェクト自身の破棄

- object自身のlogical lifetimeまたはinitialized stateを終了し、そのobjectが所有するresourceを一括して解放する`destroy` / `deinitialize` APIを対象とする。construction側のoperation名との機械的な対称性は要求しない。
- `create`または`initialize`に成功し、終了operation開始時点で有効性を確認できるオブジェクトを、取得時に確立された所有権およびライフサイクル上の事前条件を維持した状態で、そのobjectに定義された`destroy`または`deinitialize`へ渡した場合、契約で定めたlogical lifetimeまたはinitialized stateは必ず終了することを基本とする。destroy / deinitializeで実施するvalidationのbuild mode、深さおよびfail-stop条件はValidation Policyを正本とする。validationで`DATA_CORRUPTED`相当の不整合を検出した場合は、この通常終了contractの対象外とし、疑わしいfieldを辿る破棄処理を開始しない。
- オブジェクト自身の破棄処理は、呼び出し側が対処可能なfailureをoperation resultとして扱う必要がない場合、戻り値を持たない`void`関数を基本とする。引数不正を通知することだけを目的としてresult codeを持たせない。
- destroy中に外部resourceのbest-effort releaseが必要で、そのrelease結果を診断目的で観測したい場合、logical lifetimeの終了とは独立したoptional out parameterを設けてよい。out parameterは「destroyの成功／失敗」ではなく、内部で試みたreleaseの事実を通知する。
- optional out parameterはNULLを許可できる。結果が不要なcleanup pathではNULLを渡し、destroy contract自体を複雑化させない。
- low-level release failureを、上位layerのresult enumへ機械的に一対一で昇格させない。上位operationの意味上そのfailureをcallerが扱う必要がある場合だけ、そのlayerでsemanticなerrorへ変換する。
- callerが再試行、待機、保存失敗判定、durability判定またはtransaction commit判定を行う必要がある処理は、destroyのoptional diagnosticへ押し込めず、`flush`、`sync`、`commit`、`shutdown`等の明示operationとして分離する。
- `filesystem_destroy()`／`fs_stream_destroy()`はこの例である。file handleのcloseをbest-effortで実施し、object lifetimeは必ず終了する。close結果が必要なcallerだけ`bool* out_close_succeeded_`で観測できるが、read-side callerは通常これをoperation failureへ昇格させない。
- NULL、部分初期化状態、二重destroyを許容するかはAPIごとに明示する。NULLまたは破棄済み状態をno-opとして許容する場合、それは破棄処理の失敗には含めない。`void`であることからNULL許容を推測してはならない。
- 所有オブジェクトを`T**`で受け取る破棄APIでは、`destroy(NULL)`と、`object == NULL`である`destroy(&object)`の扱いをAPI contractとして明示する。GLCEでは、反復cleanup、rollback、および条件分岐の単純化に有用であり、そのAPIのsemanticを損なわない場合、両方をno-opとして許容してよい。外側のpointer storage自体の存在がoperation semantics上必要なAPIでは`destroy(NULL)`をcontract violationとしてよく、NULL許容をproject-wideに機械的強制しない。
- 有効なオブジェクトの破棄にcontext、ownerまたはbackendが必要なAPIでは、オブジェクトが有効であるにもかかわらず、それらがNULLまたは無効である状態をno-opとして扱わない。破棄を完遂できず所有リソースが残存するため、API契約違反または内部不整合として扱う。
- 無効なオブジェクト、所有権違反、破棄済みオブジェクトの再利用などは、通常の回復可能エラーではなく、API契約違反または内部不整合として扱う。
- `DATA_CORRUPTED`を検出した後のrollback／destroy／cleanupはValidation Policyを正本とする。破損objectまたはそのfieldに依存するresource破棄だけでなく、そのoperation内で独立local variableとして追跡していたtemporary resourceも通常cleanupの例外対象とはしない。

#### 識別子またはdescriptorを指定する解除

- `allocate / free`、`register / unregister`、`import / release`のように、manager、registry、allocatorなどの管理主体と、ID、handleまたはdescriptorの組み合わせによって対象を識別するAPIを対象とする。
- 解除処理は、commit前に管理主体、所有関係、識別子の有効性、および未解除状態を可能な限り検証する。
- NULLや規定範囲外の識別子など、引数自体の事前条件違反は、対応するモジュールの`INVALID_ARGUMENT`として扱う。
- 未登録ID、二重解除、staleなdescriptor、所有者の不一致など、呼び出し時のライフサイクルまたは所有権の事前条件違反は、対応するモジュールの`BAD_OPERATION`として扱う。これらを内部状態破損と混同しない。
- 事前条件の検証に失敗した場合は、管理状態および出力引数を変更しない。
- 有効な所有関係およびライフサイクルが確認され、解除処理を開始した後は、対応する解除操作が成功することを前提とする。
- 成立済み状態の解除に失敗した場合は、対応するモジュールの`DATA_CORRUPTED`として扱い、通常運転への復帰、失敗時不変、およびrollbackを保証しない。
- `DATA_CORRUPTED`を返す場合は、影響を受けたコンポーネントを`FAULTED`へ遷移させ、以後そのコンポーネントを通常運転で継続使用しない。上位レイヤーは重大エラーとして扱い、障害を安全に隔離できない場合はApplication側のFATAL処置対象とする。

#### 共通設計

- 解放側処理の完遂に必要な管理リソースは、取得側処理の成功時点までに確保する。
- 有効な解放操作は、新たな管理リソースの確保を必要とせず、メモリ不足、node不足、slot不足などのリソース不足によって失敗しない構造を基本とする。
- 解放側処理は、既存状態の解除、隣接領域との統合、管理リソースの返却など、取得時に確保済みの情報だけで完遂できるようにする。
- 複数の管理主体をまたぐ解除処理では、事前条件の検証範囲とcommit開始位置を明確にする。
- 監査ログ、通知、統計更新などの付随処理が追加リソースを必要とする場合は、論理的な解放処理と分離し、付随処理の失敗によって所有権の解放が妨げられない構造とする。

## テスト規約

- public APIの引数検証、成功経路、代表的失敗経路をテストする。
- 失敗注入が必要な場合は、モジュール専用のtest control構造体を使う。
- no-op系の失敗注入スタイルはPJ全体で統一する。
- テスト専用headerは通常ビルドへ漏らさない。
- 外部API呼び出しの失敗注入または呼び出し検証に必要な場合は、production source内に最小限のテストシームを設けてよい。
- Renderer Backend Concreteの`mock_gl*`は、通常ビルドでは実OpenGL APIへ転送し、将来のテストではOpenGL境界の失敗注入および呼び出し検証に使用するテストシームとして扱う。
- production sourceに残すテストシームと、コメントアウトされた旧テスト本体、テスト専用vtable、失敗注入設定およびtest control APIを区別する。旧テスト本体をproduction source内へ保存し続けない。
- テストシームを設けるためだけに、テスト専用APIまたは失敗注入設定をApplication向けpublic headerや`internal/`へ追加しない。

## レビュー観点

新規モジュールまたはリファクタ時は以下を確認する。

- public headerとEngine内部headerの公開範囲が混在していないか。
- `internal/`が非公開ファイルの汎用置き場になっていないか。
- `core/`が、共有範囲と所有者を確認せずに`types`、`result`、`status`および補助関数を集める汎用置き場になっていないか。
- モジュール固有の状態、設定、statusまたはprivate型を、名前だけを理由に`core/`へ移動していないか。
- 内部モジュールが独立した責務を持つ場合、責務名ディレクトリへ分離されているか。
- ファイルの拡張子ではなく、責務の所有者に基づいて配置されているか。
- 実行時の呼び出し順序または依存の深さを、所有責務を無視してディレクトリ階層へ転写していないか。
- 一つの`.c`でしか使用しない宣言のために不要なprivate headerを作っていないか。
- ファイル名と公開API prefixが一致しているか。
- 単数形/複数形が役割と一致しているか。
- public headerが過剰に下位実装へ依存していないか。
- sourceが直接依存をincludeしているか。
- include guardが実パスと一致しているか。
- Doxygenの`@file`が実ファイル名と一致しているか。
- production source内のテストシームに目的があり、テスト本体またはテスト専用APIが通常実装へ混在していないか。
- エラーメッセージにreasonと識別情報があるか。
- result変換が、下位コードの機械的複製ではなく、上位モジュールの公開操作に基づく意味変換になっているか。
- 既知かつ正当に発生し得る下位エラーを、上位モジュールが直接担当しないという理由だけで`UNDEFINED_ERROR`へ変換していないか。
- `RUNTIME_ERROR`が既知の実行時失敗に限定され、未知値、変換漏れ、内部状態破損またはAPI契約違反のfallbackになっていないか。
- `UNDEFINED_ERROR`が未定義値、変換漏れまたは到達不能なresultに限定され、元のresult値が診断情報に残されているか。
- 上位resultへ追加した固有コードによって、呼び出し側の回復処理、状態遷移または重大度判断が実際に変わるか。
- 下位固有のresultを上位で集約する境界に、原因を追跡できる十分な診断情報があるか。
- 状態を持つコンポーネントのAPIが、共通の`component_result_t`を使用しているか。
- `component_result_t.state_after`が、どの主要操作対象コンポーネントの状態を表すか一意に定義されているか。
- result codeと処理後状態を独立して判断し、回復可能な処理失敗だけを理由に`DEGRADED`または`FAULTED`へ遷移させていないか。
- 下位コンポーネントの`state_after`を上位コンポーネントへ機械的にコピーせず、障害範囲、隔離可能性および依存の必須性から上位自身の状態を決定しているか。
- `FAULTED`状態のコンポーネントに対する通常操作を拒否し、許可する診断、解放または再初期化操作をAPI契約で限定しているか。
- rollback不可の副作用が契約化されているか。
- opaqueにすべき型が公開構造体になっていないか。
- 非opaqueにすべき値型が過剰に隠蔽されていないか。
- constructor成功後に、追加の一度限り初期化なしで通常利用できるか。
- 構築専用の部分初期化状態がpublic APIへ露出していないか。
- 段階的初期化を残す場合、中間状態に独立した利用価値があり、その不変条件と許可操作が定義されているか。
- 再構築要求のために初期constructorを不必要に分割していないか。
- 再構築で無効化されるID、handle、descriptorおよびborrowed referenceが契約化されているか。
- objectが保持するpointerまたはhandleについて、ownerとborrowerが明確か。
- borrowed dependencyのlifetimeとaddressが、全borrowerのlifetime中に安定しているか。
- borrowerがdependencyをdestroyせず、上位ownerが正しいdestroy順を保証しているか。
- 同じsystem-lifetime dependencyを通常操作ごとに再指定させ、異なるdependencyを誤って渡せるAPIになっていないか。
- 同じ引数が複数APIに現れるという理由だけで、lifetimeを保証できない長寿命borrowを導入していないか。
- `const char*`、`choco_string_t`および`fs_path_t`を、所有、保持、変更、metadata管理およびsemantic identityに基づいて選択しているか。
- validation項目を増やすことだけを理由に、borrowed C文字列を所有objectへ置き換えていないか。
- getterが返すborrowed C文字列または内部pointerの有効期間と無効化条件が明示されているか。
- 所有権取得後の後続処理がrecoverable failureで失敗した場合、取得済みリソースをrollbackできるか。`DATA_CORRUPTED`確定後に通常rollbackへ進む経路が残っていないか。
- オブジェクト自身の`destroy / deinitialize`に、正当な実行条件で発生する回復可能な失敗経路が残っていないか。残っている場合、実行結果コードを返す必要があるか、破棄前処理として分離できないか。
- `destroy / deinitialize`のNULL、部分初期化状態および二重destroyに対する契約が明示されているか。DEBUG / TESTで事前validationに失敗したobjectを通常destroyしない構造になっているか。
- `T**`形式の破棄APIで、所有ポインタの格納先自体がNULLの場合と、格納されているオブジェクトがNULLの場合を区別しているか。
- 有効なオブジェクトを破棄するために必要なcontext、ownerまたはbackendの不正を、リソースを残したままのno-opとして扱っていないか。
- ID、handleまたはdescriptorを指定する解除処理が、commit前に所有関係とライフサイクルを検証しているか。
- 識別子指定による解除処理で、事前条件違反を`INVALID_ARGUMENT`または`BAD_OPERATION`、成立済み状態の解除失敗を`DATA_CORRUPTED`として区別しているか。
- `DATA_CORRUPTED`発生後に、通常運転への復帰、失敗時不変、rollbackを誤って保証していないか。
- cleanup途中でresultが`DATA_CORRUPTED`へ昇格した場合、その時点で後続のdestroy、release、free、close、unbind等を停止する構造になっているか。
- 有効な解放操作が、新たな管理リソースの確保やリソース不足に依存していないか。
- 解放に必要な管理情報が、取得側処理の成功時点までに確保されているか。
- 新規コードまたは今回の改修範囲へ、`IF_ARG_FALSE_GOTO_CLEANUP`または同等の汎用条件マクロを追加していないか。既存箇所は、関連モジュールの改修時に段階的に通常の`if`文へ置き換えているか。
- NULL検証マクロがポインタ存在条件だけに限定され、意味的条件、result code分類、状態遷移またはrollbackを隠していないか。
- NULL検証マクロの実引数が1回だけ評価され、マクロ定義末尾にセミコロンを含んでいないか。
- 値、状態、所有関係および内部不変条件の検証が、具体的なresult codeとエラーメッセージを伴う明示的な`if`文になっているか。
- 単純な事前条件のために関数ごとのprivate preconditions helperを機械的に追加していないか。
- cleanup不要の失敗経路と、共通cleanupが必要な失敗経路で、直接returnと`goto cleanup`を適切に使い分けているか。
- 新規のproduction sourceおよびheaderで、SPDX license identifier、copyright notice、Doxygen file commentの順序が統一されているか。
- SPDX追加だけを理由に、既存のcopyright yearまたはDoxygen `@date`を機械的に変更していないか。

## 適用方針

- 新規コードはこの規約に従う。
- 既存コードは、触る範囲から段階的に適用する。
- PJ全体の一括整備は、renderer_frontendの骨格が固まった後に行う。
- 公開前に、命名、include、Doxygen、エラーメッセージの全体監査を行う。