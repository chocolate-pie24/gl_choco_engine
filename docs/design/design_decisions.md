<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Design Decisions

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

この文書は、GLCEで確定した重要な設計判断と、その判断理由・却下案・見直し条件を記録する。

一般化されたCoding StyleやValidation Policyそのものは、それぞれの正本文書へ集約し、本書では再記載しない。
本書には、GLCE固有のarchitecture、ownership、module boundary、implementation choiceと、その判断に至った理由を残す。
一般policyへ依存する個別Decisionでは、必要に応じて正本文書を参照する。

---

## Decision: 将来の独立subsystemはprocess isolationとmessage passingを基本とする

**Status:** Accepted  
**Date:** 2026-08-26  
**Scope:** System Architecture / Process Isolation / Concurrency

### Decision

GLCEでは、将来Network、IPC、external device等の独立subsystemを追加する際、Engine内部へshared mutable stateを持つthreadを増やすことを基本方針としない。

独立した責務は必要に応じて別processへ分離し、subsystem間はexplicitなmessage / protocolで連携する。

GLCE Engineは、別の明確な性能要件によってparallelismが必要と判断されるまで、single-thread executionを基本とする。

process間では通常のpointerやprocess-local objectを共有せず、command、ID、value、state snapshot、result等のsemantic dataを交換する。

### Context

将来UDP / network communicationやexternal controller連携を追加した場合、I/OのためだけにEngine内部へthread、mutex、atomic、shared mutable stateを広げる設計と、subsystemを独立processへ分離する設計のどちらを基本とするかを決める必要があった。

GLCEではrenderer、registry、allocator等のmutable stateを予測可能な逐次実行モデルで扱うことを重視している。また、工場設備やcontroller分散を想定すると、描画Engineとcommunication / equipment I/Oが同一address spaceに存在することを前提にしない構造が適している。

### Rationale

- Engine内部のmutable stateへthread synchronizationを広げずに済む。
- subsystemごとのmemory ownershipとfailure boundaryをaddress space単位で分離できる。
- network / equipment I/O等を別controllerへ配置できる。
- subsystem failureが直接Engine memory corruptionへ波及する範囲を制限できる。
- subsystem間の結合点をinternal objectではなくprotocol contractへ固定できる。
- Engine内部ではpredictableなsingle-thread execution modelを維持できる。

### Rejected Alternatives

- Network I/O等の追加に合わせてEngine内部をmulti-thread化する。
  - communication要件を理由にRenderer / Registry / Allocator等へthread-safety requirementが波及する。
- shared memoryをsubsystem間の通常contractとする。
  - ownershipとsynchronization boundaryがprocess境界を越えて曖昧になる。
- process間でEngine内部objectやprocess-local handleを直接共有する。
  - address-space isolationとprotocol boundaryを崩す。
- Engine public C APIをそのままRPC / wire protocolへ1:1変換する。
  - process boundaryに不要なinternal representationを漏らす。

### Consequences

- 将来の独立subsystemは、同一process内のthreadではなく別processとして実装できる構造を優先する。
- Engine stateのauthorityはEngine process内に置き、外部processはcommand / query / notificationで操作する。
- protocolとtransportは分離し、TCP / UDP / Unix domain socket等の具体的選択は実装要求が生じた時点で決める。
- per-vertex等の細粒度なEngine internal operationをprocess boundaryへ出さず、coarse-grainedなsemantic operationを基本とする。
- supervisor、restart policy、serialization format、message versioning等は現時点では未確定のままDesign Notesで扱う。

### Revisit Conditions

- 明確な性能要件によってEngine内部parallelismが必要となり、process分離だけでは要求を満たせない場合。
- shared memoryが必要なlatency / throughput requirementが具体的に生じた場合。
- deployment modelが単一processへ恒久的に固定され、process isolationを維持する価値が失われた場合。

### Related Documents

- `design_notes.md`

---

## Decision: Texture RegistryをCPU / GPU Texture Resourceのownership commit boundaryとする

**Status:** Accepted  
**Date:** 2026-08-30  
**Scope:** Texture / Resource Ownership / Registration

### Decision

Texture importでは、Pipelineをregistration完了までのtemporary ownerとし、Texture Registryへの登録成功を長寿命ownershipのcommit boundaryとする。

基本flowは以下とする。

```text
BMP Loader / solid-color generator
        ↓ temporary pixels
CPU Texture Resource
        ↓ pixel dataを一時borrow
GPU Texture Resource
        ↓
Texture Registry entry
    owns resource name
    owns CPU Texture Resource
    owns GPU Texture Resource
```

CPU Texture Resourceはdecoded pixel dataと画像metadataを所有する。
GPU Texture Resourceはbackend textureを所有し、upload完了後にCPU pixel dataや画像metadataを重複保持しない。

Registry登録成功時にCPU / GPU ResourceのownershipをRegistryへmoveし、それ以前のfailureではPipelineがその時点で所有するtemporary resourceを処理する。

### Context

Texture importはdecode、CPU resource生成、GPU upload、Registry registrationという複数段階を持つため、途中段階ごとのownerを曖昧にするとleak、double-free、入力resourceの不意な消費が起きやすい。

またBMP file由来とsolid-color生成ではsourceが異なる一方、CPU Resource生成後のownership flowは共通である。

### Rationale

- 長寿命resource ownershipのcommit pointをRegistry registrationへ一本化できる。
- Pipelineを短命なtransaction coordinatorとして保てる。
- CPU / GPU Resource間でpixel dataや画像metadataのauthorityを重複させずに済む。
- registration failure時に、ownershipがどこにあるかを段階ごとに明確にできる。
- source種別が異なっても、CPU Resource生成後のownership flowを共有できる。

### Rejected Alternatives

- Pipelineが登録後もCPU / GPU Resourceのownershipを保持する。
  - Registryとの二重ownershipになる。
- GPU Texture ResourceがCPU pixel dataや画像metadataをupload後も保持する。
  - 同じ事実のauthorityがCPU / GPU Resourceへ重複する。
- BMP importとsolid-color生成をNULL path等のsentinelで単一source APIへ押し込める。
  - source semanticsが異なる処理を不自然なparameter stateで表すことになる。
- Registry登録前にcallerへ部分構築済みresourceを公開する。
  - ownership transactionの途中状態がpublicへ漏れる。

### Consequences

- Texture Registry entryはresource name、CPU Resource、GPU Resourceを一体として所有する。
- Pipelineはregistration成功後にCPU / GPU Resourceを破棄しない。
- CPU Resourceはpixel dataのauthority、GPU Resourceはbackend textureのauthorityとなる。
- BMP importとsolid-color importは別APIとして扱いつつ、CPU Resource生成後は同じownership transactionを使用する。

### Revisit Conditions

- CPU-side texture dataを登録後に保持しないstreaming architectureへ移行する場合。
- GPU resource lifetimeをRegistryとは別のcache / residency managerが所有する場合。
- texture asset identityとruntime GPU residencyを分離するresource modelへ変更する場合。

### Related Documents

- `design_notes.md`

---

## Decision: File I/Oは`filesystem` / `fs_stream` / `fs_path` / Applicationへ責務分離する

**Status:** Accepted  
**Date:** 2026-08-31  
**Scope:** Filesystem / Stream / Path Architecture

### Decision

File I/Oとpath constructionの責務を以下へ分離する。

```text
Application
    owns project / asset location policy
        ↓
fs_path
    owns completed path construction
        ↓
Loader / Shader / Pipeline
    consumes completed path
        ↓
fs_stream
    owns Application-facing file session
        ↓
filesystem
    owns raw file I/O state
```

`filesystem`はEngine内部moduleとし、raw file handle、open mode、raw file position等のauthorityを持つ。

`fs_stream`は一つの`filesystem_t`をexclusive ownershipし、Application-facingなfile sessionを表す。

`fs_path`は一つの完成済みpath stringを所有し、filepath / filename / extension / fullpathを別々のpersistent fieldとして重複保持しない。

Loader / Shader / Pipelineは完成済みpathを呼び出し中だけ利用し、project root推測やpath joinを行わない。

### Context

旧構造では`fs_utils`と`filesystem`の間でfile stateやopen modeの責務が重なり、path constructionもLoader側まで分散していた。

このため、file session、raw I/O、path ownership、asset location policyをどこが所有するかを分離する必要があった。

### Rationale

- raw file stateのauthorityを`filesystem`へ一元化できる。
- Applicationからraw filesystem implementationを隠せる。
- file sessionを`fs_stream`へ集約し、同一fileについて複数objectがstateを重複保持することを避けられる。
- path constructionとresource format parsingを分離できる。
- Loaderをproject layoutや`fs_path_t`へ依存させず、完成済みpathで再利用できる。
- 将来write / seek / buffer等を追加しても責務境界を維持しやすい。

### Rejected Alternatives

- 旧`fs_utils`でpath情報、mode、filesystem stateをまとめて保持し続ける。
  - 同じfile sessionのauthorityが複数moduleへ分散する。
- Applicationへ`filesystem_t`を直接公開する。
  - raw I/O implementationがApplication boundaryへ漏れる。
- Loaderごとにproject root推測やpath joinを行う。
  - asset location policyがformat-specific moduleへ分散する。
- read / write / existence queryごとに独立したstateful file objectへ分割する。
  - 一つのfile sessionを複数moduleへ分断する。

### Consequences

- `filesystem_t`はEngine内部に閉じる。
- `fs_stream_t`がApplication-facing file sessionのownerとなる。
- open modeのruntime authorityは`filesystem_t`に置く。
- Loaderは完成済みpath stringだけを受け取り、path objectを保持しない。
- existence query等のfile state queryを追加する場合も、path valueの責務ではなくI/O責務として扱う。

### Revisit Conditions

- Virtual Filesystem、archive、mount point、resource search path等を導入し、現在のfile session abstractionでは不足する場合。
- async I/Oやstreaming I/Oによってfile sessionのownership modelを再定義する必要が生じた場合。

### Related Documents

- `design_notes.md`

---

## Decision: Asset path policyはApplicationが所有し、Loaderへ完成済みpathを渡す

**Status:** Accepted  
**Date:** 2026-08-31  
**Scope:** Filesystem / Asset Location / Path Policy

### Decision

GLCE repository内assetのlocation policyはApplicationが所有する。

Applicationはexecutable directoryを基準にproject / asset locationを決定し、`fs_path`でresourceごとの完成済みpathを構築する。

Loader / Shader / Pipelineには`fs_path_t`そのものではなく、完成済みpath stringを渡す。

現在のrepository layoutでは、概念的に以下を使用する。

```text
executable_directory = project_root/glce/bin
project_root         = executable_directory/../..
asset_root           = project_root/assets
```

project rootを`.git`、Makefile、`LICENSE`等のmarker探索で推測せず、現在はlexical path policyとして扱う。

### Context

runtime current working directoryに依存したasset pathでは、起動方法によって同じbinaryがresourceを読めなくなる。

一方、Loaderがproject rootやasset layoutを知る構造にすると、resource format処理とApplicationのdeployment policyが結合する。

### Rationale

- runtime CWDに依存せず同じassetへ到達できる。
- project / asset layoutというApplication policyをLoaderから分離できる。
- Loaderをfile dialog、command line、temporary file等の別sourceから得た完成済みpathにも再利用できる。
- project root discovery mechanismを導入せず、現在のrepository contractを単純に表現できる。
- path objectのlifetimeをApplication側へ閉じ、下位moduleは必要な文字列だけをborrowできる。

### Rejected Alternatives

- Loader内で`../assets/...`等のCWD-relative pathをhard-codeする。
- Loaderへfilepath / filename / extensionを別々に渡して再結合させる。
- `.git`等のmarkerを探索してproject rootをruntime推測する。
- Loaderへ`fs_path_t`を渡し、path policyやpath object semanticsへ依存させる。
- 現段階で`realpath`、symlink解決、resource search path等を必須化する。

### Consequences

- Application codeではassetのlogical relative pathを指定する。
- Loader / Shader / Pipelineは完成済みpathを保持・再結合しない。
- 現在のpath constructionはlexicalであり、filesystem canonicalizationを責務に含めない。
- Windowsのseparator変換やexecutable directory取得は、Windows対応workとして追加する。

### Revisit Conditions

- repository layout以外のasset deploymentを正式supportする場合。
- install prefix、package resource、archive、mount point、search path等が必要になった場合。
- Application外のresource locator serviceを導入する場合。

### Related Documents

- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: Geometry resource identityはRegistryが所有しCPU Geometryへ埋め込まない

**Status:** Accepted  
**Date:** 2026-09-02  
**Scope:** Geometry / Resource Identity / Registry Ownership

### Decision

Geometry CPU Resourceはgeometryそのもののdataだけを所有し、resource nameを保持しない。

resource nameはGeometry Registry entryがresource identityとして所有する。

基本ownershipは以下とする。

```text
Geometry CPU Resource owns:
    vertices
    vertex_count
    geometry-specific data

Geometry Registry entry owns:
    resource_name
    CPU Geometry
    allocation descriptor
```

CPU GeometryはRegistryへ登録される前後で同じdata modelを維持する。

### Context

従来はresource nameをCPU Geometry側へ持たせる構造があり、geometry dataとRegistry identityが同一objectへ混在していた。

しかしresource nameは幾何データそのものではなく、Registry上でresourceを検索・識別するためのidentityである。

### Rationale

- Geometry dataとRegistry identityの責務を分離できる。
- Registry登録前のGeometryにも不要なidentity fieldを要求せずに済む。
- 同じGeometry dataを別identityで扱う場合にもdata modelを変更する必要がない。
- resource nameのauthorityをRegistryへ一元化できる。
- CPU Geometryからname string ownershipと関連APIを除去できる。

### Rejected Alternatives

- CPU Geometry自身がresource nameを所有する。
  - Geometry dataとRegistry identityが混在する。
- RegistryとCPU Geometryの両方が同じresource nameを保持する。
  - identityのauthorityが重複する。
- Pipelineがresource nameを長寿命stateとして保持する。
  - orchestration layerへidentity ownershipが漏れる。

### Consequences

- line / lit / point / UI Geometry CPU Resourceはname fieldを持たない。
- Geometry Registryがresource nameとCPU Geometryを一体として管理する。
- Pipelineはregistration時にidentityとしてnameをRegistryへ渡すが、CPU Geometryへ埋め込まない。

### Revisit Conditions

- Geometry object自体がRegistry外でも永続的なasset identityを必要とするmodelへ変更する場合。
- Registry identityとasset identityを別概念として導入する場合。

### Related Documents

- `design_notes.md`

---

## Decision: Geometry CPU ResourceはFAMではなくowned vertex allocationを使用する

**Status:** Accepted  
**Date:** 2026-09-02  
**Scope:** Geometry / CPU Resource / Memory Layout

### Decision

Geometry CPU Resourceの可変長vertex dataにはFlexible Array Memberを採用せず、`vertex_count + owned vertices pointer`による独立allocationを使用する。

Geometry object本体とvertex storageを別allocationとして扱う。

### Context

Geometry CPU Resourceのowned vertex arrayをobject本体へ連続配置する案としてFlexible Array Memberを利用できる。

一方、将来Geometry objectが複数の可変長fieldを持つ可能性や、allocation strategyを変更する可能性を考えると、FAMによってphysical layoutを固定することには制約がある。

### Rationale

- 将来複数の可変長fieldを追加する場合にobject layoutを硬直化させない。
- vertex storageのallocation strategyをobject本体から分離できる。
- Geometry typeごとのdata model拡張余地を維持できる。
- FAM化だけではraw pointer corruptionに対する安全性問題は解決しないため、layout変更をmemory-safety対策として扱わない。

### Rejected Alternatives

- Geometry object末尾にFlexible Array Memberとしてverticesを連続配置する。
  - object layoutとallocation strategyが一体化し、複数variable-length fieldへの拡張が難しくなる。
- pointerを排除すること自体をvalidator安全性の主要対策とする。
  - memory safetyの診断問題とresource layoutの問題を混同する。

### Consequences

- Geometry CPU Resourceはvertex arrayをexclusive ownershipするpointer fieldを持つ。
- constructorは入力vertex arrayからGeometry-owned storageを作成する。
- vertex storageのallocation metadata強化はMemory System側の整備と独立して進められる。

### Revisit Conditions

- profile結果からsingle-allocation layoutが明確に必要になった場合。
- Geometry data modelが固定され、FAMの制約が問題にならないことが確認できた場合。
- custom arena / packed asset representation等へdata layoutを全面変更する場合。

### Related Documents

- `design_notes.md`

---

## Decision: `camera_t`はinternal primitiveとしgeneric Camera abstractionを先行導入しない

**Status:** Accepted  
**Date:** 2026-09-07  
**Scope:** Camera / Abstraction Boundary

### Decision

`camera_t`はCamera種別共通のlow-level state / matrix処理を提供するEngine内部primitiveとし、generic public Camera interfaceとして扱わない。

現時点ではFlight Cameraをconcrete Cameraとして実装し、将来のCamera種別を想定した以下のgeneric abstractionは導入しない。

- generic `camera_registry_t`
- Camera typeを隠すvtable / Strategy / type-erasure
- generic Camera Pipeline
- 将来利用だけを目的としたgeneric Camera public interface

Orbit Camera等の別Camera種別を追加した後、実際の重複または共通contractが確認できた時点で共通化を再検討する。

### Context

Flight Camera設計時点では、将来別Camera種別を追加する可能性はあるものの、実装済みconcrete CameraはFlight Cameraだけだった。

この段階でgeneric abstractionを先行導入すると、実際の共通性ではなく予測に基づくinterfaceを固定することになる。

### Rationale

- 実在する共通機能だけを`camera_t`へ置ける。
- Flight Camera固有command / keybind / movement behaviorをcommon Cameraへ漏らさずに済む。
- 将来Camera種別の設計を、先行したgeneric interfaceへ無理に合わせる必要がない。
- abstraction layer、vtable、generic registry等の複雑性を実需要が生じるまで導入せずに済む。

### Rejected Alternatives

- Flight Cameraしか存在しない段階でgeneric `camera_registry_t`を導入する。
- vtable / StrategyによってCamera種別を最初からtype-erasureする。
- Camera Pipelineやgeneric Camera facadeを先行実装する。
- `camera_t`をApplication向けgeneric public interfaceとして公開する。

### Consequences

- Applicationはinternal `camera_t`を直接扱わず、concrete Camera APIを利用する。
- `camera_t`にはposition、frustum、matrix / cache等の実際に共通するlow-level責務だけを置く。
- 新しいCamera種別はまず独立concrete moduleとして実装する。

### Revisit Conditions

- Orbit Camera等、2種類以上のconcrete Cameraで明確な共通API / ownership modelが確認できた場合。
- ApplicationがCamera種別をruntime polymorphismで扱う具体的要件が生じた場合。

### Related Documents

- `design_notes.md`

---

## Decision: Camera identityはRegistry、active selectionとkeybinding policyはApplicationが所有する

**Status:** Accepted  
**Date:** 2026-09-07  
**Scope:** Camera / Identity / Application Policy

### Decision

Flight Camera object自身のbehavior / stateと、Registry identityおよびApplication policyを分離する。

```text
Flight Camera owns:
    camera_t
    command state
    keybind-independent Camera behavior

Flight Camera Registry entry owns:
    resource_name
    flight_camera_t

Application owns:
    active Camera selection
    key → Flight Camera command mapping
    framebuffer resize policy
```

`flight_camera_t`自身はresource nameやactive / main Cameraという概念を持たない。

### Context

Camera redesignでは、Camera objectへresource identity、active selection、具体的key layoutまで集約することも可能だった。

しかしこれらはCamera behaviorそのものではなく、Registryでの検索identityまたはApplication側の利用policyである。

### Rationale

- Camera behaviorとresource identityを分離できる。
- Registryをresource nameとCamera ownershipのsingle authorityにできる。
- active CameraというApplication-specific conceptをRegistryへ持ち込まずに済む。
- WASD等の具体的key layoutをEngineへhard-codeせずに済む。
- 将来keybindingをexternal configurationへ移してもCamera behaviorを変更せずに済む。

### Rejected Alternatives

- `flight_camera_t`自身がresource nameを所有する。
- Flight Camera Registryがactive / main Cameraを管理する。
- Flight Camera内部へWASD等の具体的key bindingをhard-codeする。
- Applicationがinternal `camera_t`を直接操作する。

### Consequences

- Flight Camera Registryがresource nameとFlight Cameraのownershipを持つ。
- ApplicationがRegistryから得るFlight Cameraはborrowとして扱う。
- active Camera IDはApplication stateとして管理する。
- key eventからFlight Camera commandへのmappingはApplication側で行う。
- keybinding external config化はCamera内部architectureを変更せず追加できる。

### Revisit Conditions

- active Camera selection自体をEngine subsystemとして共有する必要が生じた場合。
- Camera resource identityとは別のscene entity identityを導入する場合。
- input mappingを独立したEngine input subsystemへ移す場合。

### Related Documents

- `design_notes.md`

---

## Decision: TopologyごとのRender ResourceをRenderer resource ownership rootとする

**Status:** Accepted  
**Date:** 2026-09-12  
**Scope:** Renderer / Resource Ownership

### Decision

以下のRender Resourceを、mesh topologyごとの長寿命なownership rootとする。

- `line_mesh_render_resource`
- `lit_mesh_render_resource`
- `point_mesh_render_resource`
- `ui_mesh_render_resource`

各Render Resourceは、対応するShaderとGeometry Registryを所有する。

`ui_mesh_render_resource`は、加えてTexture Registryを所有する。

### Context

従来はApplication側からShader、Geometry Registry、Pipeline等が個別に見えており、lifetime、destroy order、resource operationの責務が分散していた。

### Rationale

- resource lifetimeをmesh topology単位で閉じる。
- ShaderとGeometry Registryの対応関係をownershipとして固定する。
- Application側がRenderer内部resourceを個別管理する必要をなくす。
- import / release / draw等のresource operationを同じownership rootへ集約する。
- 将来のRenderer Frontend導入時にも、下位resource ownershipを安定した単位として維持できる。

### Rejected Alternatives

- ShaderとRegistryをApplication側で個別所有する。
- Shader、Registry、Pipelineを一つの巨大なRenderer objectへ直接集約する。
- topology間でRender Resourceを相互参照させる。

### Consequences

- topologyごとに独立したRender Resourceが存在する。
- sibling Render Resource間に直接依存を持たせない。
- resource operationは原則として対応するRender Resourceを入口とする。

### Revisit Conditions

- mesh / material / render packet等の上位resource modelが完成し、ownership rootを再定義する明確な必要が生じた場合。
- topology単位のownershipが実際のlifetimeと一致しなくなった場合。

### Related Documents

- `validation_policy.md`
- `glce_coding_style.md`
- `design_notes.md`

---

## Decision: Backend ContextはRender Resourceがborrowする

**Status:** Accepted  
**Date:** 2026-09-12  
**Scope:** Renderer / Backend Lifetime

### Decision

Renderer architectureでは、各Render Resourceと`renderer_backend_context`の関係をsystem-lifetime borrowとする。

Backend Contextは、各Render Resourceより長寿命な上位orchestratorが所有する。

Rendererの終了順序は以下とする。

```text
Render Resources deinitialize
        ↓
renderer_backend_deinitialize
```

borrowに関する一般contractは`glce_coding_style.md`を正本とする。

### Context

Render Resource内部のShader、Texture、Geometry operationはRenderer Backendを必要とするが、Backend Contextそのもののlifetimeは個々のRender Resourceより広い。

Renderer固有の設計として、Backend Contextを各operationの一時引数にするか、各Render Resourceへ所有させるか、長寿命dependencyとして関連付けるかを決める必要があった。

### Rationale

- Backend Contextをsystem-level resourceとして一元所有できる。
- 各operationごとにBackend Contextを引数として再指定する必要をなくせる。
- Render ResourceのAPIをresource ownership単位で完結させやすい。
- Renderer全体のlifetime topologyを単純に保てる。

### Rejected Alternatives

- 各Render ResourceがBackend Contextを所有する。
  - Backend Contextのsystem-level lifetimeと一致せず、所有が分散する。
- 各resource operationごとにBackend Contextを呼び出し側から渡す。
  - construction時に確立したRenderer dependencyを各operationで再指定する構造になる。
- Backend Contextをglobal singletonとして暗黙参照する。
  - dependencyがAPIとobject relationから見えなくなる。

### Consequences

- Rendererのlifetime orchestratorは、Backend ContextをRender Resourcesより長く保持する。
- Render Resourceの通常operationはBackend Contextの再指定を要求しない。
- Renderer Backendのlifetime変更は、Render Resource群との関係を含めて扱う。

### Revisit Conditions

- Renderer Backendを別processへ分離し、Contextの意味そのものが変わった場合。
- Backend accessがhandle / IPC endpoint等へ置き換わった場合。

### Related Documents

- `glce_coding_style.md`
- `design_notes.md`

---

## Decision: Resource Pipelineはstateless orchestrationとして維持する

**Status:** Accepted  
**Date:** 2026-09-12  
**Scope:** Renderer / Resource Pipeline

### Decision

Resource Pipelineはstatelessなoperation layerとして維持する。

PipelineはShader、Registry、Backend Context、Render Resourceを所有しない。

Render Resourceが所有するShader / Registry等をPipelineへ渡して処理させる。

Pipeline APIへRender Resourceそのものを渡す設計は採用しない。

### Context

Resource Pipelineは、load / create / GPU upload / registry register等を一連のoperationとして組み立てる役割を持つ。

ownershipまでPipelineへ持たせると、長寿命resource topologyと短命operation orchestrationが混在する。

### Rationale

- ownershipとoperationを分離する。
- Pipelineを状態なしの再利用可能な処理単位として維持する。
- Pipelineが上位ownership objectへ依存する逆転を避ける。
- Render Resourceがownership、Pipelineがprocedureという役割を明確にする。

### Rejected Alternatives

- PipelineがShader / Registryを保持する。
- PipelineがRender Resourceを引数として受け取り、その内部へアクセスする。
- Render ResourceへPipelineの全処理を直接埋め込む。

### Consequences

- Pipeline APIには処理に必要なresourceだけを明示的に渡す。
- Pipelineは長寿命stateを持たない。

### Revisit Conditions

- Pipeline自体に独立した長寿命stateが必要となった場合。
- resource import architectureが大きく変わり、現在のoperation boundaryが不自然になった場合。

### Related Documents

- `design_notes.md`

---

## Decision: 異なるGeometry topology間の変換は独立conversion moduleへ置く

**Status:** Accepted  
**Date:** 2026-09-12  
**Scope:** Geometry / Conversion

### Decision

異なるGeometry topology間、またはGeometryから独立primitiveへの変換処理は、独立したstateless conversion moduleへ置く。

現在のLit Mesh GeometryからAABBを生成する処理は`geometry_conversion`が担当する。

Lit Render ResourceからLine Render Resourceへの直接依存は作らない。

Lit Render Resourceは自身のGeometry IDを解決し、borrowしたCPU Geometryをconversion functionへ渡す。

### Context

Lit MeshからAABBを生成し、それをLine Meshとして描画する用途が発生した。

これをRender Resource間の直接依存で実現すると、sibling topology間の結合が発生する。

### Rationale

- topology間の直接依存を避ける。
- conversionそのものをresource ownershipから分離する。
- statelessな変換処理として単独テスト・再利用しやすくする。
- 「LitからAABBを得る」と「AABBをLine Geometryとしてimportする」を別責務にする。

### Rejected Alternatives

- `lit_mesh_render_resource`から`line_mesh_render_resource`を直接呼ぶ。
- Line Mesh固有moduleへLit Mesh変換処理を置く。
- Application側でLit Geometry内部表現を直接参照して変換する。

### Consequences

- topology間compositionは上位layerが行う。
- conversion moduleはownershipを持たない。
- source topologyのID解決は、そのIDを所有するRegistry / Render Resource側で行う。

### Revisit Conditions

- Geometry conversion群が大規模化し、より明確なconversion subsystemが必要になった場合。
- mesh-level変換やasset processing pipelineとして責務が再定義された場合。

### Related Documents

- `design_notes.md`
- `glce_coding_style.md`

---

## Decision: Application↔Engineの当面の論理boundaryをapplication.h/.cとする

**Status:** Accepted  
**Date:** 2026-09-12  
**Scope:** Application / Engine Architecture

### Decision

当面、`application.h/.c`をApplicationとEngineの正式な論理boundaryとする。

Application内部のprivate moduleは、このboundaryより内側の実装詳細として扱う。

新規または責務が未成熟な機能は、まずApplication内部へ実装してよい。

責務、contract、lifetime、API boundaryが成熟した後にEngineへ抽出する。

Application内部に一時的にEngine-likeなfacade / service / orchestration moduleが存在することを許容する。

これは最終layeringではなく、意図的なtransitional architectureとする。

### Context

新しい機能を最初からEngineへ配置すると、責務やcontractが固まる前にEngine APIとして固定される危険がある。

一方で、Application内部で試行し、成熟後にEngineへ降ろす方が責務境界を実装から観察しやすい。

### Rationale

- 未成熟な設計をEngine public architectureへ早期固定しない。
- Applicationを機能の育成場所として利用できる。
- 外部との正式boundaryは`application.h/.c`へ固定できる。
- 成熟した責務だけをEngineへ移す段階的なarchitecture evolutionを可能にする。

### Rejected Alternatives

- Application private moduleも含め、最初から厳密にApplication / Engineを分離する。
- 新規機能をすべて最初からEngine moduleとして設計する。
- Application内部moduleをそのまま最終architectureとして固定する。

### Consequences

- Application配下に一時的なEngine-like moduleが存在し得る。
- それ自体を即座にlayering violationとは扱わない。
- moduleが成熟した時点でEngineへ移すか、Application固有責務として残すかを再評価する。
- source reviewでは「現在の配置がtransitionalかfinalか」を区別する。

### Revisit Conditions

- Engine API boundaryが十分成熟し、新規機能を直接Engineへ設計できる段階になった場合。
- Application内部にEngine-like moduleが恒常的に蓄積し、育成場所としての役割を超えた場合。

### Related Documents

- `project_policy.md`
- `design_notes.md`
- `glce_coding_style.md`

---

## Decision: Application Frame Stateをframe-local coordination stateとする

**Status:** Accepted  
**Date:** 2026-09-13  
**Scope:** Application / Frame Coordination

### Decision

`application_frame_state_t`を、1 frame中に複数のApplication-side moduleが更新・参照するmutable coordination stateとする。

`app_state_t`がstorage / lifetime ownerとなる。

Frame Stateには、frameを跨いで保持するcurrent stateと、各frame開始時にresetするtransient stateを共存させてよい。

現時点では以下のように扱う。

```text
persistent / current state
    framebuffer_width
    framebuffer_height

transient / frame-local state
    window_resized
    projection_dirty
    view_dirty
```

各moduleは、自身が導出を担当するstateだけをFrame Stateへ反映する。

Frame Stateを受け取る`update` APIは、現在その値を変更していない場合でも、concept上producerになり得るmoduleではmutable accessを許容する。

### Context

Event、Camera、Renderer等のApplication-side module間で、raw eventそのものを再解釈して状態変化を推測すると、同じ意味の導出責務が複数moduleへ分散しやすい。

例えばRendererが`KEY_W`を見てCamera movementを推測するのではなく、CameraがEventを解釈した結果として`view_dirty`をFrame Stateへraiseし、Rendererはその結果を利用する方が責務を分離できる。

### Rationale

- raw eventとsubsystem semanticを分離する。
- subsystemが導出した結果をdownstream moduleへ明示的に伝播できる。
- module間のhidden dependencyを減らす。
- persistent stateとframe-local change signalを一つのframe coordination contractで扱える。
- 将来Renderer等がFrame Stateへ自身の結果を書き込む余地を維持する。

### Rejected Alternatives

- 各moduleがraw Event Viewを個別に解釈して同じ意味を重複導出する。
- Renderer等を常にFrame Stateのread-only consumerとして固定する。
- framebuffer size等のcurrent stateまで毎frame zero-clearする。

### Consequences

- `application_frame_state_begin_frame()`はpersistent fieldを保持し、transient fieldだけをresetする。
- producerは自身が担当するstateをFrame Stateへ書き込む。
- 同一frame内に複数producerが存在するboolean signalは、必要に応じてraise-only / OR semanticsを採用する。

### Revisit Conditions

- Frame Stateが単なるcoordination stateを超え、独立したEngine subsystemとして扱う必要が生じた場合。
- process isolation導入により、in-process mutable state sharing自体をmessage / snapshotへ置き換える場合。

### Related Documents

- `design_notes.md`

---

## Decision: Platform SystemはOS / Window System境界のSource Systemとする

**Status:** Accepted  
**Date:** 2026-09-13  
**Scope:** Platform / System Boundary

### Decision

Platform Systemを、単なるpure OS syscall wrapperではなく、**OS / Window Systemとの境界でexternal factsを収集し、backend-independentなPlatform-level factへnormalizeして公開するSource System**と定義する。

Platformは以下を担当する。

- OS / Window Systemからexternal factsを取得する。
- GLFW等のbackend固有表現をPlatform-level factへnormalizeする。
- consumerが取得できる形でPlatform-level factのstorage / publicationを管理する。
- Platform-owned backing storageをread-only viewとして上位へ公開する。
- Window creation / destruction、framebuffer size取得等、OS / Window System境界に属する操作を提供する。
- 必要に応じて`ring_queue`等の汎用`Containers` layerを内部実装として利用してよい。

Platformは以下を担当しない。

- Platform factをCamera command等のsubsystem-specific semanticへ解釈する。
- projection / view dirty等のEngine semanticを決定する。
- Renderer update policyを決定する。
- Application終了policyを決定する。
- Engine Event System固有のstorage / merge / ordering policyを所有する。

Platform backend内部でcallback等を利用すること自体は許容する。ただしcallbackはbackend implementation detailであり、Platform public contractとして上位moduleへcallback sinkを要求しない。

具体的なGLFW event collection方式は、独立Decision「GLFW Platformのevent collectionは同期pollingとsnapshot差分方式とする」を正本とする。

### Context

従来、Platformを「最下層のOS wrapper」と捉える考えと、「Window / input eventまで扱うSystem」と捉える考えが混在していた。

この曖昧さにより、Platformが`Containers` layerへ依存してよいか、event buffering / storageをどこが所有するか、PlatformとEvent Systemの境界をどこへ置くかが不明確になっていた。

実際のPlatformはWindow lifecycle、framebuffer、keyboard、mouse等のOS / Window System由来情報を扱うため、純粋なsyscall wrapperとして扱うより、**host environmentとの境界を閉じるSource System**として定義する方が実態と一致する。

### Rationale

- OS / Window System固有の差異をPlatform境界内へ閉じ込められる。
- Platformがexternal factの収集・normalization・publicationまで所有することで、上位moduleへbackend-specific callback contractを漏らさずに済む。
- `Containers`は上位domain semanticではなく汎用基盤であるため、Platformから内部利用してもlayeringを崩さない。
- 「Platformを薄く保つ」を、依存数や内部処理量を極端に減らすことではなく、**上位domain semantic / policyを持ち込まないこと**として定義できる。
- 将来別Platform backendを追加する際も、各backendは同じPlatform-level fact contractへnormalizeすればよい。
- Event Systemとの責務境界を「Platform factの生成」対「Engine eventへの統合」に分離できる。
- Platform内部のstorage mechanismを将来変更しても、上位のEvent System contractへ波及させずに済む。

### Rejected Alternatives

- Platformをpure syscall wrapperとして扱い、Window / input event collectionを別moduleへ完全分離する。
- PlatformからApplication / Event System callbackを直接呼び続ける。
- Event System所有の`ring_queue_t*`をPlatformへ渡して直接pushさせる。
- PlatformがCamera / Renderer / Application policyまで解釈する。
- Platformが汎用`Containers` layerへ依存すること自体を禁止する。
- 特定backendのcollection implementationをPlatform全体のpublic contractとして固定する。

### Consequences

- Platformは一定のstateとevent backing storageを所有し得る。
- Platform内部のcollection / buffering / storage implementationは上位へ公開しない。
- 上位consumerはPlatform-owned dataをread-only viewとして取得する。
- Platform Event ViewはPlatform Systemのpublic fact boundaryとなる。
- Platformの「薄さ」は、上位domain semanticを持たず、host environment boundaryへ責務を限定することで維持する。
- Event SystemはPlatformのbackend detailsやcontainer implementationを知らず、Platform-level factだけをconsumeする。

### Revisit Conditions

- Window / input handlingをPlatformから独立したSource Systemへ分離する明確な必要が生じた場合。
- Platform backendがOS / Window System以外のdomain semanticを恒常的に抱え始めた場合。
- process isolation等によりPlatform Event Viewがin-process viewではなくserialized boundaryへ置き換わる場合。

### Related Documents

- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: 外部event ingressはSource Event ViewからEngine Event Systemへ集約する

**Status:** Accepted  
**Date:** 2026-09-13  
**Scope:** Platform / Event System / External Event Ingress

### Decision

GLCEへ外部から入るeventは、source systemごとに収集・normalizeし、read-onlyなSource Event Viewとして公開する。

最初のsourceとしてPlatform Systemは、OS / Window Systemから取得した情報をbackend-independentなPlatform-level factへnormalizeし、`platform_event_view_t`として公開する。

Engine Event SystemはSource Event Viewをconsumeし、Engine共通のevent spaceへnormalize / mergeして`engine_event_view_t`として公開する。

各subsystemは`engine_event_view_t`を自身のsemanticへ解釈し、必要に応じて自身のstateまたはFrame Stateを更新する。

基本flowは以下とする。

```text
OS / Window System
        ↓
Platform System
        ↓
const platform_event_view_t
        \
         \
future Network / IPC / Filesystem / Device source systems
        ↓
const source_event_view_t
         \
          ↓
    Engine Event System
          ↓
 const engine_event_view_t
          ↓
 Application / Engine Subsystems
          ↓
 subsystem state / Frame State
```

Platform Systemそのものの責務境界は、独立Decision「Platform SystemはOS / Window System境界のSource Systemとする」を正本とする。

### Context

従来はPlatformがApplication Eventのcallbackを呼び、Application Event側のsingleton stateへeventをpushする構造だった。

この方式では、PlatformとEvent Systemの間にcallback sink contractが必要となり、stateful化には`void* context`等のinstance recovery mechanismも必要になる。

一方、Source System側でexternal factsの収集とpublicationまで完結させ、read-only viewを公開すれば、public callback contractを不要にしつつSource Systemのstorage implementationを上位へ漏らさずに済む。

また将来Network、IPC、filesystem watcher、external device等が追加された場合も、Platformと同じSource System patternでEngine Event Systemへeventを供給できる。

現時点ではPlatformが唯一のSource Systemであるため、Platform event spaceとEngine event spaceはほぼ同一である。しかし、将来sourceが追加された場合、Engine event spaceは複数sourceを統合する上位のevent spaceとなる。

### Rationale

- Platform backend差をPlatform境界内で閉じる。
- public callback / hidden singleton / callback context依存を除去できる。
- Source Systemごとにcollection / buffering / overflow / coalescing policyを独立して持てる。
- Engine Event Systemを外部event ingressのnormalization / multiplexing boundaryとして固定できる。
- 現在Platform ViewとEngine Viewがほぼ同じでも、将来source追加時に責務境界を崩さず拡張できる。
- subsystem-specific semantic interpretationをEvent Systemへ持ち込まず、各subsystemに残せる。
- 将来のNetwork / IPC / process-isolated subsystemとの接続点を統一できる。
- Source SystemとEvent Systemを別ownership domainとすることで、source固有のstorage lifetimeをEngine-wide event lifetimeへ漏らさずに済む。

### Rejected Alternatives

- Platform public APIからApplication / Event System callbackを直接呼ぶ方式を維持する。
- callbackへEvent System instanceを戻すため`void* context`をpublic Platform contractへ追加する。
- Event System所有のstorageをSource Systemへ渡し、Source Systemから直接書き込ませる。
- PlatformがCamera / Renderer / Application semanticまで解釈する。
- Engine Event Systemが`KEY_W → MOVE_FORWARD`等のsubsystem-specific meaningまで決定する。
- Platformが唯一のsourceである現在だけを基準に、Platform Event ViewをそのままEngine全体のevent contractとして固定する。

### Consequences

- Platform Event Viewのbacking storageはPlatformが所有し、Event Systemはread-only viewとしてconsumeする。
- Engine Event Viewのbacking storageはEngine Event Systemが所有し、Application / Engine subsystemはread-only viewとしてconsumeする。
- View objectのaddressがproducer lifetime中安定していても、公開内容は次回のsuccessful updateによって更新され得る。
- consumerがeventを長期保存する必要がある場合は、自身のownership domainへcopyする。
- window close requestのdelivery guaranteeは、独立Decision「Window close requestは通常eventより強いdelivery guaranteeを持たせる」を正本とする。
- Platform Event型とEngine Event型はsemantic差が必要になった時点で分離する。単なる1:1 copyしか存在しない段階では、不要なpayload typeの二重化を強制しない。
- 現在は`window_event_t` / `keyboard_event_t` / `mouse_event_t`を共通payload typeとして利用する一方、Source ViewとEngine Viewのownership / abstraction boundaryは分離する。
- Application Event moduleは撤去し、ApplicationはEngine Event Viewを直接consumeする。

### Revisit Conditions

- External event sourceがすべて単一sourceへ恒久的に固定され、Engine Event Systemが純粋なpass-through layerであることが将来要件として確定した場合。
- process isolation導入によりSource Event Viewがin-process viewではなくserialized message boundaryへ置き換わる場合。
- event volume / latency requirementによりsnapshot / view方式が性能上不適切になった場合。

### Related Documents

- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: GLFW Platformのevent collectionは同期pollingとsnapshot差分方式とする

**Status:** Accepted  
**Date:** 2026-09-15  
**Scope:** Platform / GLFW / Event Collection

### Decision

GLFW Platform concreteでは、public callback transportではなく、`glfwPollEvents()`による同期pollingと`current / prev snapshot`差分によってPlatform eventを生成する。

通常frameの処理順序は概念的に以下とする。

```text
glfwPollEvents()
        ↓
collect current snapshot
        ↓
compare prev / current
        ↓
count events
        ↓
capacity preflight
        ↓
reset current event counts
        ↓
generate events
        ↓
commit prev = current
        ↓
refresh Platform Event View
        ↓
publish
```

window / keyboard / mouse eventはPlatform Backendが所有するfixed-capacity typed storageへ生成する。

event生成前に必要event数を算出し、capacityを超える場合は`LIMIT_EXCEEDED`を返す。

capacity failure時にはevent storage countや公開Viewとのrelationを壊さない。

### Context

GLFWはcallback APIを提供するため、Platform callbackからApplication / Event Systemへeventを直接通知する構造も可能である。

しかしその方式では、Platform public contractにcallback sinkやcontext recovery mechanismが入り込み、Source System自身のstorage ownershipが曖昧になる。

GLCEのApplication updateはframe単位で同期的に進むため、現在必要なのは「callbackが発生した瞬間に上位へpushすること」より、「そのframeで観測されたexternal state changeを安定したsnapshotとして上位へ公開すること」である。

### Rationale

- public callback contractを不要にできる。
- callback context / singleton recoveryを不要にできる。
- Platformが自身のevent storage ownershipを閉じられる。
- frame単位でexternal state transitionを扱いやすい。
- event生成前に必要event数を計数でき、capacity failureをtransactionalに扱える。
- Platform内部のcollection mechanismをEvent Systemへ漏らさずに済む。
- `prev / current`の比較という単純なmodelでWindow / keyboard / mouseのstate changeを統一的に扱える。

### Rejected Alternatives

- GLFW callbackからApplicationへ直接通知する。
- GLFW callbackからEngine Event Systemへ直接通知する。
- Platform public APIへcallback functionと`void* context`を導入する。
- Event System-owned queueをPlatformへ渡してcallbackから直接pushする。
- GLFW callbackをそのままEngine event transport contractとして採用する。

### Consequences

- Platform eventはcallback発生時刻そのものではなく、frame update時に観測されたstate transitionとして表現される。
- `prev`と`current`の間でpress → releaseが完結したtransitionは観測できない可能性がある。
- 現時点ではこのtradeoffを許容する。
- GLFWのsticky input等を利用して短時間入力の捕捉性を補助できる。
- 将来event ordering精度やhigh-frequency input requirementが高まった場合は、別のcollection mechanismを検討できる。
- snapshot差分方式はGLFW concreteのDecisionであり、Platform System全体のpublic contractではない。

### Revisit Conditions

- input sampling精度がframe snapshot方式では不足する場合。
- event timestamp / strict orderingがEngine contractとして必要になった場合。
- high-frequency device input等、状態snapshotでは情報を失うsourceをPlatformが扱うことになった場合。
- GLFW以外のbackendで異なるcollection mechanismの方が適切な場合。

### Related Documents

- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: Window close requestは通常eventより強いdelivery guaranteeを持たせる

**Status:** Accepted  
**Date:** 2026-09-15  
**Scope:** Platform / Window Event / Shutdown Signaling

### Decision

OS / Window Systemからwindow close requestを観測した場合、Platformは通常event生成よりclose deliveryを優先する。

close frameでは概念的に以下の処理を行う。

```text
close observed
    ↓
skip ordinary event generation
    ↓
clear ordinary event counts
    ↓
window_close_requested = true
    ↓
refresh Platform Event View
    ↓
publish SUCCESS
```

window close requestは`platform_result_t`ではなく、`platform_event_view_t.window_close_requested`というexternal factとして公開する。

Platform自身はApplication loopの終了を決定しない。

### Context

window closeはkeyboard / mouse等の通常eventと同じexternal factの一種ではあるが、Applicationが終了policyを判断するための重要なsignalでもある。

通常event生成と同じcapacity制約やerror pathにclose requestを載せると、keyboard event overflow等の無関係なrecoverable failureによってclose deliveryまで失われる可能性がある。

また、closeを`PLATFORM_WINDOW_CLOSE`のようなoperation resultとして表現すると、

```text
Platform operation success / failure
```

と、

```text
OSからclose requestを観測した
```

という異なる意味が混在する。

### Rationale

- Application termination判断に必要なclose factを、通常eventのcapacity failureから独立して届けられる。
- Platform operation resultとexternal close factを分離できる。
- Platformは「closeが要求された」というfactだけを提供し、終了policyはApplicationに残せる。
- close時に前frameのordinary event countをclearすることで、close frameで過去eventを再公開しない。
- close pathを失敗しない単純な処理へ限定することで、delivery guaranteeを明確化できる。

### Rejected Alternatives

- `PLATFORM_WINDOW_CLOSE`等のresult codeとしてcloseを通知する。
- close requestを通常window event storageへ入れ、他eventと同じcapacity制約を適用する。
- close requestを観測したPlatform自身がApplication終了を決定する。
- close frameでも通常event生成を続け、その途中errorをclose deliveryより優先する。

### Consequences

- close requestを観測したframeでは通常window / keyboard / mouse eventを新規生成しない。
- ordinary event countはclearされ、`window_close_requested = true`だけがそのframeのPlatform factとして公開される。
- close frameのPlatform updateは、close Viewを公開できる限り`SUCCESS`を返す。
- Event Systemはclose factをEngine Event Viewへ伝搬する。
- Applicationが`engine_event_view_t.window_close_requested`を解釈して終了policyへ反映する。
- close delivery guaranteeは「Applicationを必ず即時終了させる」という意味ではなく、「close factを通常event failureで失わない」という意味である。

### Revisit Conditions

- window close以外にも同等のnon-droppable / sticky signalが必要になった場合。
- Application shutdown protocolが複数stage化し、close factより強いshutdown event modelが必要になった場合。
- process isolationによりclose requestをserialized control messageとして扱う必要が生じた場合。

### Related Documents

- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: Build Configurationは手書きの`build_config.h`を唯一のcompile-time contractとする

**Status:** Accepted  
**Date:** 2026-09-17  
**Scope:** Build System / Compile-time Configuration

### Decision

GLCE binary全体へ適用されるBuild Configurationは、`config/build_config.h`へcompile-time macroとして直接記述する。

`build_config.h`をBuild Configurationの唯一のsource of truthとし、外部configuration file、generator、runtime configuration objectは設けない。

現時点でBuild Configurationが所有する項目は以下とする。

- Platform Backend
- Graphics API
- Memory Policy
- Memory Pool Size

Platform Backend、Graphics API、Memory Policyは、選択された値に対応するmacroを定義する。

Memory Pool Sizeはcompile-time numeric macroとして定義する。

例:

```c
#define GLCE_BUILD_PLATFORM_GLFW
#define GLCE_BUILD_GRAPHICS_API_GL33
#define GLCE_BUILD_MEMORY_POLICY_DESKTOP
#define GLCE_BUILD_MEMORY_POOL_SIZE (256ULL * 1024ULL * 1024ULL)
```

Build Configurationの各項目はoverrideを前提としない。

command line、environment variable、Makefile option等から同一設定を上書きする仕組みは設けない。

一時的に変更する可能性がある値、用途ごとに異なる値、runtimeで変更する値はBuild Configurationへ追加しない。

`build_config.h`には以下を明記する。

- 各configuration項目の意味
- 現在設定可能な値の一覧
- 各値を選択した場合の効果
- configuration上のconstraint

必要なconfigurationが未定義、複数選択、または不正な値である場合は、`#error`等を用いてcompile-timeにbuildを失敗させる。

Platform BackendおよびGraphics APIはruntime selectionを行わない。

従来Applicationが保持していた`app_build_config_t`、`selected_platform`、`selected_graphics_api`は廃止し、Platform SystemおよびRenderer Backend自身がBuild Configurationを参照してcompile-timeにconcrete implementationを決定する。

Memory PolicyについてもMemory System実装時に同じBuild Configurationを参照する。

`GLCE_BUILD_MEMORY_POLICY_DESKTOP` / `GLCE_BUILD_MEMORY_POLICY_EMBEDDED`はGLCE全体のbuild profileではなく、Memory Systemのbacking storage policyを表す。

### Context

従来はApplication内の`app_build_config_t`へPlatform BackendとGraphics APIを格納し、`application_create()`でGLFWおよびOpenGL 3.3をhard-codeしていた。

この構造では、本来binary全体で固定されるbuild-time policyがApplication runtime stateとして表現されていた。

Memory System再構築の設計では、Desktop環境とEmbedded環境でbacking storage取得方式を変更し、Memory Pool Sizeもbuild-timeに固定する必要が生じた。

当初は以下のような構造を検討した。

```text
build_config.conf
        ↓
Build Config Generator
        ↓
generated build_config.h / .c
        ↓
GLCE
```

さらに、generatorがMakefile向け設定とC向け設定の両方を生成する案、generatorをhost toolとしてbuildする案、Desktop / Embedded profileから複数policyをresolveする案、個別configurationをoverrideする案も検討した。

しかし、現時点で必要なconfigurationは少数であり、いずれもbinary全体で固定される強いbuild contractである。

generatorを導入すると以下の追加要素が必要になる。

- generator自身のsource / binary管理
- generatorをbuildするtiming
- configuration sourceとgenerated outputの同期管理
- host toolとtarget binaryのcompiler分離
- cross compile時のhost-side generator実行
- configuration file formatの定義
- generated fileのlifecycle管理

これらは現在のconfiguration項目を表現するためには過剰であると判断した。

また、profileやcommand lineから個別overrideを許可すると、`build_config.h`だけを見ても最終的なbinary contractを一意に判断できなくなる。

GLCEではBuild Configurationをdefault設定ではなく、そのbinary全体が無条件に信頼してよい固定contractとして扱う方針とした。

### Rationale

- Build Configurationのsource of truthを1ファイルへ限定できる。
- C preprocessorだけでcompile-time selectionとvalidationを完結できる。
- configuration fileとgenerated headerという二重表現を持たずに済む。
- generator toolchain、host tool build、generated artifact管理が不要になる。
- cross compile時にも追加のhost-side executableを必要としない。
- Git上でBuild Configurationの変更を直接確認できる。
- `build_config.h`を見れば、そのbinaryのPlatform、Graphics API、Memory Policy、Memory Pool Sizeを一意に判断できる。
- runtime stateへbuild policyを持ち込まずに済む。
- Applicationからbackend selection responsibilityを除去できる。
- configuration項目が少ない現状では、専用toolより直接的で理解しやすい。

### Rejected Alternatives

- runtimeの`build_config_t`を保持する。
  - compile-timeで固定されるpolicyをruntime stateとして扱う必要がない。
  - Application等のruntime orchestratorがbuild policyを所有することになる。
- `build_config.conf`等の専用configuration fileを設け、generatorからheaderを生成する。
  - source formatとC representationが二重化する。
  - generatorのbuild / execution / dependency managementが必要になる。
  - cross compileではgeneratorをhost executableとして別途buildする必要がある。
  - 現在のconfiguration数に対して複雑性が大きい。
- generatorから`.mk`と`.h/.c`を生成し、MakeとCの両方へconfigurationを配布する。
  - Build Configuration frameworkとしては拡張性が高いが、現状では過剰である。
- Desktop / Embedded等のprofileから複数policyのdefault値をresolveする。
  - profileとconcrete policyの関係が間接的になる。
  - Build Configurationを見ただけでbinary contractを判断しにくくなる。
- profile、command line、environment variable等から個別configuration overrideを許可する。
  - 同じ`build_config.h`でも外部条件によって最終設定が変化し、Build Configurationを信頼できなくなる。
  - overrideが必要になる可能性のある値はBuild Configurationへ含めない方針とする。

### Consequences

`config/build_config.h`はGLCE全体から参照されるcompile-time build contractとなる。

Platform SystemはBuild ConfigurationからPlatform Backendを決定する。

Renderer BackendはBuild ConfigurationからGraphics APIを決定する。

Memory SystemはBuild ConfigurationからMemory PolicyおよびMemory Pool Sizeを決定する。

Applicationはこれらのselectionを行わない。

そのため、以下のruntime selection情報は保持しない。

```text
app_build_config_t
selected_platform
selected_graphics_api
```

Build Configurationへ新しい項目を追加する場合は、以下を満たすか確認する。

```text
binary全体で一意に固定される
overrideを必要としない
runtimeで変更しない
build contractとして全moduleが信頼してよい
```

この条件を満たさない値は、各module / system固有のconfigurationとして扱う。

### Revisit Conditions

- Build Configuration項目が大幅に増え、手書きheaderの管理性が明確に悪化した場合。
- Platform / Graphics / Memory以外にも多数のbuild-time policyを組み合わせる必要が生じた場合。
- 複数target向けconfigurationを大量に維持する必要が生じた場合。
- build system自身がconfiguration内容を解析してsource selection等を行う必要が生じた場合。
- 手書きheaderよりgeneratorを導入した方が全体の複雑性を実際に低減できる状態になった場合。

その場合はBuild Configuration generatorの導入を再検討する。

### Related Documents

- `glce_coding_style.md`
- `design_notes.md`
- `glce_near_term_todo.md`
- `README.md`

---

## Decision: Free List AllocatorはMemory System配下のnon-opaque moduleとする

**Status:** Superseded  
**Date:** 2026-09-21  
**Scope:** Memory System / Free List Allocator / Module Boundary  
**Superseded by:** 2026-09-22「メモリ管理基盤を役割別Allocatorで構成し、engine/memory配下へ集約する」

### Decision

`free_list_allocator`はMemory Systemが所有する下位allocator moduleとし、Applicationレイヤーのmemory allocation / freeはMemory Systemを経由する。

`free_list_allocator_t`はopaque typeとせず、Memory System側で実体storageを保持できるcomplete typeとする。

このDecisionはFree List Allocator固有のbootstrapping requirementを記録するものであり、Engine-private headerの一般的な配置規則は`glce_coding_style.md`を正本とする。

Free List Allocator固有のbacking memory pool、alignment、block layout等の利用contractは、`free_list_allocator.h`のModule Boundary Contractを正本とする。

### Context

Memory System再構築では、CPU側のdynamic allocationをMemory Systemへ集約し、その内部allocatorとしてFree List Allocatorを使用する。

Free List Allocator自身をheap allocationで生成すると、そのheap allocationを提供するMemory Systemの起動前にFree List Allocatorが必要になるため、初期化順序が循環する。

一方、`free_list_allocator_t`をopaque typeにすると、caller側では型サイズが分からず、Memory System自身のallocation serviceが利用可能になる前に実体storageを保持しにくい。

また、ApplicationがFree List Allocatorを直接利用できる構造にすると、Memory Systemを経由しないallocation pathが生まれ、system-level memory management responsibilityが分散する。

### Rationale

- Memory System起動前にFree List Allocatorの実体を確保できる。
- Free List Allocator自身の生成にMemory Systemまたは別heap allocatorを要求せずに済む。
- Memory System初期化とallocator生成の循環依存を避けられる。
- Application側のmemory operation入口をMemory Systemへ一元化できる。
- Free List AllocatorのrepresentationをEngine内部へ留めたまま、bootstrappingに必要なcomplete typeを利用できる。

### Rejected Alternatives

- `free_list_allocator_t`をopaque typeとし、`free_list_allocator_create()`でheapから生成する。
  - Memory System起動前にallocator生成用heapが必要となり、初期化順序が循環する。
- `free_list_allocator_t`をopaque typeのままにし、callerが十分大きなraw storageを渡す。
  - type size / alignment contractを別APIや定数として公開する必要が生じ、現在の用途に対して複雑になる。
- Free List AllocatorをApplicationレイヤーへ直接公開する。
  - Memory Systemを迂回するallocation pathが生まれ、memory management responsibilityが分散する。
- Free List Allocator自体をglobal singletonとして直接利用する。
  - allocator implementationとsystem-level memory APIの責務が混在し、上位Memory Systemを置く意味が弱くなる。

### Consequences

- Memory SystemがFree List Allocatorのlifetimeを管理し、system-level memory management authorityとなる。
- ApplicationレイヤーはMemory System APIをmemory operationの入口として使用する。
- Free List Allocator固有のrepresentation変更は、Memory Systemとの内部boundaryで吸収する。

### Revisit Conditions

- Memory Systemより前に利用可能な専用bootstrap allocatorを導入し、Free List Allocator自身を動的生成できるようになった場合。
- opaque typeを維持したままcaller-side storageを安全かつ単純に提供できる共通mechanismを導入した場合。
- Memory SystemとFree List Allocatorの責務境界そのものを再設計する場合。
- Applicationレイヤーへ低レベルallocatorを直接公開する明確な必要が生じた場合。

### Related Documents

- `free_list_allocator.h`
- `glce_coding_style.md`
- `design_notes.md`
- `glce_near_term_todo.md`

## Decision: メモリ管理基盤を役割別Allocatorで構成し、engine/memory配下へ集約する

**Status:** Accepted
**Date:** 2026-09-22
**Scope:** Memory Management / Allocator Architecture / Module Ownership / Directory Structure

### Decision

GLCEのメモリ管理基盤は、単一の`Memory System`によって全てのmemory management responsibilityを表現する構成を採用しない。

代わりに、用途とlifetimeが異なる上位Allocatorを、それぞれ独立したmoduleとして構成する。

現時点で想定する上位Allocatorは以下である。

```text
General Allocator
    process-wideな汎用動的allocation / freeを担当する。

Subsystem Allocator
    ApplicationおよびEngine subsystem等のsystem lifetime allocationを担当する。

Frame Allocator
    frame単位で破棄可能なtemporary allocationを担当する。
```

上位Allocatorは、それぞれの用途に適したlow-level allocatorを内部mechanismとして使用する。

```text
General Allocator
    ↓
Free List Allocator

Subsystem Allocator
    ↓
Linear Allocator

Frame Allocator
    ↓
Linear Allocator
```

Free List AllocatorおよびLinear Allocatorのように、allocation algorithmそのものを提供するmoduleは`engine/memory/low_level_allocators/`配下へ配置する。

上位Allocatorは`engine/memory/`配下に、それぞれ独立したmoduleとして配置する。

```text
engine/memory/
├── general_allocator/
├── subsystem_allocator/
├── frame_allocator/             // 導入時
└── low_level_allocators/
    ├── free_list_allocator/
    └── linear_allocator/
```

Memory関連moduleはPlatform SystemやEvent Systemのようなruntime subsystemではなく、Engine全体から利用される基盤的なmemory domainであるため、`engine/systems/`配下には配置しない。

Allocatorのlifetimeは、そのAllocatorを実際に使用するownerのlifetimeに従わせる。

想定ownershipは以下とする。

```text
process / entry
└── General Allocator

Application
└── Subsystem Allocator

Renderer Frontend
└── Frame Allocator
```

General Allocatorはprocess-wideな汎用allocation authorityとして扱う。

Subsystem AllocatorおよびFrame Allocatorはglobal singletonとはせず、それぞれを必要とするownerが保持する。

本Decision策定後、Subsystem Allocatorは実装済みとなった。Frame Allocatorは引き続き将来導入予定のarchitectureであり、本Decisionはその即時実装を要求しない。

### Context

従来の設計では、CPU-side memory managementを`Memory System`という単一のsystemとして扱い、その内部allocatorとしてFree List Allocatorを使用する方針としていた。

しかし、System lifetime用AllocatorやFrame lifetime用Allocatorまで含めて検討すると、memory managementには単一のlifetimeや単一のownerが存在しないことが明確になった。

例えば、

```text
General Allocator
    process lifetime

Subsystem Allocator
    Application lifetime

Frame Allocator
    Renderer / frame processing lifetime
```

のように、それぞれのAllocatorは異なるownerとlifetimeを持つ。

これらを一つの`Memory System` objectへ集約すると、Memory Systemは実際には複数Allocatorを束ねるcontainerまたはservice locatorとなり、それぞれのAllocatorの自然なownershipが不明瞭になる。

そのため、`Memory System`という上位objectを設けるのではなく、それぞれのmemory lifetimeとallocation semanticsを表すAllocatorを独立したmoduleとして扱う方針へ変更した。

### Memory moduleをsystems/配下から移動する理由

従来のMemory Systemは`engine/systems/`配下に配置していた。

しかし、Platform SystemやEvent System等は、Engine内部で一定のruntime responsibilityを担当するsubsystemである。

一方、memory allocationはそれらのsystem自身からも利用される、より基盤側のmechanismである。

概念的には、

```text
Application / Engine Systems
        ↓
Memory Management
        ↓
Allocation Mechanism
```

というdependencyになる。

そのためMemory関連moduleを`engine/systems/`配下へ配置すると、memory managementと、それを利用するruntime system群が同じ階層に並び、dependency directionをdirectory structureから読み取りにくくなる。

Memory関連moduleは独立した基盤domainとして、

```text
engine/memory/
```

配下へ集約する。

これにより、

```text
Application / Engine subsystem
        ↓
High-level Allocator
        ↓
Low-level Allocator
```

というdependency directionを明確にする。

### General Allocator

従来`Memory System`として設計していた機能のうち、process-wideな汎用動的allocation / free、backing memory pool管理、memory tag別accountingをGeneral Allocatorの責務とする。

General AllocatorはFree List Allocatorを内部allocation mechanismとして使用する。

Build Configurationで指定する、

```text
GLCE_BUILD_MEMORY_POLICY_DESKTOP
GLCE_BUILD_MEMORY_POLICY_EMBEDDED
GLCE_BUILD_MEMORY_POOL_SIZE
```

はGeneral Allocatorのbacking storage構成に使用する。

ただし、これらはGeneral Allocator固有のAPI policyではなくGLCE全体のMemory Policyを表すため、`MEMORY_POLICY`という名称を維持する。

### Subsystem AllocatorとGeneral Allocatorのstorage dependency

Subsystem Allocatorのallocator objectおよびbacking memory poolはGeneral Allocatorから確保する。

概念的なdependencyは以下となる。

```text
General Allocator
        ↑
        │ object / backing storage
        │
Subsystem Allocator
        │
        └──> Linear Allocator
              allocation mechanism
```

Subsystem AllocatorはGeneral Allocatorから取得したallocator objectとbacking memory poolを所有する。

Linear AllocatorはSubsystem Allocatorが所有するbacking memory poolを管理するlow-level allocation mechanismであり、そのstorage自体は所有しない。

#### General Allocator dependencyを外す案

Subsystem Allocator設計時には、General Allocatorへのdependencyを持たせず、Subsystem Allocator用memory domainを独立させる案を検討した。

独立storageを使用すれば、概念的には以下の構成にできる。

```text
General memory domain
    General Allocator
        ↓
    Free List Allocator

Subsystem memory domain
    Subsystem Allocator
        ↓
    Linear Allocator
        ↓
    dedicated backing pool
```

この構造ではGeneral AllocatorとSubsystem Allocatorが同一backing storageを共有しないため、一方のmemory domainで発生した問題が他方へ直接影響する範囲を狭められる。

また、Subsystem lifetime用memory capacityをGeneral Allocatorのcapacityから独立して設定できるため、process-wide dynamic memoryとsubsystem lifetime memoryを物理的にも別resourceとして管理できる。Embedded環境ではSubsystem Allocator専用のstatic storageを与える構成も可能になる。

したがって、fault isolation、resource isolation、memory domain separationだけを比較すれば、General Allocatorからstorageを取得しない構造の方が強い。

一方、General Allocatorからstorageを取得する構造では、次のlifecycle dependencyが生じる。

```text
General Allocator create
        ↓
Subsystem Allocator create
        ↓
Subsystem Allocator lifetime
        ↓
Subsystem Allocator destroy
        ↓
General Allocator destroy
```

Subsystem Allocatorの生成から破棄までGeneral Allocatorが利用可能である必要があり、Subsystem Allocatorのstorage capacityもGeneral Allocatorのmemory poolを消費する。

このため、General Allocator dependencyそのものにはmemory architecture上のfault isolationやresource isolationを強める利点はない。

#### External storageをcallerから渡す案

General Allocator dependencyを解消する方法として、Subsystem Allocatorへallocator object storageまたはbacking memory poolを外部から渡す方式も検討した。

```text
Application
    ↓ storage
Subsystem Allocator
    ↓
Linear Allocator
```

この方式ではSubsystem Allocator自身はGeneral Allocatorを知らずに済む一方、「Subsystem Allocator用storageをどこから取得するか」というpolicyがcaller側へ移動する。

Applicationは、storageの取得元、backing pool size、storage lifetime、release order、Desktop / Embeddedごとのstorage供給方法等を認識する必要がある。

ApplicationがそのstorageをGeneral Allocatorから取得するのであれば、General Allocatorへの実質的なdependencyが上位へ移動するだけである。

Subsystem Allocator専用static pool等を用意する場合は、専用のstorage policy、Build Configuration、およびlifetime管理を新しく設計する必要がある。

これはmemory domain separationとしては強い構造になるが、現時点のSubsystem Allocatorを成立させるためには必要ない責務まで追加することになる。

#### Storage provider abstractionを導入する案

Subsystem AllocatorがGeneral Allocatorそのものへ依存せず、抽象化されたstorage providerまたはallocator interfaceからstorageを取得する方式も候補となる。

しかし現時点ではstorage providerを差し替える具体的なuse caseが存在せず、

```text
Subsystem Allocator
    ↓
generic storage provider interface
    ↓
General Allocator
```

というindirectionを追加しても、実際にはGeneral Allocatorしか使用しない。

将来の差し替え可能性だけを目的としてこのabstractionを導入する方針は採用しない。

#### Final Decision

現時点では、Subsystem Allocatorのallocator objectおよびbacking memory poolをGeneral Allocatorから取得する。

この判断は、General Allocator dependencyの方がmemory isolation上優れているためではない。

独立memory domainの方がfault isolationおよびresource isolationの観点では強いが、それを実現するためにはexternal storage contract、dedicated storage policy、Build Configuration、caller-side lifetime management、またはstorage provider abstraction等を追加する必要がある。

現在のSubsystem Allocatorではそこまでの分離を要求する具体的なuse caseがなく、General Allocatorを既存のprocess-wide storage providerとして使用する方が、追加architectureを持ち込まずに現在のlifetime modelを成立させられる。

したがって現在は、

```text
General Allocator
    storage authority

Subsystem Allocator
    subsystem-lifetime ownership / logical accounting

Linear Allocator
    linear allocation mechanism
```

という責務分離を採用する。

General AllocatorはSubsystem Allocatorのallocation semanticsやlogical accountingを所有しない。

Subsystem AllocatorもGeneral Allocatorのallocation mechanismを再定義せず、General Allocatorをallocator objectおよびbacking storageの供給元として利用する。

将来、memory domain間のfault isolation、dedicated memory capacity、Embedded向けphysical separation等が具体的なrequirementとなった場合は、Subsystem Allocatorのstorage sourceをGeneral Allocatorから分離することを再検討する。

### Low-level Allocator

Free List AllocatorおよびLinear Allocatorは、memory lifetimeや用途などの上位semanticを持たないallocation mechanismとして扱う。

low-level allocatorは以下のような情報を自身の責務として持たない。

```text
memory tag
Application / Renderer等のowner semantic
system lifetime / frame lifetimeという用途分類
process-wide policy
```

これらのsemanticは上位Allocatorまたはそのownerが管理する。

Free List Allocatorは任意順序のallocate / freeを行うためのmechanismを提供する。

Linear Allocatorはlinear allocation mechanismを提供する。

どのlifetimeのためにそれらを使用するかはlow-level allocator自身では決定しない。

### Memory Tag

`memory_tag`はlow-level allocatorのallocation mechanism semanticには含めず、そのtag分類を必要とする上位Allocatorのpolicy / accounting情報として扱う。

General AllocatorはGeneral Allocator固有のmemory tag domainを持ち、Subsystem AllocatorはSubsystem Allocator固有のmemory tag domainを持つ。両者のtag型や分類をMemory domain全体の共通conceptとして統合することは要求しない。

Free List AllocatorおよびLinear Allocatorはmemory tagを認識しない。

Free List Allocatorはallocation size、block size、block state、block linkage等、自身のallocation mechanismを成立させるために必要なmetadataのみを管理する。

General Allocatorはallocation時に自身のmemory tagを受け取り、tag別使用量を更新する。

free時にもcallerからmemory tagを受け取るcontractとし、Free List Allocatorへmemory tagを保存するためだけのmetadataは追加しない。

allocate時とfree時に同一tagを指定することはGeneral Allocator APIのcontractとする。

Subsystem Allocatorもcallerが要求したlogical allocation sizeを自身のmemory tag別にaccountingするが、そのtag semanticをLinear Allocatorへ流入させない。

#### memory_tag配置とdependencyの問題

従来はFree List Allocatorの各allocation metadataにmemory tagを保持していた。

しかし、Memory SystemをGeneral Allocatorとlow-level allocatorへ分離したことで、memory tagをどのmoduleへ所属させるべきかを改めて検討する必要が生じた。

memory tagが表しているのは、

```text
SYSTEM
STRING
RING_QUEUE
RENDERER
FILE_IO
CAMERA
TEXTURE
GEOMETRY
...
```

のような、allocation mechanismそのものではなく、「何の用途でmemoryを使用しているか」をGeneral Allocatorが集計・reportするためのsemanticである。

したがって、このGeneral Allocator用memory tagの自然なownerはGeneral Allocatorである。

ここでmemory tag型をGeneral Allocator側へ配置したまま、Free List Allocatorがmemory tagをblock metadataとして保持すると、dependencyは以下になる。

```text
General Allocator
    ↓
Free List Allocator
    ↓
General Allocator側のmemory tag型
```

General Allocatorはallocation mechanismとしてFree List Allocatorへ依存する一方、Free List AllocatorもGeneral Allocator側の型へ依存する。

結果として、

```text
General Allocator
        ↓
Free List Allocator
        ↓
General Allocator
```

という循環dependencyが生じる。

これは今回採用した、

```text
High-level Allocator
        ↓
Low-level Allocator
```

という一方向dependencyと矛盾する。

low-level allocatorが上位allocator固有のpolicy typeを参照する構造も、責務の方向として適切ではない。

#### memory_tagを共通moduleへ分離する案

循環dependencyを避ける方法として、`memory_tag`をGeneral Allocatorから切り離し、

```text
engine/memory/memory_tag.h
```

等の独立した共通moduleへ配置する案も検討した。

この場合、

```text
General Allocator ───┐
                     ├──> Memory Tag
Free List Allocator ─┘
```

となるため、直接的な循環dependencyは発生しない。

しかし、この構成では当時検討していたGeneral Allocatorのaccounting policyとして必要なsemanticを、Memory domain全体の共通conceptへ昇格させることになる。

Free List Allocatorにとって、

```text
MEMORY_TAG_TEXTURE
MEMORY_TAG_CAMERA
MEMORY_TAG_RENDERER
```

等の分類は、first-fit search、split、allocate、free、coalesce等のallocation algorithmを実行するためには必要ない。

Linear Allocatorや将来追加される他のlow-level allocatorについても同様である。

つまり、memory tagを共通module化することは循環dependency自体は解消するものの、General Allocator用のtag policyを下位memory architecture全体へ広げることになる。

そのため、循環dependencyを共通module化によって回避するのではなく、そもそもFree List Allocatorからmemory tag dependencyを除去する方針を採用した。

#### Free List Allocatorが保持すべきmetadata

Free List Allocatorがallocation mechanismを成立させるために必要なのは、例えば以下である。

```text
block_size
allocation_size
block_state
prev / next
```

これらはblockのallocation、free、split、coalesce、first-fit search等に直接使用される。

一方memory tagは、これらの処理には使用しない。

したがってmemory tagはFree List Allocatorのmechanism invariantには含まれず、block metadataとして保持する必然性がない。

General Allocatorのtag別accountingは、

```text
allocate:

caller
  │ size + tag
  ▼
General Allocator
  │ size
  ▼
Free List Allocator
```

および、

```text
free:

caller
  │ ptr + tag
  ▼
General Allocator
  │
  ├── Free List Allocatorからallocation sizeを取得
  │
  └── tag別accountingを減算
```

という構造で成立する。

このため、tag別accountingを実現するためだけにFree List Allocatorへmemory tagを保存する必要はない。

#### Memory Tagに関するDecision

memory tagは、それを使用するhigh-level Allocator自身のpolicy / accounting semanticとして、そのAllocator側に配置する。

```text
General Allocator
    ├── General固有memory tag
    ├── tag別accounting
    │
    └──> Free List Allocator
            └── memory tagを認識しない

Subsystem Allocator
    ├── Subsystem固有memory tag
    ├── tag別logical accounting
    │
    └──> Linear Allocator
            └── memory tagを認識しない
```

これにより、

```text
High-level Allocator
        ↓
Low-level Allocator
```

という一方向dependencyを維持する。

上位Allocator固有のaccounting semanticをlow-level allocatorへ流入させず、Free List AllocatorとLinear Allocatorをallocation mechanismとして独立させる。

### Ownership

Allocatorは、そのmemory lifetimeを管理する自然なownerが保持する。

General Allocatorはprocess-wideなresourceであるため、process lifetimeに属する。

Subsystem AllocatorはApplicationおよびApplication配下のsubsystem lifetimeを管理するため、Applicationが所有する。

Frame Allocatorはframe processing lifetimeを管理するため、将来的にframe lifecycleを担当するRenderer Frontend等が所有する。

この構造により、

```text
owner lifetime
    =
allocator lifetime
    =
allocatorが管理するmemory lifetime
```

をできるだけ一致させる。

### Rationale

Memory managementを用途別Allocatorへ分解することで、それぞれのlifetime、ownership、allocation strategyを独立して決定できる。

General AllocatorはFree List Allocatorを使用して任意順序のallocate / freeを提供する。

Subsystem AllocatorおよびFrame AllocatorはLinear Allocatorを利用し、それぞれのlifetimeに適したallocation modelを構築できる。

上位Allocatorがlow-level allocatorを組み合わせる構造にすることで、low-level allocatorはallocation algorithmそのものへ責務を限定できる。

また、Allocatorを実際のownerが保持することで、memory resourceのlifetimeとsoftware objectのlifetimeを一致させやすくなる。

`engine/memory`を独立domainとすることで、runtime systemとmemory founda

---

## Decision: Runtime StatusとValidation Reportを分離し、diagnostic policyをApplicationに配置する

**Status:** Accepted  
**Date:** 2026-09-26  
**Scope:** Diagnostics / Validation / Application / Event System

### Decision

GLCEのruntime diagnosticsは、Runtime Status ReportとValidation Reportを異なるresponsibilityとして扱う。

```text
Runtime Status Report
    現在のruntime stateを観測する

Validation Report
    現在のstateがcontract上validであるかを診断する
```

Runtime Status Reportはmemory usage、resource usage、subsystem state等のruntime informationを提示するためのものであり、validation resultを表すものではない。

Validation Reportは各moduleのvalidatorを明示的に実行し、対象stateのintegrityを診断する。

このため、runtime statusを表すstatus structureへvalidation resultを組み込まない。

また、diagnostic operationを起動するためのinput policyはEngine Event SystemではなくApplication側へ配置する。

```text
Platform
    ↓
Engine Event System
    ↓ event fact
Application Diagnostics
    ↓ Application policy
application_frame_state
    ↓
Runtime Status / Validation request
```

Engine Event Systemはkey input等の外部情報をEngine eventとして提供する責務までとし、「どのkeyがRuntime Status ReportまたはValidation Reportを要求するか」という意味を持たない。

Application側の`application_diagnostics` moduleがdiagnostic key bindingを解釈し、frame単位のdiagnostic requestへ変換する。

`application_diagnostics`自身はdiagnostic requestのためのpersistent runtime stateを所有しない。

Engine Event Systemからkey transition eventが提供されるため、Application DiagnosticsはconfigとEngine Event Viewから現在frameのrequestを生成するstateless moduleとして構成する。

reportのorchestrationおよびpresentationもApplication側diagnosticsのresponsibilityとする。Engine側moduleは自身のstate取得APIおよびvalidatorを提供し、system-wide reportのpresentation policyを持たない。

### Context

runtime diagnosticsの導入にあたり、status取得とvalidationを同じreport modelへ統合するか、Engine Event System自身にF1 / F2等のdiagnostic semanticsを持たせるか、Application Diagnosticsをstatefulなsubsystemとして構成するかを検討した。

しかし、Runtime Status ReportとValidation Reportでは目的が異なる。

Runtime Status Reportは「現在どのようなstateにあるか」を観測するものであるのに対し、Validation Reportは「そのstateがcontract上validか」を検査する。

また、public API内部で行われるautomatic validationはoperation safetyやstable state確認のためのものであり、Runtime Status Reportのpresentation semanticとは異なる。

key bindingについても、Engine Event Systemが知るべき事実はkey eventそのものであり、そのkeyへApplication固有のdiagnostic actionを割り当てることはApplication policyである。

さらに、Engine Event Systemがkey transitionをeventとして提供するため、Application Diagnostics側でheld stateやprevious key stateを重複して保持する必要はない。

### Rationale

- runtime stateの観測とcontract integrityの検査を独立して拡張できる。
- status structureへvalidation semanticsを混在させずに済む。
- automatic validationと明示的diagnostic validationを別の責務として扱える。
- Engine Event SystemをApplication固有のF1 / F2 semanticsから独立させられる。
- diagnostic key bindingをApplication policyとして変更できる。
- Application Diagnosticsがpersistent stateを持たず、lifecycle管理を必要としない。
- Engine moduleをdata / validation providerとして保ち、system-wide presentation responsibilityをApplicationへ集約できる。

### Rejected Alternatives

- Runtime Statusへ`is_valid`等のvalidation resultを含める。
  - status取得とintegrity diagnosisの責務が混在する。
  - validation depthや実行時点を単一のstatus fieldでは表現しにくい。

- Engine Event SystemへRuntime Status / Validation Reportのkey bindingを持たせる。
  - Engine側へApplication固有のdiagnostic policyが流入する。

- Application Diagnosticsをstateful objectとして生成し、previous key state等を保持する。
  - Engine Event Systemがすでにkey transitionを提供するため、同じinput stateを重複して管理することになる。

- 各Engine moduleが自身のreport formattingまで担当する。
  - system-wide reportのpresentation policyがEngine各所へ分散する。

### Consequences

- Runtime Status ReportとValidation Reportは独立したdiagnostic pathとして発展させる。
- system-wide diagnostic orchestrationはApplication側に配置する。
- Engine moduleは必要に応じてstatus queryとvalidatorを提供する。
- Application Diagnosticsはdiagnostic request解釈とreport orchestrationを担当する。
- diagnostic key bindingはApplication configurationとして扱う。
- Validation Reportのstructured representationおよびsystem-wide validationの具体的な実行方式は別途設計する。

### Related Documents

- `validation_policy.md`
- `design_notes.md`
- `application/diagnostics/application_diagnostics.h`
- `application/diagnostics/application_diagnostics.c`
- `application/event/application_frame_state.h`

---

## Decision: Linear Allocatorはper-allocation metadataを持たないbump allocatorとする

**Status:** Accepted  
**Date:** 2026-09-27  
**Scope:** Memory / Linear Allocator / Allocation Tracking / Rollback

### Decision

Linear Allocatorは、allocationごとのmetadataを保持しないbump allocatorとして構成する。

allocator自身が保持する主要stateは、概念的に以下だけとする。

```text
memory_pool
capacity
head_ptr
```

allocationは、callerが要求したsizeを`alignof(max_align_t)`へ切り上げ、現在の`head_ptr`から連続領域を返した後、消費した物理size分だけ`head_ptr`を前進させる。

```text
required_size
    ↓ align_up(max_align_t)
block_size
    ↓ capacity check
return current head
    ↓
head += block_size
```

Linear Allocatorは、次のper-allocation informationを保持しない。

```text
allocation_size
block_size metadata
block_state
allocation identity
allocation generation
allocation chain
```

individual freeは提供しない。

過去のallocator positionへ戻す機能は、per-allocation metadataではなくcaller-ownedなrollback pointで実現する。

rollback pointは`memory_pool`から`head_ptr`までのbyte offsetをsnapshotとして保持する。

```text
rollback point
    offset = head_ptr - memory_pool
```

rollback時は、現在の使用range内にありalignment requirementを満たすoffsetであることを確認した上で、`head_ptr`を`memory_pool + offset`へ戻す。

pointer queryについては、allocation identityを判定するAPIではなく、現在使用中のmemory rangeへのmembershipだけを判定する。

```text
linear_allocator_ptr_is_in_use_range(ptr)
    true  : memory_pool <= ptr < head_ptr
    false : 上記以外
```

このqueryは、pointerがallocation payloadの先頭であること、特定allocationに属すること、またはrollback / reset前のhistorical allocation identityと一致することを保証しない。

### Context

Linear Allocatorが提供するpointer validity informationを上位validationで利用することを検討する中で、allocation単位のlivenessやallocation boundaryをより厳密に判定できるよう、per-allocation metadataを追加する案を検討した。

候補として、以下を検討した。

```text
allocation_size
block_size
block_state
terminal / sentinel header
```

また、memory poolの先頭からmetadata chainを辿ることでcanonical validation、`ptr_is_alive`相当のquery、rollback boundary validation等を強化する案も検討した。

しかし、metadataを増やすほどLinear Allocatorは個々のallocationを認識するallocatorへ近づく。

特にblock stateやallocation chainを持たせると、reset / rollback時のstate更新、metadata consistency、canonical traversal等の追加責務が生じる。

これはLinear Allocatorが持つ、

```text
allocation = headを前進させる
reset      = headを先頭へ戻す
rollback   = headを過去位置へ戻す
```

という単純なallocation modelを弱める。

GLCEにはすでに、個々のblockを認識し、allocation / free / split / coalesceを管理するFree List Allocatorが存在する。

Linear Allocatorへ同種のmetadata responsibilityを追加するより、個別allocation trackingが必要なdomainではFree List Allocatorを使用し、Linear Allocatorはbump allocationの単純性を維持する方がallocator間の責務分離が明確であると判断した。

### Rationale

- Linear Allocatorのallocation hot pathを、alignment計算、capacity確認、head更新という単純な処理に維持できる。
- per-allocation metadata write、metadata traversal、state maintenanceを不要にできる。
- resetを`head_ptr = memory_pool`というO(1) operationとして維持できる。
- rollbackもoffset snapshotからのhead復元だけで実現でき、allocation historyをallocator内部へ保持する必要がない。
- Linear Allocatorがindividual allocation identityを管理しないことを明確にできる。
- exact allocation trackingを行うFree List Allocatorとの役割差が明確になる。
- validation強化だけを目的としてallocation mechanism自体を複雑化することを避けられる。
- allocation identityを完全に追跡できない制約を、`ptr_is_in_use_range`というAPI semanticsへ明示できる。

### Rejected Alternatives

- `allocation_size`を各allocationのmetadataとして保持する。
  - requested sizeは取得できるが、次のblock位置を求めるにはlayout再計算が必要となる。
  - Linear Allocator自身のallocation algorithmにはrequested sizeの永続保持を必要としない。

- `block_size`を各allocationのmetadataとして保持する。
  - metadata chain traversalは容易になるが、allocationごとのheader writeとstorage overheadが発生する。
  - 主な利益がvalidation / diagnosticsであり、bump allocation mechanism自体には必要ない。

- `block_state`を保持する。
  - individual freeを持たないLinear Allocatorでは通常stateがほぼALLOCATEDのみとなる。
  - reset / rollback後の旧block stateを正確に維持しようとすると、O(1) operationという特徴を崩す。

- `head_ptr`位置へsentinel metadataを常設する。
  - end boundaryの冗長表現としてcorruption detectionを強化できるが、次sentinelのstorage reservationと追加writeが必要になる。
  - canonical validationのためだけにallocation layoutへmetadataを導入する理由としては過剰と判断した。

- stale pointer / allocation-start / interior pointerをLinear Allocator自身で厳密に判定する。
  - allocation historyまたはper-allocation metadataが必要となり、Linear Allocatorの責務がFree List Allocatorへ近づく。

### Consequences

- Linear Allocatorのcanonical validatorはallocator root stateを中心に検証し、allocation chain traversalは行わない。
- `linear_allocator_ptr_is_in_use_range()`はrange membershipのみを提供する。
- interior pointerやalignment padding内のaddressも、`[memory_pool, head_ptr)`内であればin-use rangeとして扱われる。
- rollback / reset後に同一addressが再利用された場合、historical pointer identityを区別しない。
- rollback pointのgenerationやallocation historyはallocator内部では追跡しない。
- Subsystem Allocatorや将来のFrame Allocatorがより強いlogical accountingやrollback semanticsを必要とする場合、それらは上位Allocator側のresponsibilityとして構築する。
- owned pointer validationでは、すべてのallocator domainへ同一のallocation-level guaranteeを要求せず、そのpointerを管理するallocatorが提供可能なinformationを使用する。

### Revisit Conditions

- Linear Allocatorを利用する実際のuse caseで、range membershipでは不足し、allocation identityの厳密な追跡が必須となった場合。
- measured performance / memory overheadを許容してでもper-allocation diagnosticsを常時保持する明確な要件が生じた場合。
- rollback pointにgeneration validation等が必要となり、caller-owned offset snapshotだけではAPI safety requirementを満たせなくなった場合。
- Linear Allocatorとは別に、allocation trackingを持つarena / region allocatorを導入する方が適切な要件が生じた場合。

### Related Documents

- `design_notes.md`
- `validation_policy.md`
- `glce_near_term_todo.md`

---

## Decision: `engine/memory`をallocation-level validationのtrust rootとする

**Status:** Accepted  
**Date:** 2026-09-28  
**Scope:** Memory / Validation / Trust Boundary / Allocation Authority

### Decision

GLCE内部では、`engine/memory`をallocation-level validationのtrust rootとする。

上位moduleのvalidatorは、対象pointerを管理するMemory moduleが提供するallocation query結果をauthorityとして扱い、そのqueryを成立させているallocator metadataのvalidityを再帰的に検証しない。

`engine/memory`内部では、各low-level allocatorが自身の管理するmemory poolについてallocation stateのauthorityを持つ。

```text
engine/memory
    allocation-level validation trust root

    General Allocator
        ↓
    Free List Allocator
        per-allocation state authority

    Subsystem Allocator
        ↓
    Linear Allocator
        current in-use range authority
```

Free List Allocatorはallocation mechanismとしてper-block metadataを保持するため、current allocationのpayload先頭やallocation size等のallocation-level informationを提供できる。

Linear Allocatorはper-allocation metadataを保持しないため、allocation identityではなく`[memory_pool, head_ptr)`というcurrent in-use rangeをauthorityとして提供する。

上位validatorは、管理Allocatorが提供していないallocation identity、allocation boundary、historical liveness等を独自に仮定しない。

Memory基盤自身のvalidatorは、allocator metadataが完全に無傷であることを別のallocation authorityによって証明しない。既知のbacking memory range、capacity、physical layout、および局所metadata間の整合性から検出可能なcorruptionを検査する。

このtrust rootはGLCE software architecture内部のvalidation停止点であり、OS、libc、linker、virtual memory、MMU、hardware等を含むsystem全体の完全性を保証するものではない。

### Context

owned raw pointerのvalidationを強化する際、allocator metadataそのものがcorruptionしている可能性をどこまで検証するかが問題となった。

allocator metadataを検証するために別のallocatorへ問い合わせ、そのallocator metadataをさらに別のallocatorで検証する構造を要求すると、allocation validityの証明は再帰的に下位へ続き、GLCE内部だけでは停止点を定義できない。

```text
upper object validator
    ↓
allocator metadata validator
    ↓
そのmetadataを保持するmemoryのvalidator
    ↓
さらにそのmetadataのvalidator
    ↓
...
```

どこかでroot assumptionを受け入れなければならない。

GLCEではMemory基盤がallocation、ownership、backing range、allocator-specific metadataを管理しており、上位moduleから見たallocation validityの自然なauthorityでもある。このため、trust boundaryを個別の`free_list_allocator`や`linear_allocator`ではなくMemory domain全体である`engine/memory`に置くこととした。

### Rationale

- allocation validityの再帰的な自己証明を要求せず、validation architectureの停止点を明確にできる。
- 上位moduleがlow-level allocator representationへ依存せず、Memory moduleの公開queryをauthorityとして使用できる。
- Free List AllocatorとLinear Allocatorの異なるallocation semanticsを維持したまま、それぞれが提供可能なvalidation capabilityを利用できる。
- 将来low-level allocator implementationを変更しても、上位moduleから見たtrust boundaryを`engine/memory`に維持できる。
- Memory内部ではknown backing range、capacity、physical layout、local metadata relation等、allocator自身から検証可能なinvariantへ集中できる。

### Rejected Alternatives

- `engine/memory/low_level_allocators`自体をproject-wide trust rootとする。
  - 上位moduleがGeneral / Subsystem Allocatorではなくlow-level implementationを直接意識する構造になり、high-level allocator boundaryを弱める。
  - allocator implementation追加・変更時に上位側のtrust modelまで影響しやすい。

- allocator metadataのvalidityを別のGLCE allocatorで再帰的に証明する。
  - 最終的に別のmetadata authorityが必要となり、無限後退を解消できない。
  - bootstrappingとdependencyを不必要に複雑化する。

- すべてのAllocatorへ同一のper-allocation metadata modelを導入する。
  - Linear Allocator等、本来allocation identityを必要としないmechanismまで複雑化する。
  - allocatorごとのperformance goalとsemantic differenceを失う。

### Consequences

- 上位validatorは、Memory moduleが提供するallocation query結果をallocation-level validityのauthorityとして使用できる。
- 上位validatorはallocator metadataそのものを再帰的に検証しない。
- Free List based allocationとLinear based allocationではvalidation capabilityが異なることを明示的に受け入れる。
- `engine/memory`自身のvalidatorも、root assumptionから導出可能なcorruption detectionを行うのであり、arbitrary corruptionを完全に証明するものではない。
- guard page、sanitizer、MPU等の独立したdiagnostic / protection mechanismは、必要に応じてこのtrust boundaryを補強する別mechanismとして追加できる。

### Revisit Conditions

- Memory metadataをMemory domainとは独立したauthorityで常時検証するmechanismを正式に導入した場合。
- process isolation、MPU / MMU protection、shadow metadata等によりallocation validityのtrust model自体を再設計する場合。
- `engine/memory`より下位に、複数Allocator共通の明確なmemory protection / allocation authority layerを新設する場合。

### Related Documents

- `boundary_model.md`
- `memory_model.md`
- `validation_policy.md`
- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: Texture resource validation responsibilityをBMP Loader / Resource Core / Texture CPU Resourceへ分離する

**Status:** Accepted  
**Date:** 2026-10-02  
**Scope:** Texture / Resource / Validation / Trust Boundary

### Decision

Texture resource pathでは、external BMP representationのtrust確立、GLCE共通texture metadataのsemantic validity、CPU-side resourceのownership / lifetimeを異なるresponsibilityとして分離する。

```text
External BMP file
    ↓ untrusted representation
BMP Loader
    external resource trust boundary
    - BMP header / layout / file rangeを検証
    - pixel representationをnormalize
    ↓ trusted normalized pixels
    + texture_resource_info_t candidate
Resource Core
    - texture_resource_info_tのsemantic owner
    - texture_resource_info_is_valid()でmetadata validityを定義
    ↓ valid texture metadata
Texture CPU Resource
    - trusted pixel storageとvalid metadataを保持
    - pixel storageのownership / lifetimeを管理
```

BMP Loaderはexternal BMP fileをGLCE内部で利用可能なtrusted pixel representationへ昇格させるtrust boundaryとする。BMP固有のformat validation、row layout導出、padding removal、channel order変換、orientation normalization等はLoader responsibilityとする。

`texture_resource_info_t`のwidth、height、channel_count、pixel_data_sizeおよびfield間relationのsemantic ownershipはResource Coreへ置く。これらのvalidityは`texture_resource_info_is_valid()`が一元的に定義する。

Texture CPU Resourceは、upstream trust boundaryで受理済みのpixel dataとvalidな`texture_resource_info_t`を受け取り、そのstorageのownershipとlifetimeを管理する。Texture CPU Resourceはexternal resource trust boundaryとしてBMP semanticやpixel element内容を再認証しない。

BMP Loader内部の`pixel_layout_t`は、validated headerからchecked transformationによって導出されるprivate processing contextとする。独立したpublic lifecycleや任意mutationを持たないため、standalone canonical validatorを設けず、initializer成功後はvalid-by-constructionとして扱う。

### Context

Texture pathのvalidationを整理する中で、BMP Loader、Resource Core、Texture CPU Resourceの各moduleがどこまで同じconditionを検査すべきかが問題となった。

特に、width / height / channel count / pixel data size relationをLoaderとCPU Resourceの双方で個別に定義すると、同一semanticのauthorityが複数moduleへ分散する。また、external BMPをnormalizationした後もCPU Resource側でBMP formatやpixel内容を再検査すると、trust boundaryが不明確になり、validation responsibilityとownership responsibilityが混在する。

一方、BMP Loader内部の`pixel_layout_t`は外部から直接受け取るdomain objectではなく、validated headerから一方向に構築され、その後のprivate processingだけで使用される。この型へ独立validatorを追加すると、checked transformationで成立させた同一relationを後段で重複して再検査する構造になる。

### Rationale

- external resourceのtrust確立位置をBMP Loaderへ限定できる。
- BMP固有format semanticをResource CoreやTexture CPU Resourceへ漏らさずに済む。
- texture metadataのsemantic authorityを`texture_resource_info_is_valid()`へ一元化できる。
- Texture CPU Resourceをpixel storageのownership / lifetime管理へ集中させられる。
- upstreamでtrustedとなったpixel representationをdownstreamで機械的に再認証せずに済む。
- private derived contextはchecked transformationによるvalid-by-constructionを利用でき、validator architectureを不必要に増やさずに済む。
- 将来BMP以外のLoaderやprocedural texture sourceを追加しても、Resource Core以降のmetadata semanticを共有できる。

### Rejected Alternatives

- BMP LoaderとTexture CPU Resourceの双方でwidth / height / channel count / pixel data size relationを個別に検査する。
  - 同一semanticの定義が複数moduleへ分散し、変更時に不整合が発生しやすい。

- Texture CPU Resourceをexternal resource trust boundaryとして扱い、pixel elementやBMP由来semanticを再検証する。
  - Loaderで成立済みのtrustを下流で再認証することになり、ownership responsibilityとformat validation responsibilityが混在する。

- Resource CoreへBMP固有のbit count、compression、row padding、orientation等のconditionを持たせる。
  - GLCE共通texture metadataへsource-format固有semanticが流入する。

- `pixel_layout_t`へstandalone canonical validatorを設け、各private helperで再検証する。
  - private checked transformationで成立させたrelationを重複検証することになり、現在のlifecycleに対してvalidatorの責務が過大になる。

### Consequences

- `texture_resource_info_is_valid()`はtexture metadataのsemantic validityを検査し、pixel storageの存在、allocation validity、ownership relation、pixel element内容は検査しない。
- BMP Loaderはexternal BMPの受理可否とnormalization responsibilityを持つ。
- Texture CPU Resourceのcanonical validityは、validな`resource_info`とowned live pixel storage、およびそのownership relationを中心に構成する。
- Texture CPU Resource内では`resource_info`固有semanticを再定義せず、Resource Coreのvalidatorへ委譲する。
- BMP Loader内部の`pixel_layout_t`はinitializer成功後のinternal contractをprivate helper間で信頼する。
- GPU / device固有のtexture size limit等はResource Coreのmetadata semanticへ含めず、Renderer Backend等のdevice-capability ownerで扱う。

### Revisit Conditions

- Texture CPU Resourceがexternal / untrusted pixel representationを直接受け取るAPIへ変更された場合。
- `texture_resource_info_t`が現在より広いresource semanticやstorage ownershipを持つ型へ変更された場合。
- BMP以外を含む共通Image Decoder / Loader layerを導入し、trust boundaryを上位へ再配置する場合。
- `pixel_layout_t`がmodule外へ公開される、複数moduleで共有される、または独立mutation / lifecycleを持つdomain objectへ昇格した場合。
- pixel storage provenanceやactual allocation sizeをResource contractとして厳密に追跡するownership descriptor等を導入した場合。

### Related Documents

- `boundary_model.md`
- `validation_policy.md`
- `glce_coding_style.md`
- `design_notes.md`
- `glce_near_term_todo.md`

---

## Decision: GLCEのBoundary ModelをExternal Trust / Engine API Trust / Internal Integrityの3分類で正式化する

**Status:** Accepted  
**Date:** 2026-10-03  
**Scope:** Architecture / Boundary / Trust / Authority / Ownership / Validation

### Decision

GLCEでは、project-wideなboundary architectureを次の3種類で整理する。

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

各Boundaryは、少なくとも次の観点から記述する。

```text
Purpose
Boundary Owner
Outside
Inside
Admission / Trust Promotion
Outside Failure
Owner Failure
```

必要に応じて、semantic authority、resource authority、ownership / lifetime、provenance restriction、remaining external state等も併記する。

この3分類はmoduleを排他的に分類するものではない。同一moduleが複数種のBoundaryを持ってよい。

代表例:

```text
Loader
    External Trust Boundary

Platform System
    External Trust Boundary
    Engine API Trust Boundary
    Internal Integrity Boundary

Renderer
    Engine API Trust Boundary
    複数のInternal Integrity Boundary

Memory module
    Engine内部private domain
    caller request / storageとのboundary
    allocator-managed established stateのInternal Integrity Boundary
```

Boundary Modelはvalidation execution policyそのものではない。

canonical validatorをいつ実行するか、shallow / canonicalのどのdepthを使うか、BUILD_MODEごとのautomatic validation、`DATA_CORRUPTED`確定後のfail-stop / cleanup規則、FATAL / DEGRADED等のsystem state transitionは、それぞれ`validation_policy.md`または将来のError Handling architectureの責務とする。

Boundary Modelの正本は`boundary_model.md`とする。

### Context

#### 1. 発端はLoaderにおけるExternal Trust Boundaryだった

今回のBoundary Model整理は、Loaderのvalidation responsibilityを考えるところから始まった。

BMP / STL等のLoaderでは、外部fileのbytesをそのままEngine内部dataとして信用することはできない。

```text
external file bytes
    ↓
parse / validate / normalize
    ↓
trusted internal representation
```

このときLoaderは、単なるfile parserではなく、外部representationをGLCE内部representationとして受理するtrust boundaryを形成している。

この考え方を使うと、malformed fileやunsupported representationは、Engine内部corruptionではなく「boundaryを通過できなかったoutside input」として自然に扱える。

BMP Loaderでは、header / layout / file range / pixel representation等を確認し、正常に受理できたdataだけをnormalized internal representationへ昇格させる。

STL Loaderでも、external representationのparse / numeric validation / topologyとして必要な条件等をLoader側で確認した上で、Engine内部Geometryへ渡す。

この段階では、主に次の1本のBoundaryを意識していた。

```text
External
    ↓ Trust Boundary
Internal
```

#### 2. OS / Platformも同じ構造を持つことに気づいた

Loaderの考え方を他のmoduleへ適用すると、Platform Systemにも同種のBoundaryが存在する。

```text
OS / window system / device
    ↓
Platform System
    ↓
GLCE platform state / event representation
```

ただしPlatformはLoaderとは異なりresident systemであり、persistent stateを持つ。

そのため、次の2つを区別する必要が生じた。

```text
Outside fault
    OS / device / window / external channel側の異常

Owner failure
    Platform-owned established stateのintegrity failure
```

例えば、特定external channelのfailureをPlatformが明確に局所化でき、GLCE内部stateへ影響していないなら、そのchannelをreject / close / disableしてcontainできる可能性がある。

一方、Platform自身が管理しているestablished stateのinvariantが壊れているなら、それはoutside failureではない。

ここからBoundaryには「何をinsideへ入れるか」だけでなく、「outside側の異常とBoundary Owner自身の破損を分ける」という役割があることが明確になった。

#### 3. Application → Engineにも別種のTrust Boundaryが存在する

次に、ApplicationからEngineへ渡されるconfig、descriptor、value、array等を考えた。

Applicationは同一process内に存在していても、Engineが継続的integrityを保証しているdomainではない。

```text
Application-controlled value
    ↓
Engine API
    ↓ validate / normalize / copy / construct
Engine-controlled state
```

このBoundaryはexternal asset fileやOSとは性質が異なる。

ApplicationはGLCEの一部ではあるが、Engineに対してcallerであり、自身のvalueを自由に変更できる。

そのため、Engine API entryに来たvalueを「Engine内部で生成された型だからtrusted」とみなすことはできない。

ここで、External Trust Boundaryとは別に**Engine API Trust Boundary**を明示する必要が生じた。

#### 4. Engine内部にはTrust Promotionとは異なるIntegrity Boundaryがある

さらに、Registry、Allocator、Choco String、Ring Queue、Geometry、Texture Resource等を確認すると、外部dataをinternal representationへ昇格させるBoundaryとは別に、module自身がpersistent stateとinvariantを管理しているBoundaryが存在する。

```text
module-owned established state
    ↓
module API / validator
    ↓
owner-managed invariant
```

ここでは中心となる問いが異なる。

External Trust Boundaryでは、

```text
何をinsideへ受理してよいか
```

が中心である。

Internal Integrity Boundaryでは、

```text
誰がどのstate / ownership relation / invariantを管理しているか
```

が中心となる。

このため、第三の分類として**Internal Integrity Boundary**を採用した。

#### 5. 旧試行版のTrust / Resource Authorityの2軸は捨てず、横断軸へ移した

Boundary Modelの初期試行では、主に次の2軸で整理していた。

```text
Trust Boundary
Resource / Authority Boundary
```

Trust Boundaryはuntrusted representationからtrusted representationへの昇格を表し、Resource / Authority Boundaryはallocation identity、file handle、GPU object等について誰がauthorityを持つかを表す。

この区別自体は有効だった。

しかし、これだけではApplication → EngineとEngine内部state integrityの差を十分に表現できなかった。

そこで最終的には、

```text
External Trust Boundary
Engine API Trust Boundary
Internal Integrity Boundary
```

を主要categoryとし、

```text
trust
semantic authority
resource authority
ownership
lifetime
provenance
failure containment
```

を各Boundaryを分析する横断軸として残した。

つまり旧試行版を破棄したのではなく、**2軸の観察結果を3つの主要Boundary categoryへ再編した**。

### Boundary OwnerとFailureの整理

Boundaryを議論する中で、Boundary Ownerは「全部の異常をcorruptionとして判断するmodule」ではなく、主に次を決めるmoduleとして整理された。

```text
outside representationを受理するか
どのrepresentationへ変換するか
どこでownershipをcommitするか
outside failureをboundary外へcontainできるか
```

Boundary Owner自身のpersistent stateが壊れている場合は、outside input rejectionとは別問題である。

この区別により、例えば次を同じerror categoryへ押し込まずに済む。

```text
malformed BMP
    → Loaderがreject

OS / file stream error
    → Platform / Filesystemがexternal failureとして処理

invalid Application config
    → Engine APIがreject

allocator metadata corruption
    → Memory internal integrity failure
```

### Rationale

- External input rejectionとEngine internal integrity failureを同じ概念として扱わずに済む。
- Applicationを同一process内だからという理由だけでEngine trusted domainへ含めず、Engine API entryでtrust promotionを明示できる。
- Registry、Allocator、Container、Resource等のpersistent internal stateについて、誰がinvariant authorityを持つかを明確にできる。
- Loader / Platform / Filesystem / Memory / Renderer等、性質の異なるmoduleを共通語彙で議論できる。
- `DATA_CORRUPTED`等のresult classificationと、Boundary architectureそのものを分離できる。
- canonical validationのdepth / timingをBoundary categoryから機械的に導出せず、既存のoperation semantics / semantic-owner modelを維持できる。
- trust、authority、ownership、lifetime、provenanceを一つの概念へ潰さず、必要に応じて重ねて分析できる。
- outside failureをcontainできる場所と、inside integrityを失った場所を区別でき、将来のfault isolation / process isolation設計へ接続しやすい。
- AIによる将来のreviewで「このmoduleはなぜここを検査するのか」「このfailureは誰の責任か」という問いを復元しやすくなる。

### Rejected Alternatives

#### Trust BoundaryとResource / Authority Boundaryの2軸だけを正式モデルとする

初期試行では、この2軸だけで十分か検討した。

```text
Trust Boundary
Resource / Authority Boundary
```

しかし、Application → Engineのcaller-controlled inputと、Engine内部のestablished state integrityの差を表すには粒度が粗かった。

また、Trust Boundaryだけでは「外から何を入れるか」と「内部で誰がstateを守るか」が同じ語彙へ寄りすぎる。

そのため、2軸は横断的観点として残し、主要categoryを3分類へ発展させた。

#### Boundaryごとに専用のvalidation depthを固定する

Boundary categoryごとに、例えば、

```text
External Trust Boundary → canonical
Engine API Boundary     → shallow
Internal Integrity      → canonical
```

のような固定規則を設ける案は採用しなかった。

validation depthは、実際にoperationがconsumeするstate、memory safety requirement、finite execution requirement、ownership closure、performance cost等からmodule / API単位で決めるべきだからである。

Boundary ModelがValidation Policyを上書きすると、既に確立しているsemantic-owner modelやAPI-specific validation depthの考え方が弱くなる。

#### Boundary Ownerだけがcanonical validationを実行する

一時、state / Boundary Ownerだけがcanonical validatorを実行し、forwarding / downstream moduleではcanonical validationを行わないという強いルールを検討した。

この案は「責任者を1箇所へ固定できる」という点では魅力があった。

しかし、実際のAPIでは、

- read-only operationでもownership closureをconsumeする場合がある。
- mutation operationではpostcondition diagnosticが有効な場合がある。
- getterでもoutput viewを安全に公開するために必要なdepthが異なる。
- private helperでも自身が直接consumeするsemantic preconditionは必要になり得る。

など、operation semanticsごとの差が大きい。

Ownerだけというルールは、validation responsibilityとvalidation executionを混同し、かえって例外を増やすと判断した。

そのため、Boundary Ownerはtrust / authorityの概念として残し、canonical validationをいつ実行するかは`validation_policy.md`へ委ねる。

#### Boundary Ownerにprovenance-awareなresult reclassificationを持たせる

下位moduleが`INVALID_ARGUMENT`を返した場合に、上位Boundary Ownerがinput provenanceを調べ、Engine内部producer由来なら`DATA_CORRUPTED`へ再分類する案も検討した。

```text
lower API
    ↓ INVALID_ARGUMENT
Boundary Owner
    ↓ provenance check
internal producer由来なら DATA_CORRUPTED
```

この案はroot causeをより正確に表現できる可能性がある。

しかし採用しなかった。

理由:

- value provenanceを保持・追跡する追加mechanismが必要になる。
- caller contextによって同じ下位failureの意味が変わる。
- `result_convert()`が単純なlayer間mappingではなくなる。
- moduleが上位call graphやproducer identityを意識するようになる。
- valueが複数moduleを経由した場合のprovenance ownershipが複雑になる。
- exact root causeが分からない状況でも「内部由来らしい」という推測でpanic-class resultへ昇格する危険がある。

この問題は、後述するresult classification decisionで、consumerは観測可能なfailureだけを分類するという方針へ整理した。

#### Resultとは別にFault Context / Fault Managerを導入する

議論中には、result codeだけではroot causeとsystem stateを表現しきれないとして、次のような別軸を導入する案も検討した。

```text
Result
    operation result

Fault
    integrity / trust / provenance context

Subsystem State
    RUNNING / DEGRADED / FATAL
```

また、`engine/fault`等の専用domainを設け、dynamic allocationを使わないfault recordを保持する案も考えた。

これは将来的なError Handling architectureとしては成立し得る。

しかし今回のBoundary Modelを成立させるために必要なmechanismではなかった。

Boundary整理と同時にFault infrastructureまで導入すると、

- system state transition
- logging
- fault lifetime
- fault propagation
- recovery / shutdown

まで同時に設計する必要が生じ、現在の問題を超えてscopeが拡大する。

したがって今回は採用せず、Error Handling architectureの将来課題として残す。

### Consequences

- `boundary_model.md`をproject-wideなBoundary architectureの正本とする。
- `validation_policy.md`はvalidation execution、depth、result classification、fail-stopの正本として維持する。
- module固有のBoundaryはheaderのModule Boundary Contractおよび必要なsource documentationで具体化する。
- Boundary Ownerであることだけを理由にcanonical validationを毎回実行しない。
- Boundary categoryからFATAL / DEGRADED等のsystem stateを機械的に決定しない。
- External Trust Boundaryでは、outside inputのinvalidityをinside corruptionと区別する。
- Engine API Trust Boundaryでは、Application / Frontend-controlled inputをEngine-owned stateへtrust promotionする位置を明示する。
- Internal Integrity Boundaryでは、module-managed established state / ownership relation / invariantのauthorityを明確にする。
- trust / authority / ownership / provenanceは3種Boundaryとは別の横断軸として使用する。
- `engine/memory`をallocation-level validation trust rootとする既存Decisionは、本Boundary ModelのMemory-specificな具体化として維持する。
- BMP / STL Loader等のexternal resource validationは、External Trust Boundaryの具体例として扱う。

### Revisit Conditions

- Application / Engine boundaryそのものを廃止または大幅再設計した場合。
- Engine Frontendを複数導入し、Engine API Boundaryが現在と異なるtrust topologyを持つ場合。
- process isolationにより現在のin-process module boundaryがIPC / protocol boundaryへ移行した場合。
- capability / permission model等、authorityを型やhandleで明示的に表すarchitectureを導入した場合。
- Fault Manager / Supervisor等を正式導入し、Boundary ownershipとfault containmentを別のformal modelで表現する必要が生じた場合。
- module間でshared mutable stateを大規模に導入し、現在のowner-centric integrity modelでは表現できなくなった場合。

### Related Documents

- `boundary_model.md`
- `validation_policy.md`
- `design_notes.md`
- `project_policy.md`
- `glce_near_term_todo.md`

---

## Decision: caller-controlled input failureとestablished internal state corruptionを区別し、`DATA_CORRUPTED`をvalidator failureから機械的に導出しない

**Status:** Accepted  
**Date:** 2026-10-03  
**Scope:** Validation / Error Classification / Engine API Boundary / Internal Integrity

### Decision

GLCEでは、invalidな値またはstateを検出したとき、`is_valid() == false`という事実だけから`DATA_CORRUPTED`を機械的に返さない。

result classificationの主要な判断軸を、次のように定める。

```text
caller-controlled input
    ↓ validity failure
原則 INVALID_ARGUMENT
または operation semanticsに応じたrecoverable result

established internal state
    ↓ canonical integrity failure
DATA_CORRUPTED candidate
```

External Trust Boundaryでは、trust promotion前のcandidate / external representationのinvalidityは、原則としてboundary固有のrecoverable rejectionとして扱う。

```text
malformed / unsupported external representation
    ↓
UNSUPPORTED_FILE / RUNTIME_ERROR / I/O result 等
```

`DATA_CORRUPTED`は、正規のlifecycleを経て成立済みであり、module / Engineがvalidであることを前提として管理しているstateについて、internal invariant / ownership relation / metadata等のintegrity failureを確認した場合に使用する。

型がopaqueかpublic structかは、本質的なclassification axisとはしない。

重要なのは、API call時点で誰がそのstateをcontrolしているか、そしてそのstateがすでにtrusted / established internal stateとして成立しているかである。

```text
public representation
    ≠ 必ず caller-controlled input

opaque representation
    ≠ それだけで DATA_CORRUPTED
```

例えば`engine/memory`以下はEngine内部private domainであるため、`free_list_allocator_t`や`linear_allocator_t`のstruct definitionが内部headerから見えることは、Application-controlled public valueであることを意味しない。

initialize成功後のallocator stateはestablished internal stateとして扱える。

一方、rollback point、allocation size、memory tag、ID、config等、operationへ持ち込まれるvalueは、それぞれのAPI contractに従ってcaller-controlled inputとして分類できる。

### Context

#### 1. 出発点は「validator failureを`DATA_CORRUPTED`と呼んでよいのか」という疑問だった

Validation Policy横断適用を進める中で、canonical validatorの整備が進み、多くのAPIで次のようなpatternが現れた。

```text
if (!xxx_is_valid(value_or_object)) {
    return DATA_CORRUPTED;
}
```

成立済みinternal objectについては自然に見える。

しかし、callerから渡されたvalueまで同じ分類にすると問題がある。

例えばApplicationがconfig fieldを書き換え、Engine APIへinvalid configを渡した場合、Engine自身のinternal stateはまだ何も壊れていない。

```text
Application
    invalid config
        ↓
Engine API
```

ここでcanonical validatorがfalseだったという理由だけで`DATA_CORRUPTED`を返すと、

```text
caller input error
    ↓
internal corruption signal
```

という意味の反転が起こる。

`DATA_CORRUPTED`をfail-stop / panic-class signalとして扱う設計では、この誤分類は特に重い。

#### 2. 最初は「opaque object / public value」で分類できると考えた

この問題への最初の整理として、次のheuristicを検討した。

```text
opaque object
    → moduleだけが内部を変更できる
    → invalidなら DATA_CORRUPTED と考えやすい

public value
    → callerが自由に変更できる
    → invalidなら INVALID_ARGUMENT と考えやすい
```

この考え方は多くの既存moduleによく当てはまった。

例えば、

- `choco_string_t`はopaque objectで、内部buffer / len / capacityをChoco String moduleが管理する。
- `texture_resource_info_t`はcallerからvalueとして渡される。
- `aabb_3d_t`もcaller-provided valueとして使用される。

そのため一時点では、

```text
opaque → DATA_CORRUPTED
public value → INVALID_ARGUMENT
```

を一般則にできる可能性があると考えた。

#### 3. Memory moduleを検討して、representation visibilityは本質ではないと分かった

その後、`free_list_allocator_t`と`linear_allocator_t`を確認すると、両者はstruct definitionがheader上に存在する。

ただしこれは、Applicationが直接mutable valueとして扱うためではない。

`engine/memory`以下はEngine internal private moduleであり、allocator objectをcaller-side storageへ置く必要がある等のimplementation reasonによってrepresentationが内部headerへ見えているだけである。

initialize成功後のallocator stateは、正規APIを通して成立したmodule-managed stateである。

つまり、

```text
struct definitionが見える
```

ことと、

```text
caller-controlled inputである
```

ことは同義ではない。

ここで分類軸をrepresentationからauthority / lifecycleへ移した。

最終的な軸は、

```text
caller-controlled input
established internal state
```

となった。

#### 4. `INVALID_ARGUMENT`へ寄せると、本当のinternal corruptionを逃すのではないかを検討した

次に問題となったのは逆方向である。

Engine内部module Aのbugによってinvalid valueが生成され、それをmodule Bへ渡した場合を考えた。

```text
Module A
    bug
    ↓ invalid value
Module B
    ↓ validation failure
INVALID_ARGUMENT
```

root causeはEngine内部bugである。

それにもかかわらずBが`INVALID_ARGUMENT`を返すと、本当は`DATA_CORRUPTED`にすべきfailureを逃しているように見える。

ここから、value provenanceを追跡し、Engine内部producer由来ならconsumer側または上位layerで`DATA_CORRUPTED`へ再分類する案を検討した。

しかし最終的に採用しなかった。

module Bから観測できる事実は、

```text
APIへ渡されたinputがrequired validityを満たしていない
```

ということだけである。

Bは、そのvalueが、

- Applicationにより壊されたのか
- module Aのimplementation bugで生成されたのか
- memory corruptionによって変化したのか
- stale / incorrect data flowによって渡されたのか

を一般には判定できない。

その原因を推測してpanic-class resultへ昇格するより、Bは自身のAPI contractとして観測できるfailureを返す方がmodule responsibilityが明確である。

```text
consumerは原因を推測しない
consumerは観測可能なinput failureを分類する
```

さらに、Bがinvalid inputをrejectすれば、B自身のtrusted stateへそのvalueを取り込まずに済む。

つまり、

```text
Bのinternal stateはまだ壊れていない
```

可能性が高い。

一方、Aがinvalid valueを自身のpersistent / established stateへcommitしているなら、A自身のcanonical validation、Commit eligibility、Postcondition validation、Validation Report等がcorruption detection pointになり得る。

```text
Module A
    established stateを構築 / mutate
        ↓
    canonical / postcondition diagnostic
        ↓
    DATA_CORRUPTED
```

Aが単なるtemporary valueを誤生成しただけでestablished stateを壊していない場合は、operation failureとして局所化されてもよい。

この検討により、**internal bugを必ずその最初のconsumerで`DATA_CORRUPTED`へ分類することは要求しない**と決めた。

#### 5. 逆方向の誤分類の方が危険であることを重視した

さらに重要だったのは、`INVALID_ARGUMENT`にすべきfailureを`DATA_CORRUPTED`へ誤分類する方向である。

例えばApplicationのbugでinvalid configが渡された場合、Engine内部stateが健全でも、

```text
Application bug
    ↓ invalid input
Engine
    ↓ DATA_CORRUPTED
fail-stop / panic-class handling
```

へ進む可能性がある。

将来`DATA_CORRUPTED`をFATAL system stateへ結びつける場合、この誤分類によって、本来rejectして継続可能だったApplication input errorだけでsystem全体を停止する可能性がある。

そのため、今回のDecisionでは次の非対称性を明示的に受け入れる。

```text
false negative側
    internal producer bugがconsumerではINVALID_ARGUMENTとして見える可能性
    → 許容する

false positive側
    caller input errorをDATA_CORRUPTEDへ昇格する
    → 強く避ける
```

これは「corruption detectionを弱くする」ことを目的としたものではない。

`DATA_CORRUPTED`へ強いfail-stop semanticsを持たせる以上、そのsignalにはestablished internal stateが壊れているという追加根拠を要求する、という設計判断である。

### Existing Code Audit

今回のルールが既存GLCEへ適用可能かを確認するため、Validation Policy横断適用済みmoduleを中心にsource auditを行った。

結果として、既存codeの多くはすでにこの考え方と整合していた。

#### `texture_resource_info_t` / Texture CPU Resource

`texture_cpu_resource_create()`では、callerから渡された`texture_resource_info_t`がinvalidなら`RESOURCE_INVALID_ARGUMENT`とする。

```text
texture_resource_info_t
    caller-controlled input
        ↓ invalid
RESOURCE_INVALID_ARGUMENT
```

一方、valid inputをcopyして構築した`texture_cpu_resource_t`について、Commit eligibilityのcanonical validationが失敗した場合は`RESOURCE_DATA_CORRUPTED`とする。

```text
valid resource_info
    ↓ copy / construct
texture_cpu_resource_t candidate
    ↓ module-created internal state
canonical failure
    ↓
RESOURCE_DATA_CORRUPTED
```

また、成立済みTexture CPU Resource内部に保持される`resource_info`が後のcanonical / shallow validationでinvalidなら、それはresource-owned established stateのintegrity failureとして扱える。

この構造は今回のDecisionの代表例となった。

#### `choco_string_t`

`choco_string_t`はopaque objectであり、object自身とowned bufferのlifetime / representationをChoco String moduleが管理する。

`choco_string_copy()`の`src_` / `dst_`は、Module Boundary Contract上、Choco String APIが生成しlifetime中にあるestablished objectである。

したがって、DEBUG / TESTのcanonical validationで`src_`または`dst_`がinvalidと判定された場合は`CHOCO_STRING_DATA_CORRUPTED`でよい。

一方、`src_ == NULL` / `dst_ == NULL`はAPI input contract violationとして`INVALID_ARGUMENT`となる。

```text
NULL input
    → INVALID_ARGUMENT

established choco_string state invalid
    → DATA_CORRUPTED
```

さらにcopy後の`dst_` postcondition failureは、validと仮定していたmodule-managed stateをChoco String自身がmutationした後のfailureであり、`DATA_CORRUPTED`の根拠がより強い。

#### `ring_queue_t`

Ring Queueでは、queue自身はopaque established stateであるため、canonical failureを`RING_QUEUE_DATA_CORRUPTED`としている。

一方、`ring_queue_push()` / `pop()`へcallerが渡した`element_size_` / `element_align_`がqueue formatと一致しない場合は、caller-controlled conditionとして`RING_QUEUE_INVALID_ARGUMENT`となる。

```text
queue state invalid
    → DATA_CORRUPTED

caller-provided element format mismatch
    → INVALID_ARGUMENT
```

この区別も今回のDecisionと直接一致する。

#### `aabb_3d_t`

`aabb_3d_t`はvalueとしてAPIへ渡される。

`aabb_3d_vertices_get()`でinvalid AABBを受け取った場合は`AABB_3D_INVALID_ARGUMENT`となる。

これはcaller-controlled valueのinvalidityをinternal corruptionへ昇格しない例である。

#### Memory Allocators

`free_list_allocator_t` / `linear_allocator_t`はrepresentationがEngine内部headerから見えるが、`engine/memory`以下はEngine private domainである。

正規initialize後のallocator stateはestablished internal stateとして扱う。

したがってallocator自身のcanonical invariant破損は`DATA_CORRUPTED` candidateである。

一方、allocation size、pointer、tag、rollback point等のoperation inputはcaller-controlled requestであり、それぞれ`INVALID_ARGUMENT` / `BAD_OPERATION`等のAPI semanticsに従う。

この確認により、representation visibilityをclassification axisとする必要がないことが再確認された。

#### Filesystem / FS Stream

Filesystem系では、

```text
fullpath / mode / read size
    → caller input

filesystem_t / fs_stream_t wrapper state
    → established internal state

FILE*内部のposition / EOF / error
    → external authority state
```

が既に分離されていた。

caller input failure、wrapper corruption、external I/O failureを同じresult categoryへ潰さない現在の構造は、Boundary Modelおよび今回のclassificationと整合する。

#### Geometry

Geometry object自身はopaque established stateとして扱える。

source vertex / AABB等のcaller-controlled valueについては、upstream trust boundaryまたはsemantic-consuming operationでvalidityを確認する。

今回source auditで見つかったdirect vertex import経路のvalidation gapは、現在進行中のValidation Policy横断適用でEngine API Trust Boundary側へvalidationを配置することで解消する方針とした。

Geometry constructor側は、upstreamで成立済みのsource semanticを信頼し、単純copyだけを理由にelement semantic validationを重複しない現在のsemantic-owner modelを維持する。

### Rationale

- `DATA_CORRUPTED`を「validatorがfalseだった」という実装詳細ではなく、stateのtrust / lifecycle文脈に基づいて分類できる。
- Application / Frontend-controlled inputをEngine corruptionと誤認しにくくなる。
- fail-stop / panic-class resultのfalse positiveを減らせる。
- consumer moduleがproducer provenanceやcall graphを知る必要がない。
- result conversionを単純なlayer間mappingとして維持しやすい。
- Engine内部bugをproducer側のCommit eligibility / Postcondition / canonical diagnosticで局所化できる。
- opaque / public structというC implementation detailにarchitecture ruleを依存させずに済む。
- `engine/memory`のようなinternal visible structを自然にestablished internal stateとして扱える。
- existing Validation Policyのsemantic owner、trusted internal contract、stable boundary、Commit eligibilityと自然に接続できる。
- BMP / STL等のExternal Trust Boundaryでは、malformed external dataをEngine corruptionへ誤分類せずに済む。
- `DATA_CORRUPTED`に強いfail-stop semanticsを与えたまま、その適用対象だけを慎重にできる。

### Rejected Alternatives

#### `is_valid() == false`を一律`DATA_CORRUPTED`とする

最も単純な実装規則ではある。

しかしvalidatorは、caller-controlled value、trust promotion前candidate、established internal stateのいずれにも使用できる。

同じvalidator failureでも意味は同じではない。

```text
invalid caller config
invalid external candidate
invalid established object
```

をすべて`DATA_CORRUPTED`へ分類すると、recoverable input failureとinternal integrity failureが混在する。

特にfail-stop / panic-class handlingと組み合わせた場合、Application bugだけでEngine全体を停止する可能性があるため採用しなかった。

#### `opaque object → DATA_CORRUPTED / public struct → INVALID_ARGUMENT`を正式規則とする

議論途中では有力なheuristicだった。

しかし、`free_list_allocator_t` / `linear_allocator_t`のようにEngine内部private typeでありながらstorage理由でstruct definitionが見える型がある。

またopaque typeでも、正規objectではないarbitrary pointerをcallerが渡した場合、opacityだけから「内部stateが壊れた」と断定できるわけではない。

したがってrepresentation visibilityではなく、caller controlとestablished lifecycleを主要軸とした。

#### Engine内部producer由来の`INVALID_ARGUMENT`をconsumer側で`DATA_CORRUPTED`へ変換する

root causeを表現する点では魅力がある。

しかしconsumerには一般にproducer originを判定する責務も情報もない。

この規則を導入すると、API resultが引数の値だけでなくcall provenanceへ依存する。

```text
same invalid value
    Application由来 → INVALID_ARGUMENT
    internal module由来 → DATA_CORRUPTED
```

を実現するにはprovenance trackingが必要であり、module間couplingが増える。

そのため採用しなかった。

#### 上位layerの`result_convert()`でinternal provenanceを見て再分類する

下位moduleを単純に保ちつつ、上位layerだけでroot causeを補正する案も検討した。

しかし、上位layerが全producer / consumer relationを知る必要があり、result conversionがarchitecture-specific reasoningを持つことになる。

また、複数layerを経由した場合に「どの時点のprovenanceを正本とするか」が不明確になる。

通常の`result_convert()`を単純なcategory mappingとして維持する方が理解しやすいため採用しなかった。

#### Boundary Ownerだけが最終corruption classificationを行う

Boundary Ownerへclassification authorityを集中させる案も検討した。

しかし、Boundary Ownerの概念はtrust admission / authority / containmentを説明するためのものであり、すべてのoperation failureのroot cause classifierとすると責務が広すぎる。

各module APIが自身の観測可能なconditionを分類し、established state integrityを所有するmoduleが自身のcanonical failureを`DATA_CORRUPTED`として扱える現在の方が局所的である。

#### Resultとは別にFault Contextを必須化する

```text
result = INVALID_ARGUMENT
fault.origin = INTERNAL
fault.severity = CORRUPTION
```

のように、operation resultとroot-cause metadataを分離する案も理論上は可能である。

これによりconsumer resultを単純に保ちながらinternal bug情報を保持できる。

しかし現時点では、fault contextのlifetime、propagation、storage、logging、system state transition等を追加設計する必要があり、問題規模に対して過剰である。

将来Error Handling architectureが成熟し、実際にroot-cause propagationが必要になった場合の再検討候補とする。

#### `DATA_CORRUPTED`を弱いrecoverable resultへ変更する

誤分類時の影響を減らすため、`DATA_CORRUPTED`自体を通常errorへ弱める案は採用しない。

Established internal stateのintegrity failureを検出した場合に、suspect ownership graphを辿ったcleanupや通常operationを継続しないという現在のfail-stop思想には価値がある。

問題は`DATA_CORRUPTED`が強すぎることではなく、**その強いsignalをcaller-controlled inputへ安易に適用すること**である。

そのためsignal semanticsは維持し、classification criteriaを厳密にした。

### Consequences

- caller-controlled inputのchecked validity failureは原則`INVALID_ARGUMENT`、`BAD_OPERATION`その他のrecoverable resultへ分類する。
- External Trust Boundaryのinvalid candidateは`UNSUPPORTED_FILE`、runtime / I/O result等のboundary固有rejectionへ分類する。
- established internal stateのcanonical integrity failureは`DATA_CORRUPTED` candidateとする。
- `is_valid() == false`だけを理由に`DATA_CORRUPTED`を返さない。
- struct definitionのvisibilityをclassification ruleに使用しない。
- consumerはinput provenanceを推測してresultを再分類しない。
- internal producer bugがconsumer側では`INVALID_ARGUMENT`として表面化する可能性を許容する。
- producerがestablished stateを壊した場合は、producer側canonical validation / Commit eligibility / Postcondition / Validation Report等をcorruption detection pointとして利用する。
- temporary value生成bugがestablished stateを壊さずconsumerにrejectされた場合、recoverable operation failureとして局所化されることを許容する。
- `DATA_CORRUPTED`確定後のfail-stop policyは維持する。
- `DATA_CORRUPTED`が将来FATAL stateへ接続される場合でも、caller input errorから誤ってFATALへ遷移しにくいclassification foundationを得る。
- result propagation / `result_convert()`はprovenance-aware logicを持たず、現在の単純な変換を維持できる。
- Validation Policy横断適用では、このruleをproject-wideな`INVALID_ARGUMENT` / `DATA_CORRUPTED`分類基準として使用する。

### Revisit Conditions

- Resultとは別に正式なFault Context / Error Contextを導入し、root cause provenanceをlosslessに伝播する必要が生じた場合。
- Engine APIへcapability handleやvalidated descriptor等を導入し、caller-controlled / trusted stateの区別が型system上明示されるようになった場合。
- process isolationによりmodule間failure provenanceをprotocol metadataとして保持する必要が生じた場合。
- `DATA_CORRUPTED`のsystem-level semanticsをpanic / fail-stopから大きく変更した場合。
- Application / Engineのtrust relationを再設計し、Application stateの一部をEngineが直接authority管理する構造へ変更した場合。
- production diagnosticsでinternal producer bugのroot cause追跡が実際に不足し、provenance-aware telemetryの導入価値が複雑性を上回ることが確認された場合。

### Related Documents

- `validation_policy.md`
- `boundary_model.md`
- `design_notes.md`
- `project_policy.md`
- `glce_near_term_todo.md`



---

## Decision: `Commit eligibility`を独立phaseとして廃止し、result candidate validationを`Result validation`として分離する

**Status:** Accepted
**Date:** 2026-10-05
**Scope:** Coding Style / Validation / Function Phase Model

### Decision

GLCEのfunction internal phase modelでは、`Commit eligibility`を独立phaseとして使用しない。

関数内部の処理phaseは、必要に応じて以下の語彙を使用する。存在しないphaseを機械的に追加しない。

```text
Preconditions
Prepare
Preflight
Result validation
Commit
Postconditions
Output
Cleanup / rollback
```

各phaseの責務を以下のように整理する。

```text
Preconditions
    operation開始前にcaller / API contractとして成立している必要があるconditionを確認する。

Prepare
    後続処理に必要なtemporary、candidate、derived value、resource等を準備する。
    operationのsemantic effectはまだ成立させない。

Preflight
    operationのsemantic effectを開始する前に、
    resource、capacity、target、relation等からoperationを実行可能か確認する。

Result validation
    Prepare等で構築したresult candidateが、
    APIがsuccess時に保証するoutput contractを満たしているか確認する。

Commit
    existing semantic state、ownership、lifecycle、registration、
    external state等に対するsemantic transitionを成立させる。

Postconditions
    Commit完了後のstable stateがcontractを満たしているか確認する。

Output
    確定済みのresult、value、borrowed view、status、ID等をcallerへ伝達する。
    Output自体はmodule-owned semantic stateのtransitionを意味しない。

Cleanup / rollback
    必要なoperationに限り、failure時のresource解放またはstate restorationを行う。
```

`Commit eligibility`で扱っていた検査は、その目的に応じて以下へ分離する。

```text
operationを実行可能か確認する
    → Preflight

constructed resultがoutput contractを満たすか確認する
    → Result validation

Commit後のstable stateがcontractを満たすか確認する
    → Postconditions
```

### Context

`glce_config_utility_key_value_parse()`のvalidation documentationを整理する中で、result candidateをcallerへOutputする前のvalidationをどのphaseへ分類するかを再検討した。

このAPIでは概念的に次の処理を行う。

```text
Prepare
    temporaryなkey/value viewを構築する
        ↓
validation
    temporary viewがsuccess時のAPI contractを満たしているか確認する
        ↓
Output
    validated viewをcallerのoutput variableへcopyする
```

このvalidationを`Commit eligibility`と呼ぶ案では、API自身がmodule-owned semantic state、ownership、lifecycle等をCommitしていないにもかかわらず、存在しないCommitに対するeligibilityという不自然さが生じた。

一方、このvalidationを`Postconditions`と呼ぶ案では、caller-visibleなOutputを行う前のtemporary candidateを検査しているため、Commit後のstable state validationと同じphase名へまとめると意味が曖昧になる。

この問題を整理した結果、result candidateを構築した後、そのcandidateがsuccess時のoutput contractを満たしているかを確認する処理を`Result validation`として独立して表現する方が、実際のoperation semanticsと一致すると判断した。

この整理により、pureなresult construction operationは例えば次のように表現できる。

```text
Preconditions
    ↓
Prepare
    ↓
Result validation
    ↓
Output
```

一方、既存stateを変更するstateful operationは例えば次のように表現できる。

```text
Preconditions
    ↓
Prepare
    ↓
Preflight
    ↓
Commit
    ↓
Postconditions
    ↓
Output
```

ここで`Result validation`と`Postconditions`は目的が異なる。

```text
Result validation
    callerへ返すresult candidateのvalidityを確認する

Postconditions
    Commitによって成立したstable stateのvalidityを確認する
```

また、`Preflight`はresult validityではなく、semantic effectを開始する前のoperation feasibilityを確認するphaseとして維持する。

### Rationale

- result candidate validationを、存在しないCommitへ結びつけずに表現できる。
- `Preflight`、`Result validation`、`Postconditions`の問いをそれぞれ分離できる。
- pure output operationとstateful operationを同じphase vocabularyで自然に表現できる。
- `Commit`を既存semantic state、ownership、lifecycle等のtransitionへ限定できる。
- `Output`を、確定済みresultをcallerへ伝達する処理として明確に維持できる。
- API success contractを満たすtemporary resultをOutput前に検査する一般的な実装patternを、直接的な名称で表現できる。
- phase名を増やすこと自体を目的とせず、実際に異なる責務だけを区別できる。

### Rejected Alternatives

#### `Commit eligibility`を維持し、result publicationもCommitとして扱う

caller-visibleなoutputへresultをcopyすることまでCommitの意味を広げる案を検討した。

しかし、getterやvalue conversion等のpure OutputまでCommitと解釈すると、`Commit`と`Output`の区別が弱くなる。

GLCEでは、existing semantic state、ownership、lifecycle等のtransitionを`Commit`とし、成立済みresultのcallerへの伝達を`Output`として分離する方がoperation semanticsを直接表現できると判断した。

#### result candidate validationを`Postconditions`とする

APIがsuccess時に保証するconditionを検査しているという意味では、postcondition validationと呼ぶことも可能である。

しかし、temporary candidateをOutput前に検査する処理と、Commit後のstable stateを検査する処理ではvalidation対象とtimingが異なる。

両者を同じphase名へまとめるより、前者を`Result validation`、後者を`Postconditions`として分離する方が明確であると判断した。

#### genericな`Check` phaseを導入する

`Check`はinput validation、resource availability、candidate validity、stable state validation等のいずれにも使えるため、phase名だけでは何を確認しているか分からない。

各validationの目的をphase名から読み取れるようにするため採用しなかった。

#### `Output eligibility`を導入する

callerへOutputしてよいかを確認するphaseとして`Output eligibility`を設ける案も検討した。

しかし、実際に検査している対象はOutput operationそのものではなく、Outputしようとしているresult candidateのvalidityである。

このため、対象を直接表現する`Result validation`を採用した。

### Consequences

- function internal phase modelから`Commit eligibility`を削除する。
- operation feasibilityの確認は`Preflight`で扱う。
- result candidateのsuccess contract validationは`Result validation`で扱う。
- Commit後のstable state validationは`Postconditions`で扱う。
- pure result construction / conversion等では、必要に応じて`Preconditions → Prepare → Result validation → Output`を使用する。
- stateful mutation / ownership transition等では、必要に応じて`Preconditions → Prepare → Preflight → Commit → Postconditions → Output`を使用する。
- すべてのfunctionへすべてのphaseを機械的に追加しない。
- `validation_policy.md`および`glce_coding_style.md`のfunction phase / validation記述は、本Decisionに合わせて更新する。

### Revisit Conditions

- `Result validation`と`Postconditions`の区別が実装上ほとんど機能せず、phase vocabularyを単純化した方が理解しやすいことが確認された場合。
- caller-visible output自体にownership / lifecycle transition等のsemantic Commitを伴うAPI patternが増え、`Output`と`Commit`の関係を再定義する必要が生じた場合。
- transaction modelやerror handling architectureを導入し、function phase全体を別のformal modelで整理する場合。

### Related Documents

- `validation_policy.md`
- `glce_coding_style.md`

## Decision: Untextured / Textured MeshのMaterial構成とTexture参照lifetimeを分離する

**Status:** Accepted
**Date:** 2026-10-09
**Scope:** Renderer / Material / Texture Reference / Resource Ownership

### Decision

GLCEでは、Phong系Lightingを使用する3D Meshの描画方式を、Textureを使用しない`untextured_mesh`と、1種類以上のTexture Mapを使用する`textured_mesh`へ分離する。いずれも独立したRender Resource ownership rootとして扱う。

Materialのデータ表現には次の**3つの非opaque value type**を採用し、共通の万能`material_t`や独立した`material_properties_t`は設けない。

- `untextured_material_t`：`ambient`、`diffuse`、`specular`、`shininess`からなるPhong系の基本反射特性を保持する。Untextured Mesh用のMaterial表現であると同時に、Textured Materialの基本反射特性を表す値としても利用する。
- `texture_map_t`：MaterialからTextureを参照するMap単位の値表現。Textureの実体を所有しない。
- `textured_material_t`：基本反射特性とTexture Map参照を組み合わせる値表現。具体的なfield layoutはTextured Mesh実装時に決定する。

Untextured MeshはShader、Geometry Registry、Untextured Material Registryを所有し、Texture RegistryやTexture参照状態を持たない。

Textured MeshはShader、Geometry Registry、Textured Material Registry、Texture Registryを所有する。Diffuse / Specular / NormalのTexture Mapはそれぞれ独立して使用・未使用を選択できるようにする。少なくとも1種類のTexture Mapが存在する構成をTextured Meshが担当し、3種類とも使用しない構成はUntextured Meshが担当する。Textureを使用する7通りの組み合わせを設計上の対応範囲とするが、実装時に7種類のShaderやMeshを要求するものではない。

Material RegistryはMesh種別ごとに独立して所有し、異なるMesh間の共通Registryは作らない。UI MeshにはPhong系Materialを強制せず、既存の直接的なTexture Registry利用を維持する。

Texture Resourceの実体、Resource Name、および登録IDのauthorityはTexture Registryへ置く。一方、Materialが参照するTextureを解放しないという**参照lifetimeとrelease順の成立責任**はApplicationに持たせる。参照中のTextureを解放する前に、Applicationは当該Textureを参照するMaterialの利用を終了させなければならない。

この契約について、generation付きHandle、Reference Counting、Reverse Reference Tracking、Textureの自動解放抑止は導入しない。古いIDが解放後に再利用された場合、Engineがその参照違反を自動検出することは保証しない。

### Context

従来の`lit_mesh`はGeometryの法線を持つが、固定色で描画しており、Material Propertiesの概念とLightingが未実装であった。はじめは`solid_color_mesh`と、Diffuse Textureを持つ`textured_mesh`の二種類を想定した。

しかしMaterialの表現を検討する過程で、`ambient`、`diffuse`、`specular`、`shininess`は単なるColorではなくPhong系の材料特性であり、Textureの有無とは独立した共通の基本値であることが明確になった。

さらに、Diffuse / Specular / Normal Textureを独立して使用する用途があるため、Diffuse TextureをTextured Meshの必須条件とすると、Normalのみ・Specularのみ等の合理的な構成が除外される。

Texture参照のlifetimeについては、Registryにgeneration等のstale-ID検出機構を追加する案を検討したが、GLCEではApplicationがMaterialとTextureの利用関係を把握でき、かつgenerationを導入しても適切な解放順やlifetimeを自動的に保証できるわけではない。

### Rationale

- Textureを使用しないSTL等のGeometryに、不要なTexture IDやTexture Registry依存を持ち込まない。
- Materialの基本反射特性とMap参照をvalueとして表し、内部heap所有やprivate stateを必要としない型をopaque化しない。
- 各Meshの描画方式とResourceのownershipをVertical Slice内へ閉じ、兄弟Mesh間の依存や共有Registryを避ける。
- Texture Mapの種類を一つに固定せず、Diffuse / Specular / Normalの部分的な利用を表現できる。
- Resourceのphysical ownershipをRegistryに置くことと、参照元のrelease順をApplicationが保証することを区別する。
- 今は必要ない自動参照追跡やgeneration managementのstate / validation / API surfaceを先行導入しない。

### Rejected Alternatives

- 全Meshで共通の`material_t`にTexture IDやTexture Nameを持たせる。
  - Untextured Meshにも不要なfieldと状態組合せが生まれる。
- `material_properties_t`を独立した型として保持し、その周囲に`untextured_material_t`をさらに設ける。
  - この段階では基本反射特性とUntextured Materialが同じ概念であり、二重の型を要求する必要がない。
- `solid_color_mesh` / `lit_mesh`の名前を維持する。
  - Diffuse / Specularの反射色やLightingの存在を十分に表さず、Textureの有無による描画方式の違いが不明確になる。
- Textured MeshでDiffuse Textureを必須にする。
  - Specularのみ、Normalのみ等、実用的なMap構成を表せない。
- Diffuse / Specular / Normalの7通りごとに独立したMesh種別を設ける。
  - 同じTexture使用描画方式を組合せごとに不必要に分断する。
- Material間で共通のTexture Registry / Material Registryを共有する。
  - Render Resourceごとの独立ownership domainを曖昧にする。
- generation付きHandleや参照カウントを必須化する。
  - 現段階ではApplicationによる明示lifetime contractで足り、追跡機構を追加する具体的要件がない。generationはstale参照の検出に役立ち得るが、resource lifetime自体を自動で管理する仕組みではない。

### Consequences

- `lit_mesh`は`untextured_mesh`へ改名し、Material Registry / PipelineとPhong系Directional Lightingを導入する。
- まずApplicationから`untextured_material_t`を直接登録し、描画を確認した後にMaterial Configuration Loaderを追加する。
- Material Configuration Fileは当面1ファイル1 Material、sectionなしの`key=value`形式を予定し、具体的format contractはLoader実装時に決める。
- Texture Mapの未使用状態には`INVALID_ID_U16`を用いる方針とし、該当IDをRegistryが発行しないことを保証する。
- `texture_map_t`の具体field、Textured Materialの構造、Mapと反射値の合成、Tangent-space Normal Mapping、Shader Variant、Textureの登録・共有・失敗時rollbackはTextured Mesh実装時に決める。
- MaterialとTextureの参照関係を永続追跡していないことを、Material / Texture releaseの公開contractへ明記する。
- Render Resourceが内部的に利用するShader Headerについて、不要なApplication公開をやめてEngine privateへ移す。

### Revisit Conditions

- Application側での参照追跡や正しいrelease順の保証が、具体的な利用規模や動的更新要件によって実用的でなくなった場合。
- runtimeのhot reload、streaming、texture eviction、cross-owner sharing等が必要になった場合。
- stale handleのautomatic detectionが明確なAPI safety requirementとなった場合。
- Phong以外のmaterial / shading modelが導入され、基本反射特性の共有範囲やMesh種別を再定義する合理的理由が生じた場合。

### Related Documents

- `design_notes.md` — Material and Mesh Rendering Architecture
- `glce_near_term_todo.md` — Untextured Mesh Material / Lighting Implementation
