<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Coding Style

## このドキュメントの使い方

本書はGLCEのディレクトリ配置、命名、API設計、関数構成、include、エラーメッセージ等に関する共通規約の正本である。新規実装、レビュー、リファクタリングの際に、関係する項目を参照する。

GLCEでは、詳細な説明コメントに依存せず理解できる実装を優先する。内部処理の説明が必要な場合は、まず責務分離、命名、関数構成、制御フローを改善できないか検討する。実装の事実は最新のソースコードから確認し、公開契約は`.h`のDoxygenへ記述する。

関連する正本文書:

- `documentation_guide.md`: Doxygenとソースコメントの記述方針
- `validation_policy.md`: Validation Architecture、Validation Depth、Result Classification、Fail-stop
- `boundary_model.md`: Boundary、Trust、Authorityの共通モデル
- `design_decisions.md`: 重要な設計判断とその経緯

本書に存在しない規則を一般的慣習だけでGLCE固有の必須規則として追加しない。本書も普遍的なbest practiceを主張するものではなく、GLCE固有の規範である。実装経験に基づき必要に応じて変更する。

最終更新: 2026-11-22

---

## 1. 基本方針

- 名前から所属モジュールと責務を把握できることを優先する。
- 公開APIでは一貫性、private関数では局所的な読みやすさを重視する。
- Headerからは利用者に必要な型とAPIのみを公開する。
- Sourceは自身が直接使う依存を自分でincludeする。
- 明確なOwnership、Lifetime、Commit境界を維持する。
- 命名、include、コメント、メッセージの整理では原則として動作とAPI契約を変更しない。変更を伴う場合は区別して扱う。
- Documentの網羅性ではなく、実装の自己説明性と公開API契約の正確さを優先する。

## 2. ディレクトリとファイルの配置

### 2.1 ディレクトリ名とモジュール名

- ディレクトリは配置、分類、レイヤーを表す。カテゴリディレクトリは複数形を許容する。
- ファイル名と公開API prefixはCシンボル上のモジュール名を表す。
- 親ディレクトリ名をファイル名やAPI prefixへ機械的に転写しない。
- `pipeline`、`registry`、`shader`、`loader`のようにモジュールの役割そのものを表す語はモジュール名へ含めてよい。
- 同名衝突を避ける必要がある、ファイル単体では責務が曖昧になる、または親名が概念の一部である場合には親の語を含めてよい。
- 名前が不自然に長くなる、同じ概念が重複する、単なるパスの再現に過ぎない場合は含めない。

例:

```text
resources/pipelines/geometries/untextured_mesh_geometry_pipeline.c
untextured_mesh_geometry_pipeline_import_from_file()
```

### 2.2 公開範囲を先に決める

- Applicationを含むEngine利用者向けのheaderは`include/engine/`に置く。
- Engine内部でのみ使用する`.c`と`.h`は`engine/`に置く。
- `engine/`内のheaderは内部モジュール間の契約であり、Application向け公開APIではない。
- testによる内部header参照はwhite-box testの特権的アクセスとし、通常の公開APIとは区別する。
- `.c`／`.h`という拡張子だけでなく、公開範囲と責務の所有者で配置を決める。

### 2.3 `internal/`

親モジュールのprivate表現を複数の実装`.c`で共有するために使用する。

対象例は、公開headerでopaqueにした型の実体、共有private型、共有private宣言、親モジュール内部だけの補助定義である。

独立した内部モジュール、Concrete固有実装、一つの`.c`だけで使うstatic関数やprivate型、test専用APIは入れない。`internal/`を非公開ファイルの汎用置き場としない。

一つの`.c`でしか使われない宣言のためにheaderを作らず、その`.c`へ閉じ込める。

### 2.4 `core/`

親領域内の複数の兄弟モジュールで共有する型、result、定数、error utility、result変換、下位APIラッパー等を置く。

`types.h`、`result_t`、`status_t`という名前だけを理由に配置しない。一つのモジュールの状態・設定・補助関数も、共有実態がなければ移動しない。型のsuffixではなく共有範囲と意味上の所有者を優先する。

例えば、Range Allocatorが所有するallocation descriptorは、利用者がVBO ManagerであってもRange Allocator側へ置く。必要なら前方宣言を用いる。階層ごとに`core/`を機械的に増やさない。

### 2.5 責務名ディレクトリ

複数のファイルが独立した責務、契約または実装ファミリーを形成する場合は、`internal/`ではなく責務名で分類する。

例:

- `vtables/`: Graphics API非依存のStrategy契約
- `renderer_backend_concretes/`: Concrete実装の分類
- `renderer_backend_concretes/gl33/`: OpenGL 3.3実装ファミリー

Concreteが一種類でも、契約と具象実装の責務分離に意味があれば専用ディレクトリを設けてよい。契約型を使うことだけを理由に、具象実装を契約側へ移動しない。

### 2.6 配置判断の順序

1. Engine利用者へ公開する必要があるか。
2. 親モジュール内部の共有private表現か。
3. 独立した内部モジュール・責務・実装ファミリーか。
4. 一つの`.c`へ閉じ込められるか。

ディレクトリを実行時の呼び出し順や依存の深さから機械的に構築しない。公開範囲とsemantic ownershipを優先する。

### 2.7 ファイル名、単数形・複数形

ファイル名はモジュールの責務を表す完全名とし、公開API prefixとの一致を基本とする。include guardは実際のファイルパスへ追従させる。単数・複数は対象が一つの概念なのか、複数要素を扱う分類なのかで決め、隣接する既存モジュールとの一貫性を確認する。

## 3. 関数・APIの命名

### 3.1 公開API

- 基本形は`<module>_<object>_<action>()`とし、実際のモジュール名に既にobjectが含まれる場合は重ねない。
- 呼び出し側から見たoperationを名前にする。内部実装の手順を公開名に露出させない。
- `bool`を返す場合、`true`の意味を名前から判断できるようにする。
- 存在確認は`exists`、検索結果を取得する場合は`find`を使用する。
- 妥当性判定は`*_is_valid`、所有確認は`*_has_*`、状態判定は`*_is_*`を基本とする。意味が曖昧な`check`単独は避ける。

### 3.2 動詞の語彙

| 動詞 | 意味 |
|---|---|
| `create` | 新しいobject identityを生成し、storage/addressを取得して公開する |
| `initialize` | objectや値を利用可能な初期状態にする |
| `destroy` | logical lifetimeを終了し、対象自身の独立解放可能なbacking storageも解放する |
| `deinitialize` | 利用可能状態とowned subresourceを終了し、対象自身のbacking storageは残す |
| `reinitialize` | 既存objectを明示的に再構築する |
| `register / unregister` | Registryへの登録・登録解除 |
| `exists` | 存在を判定する |
| `find` | ID、pointer、index等の探索結果を取得する |
| `get` | 値または参照を取得する |
| `load` | ファイル等からCPU側へ読み込む |
| `upload` | CPUからGPUへ転送する |
| `append` | 既存bufferの末尾へ追加する |
| `write` | 指定位置へ書き込む |
| `update` | 既存状態・領域を更新する |
| `import` | load、変換、upload、register等を統括する |
| `release` | 確保済み領域や登録済みResourceの利用権を返却・解放する |

`create / initialize / destroy / deinitialize`は相互の語形対称性やallocatorの種類ではなく、各operationの実際の意味で独立に選ぶ。外部allocatorを使う`create`、`create -> deinitialize`という組合せも、実態と一致していれば許容する。軽量value型の値返却による構築に`initialize`を用いてもよい。既に自然で正確な語を、より実装寄りにできるというだけでrenameしない。

### 3.3 引数順序

原則は、(1) context／manager／registry等の実行主体、(2) 操作対象、(3) 入力値、(4) in/out、(5) 出力先の順とする。例外はAPIの意味を優先する。

### 3.4 引数名・ローカル変数名

型名を機械的に複写せず、scope内でのsemantic roleを表す。通常、型の具体名から一段上のsemantic abstractionを使用する。

```c
linear_allocator_t* allocator_;
renderer_backend_context_t* backend_context_;
flight_camera_t* camera_;
vbo_manager_t* vbo_manager_;
```

- `backend_context`や`vbo_manager`等、対象・用途を成立させる語は削らない。
- `event_view`を単なる`view`にするなど、semantic informationを失う短縮はしない。
- `config_`のようにscope内で一意に理解できる定着したrole名は使用してよい。
- 同じscope内に複数の類似objectがある場合は、`persistent_allocator_`／`temporary_allocator_`等、役割を区別するqualifierを付ける。
- 将来同種のobjectが増えるかもしれないという理由だけで不要なqualifierを先取りしない。
- 構造体fieldは同種の値を複数保持することがあるため、module名やrole qualifierを積極的に保持してよい。

## 4. オブジェクト構築と公開状態

### 4.1 完全コンストラクタ

`create`／`initialize`は、成功時に通常操作へ利用できるstable stateを公開する完全コンストラクタを基本とする。

- 必須resource、ownership、dependencyを成功時までに確立する。
- 構築途中のpartial stateはprivate temporaryへ閉じ込める。
- 一度限りの必須初期化を合理的な理由なく複数の公開APIへ分割しない。
- optional／lazy／dynamic resourceをすべて構築時に取得する必要はない。
- 必須resource取得は可能な範囲でconstructor内にまとめ、取得・temporary ownership・commit・rollbackの流れを追いやすくする。
- 取得前に判定できる入力条件、サイズ、overflowは取得前に確認する。
- 責務の独立性、共有、可読性が改善する場合にはprivate helperへ分けてよい。関数の短縮だけを目的として分割しない。
- public destroyがstable objectを要求する場合、construct中のknown partial state用に`destroy_unchecked`等のprivate cleanup helperを設けてよい。
- unchecked helperはそのconstruction pathのpartial stateに限定し、public APIのvalidationを迂回する一般的な破棄手段にしない。`DATA_CORRUPTED`確定後は呼ばない。

### 4.2 完成・公開・非公開化・破棄

次の遷移を区別する。

```text
構築中 → 完成した未公開状態 → 登録・公開 → 非公開化 → 既存参照の終了 → 破棄
```

- 完成前のobjectをRegistry等で参照可能にしない。
- 公開中のobjectは新規参照取得を停止してから破棄する。
- 登録解除後の既存reference／borrowの有効期間を契約化する。
- reference countingやdeferred destructionは必要な場合だけ採用する。

### 4.3 公開する段階的初期化と再構築

段階的初期化は、中間状態に独立した価値がある、callerが工程を選択する必要がある、外部resourceや非同期処理で単一transactionが成立しない等の場合に限定する。各stable stateの不変条件、許可操作、禁止操作と終了条件を公開契約として定義する。

再構築が必要な場合は、初期constructorを分解せず`reinitialize`等の明示APIを検討する。無効化されるID／handle／descriptor／borrow、commit境界と失敗後状態を契約化する。可能なら新状態をtemporaryに構築してから置き換える。

## 5. OwnershipとBorrow

- pointerやhandleを保持するときはownershipかborrowかを明確にする。
- ownerはresourceのlifetime終了・destroy／deinitialize／releaseに責任を持ち、borrowerは破棄しない。
- borrowed pointerのコピーはownershipのコピーではない。
- borrowはmutabilityと独立する。読み取り専用なら`const T*`、必要ならborrowed `T*`を用いてよい。
- 呼び出し中だけの一時borrowとobject fieldへ保持する長寿命borrowを区別する。
- Moveでは、ownership取得のcommit境界と、移転後のsource pointerの扱いを明確にする。

### 5.1 System-lifetime dependency

長寿命のdependencyをborrowとして保持できるのは、そのlifetimeが全borrowerを包含し、addressが安定し、同一dependencyを継続利用し、途中で暗黙に差し替えず、borrowerはdestroyせず、上位ownerがdestroy順を保証できる場合である。

この条件を満たす場合はconstructorで一度関連付け、通常操作やdestroyごとに同一dependencyを再指定させない。複数APIに同じ引数が現れるという理由だけで長寿命borrowへ変更しない。

## 6. 文字列表現

| 表現 | 用途 |
|---|---|
| `const char*` | 呼び出し中だけ参照するNUL終端文字列 |
| `choco_string_t` | 文字列を所有、保持、変更し、length／capacity等を管理する |
| `fs_path_t` | pathとしてのidentity・不変条件・ownershipを持つ |

- validation項目を増やすためだけに`const char*`を所有objectへ置き換えない。
- 借用されたC文字列はcallerが呼び出し完了まで読み取り可能・NUL終端を保証する。
- getterでowner内部の文字列を返す場合はborrowed viewとし、callerは解放せず、owner変更・破棄後に使用しない。
- object validatorであっても、無効pointerのlivenessやreadability自体を万能に検証できるわけではない。

## 7. 事前条件検証

Validation responsibility／depth／fail-stopの詳細は`validation_policy.md`を正本とする。

### 7.1 基本方針

- Public APIは自分がdereferenceするpointer、ownership／lifecycle、semantic ownershipを持つ条件を、必要なタイミングで検査する。
- 下位moduleが所有するsemantic conditionをforwarding／facade／orchestration側で重複検証しない。
- pointerの単純な存在条件と、値／状態／関係／ownershipに関する意味的条件を分ける。
- 意味的条件は通常の`if`で示し、対応するresult codeと診断を局所化する。
- private preconditions helperは、その検証が独立した責務になり読みやすさや共有に寄与する場合だけ設ける。

### 7.2 NULL検証マクロ

`IF_ARG_NULL_GOTO_CLEANUP`および`IF_ARG_NOT_NULL_GOTO_CLEANUP`は、ポインタのNULL／非NULL条件だけに限定して使用してよい。

- result codeの分類、状態遷移、ownership、rollbackをマクロへ隠さない。
- 呼び出し側でエラー分類とresultを指定する。
- `GOTO_CLEANUP`を名前に含め、分岐の存在を示す。
- 実引数は1回だけ評価し、副作用のある式を渡さない。
- マクロ本体は`do { ... } while(0)`を基本とし、定義末尾にセミコロンを含めない。
- 任意のbool条件を隠す`IF_ARG_FALSE_GOTO_CLEANUP`や同種の汎用マクロは新規追加しない。既存箇所は該当モジュールの改修時に通常の`if`へ段階的に置き換える。

### 7.3 Error Classification

- APIの入力値・形式・範囲の不正は`INVALID_ARGUMENT`。
- 初期化順、登録状態、所有関係などcallerのlifecycle契約違反は`BAD_OPERATION`。
- 成立済みのはずの内部不変条件やownershipの破損は`DATA_CORRUPTED`。

Classificationの詳細な横断判断は`validation_policy.md`を参照し、単純なfalse条件だけをもとに分類しない。

### 7.4 ReturnとCleanup

- Cleanup対象が存在しない段階の失敗では直接returnしてよい。
- Resource取得や外部副作用の開始後に共通cleanupが必要な場合は`goto cleanup`を使用してよい。
- 分岐先は実在させ、未初期化のlocalや無効な対象をcleanupしない。
- Cleanup中に`DATA_CORRUPTED`へ昇格した場合は、後続の通常cleanupを停止する。

## 8. 関数内部の処理フェーズ

必要な関数では次のphaseを区別する。存在しないphaseの空コメントを追加しない。

| Phase | 責務 |
|---|---|
| `Preconditions` | APIの操作前条件を確認する |
| `Prepare` | value、size、temporary、result candidate等を準備する |
| `Preflight` | semantic effectの開始前にoperationが可能か確認する |
| `Result validation` | 公開前のresult candidateがsuccess output contractを満たすか確認する |
| `Commit` | state／ownership／lifecycle／registration等のsemantic transitionを確定する |
| `Postconditions` | Commit後のstable stateを確認する |
| `Output` | 確定した値、ID、view、ownership等をcallerへ反映する |
| `Cleanup / rollback` | 必要な場合に失敗時の解放・復元を行う |

`Commit`と`Output`の境界はoperationの意味に応じて決め、双方を機械的に分けない。`Result validation`はoperation feasibilityの確認ではなく、`Preflight`とも`Postconditions`とも区別する。同一条件をResult validationとOutput後に機械的に再検査しない。

### 8.1 Commit helperのvoid化

開始前に必要な条件が成立し、recoverable failureなしで最後まで完遂できるprivate mutation helperは`void`化してよい。

- `void`化のためにfailureを握りつぶさない。
- Helper内部で初めて判明するallocation failure、外部API失敗、探索失敗、capacity不足、overflow等が残る場合は`void`化しない。
- 成立済みの強い内部契約を再検査しない場合、コードから判断しにくければ直前に短い通常コメントで注意を示す。
- `Contract / Preconditions / Responsibility / Postconditions / Validation`等の定型ブロックやDoxygenは要求しない。

```c
// PreflightでCommit条件は成立済み。ここでは再検査しない。
static void allocation_commit(...);
```

## 9. 型とResult

### 9.1 Opaque／非Opaque

- 状態、owned resource、lifecycle、cache、Backend差分等を隠す型はopaqueを基本とする。
- 実体は単一`.c`、複数`.c`で共有する場合は所有モジュールの`internal/`へ置く。
- enum、result、単純な値型、頂点要素等は非opaqueでよい。
- owner／callerはopaque childのprivate representationからdestroyやrollback可否を推測しない。

### 9.2 Enum

通常の連続enumは0開始とし、末尾の`*_MAX`を有効値でない範囲上限とする。

```c
typedef enum {
    EXAMPLE_A = 0,
    EXAMPLE_B,
    EXAMPLE_MAX
} example_t;

if((unsigned int)value >= (unsigned int)EXAMPLE_MAX) {
    return false;
}
```

bit flag、protocol値、外部ABI対応など、非連続値が必要なenumは例外とし、固有のvalidation contractを定める。

### 9.3 Raw ArrayとValidator

- Raw arrayの全要素を走査するだけのために`xxx_array_is_valid()`を新設しない。
- Owned elementの検査はowner objectのcanonical validatorの責務とする。
- elementが単純な値型でも、単体の意味がある場合はelement validatorを設けてよい。
- ordering、uniqueness、count／capacity等のpersistent invariantが必要なら、collection自体のobject化とcanonical validatorを検討する。
- automatic validationで全要素を走査するかは`validation_policy.md`で決める。

### 9.4 Result型と変数名

- module／subsystemごとに責務に応じた`<module>_result_t`を定義する。
- 実行結果コードの語は`result`を使用し、`rslt`を使用しない。
- 現在の関数のresult変数は`ret`、下位moduleのresultは`ret_<module>`を基本とする。
- 同一moduleの複数resultは`ret_cleanup`等、semantic roleで区別してよい。
- 下位のResultを上位に返す場合、上位の公開operationの意味に変換する。
- `bool`、単純な値操作、Utilityへ複合Resultを機械的に導入しない。

```c
resource_pipeline_result_t ret;
loader_result_t ret_loader;
buffer_manager_result_t ret_buffer_manager;
```

### 9.5 Component State／Component Result

状態を持つcomponentの共通モデルでは、次を区別する。

| State | 意味 |
|---|---|
| `UNINITIALIZED` | 必須の所有関係・内部状態が成立していない |
| `READY` | 通常機能を利用できる |
| `DEGRADED` | 内部整合性はあるが機能・性能の一部を失っている |
| `FAULTED` | 内部整合性・必須機能を失い通常運転を継続できない |

状態は障害隔離単位に保持し、無状態Utilityや小さな値型へ機械的に追加しない。状態の変更権限はowner componentが持つ。

必要な場合に用いる共通Resultのモデル:

```c
typedef struct {
    int32_t code;
    component_state_t state_after;
} component_result_t;
```

- `code`は今回の操作結果、`state_after`は主要操作対象componentの終了時状態を表す。
- `state_after`は直近エラーの保存先ではなくsnapshotであり、複数対象で主要対象が定まらないAPIへ機械的に適用しない。
- failureでもrollbackで整合性が維持されれば`READY`を保持できる。successでも既知の制限があれば`DEGRADED`となり得る。
- 成立済み状態の破損やrollback失敗では`FAULTED`とする。
- `FAULTED -> READY`の直接遷移を許さず、完全な再初期化等による保証の再確立を要する。
- `FATAL`はcomponent stateではなくApplicationの処置である。
- object自身のdestroy後には状態自体が存在せず、所有pointerのNULL化等で終了を表す。
- 上位componentは下位の`state_after`を機械的にコピーせず、依存の必須性、隔離可能性から自身の状態を判断する。
- `DATA_CORRUPTED`を単に`DEGRADED`へ降格しない。

### 9.6 Result変換

Public utilityは`<module>_result_to_str()`および`<destination_module>_result_convert_<source_module>()`を基本とする。File-local converterではdestination prefixを省略してよい。変換引数名は`result_`とする。

- 結果変換はenumの機械的な写像ではなく、上位operationにとっての意味への射影とする。
- `SUCCESS`／`INVALID_ARGUMENT`／`NO_MEMORY`／`LIMIT_EXCEEDED`／`BAD_OPERATION`／`DATA_CORRUPTED`／`OVERFLOW`等、意味が維持される結果は対応づける。
- 既知で正当に発生し得る下位固有エラーは上位operationに応じた結果へ集約し、単に上位に同名codeがないからといって`UNDEFINED_ERROR`にしない。
- `RUNTIME_ERROR`は、適切な呼び出しで起こり得る意味の分かっている実行時失敗に使用する。未知値、内部破損、契約違反のfallbackにしない。
- `UNDEFINED_ERROR`は未定義enum値、変換漏れ、通常到達しない結果に用い、元の値を診断へ残す。
- 内部不変条件の破損は`DATA_CORRUPTED`とする。
- 上位に固有のresult codeを増やすのは、callerの回復、再試行、状態遷移、重大度判断が変わる場合に限定する。
- 結果が集約される境界では下位result、上位result、対象情報等の診断を残す。十分な下位ログがあれば上位は操作文脈のみを追加する。
- Shaderのcompile／linkのように下位では分ける結果も、上位の回復処理が同じなら集約してよい。

## 10. Source File構成

`.c`では、実際に存在する責務ごとにセクションを整理する。次の構成は必要なものだけ採用する例である。

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "..."

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
// Allocation Helpers
// ============================================================

// ============================================================
// Utilities
// ============================================================

// ============================================================
// Validators
// ============================================================
```

- File先頭に`Module Internal Contract`、`Module Validation Policy`、その他の説明用定型ブロックは設けない。
- `.c`先頭のDoxygenによるfile descriptionは原則設けない。
- `Private Type Definitions`には一つの`.c`だけのprivate型を置く。
- `Private Constants`にはfile-local定数・tableを置く。
- `Private Function Declarations`にはforward declarationを置き、必要なら責務ごとにsubgroup化する。宣言と定義の並びを可能な範囲で一致させる。
- `Public API`には公開関数の定義をまとめ、公開canonical validatorもここに置く。
- Private helperは`Allocation Helpers`、`Parsing Helpers`等、実際の責務単位へ分ける。
- Utilitiesへ独立した大きな責務を押し込めない。
- `Validators`にはprivate validatorのみを置く。
- 存在しないsectionやphaseを形式のために追加しない。

### 10.1 コメントの階層

File-level sectionは必要な場合だけbanner形式を使用する。

```c
// ============================================================
// Parsing Helpers
// ============================================================
```

Section内のsubgroupは短い`//`コメント、関数内部のphaseも短いコメントとする。処理が自明ならphaseコメントを省略してよい。

```c
// Preconditions.
// Prepare.
// Commit.
// Output.
```

`API-specific Validation Policy`を関数定義直前に追加しない。公開APIの契約は`.h`の宣言のDoxygenで説明する。`.c`では関数名・実装から復元しにくい意図や注意事項だけを該当箇所へ短く記録する。

## 11. Private関数

- `static`関数ではmodule名prefixを原則省略する。
- 同じファイルで複数の対象を区別するときのみ必要な対象語を付ける。
- bool関数は`true`の意味が分かる名前とする。
- `find_free_slot`、`find_by_name`、`internal_state_is_valid`、`id_is_valid`、`resolve_*`等の役割を明確にする。
- State Query Helperは成立済みの内部契約を信頼して効率的に状態を判定する。Canonical Validatorはfield間の整合性を含むInvariantを検証し、原則State Query Helperへ依存させない。
- Private helper／private validatorのコメントは目的および必要な注意事項だけとし、自明なら省略する。固定のContractやValidation Scopeブロック、Doxygenを要求しない。

## 12. IncludeとHeader

### 12.1 Include

- Headerは利用者に必要な型と宣言だけを公開する。前方宣言で足りれば使用し、完全型が必要な場合はincludeする。
- `size_t`／`bool`／固定幅整数を直接使うheaderは、それぞれ`<stddef.h>`／`<stdbool.h>`／`<stdint.h>`を直接includeする。
- Sourceは自身が直接使用する型・関数・macroのheaderを、自分でincludeする。偶然の間接includeに依存しない。
- Sourceの最初のincludeは原則として対応する自モジュールheaderとする。

原則的なinclude順序:

1. 対応する自モジュールheader
2. C標準header
3. 外部ライブラリheader
4. `engine/base`
5. `engine/core`
6. `engine/containers`
7. `engine/io_utils`
8. 他のEngine infrastructure／resource header
9. `engine/systems`
10. Application header
11. Test専用header

同一領域では共通型、error utility、context／interface、具象module等の依存順を考慮する。ディレクトリ移動に応じて分類名の旧表記へ固執しない。

### 12.2 Include Guard

Include guardは実ファイルパスに追従させ、rename／move時に更新する。コピー元の別モジュール名や古い単複数表記を残さない。

## 13. License／Copyright Header

新規のproduction `.c`／`.h`の先頭は次を基本とする。

```c
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24
```

- `LICENSE`を完全なLicense本文の正本とし、各ファイルで説明を重複しない。
- Author表記は`chocolate-pie24`に統一する。
- 新規ファイルのcopyright yearは作成年とする。既存のyearをheader整備年へ機械的に変更しない。
- 必要に応じて記載されたDoxygen `@date`はcopyright yearと独立して扱い、単なる形式統一で書き換えない。
- 既存ファイルやtestは対象改修時に段階的に整備し、全体を機械的に書き換えない。
- `.h`に必要なfile-level Doxygenを設ける場合はLicense Headerの後に置く。`.c`にfile-level Doxygenを追加することは原則としない。

## 14. Doxygenとコメント

ドキュメントの粒度・配置は`documentation_guide.md`を正本とする。

### 14.1 公開Header

- 利用者に必要な契約は、各API宣言のDoxygen `@brief`、`@param`、`@return`、`@note`、`@pre`、`@post`等で説明する。
- Constructorの成功時stable state、失敗時の出力・ownership状態を利用者が理解できるようにする。
- Resource入力はcopy／move／borrowを区別し、解放方法、解放担当、borrow lifetime、無効化条件を明示する。
- `component_result_t`を使用する場合は、`state_after`がどのcomponentを指すか明示する。
- 外部形式など、複数APIに共通する利用者向けの事項は必要に応じてfile-level Doxygenへ簡潔に記述してよい。
- 固定の`Public API Contract`／`Module Boundary Contract`等のsection構成を要求しない。

### 14.2 Source／Private

- `.c`先頭のDoxygenは原則不要とする。
- `Module Internal Contract`、`Module Validation Policy`、`API-specific Validation Policy`等の独立した説明ブロックは設けない。
- Private helper／validatorには目的と必要な注意事項だけを書き、関数名とコードから明らかな場合はコメントを省略してよい。
- 実装から復元しにくい意図、意図的なValidation省略、Rollbackの注意事項等は該当するコードの近くに短く記述する。
- 実装動作は最新のソースを正本とする。外部に保証する契約は公開HeaderのDoxygenを正本とし、矛盾があれば不整合として報告する。

### 14.3 AI支援表記

AIによる設計文書コメントの草案を採用した場合は、必要に応じて対象が分かる位置へ簡潔に支援の事実を記録してよい。通常の実装コメントへ機械的にAI支援表記を付加しない。

## 15. エラーメッセージ

基本形式:

```text
function_name(RESULT) - Failed to <operation>. reason=<reason>, key=value
```

- Public APIでは失敗したoperation、原因、対象名／ID／size／offset等の診断情報を記録する。
- Private helperでは上位で十分な診断が得られる場合にログを重複しない。
- `destroy`／`deinitialize`のNULL no-op等、契約上の正常処理を無用にERROR扱いしない。
- Cleanupの失敗は握りつぶせるものと重大なものを区別する。
- Format Stringと実引数の数・型を一致させ、rename後に古い関数名・モジュール名を残さない。

## 16. 浮動小数点のNaN／Inf

- Genericな`isfinite()`をすべての浮動小数点APIへ機械的に追加しない。
- 外部ファイル／通信／device等から取得した値は、Trusted Dataへ受理する境界で必要に応じてfinite性を確認する。
- 内部演算では、divisor非ゼロ、normalization対象の非ゼロ性、matrix invertibility、frustumのsemantic range等、演算固有の成立条件を確認する。
- 有限な入力でも内部演算でInf／NaNが生成され得るため、内部値の一律finiteを共通契約にしない。
- Canonical Validatorにfinite checkを設けるのは、finite性自体がその型の明示的Invariantである場合に限る。

## 17. 失敗時不変とResource解放

### 17.1 基本方針

- Recoverable failureでは可能な限り失敗時不変を目指す。
- Rollbackできない外部副作用は公開API契約へ明記する。
- 複数Resourceを扱うoperationではOwnershipとCommit境界を明確にする。
- Commitに入る前に判定可能なFailureをPreconditions／Prepare／Preflightで処理し、Commit開始後は可能な限り最後まで完遂できる構造を優先する。
- Result candidateの妥当性確認はResult validation、Commit済みstable stateの検証はPostconditionsと区別する。
- `DATA_CORRUPTED`が確定した場合は、通常のRollback／Cleanupを停止する。詳細は`validation_policy.md`を正本とする。

### 17.2 Resource取得

- `create`、`initialize`、`allocate`、`import`、`register`等は通常のresource条件により失敗し得る。
- 成功時にのみ有効なobject、ID、descriptorを公開する。
- Recoverable failureでは取得途中のresourceを解放し、Ownershipをcallerへ移転しない。
- 取得成功後に後続operationが失敗した場合、取得済みresourceを対応するRelease／DestroyでRollbackする。
- 解放側が新たな管理resourceを取得しなければ完遂できない設計を避ける。

### 17.3 Object自身のdestroy／deinitialize

- Objectのlogical lifetimeの終了、またはinitialized stateの終了を扱う。取得側operationとの機械的な命名対称性は要求しない。
- 有効なobjectと正しいOwnership／Lifetime契約のもとでは、終了operationを完遂できる構造を基本とする。
- 呼び出し側にrecoverableな結果を返す必要がなければ`void`を基本とする。外部resourceのbest-effort releaseの成否が必要ならoptional out parameter等で別途通知してよい。
- 再試行、durability、保存失敗等をcallerが扱う必要がある処理は`flush`／`sync`／`commit`／`shutdown`等へ分離を検討する。
- NULL、partial state、二重destroyを許容するかはAPIごとに定める。`void`だからNULL no-opとは限らない。
- `T**`形式では`destroy(NULL)`と`destroy(&object)`（`object == NULL`）の扱いを区別して契約化する。
- 有効なobjectを破棄するのに必要なcontext／owner／backendが不正な場合、resourceを残したままsilent no-opとしない。
- 正常なdestroy後はobjectの状態そのものが存在しなくなる。状態遷移と区別する。

### 17.4 Manager／Registry等による解除

- `allocate / free`、`register / unregister`、`import / release`のように、管理主体とID／descriptor／handleで対象を特定する操作を区別する。
- 解除前に管理主体、識別子、Ownership、未解除状態等を検証する。
- Argument契約違反、lifecycle／ownership違反、成立済み内部状態の破損を混同しない。
- 有効性が確定した解除処理は、新たな管理リソース不足で失敗しない構造を基本とする。
- 成立済み状態の解除失敗は`DATA_CORRUPTED`として扱い、通常運転継続やRollbackを保証しない。
- 監査ログ・通知・統計更新などの付随処理の失敗が、論理的な所有権解除を妨げない設計を優先する。

### 17.5 Moveと一時Allocation

- Resourceを外部に返してcallerへOwnershipを渡すAPIは、対応する解放APIを公開Doxygenで明示する。
- Module内部でまだ完成していないAllocationのRollbackは、対応するAllocatorのfreeを使ってよい。完成済みResource用のdestroyを機械的に再利用しない。
- MoveによってOwnerが変わる場合、移転先の解放契約とMemory Tag等の必要なStorage条件を公開契約へ明示する。
- Moveの結果、元Owner側に解放責務が残らないことをコードと契約で明確にする。
- Allocation元と解放担当が異なること自体を契約違反とみなさない。明示されたOwnership／Allocation条件との整合性で判断する。

## 18. テスト規約

- Public APIの事前条件、成功経路、代表的Failure経路をテストする。
- 失敗注入には必要に応じてモジュール専用のtest controlを使用する。
- テスト専用headerを通常ビルドの公開APIへ漏らさない。
- 外部API失敗注入に必要な最小限のproduction test seamは許容するが、旧テスト本体やテスト専用設定をproduction sourceへ保存し続けない。
- Renderer Backend Concreteの`mock_gl*`のような薄い境界は、通常ビルドでは実APIを呼び、テスト時のみ失敗注入・呼び出し検証に利用してよい。

## 19. レビュー観点

主な確認事項:

- ファイルの配置、公開範囲、所有者とAPI prefixが一致しているか。
- `internal/`、`core/`、責務名ディレクトリの使い分けが明確か。
- 一つの`.c`だけの実装のために不要なheaderを作っていないか。
- Engine内部の型やAllocation descriptorを必要なくApplicationへ公開していないか。
- `create / initialize / destroy / deinitialize`が実際のLifecycleと一致しているか。
- 所有権、Borrow、Move、Resource取得・解放の関係が明確か。
- 完全コンストラクタがstable stateのみ公開し、RollbackとCommitを追跡できるか。
- 不要な検証の重複や汎用条件マクロの導入がないか。
- ResultとComponent Stateの意味を混同していないか。
- Result変換で既知の下位エラーを意味のない`UNDEFINED_ERROR`へ変換していないか。
- `DATA_CORRUPTED`後の通常Cleanupが残っていないか。
- Release／Destroyが新たな管理リソース不足に依存しないか。
- 公開契約が`.h`のAPI Doxygenから読み取れるか。
- `.c`の詳細な説明ブロックを残す代わりに、実装構造を改善できないか。
- Private helperのコメントが目的・注意事項に収まっているか。
- DoxygenがIDEのマウスオーバーで公開API仕様を妨げないか。
- SPDX、Copyright、Include Guard、直接include、エラーメッセージが正しいか。

## 20. 適用方針

- 新規コードは本規約に従う。
- 既存コードは対象モジュールの修正時に段階的に適用する。今回のDocumentation方針変更だけを理由に、全ソースを機械的に一括修正しない。
- 既存のAPI-specific Validation PolicyやModule Contract等の詳細コメントを削除する場合、公開契約や実装から復元しにくい重要な意図を失わないようにする。
- 公開前には、命名、include、公開Doxygen、エラーメッセージの横断監査を行う。
