<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Boundary Model

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

最終更新: 2026-10-05
状態: Accepted / Project-wide Boundary Model

## 1. 文書の位置づけ

本書は、GLCEに存在するboundaryをproject-wideに整理し、各boundaryについて次を明確にするための設計正本である。

- boundaryのoutside / insideは何か。
- boundary ownerは誰か。
- ownerが何についてauthorityを持つか。
- どの時点でtrustが成立するか。
- outside側の異常と、owner自身のinternal integrity failureをどう区別するか。
- caller-controlled inputと、成立済みEngine internal stateをどう区別するか。
- 異常をboundary外側へreject / containできる場合と、inside側のintegrity failureとして扱う場合をどう分けるか。

本書は、個々のAPIでcanonical validatorをいつ実行するか、DEBUG / TEST / RELEASEごとのvalidation depth、`DATA_CORRUPTED`後のcleanup規則、FATAL / DEGRADED等のsystem state transitionを直接規定しない。

validation固有の実行規則とresult classificationの正本は`validation_policy.md`とする。module固有のownership、lifetime、representation、validationは各header / sourceのModule Boundary Contract、Module Internal Contract、Module Validation Policy、API-specific Validation Policyで定義する。

本書は、layer architectureとも独立した軸である。layerはdependency / responsibilityの配置を説明し、Boundary Modelはtrust / authority / ownership / integrityの切り替わりを説明する。

---

## 2. 基本用語

### 2.1 Boundary

異なるtrust domain、authority domain、ownership domain、またはintegrity domainの間を分ける境界。

boundaryは必ずしもprocess boundary、layer boundary、module boundaryと一致しない。同一moduleが複数種のboundaryを同時に所有する場合がある。

### 2.2 Boundary Owner

boundaryを越えるdata / resource / stateについて、受理、拒否、変換、normalization、ownership transfer、または自身のstateへの反映を決定するmodule。

Boundary Ownerは、そのboundaryで何をinsideへ受け入れるかについてauthorityを持つ。

ただしBoundary Ownerであることは、system-wideなFATAL / DEGRADED state transition authorityを自動的に持つことを意味しない。

### 2.3 Outside / Inside

- **Outside**: Boundary Ownerが継続的なintegrityを保証できない側。
- **Inside**: Boundary OwnerまたはGLCE内部ownerが、成立済みcontractに基づいて扱う側。

Outsideは必ずしもGLCE process外を意味しない。

例えば次もoutsideになり得る。

- Application-owned mutable config
- caller-supplied value
- caller-supplied storage
- OS / driver / filesystem側state
- external asset bytes
- 別moduleがauthorityを持つresource / handle

### 2.4 Trust Promotion

outside representationをvalidation、parse、normalization、copy、construction等によってinside representationとして受理すること。

```text
outside representation
        ↓
validate / parse / normalize / construct
        ↓
accepted internal representation
```

Trust Promotion後に成立済みとして扱うconditionは、各boundary / API contractで定義する。

### 2.5 Established Internal State

正規のconstructor / initializer / mutation APIを通して成立し、そのmoduleまたはEngineがvalidであることを前提として管理しているstate。

Established Internal Stateは、単なるcaller inputとは区別する。

### 2.6 Caller-Controlled Input

API call時点でcallerが値またはstorageの内容を決定でき、callee側がそのintegrityを継続保証していないinput。

重要なのはC上のrepresentation visibilityではなく、**誰がmutation authorityを持つか**である。

したがって、struct definitionがheaderに見えていることだけを理由としてcaller-controlled valueとは分類しない。

### 2.7 Fault Containment

outside側で発生した異常について、insideのtrusted stateへ影響していないことを確認した上で、inputのreject、channelのclose / disable、resource admissionの中止等によって異常をboundary外側へ局所化すること。

Containment可能性はboundary固有であり、`DATA_CORRUPTED`の一般規則とは分離して考える。

---

## 3. Boundaryの主要分類

GLCEでは、現時点で次の3種類を主要なBoundary categoryとする。

```text
External Trust Boundary
    external / owner-uncontrolled domain
        ↓
    GLCE trusted representation

Engine API Trust Boundary
    Application / Frontend
        ↓
    Engine public API
        ↓
    Engine-controlled domain

Internal Integrity Boundary
    established Engine internal state
        ↓
    owner-managed invariant / ownership relation
```

これらは排他的なmodule分類ではない。

例:

- Platform SystemはExternal Trust Boundary、Engine API Trust Boundary、Internal Integrity Boundaryを持ち得る。
- RendererはEngine API Trust Boundaryを持ち、その内部に複数のInternal Integrity Boundaryを持つ。
- LoaderはExternal Trust Boundaryを持つが、可能な限りpersistent stateを持たない。
- Memory moduleはEngine内部private moduleであるが、自身のcallerから渡されるstorage / requestと、自身が管理するallocator stateの間にboundaryを持つ。

---

## 4. Boundaryを記述する横断的な観点

旧Boundary Model試行で整理したtrust / authority / ownershipの観点は、主要categoryとは別の横断軸として残す。

各boundaryについて、必要に応じて次を記録する。

| 観点 | 内容 |
|---|---|
| Boundary Outside | ownerが継続的integrityを保証しないdata / resource / state |
| Boundary Inside | ownerが成立済みcontractに基づいて扱うrepresentation / state |
| Boundary Owner | admission / conversion / ownership transitionを決定するmodule |
| Semantic Authority | semantic validityを最終的に定義するmodule / external system |
| Resource Authority | resource identity / lifetime / handle validity等を最終的に判断できる主体 |
| Acquisition / Conversion | boundaryを越えるoperation |
| Admission Condition | insideへ受理するために必要なcondition |
| Established Trust | boundary通過後に成立済みとして扱うcontract |
| Remaining External State | inside canonical invariantへ取り込まずexternal authorityへ委ねるstate |
| Ownership / Lifetime | 誰が取得し、誰が破棄するか |
| Outside Failure | outside側の異常をどう扱うか |
| Owner Failure | owner自身のestablished state破損をどう扱うか |
| Provenance Restriction | 特定API / allocator / handle origin等をcontractとして要求するか |

すべてのboundaryで全項目を埋める必要はない。

---

## 5. External Trust Boundary

### 5.1 定義

GLCE外部、またはBoundary Ownerがintegrityを継続保証できないdomainから、GLCE内部representationへdata / resource / stateを受け入れる境界。

Boundary Ownerは、outsideから来た情報をinsideへ受理してよいかを判断する。

Outside側の異常をboundary外側へ明確に局所化できる場合、Ownerはそのinputをrejectする、または該当channelをclose / disableすることでcontainしてよい。

一方、Owner自身のestablished state破損はoutside failureとは区別する。

### 5.2 Boundary Map

| Boundary | Owner | Outside | Inside | Outside側の異常 | Owner自身の破損 |
|---|---|---|---|---|---|
| Asset File | Loader | resource / config等のexternal bytes | typed / normalized internal representation | assetをrejectし、trusted representationとして受理しない | Loaderは可能な限りstatelessとする。依存先internal stateの破損はoutside asset failureと区別する |
| OS / Platform | Platform System | OS、window system、input device、platform backend state | GLCE platform state / event representation | 原因を特定channelへ局所化できる場合、reject / close / disableしてcontainする | Platform-owned established stateのintegrity failureとして扱う |
| External File Stream | Filesystem / FS Stream | OS / C runtimeが管理するfile stream state | GLCE wrapper stateとread result | EOF / I/O error / open failure等として処理し、wrapper corruptionと区別する | Filesystem / FS Stream wrapper自身のcanonical state破損として扱う |
| Memory Input | Memory module | caller-supplied pointer / storage / allocation request | allocator-managed state / allocation result | invalid request / invalid outside inputとしてrejectする | allocator metadata、pool relation、allocation tracking等のintegrity failureとして扱う |

### 5.3 Asset File Boundary

```text
external asset bytes
        ↓
Loader
    parse
    validate
    normalize
        ↓
accepted internal representation
```

Loaderは、自身が受理するasset formatについてExternal Trust Boundaryを所有する。

原則:

- external assetは受理前にはtrusted internal representationとして扱わない。
- malformed / unsupported / out-of-range representationはrecoverable rejectionとして扱う。
- Loaderは可能な限りstatelessとする。
- temporary parse stateはpersistent trusted stateとは区別する。
- candidateはadmission完了までestablished internal stateとはみなさない。
- external assetのinvalidityだけを理由としてinternal corruptionへ分類しない。

### 5.4 OS / Platform Boundary

```text
OS / window system / device
        ↓
Platform System
        ↓
GLCE platform state / event representation
```

Platform Systemはresident systemであるため、次を明確に区別する。

```text
Outside fault
    OS / device / window / external channel側の異常

Owner failure
    Platform-owned established stateのintegrity failure
```

Outside faultを特定channelへ局所化でき、inside stateの健全性を維持できる場合は、channel close / disable等によるcontainmentを許容する。

### 5.5 Filesystem / File Stream Boundary

Filesystem wrapperが保持する`FILE*`等のhandleと、C runtime / OS側が保持するfile position、EOF indicator、error indicator等はauthorityが異なる。

```text
GLCE wrapper state
    file handle identity
    open mode

External stream state
    file position
    EOF
    I/O error
```

external stream stateの変化やI/O failureを、GLCE wrapperのcanonical corruptionとは扱わない。

### 5.6 Memory Input Boundary

`engine/memory`以下はEngine内部private domainであり、Application APIではない。

そのため、`free_list_allocator_t`や`linear_allocator_t`等のrepresentationが内部header上で可視であることは、Application-controlled public valueであることを意味しない。

Memory moduleでは次を区別する。

```text
caller-supplied request / storage
        ↓
Memory API
        ↓
allocator-managed state
```

- allocation size、pointer、tag、rollback point等のcaller-controlled request
- initialize済みallocator state、metadata、pool relation等のestablished internal state

前者のinvalidityと後者のintegrity failureは同じものとして扱わない。

---

## 6. Engine API Trust Boundary

### 6.1 定義

Application / FrontendとEngine public APIの間に存在するtrust boundary。

Applicationから渡されるconfig、value、pointer、request等は、Engine-owned established stateとは区別する。

```text
Application-controlled input
        ↓
Engine public API
    validate
    copy / normalize / construct
        ↓
Engine-controlled state
```

同一process内で生成されたvalueであっても、Application側がmutation authorityを持つ限り、Engine internal stateと同じtrust levelでは扱わない。

### 6.2 Boundary Map

| Boundary Owner | Outside | Inside | Outside側の異常 | Owner自身の破損 |
|---|---|---|---|---|
| Platform System | Application-supplied platform config / request | Platform-owned runtime state | invalid caller inputとしてreject | Platform internal state corruption候補 |
| Renderer / Renderer Frontend相当 | renderer config、camera / matrix / resource request等 | Renderer-owned runtime state / resource relation | argument / operation contractに応じてreject | Renderer-owned established state corruption候補 |
| Event System | Application / orchestration側request | Engine event state / published view backing state | invalid requestとしてreject | Event System internal state corruption候補 |
| Camera / Camera Registry | config、keybind、camera operation request | camera / registry-owned state | invalid input / bad operationとしてreject | established camera / registry state corruption候補 |
| Resource creation APIs | config / descriptor / value metadata | opaque resource object | input validity failureとしてreject | constructed / established resource state corruption候補 |

### 6.3 Core Rule: caller-controlled input vs established internal state

今回のBoundary Modelで最も重要な区別は次である。

```text
caller-controlled input
    ↓ invalid
recoverable input failure

established Engine internal state
    ↓ integrity failure
internal corruption candidate
```

この区別は、structがpublicかopaqueかだけでは決めない。

### 6.4 Representation visibilityは分類軸ではない

次の単純化は採用しない。

```text
public struct  → INVALID_ARGUMENT
opaque object  → DATA_CORRUPTED
```

これはrepresentation visibilityに依存しすぎるためである。

代わりに、次を分類軸とする。

- callerが現在のinput valueを自由に決定 / mutationできるか。
- そのstateが正規lifecycleを通して成立済みか。
- そのstateのintegrityを誰が保証するcontractになっているか。

例えば`engine/memory`配下のallocator structはinternal header上でfieldが見えていても、Engine内部private module stateとして扱う。

### 6.5 Caller-Controlled Input

Engine API Trust Boundaryを越えてcallerから入力されたvalueが、そのAPIで要求されるvalidityを満たさない場合、原則としてrecoverable input failureとして扱う。

result classificationの詳細は`validation_policy.md`を正本とするが、代表的には`INVALID_ARGUMENT`である。

重要なのは、calleeがそのvalueのprovenanceを推測しないことである。

例えば、Engine内部の別moduleがbugによってinvalid valueを生成し、そのvalueがAPI inputとして渡された場合でも、受け手が観測できる事実が「invalid input」でしかないなら、受け手側で機械的に`DATA_CORRUPTED`へ昇格させない。

### 6.6 Established Engine Internal State

正規のconstructor / initializer / mutation APIを通して成立し、Engineがvalidであることを前提として管理しているstateについてcanonical invariantが失われていることを確認した場合は、caller input failureとは意味が異なる。

これはinternal corruption classificationの根拠になり得る。

ただし、`is_valid() == false`だけを理由として機械的に`DATA_CORRUPTED`へ分類しない。具体的なresult classificationは`validation_policy.md`を正本とする。

### 6.7 Config / Descriptor / Value Type

Configやdescriptorは、Engine-owned runtime stateではなくApplicationからEngineへ渡すinput descriptionとして扱う。

```text
public config / descriptor / value
        ↓
Engine create / initialize / operation API
        ↓ validation
copy / normalize / consume
        ↓
Engine internal state
```

Engine側は、create / initialize後にcaller-owned mutable storageへ不必要に依存しない構造を優先する。

pointer field等を保持する場合はownership / lifetime contractを明示する。

### 6.8 Borrowed Output / Published View

representationがpublicであっても、Engine-owned stateをborrow / snapshotとして公開しているだけの場合がある。

この場合、型定義が公開されていることを理由にcaller-controlled inputとは分類しない。

callerがmutation権限を持つか、const borrowなのか、copyなのかをcontractで判断する。

---

## 7. Internal Integrity Boundary

### 7.1 定義

Engine内部でpersistent state、ownership relation、resource identity、registry relation、allocator metadata、その他module-specific invariantを所有するmoduleの境界。

Internal Integrity Boundary Ownerは、自身のowned stateについてauthorityを持つ。

このBoundary Modelは、Ownerがcanonical validatorをいつ・どのdepthで自動実行するかまでは規定しない。

### 7.2 Boundary Map

| Owner | Owned State / Authority | Boundary外から入るもの | Incoming側の異常 | Owner自身の破損 |
|---|---|---|---|---|
| Resource Registry | registry table、resource identity、registration relation、count / capacity | resource / registration request | argument / operation contractに応じてreject | registry internal relation / metadataのintegrity failure |
| Resource / Shader | resource lifecycle、shader-owned CPU/GPU relation | config、geometry / texture reference、update request | input / operation-specific failure | established resource / shader stateのintegrity failure |
| Event System | event aggregation state、buffer / published view relation | Platform由来event representation | update / input failureとして処理 | Event System-owned state / relationのintegrity failure |
| Camera Registry | registered camera identity、registration relation | camera object / registration request | input / bad operationとしてreject | registry internal relation / metadataのintegrity failure |
| Containers | queue / string objectとowned storage | operation argument / source data | input contractに応じてreject | established container state / owned storage relationのintegrity failure |
| Geometry / Texture Resource | owned CPU resource / metadata / storage relation | source value / descriptor / candidate data | input validity failureとしてreject | established resource stateのintegrity failure |
| Memory Allocator | allocator metadata、pool relation、allocation state | allocation / free / rollback request | request contractに応じてreject | allocator-managed stateのintegrity failure |

### 7.3 Owner Responsibility

Internal Integrity Boundary Ownerは、少なくとも次についてauthorityを持つ。

- 自身が所有するpersistent stateのdefinition
- lifecycle / ownership relation
- internal invariant
- state mutation operation
- 自身のboundary外から受け取るinputのAPI contract

ただし、Ownerであることは次を自動的には意味しない。

- 全public APIでcanonical validatorを毎回実行すること
- deep validationをRELEASE_BUILDで常時実行すること
- lower moduleのstateを重複validationすること
- system-wide FATAL / DEGRADED stateを直接決定すること

### 7.4 Lower module resultとの関係

上位moduleは、lower moduleのestablished state integrityについて独自に再判定するのではなく、lower APIが返したresultとcontractを基本的に信頼する。

Boundary Modelは、上位moduleごとにcaller provenanceを再推論してresultを再分類する一般規則を導入しない。

これにより、通常のresult propagation / result conversionを単純に保つ。

---

## 8. Failure Classificationとの関係

Boundary Modelはresult code taxonomyそのものではないが、failureの意味を整理する判断材料を提供する。

### 8.1 基本的な方向

| Case | 基本的な方向 |
|---|---|
| caller-controlled inputが要求validityを満たさない | recoverable input failure。代表的には`INVALID_ARGUMENT` |
| valid arguments間のlifecycle / ownership / operation relationが成立しない | `BAD_OPERATION`等 |
| external asset / external representationがformat contractを満たさない | `UNSUPPORTED_FILE`等のresource-specific rejection |
| OS / file stream / external channelでruntime failure | runtime / I/O-specific result |
| established internal stateのintegrity failureを確認 | `DATA_CORRUPTED`候補 |
| temporary candidateを構築した結果、module自身のconstruction / output contractを満たさない | Result validation failureとしてinternal corruption候補 |

### 8.2 `DATA_CORRUPTED`を安易に使わない

本Boundary Modelでは、次を重要な原則とする。

> `is_valid() == false`という事実だけから、機械的に`DATA_CORRUPTED`を導出しない。

`DATA_CORRUPTED`は、成立済みinternal stateのintegrityが失われているという追加の根拠がある場合に検討する。

これにより、Application側の入力ミスやcaller-controlled valueのinvalidityを、Engine internal corruptionとして過剰に扱うことを避ける。

### 8.3 Internal bugを`INVALID_ARGUMENT`として観測する可能性

Engine内部producerのbugによりinvalid valueが生成され、それが別module APIへinputとして渡された場合、consumer側では`INVALID_ARGUMENT`等のrecoverable failureとして観測される可能性がある。

これは許容する。

consumerが観測できる事実だけでは、次のどれかを一意に判定できない場合があるためである。

- callerがvalueを誤って構築した。
- upstream internal moduleがbugでinvalid valueを生成した。
- external inputからinvalid valueが流入した。
- memory corruption等でvalueが変化した。

このprovenanceを全call chainで追跡して自動再分類する仕組みは、現時点では導入しない。

producer側がestablished stateを壊している場合は、producer自身のcanonical validation / postcondition等でinternal corruptionを検出する余地がある。

### 8.4 Application bugをFATAL級corruptionへ誤分類しない

逆方向の誤分類にも注意する。

Application-controlled valueのinvalidityを`DATA_CORRUPTED`として扱うと、将来`DATA_CORRUPTED`をFATAL方向へ伝播させるsystem state policyと組み合わさった際に、単なるApplication bugやAPI misuseでEngine全体を停止させる可能性がある。

そのため、Engine API Trust Boundaryではcaller-controlled inputとestablished internal stateを明確に区別する。

---

## 9. Boundary OwnerとContainment

Boundary Ownerは、outside failureを無条件にinternal failureへ昇格させるのではなく、自身のboundaryでreject / containment可能かを判断できる。

```text
outside failure
    ↓
Boundary Owner
    ├─ insideへ未受理 / 影響局所化可能
    │      → reject / contain
    │
    └─ inside established stateのintegrityを保証できない
           → internal integrity failureとして上位へ伝播
```

ただし、Boundary Ownerはlower moduleのtrue internal corruptionを都合よくoutside failureとして書き換えるauthorityを持つわけではない。

containmentには、fault causeとimpactを自身のboundary外側へ局所化できる根拠が必要である。

---

## 10. Layer Modelとの関係

Boundary Modelはdependency layerとは別軸である。

現在のGLCEの概念的なlayer構成は、おおむね次のように捉える。

```text
Application Layer
        ↓
Subsystem Layer
    Platform / Renderer / Camera / Event
        ↓
Resource Layer
    Texture / Geometry / Shader / Registry
        ↓
I/O Utility Layer
    fs_path / fs_stream
        ↓
Containers Layer
    ring_queue / choco_string
        ↓
Core Layer
        ↓
Memory Layer
        ↓
Base Layer
```

Boundaryはこのlayer構成を横断する。

例:

- Asset File BoundaryはLoaderを中心にResource / I/Oを跨ぐ。
- OS BoundaryはSubsystem LayerのPlatformが所有する。
- Engine API Trust BoundaryはApplicationとEngine public API群の間に存在する。
- Internal Integrity Boundaryは各layer内のstate ownerごとに存在し得る。
- Memory Layerは低layerだが、Engine foundationとして広いimpactを持つ。

したがって、layerの低さとfault impactの小ささは同義ではない。

---

## 11. 現時点で採用しない一般化

今回のBoundary Model検討では、複数のより強い一般化案を検討したが、現時点では採用しない。

### 11.1 Boundary Ownerだけがcanonical validationを実行する規則

Ownerだけにcanonical validation authorityを集約し、各public API callでdeep validationする案を検討した。

しかし、validation authorityとexecution frequency / depthを一律に結びつけるとmoduleごとのoperation semanticsやcost modelを損ね、かえってValidation Policyを複雑にするため採用しない。

canonical validation depth / timingは`validation_policy.md`とmodule-specific policyで決定する。

### 11.2 caller provenanceによるresult再分類

lower APIが`INVALID_ARGUMENT`を返した場合に、callerがinternal moduleなら上位で`DATA_CORRUPTED`へ再分類する案を検討した。

しかし、各call siteでprovenance-awareなif / else classificationが必要になり、現在の単純なresult conversion / propagation modelを大きく複雑化するため採用しない。

### 11.3 Error / Fault Management Systemによる一元分類

resultとは別にfault domain、origin、containment state等を持つFault Infrastructureを導入する案も検討した。

将来必要になる可能性はあるが、現時点の具体的problemに対しては過剰であり、まずBoundary Modelと慎重な`DATA_CORRUPTED` classificationで進める。

### 11.4 public / opaqueだけによる分類

`public value → INVALID_ARGUMENT`、`opaque object → DATA_CORRUPTED`という単純規則は採用しない。

本質はrepresentation visibilityではなく、caller-controlled inputか、established module-managed stateかである。

---

## 12. 本書で規定しない事項

以下はBoundary Modelとは分離して扱う。

- canonical validatorのautomatic execution frequency
- DEBUG / TEST / RELEASE別のvalidation depth
- `DATA_CORRUPTED`検出後のcleanup / fail-stop規則
- `DATA_CORRUPTED`とFATAL stateの最終対応
- RUNNING / DEGRADED / FATAL等のsystem / subsystem state machine
- logging / diagnostic severity
- generic Error Management / Fault Management System
- assertion / panic implementation
- lower resultをupper layerで再分類する一般機構

これらは`validation_policy.md`、`design_notes.md`、将来の専用設計で扱う。

---

## 13. Module Documentationとの関係

```text
boundary_model.md
    ↓
project-wide boundary / trust / authority / integrity model

module header
    ↓
Module Boundary Contract

module source
    ↓
Module Internal Contract
    ↓
Module Validation Policy
    ↓
API-specific Validation Policy
```

本書は個別moduleのcontractを置き換えない。

各moduleでは、本書のBoundary categoryを必要に応じて参照しつつ、実際のownership、lifetime、validation、result、cleanup behaviorをmodule固有のcontractとして定義する。

---

## 14. AIに読み込ませる場合の質問例

本書は、GLCEのboundary設計をAIへ共有するためのproject-wide資料として利用する。

内容を確認したい場合は、本書を読み込ませた上で、例えば次のように質問する。

- 「このAPIはどのBoundary categoryに属するか？」
- 「このargumentはcaller-controlled inputか、established internal stateか？」
- 「このinvalidityはoutside failureか、owner自身のintegrity failureか？」
- 「このmoduleはExternal Trust BoundaryとInternal Integrity Boundaryの両方を持つか？」
- 「このfailureをboundary内でreject / containできるか？」
- 「この型のstruct definitionが見えていることは、caller-controlled valueであることを意味するか？」
- 「このvalidator failureを`INVALID_ARGUMENT`と`DATA_CORRUPTED`のどちらへ分類すべきか。Boundary ModelとValidation Policyの両方から説明して。」
- 「このstateはどのmoduleがsemantic / resource authorityを持つか？」
- 「このmodule固有のBoundary Contractへ何を明記すべきか？」
