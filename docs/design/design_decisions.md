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

最終更新: 2026-10-21

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

**Status:** Superseded（target architecture。旧実装は移行完了まで存続）  
**Superseded by:** 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」。その後、2026-10-19「Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する」で長寿命Owner構造を再改訂  
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

**Status:** Accepted（Backend Contextの上位所有・下位Borrow原則を維持。Borrow元・対象は2026-10-19のRenderer再編Decisionで更新）  
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

**Status:** Accepted（Stateless / Temporary Transactionという基本方針を維持。Import専用の命名・配置・Release分離は2026-10-19のRenderer再編Decisionで更新）  
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

---

## Decision: Untextured Material ConfigurationはDiffuseのみを必須とし、その他の反射特性にデフォルト値を適用する

**Status:** Accepted  
**Date:** 2026-10-13  
**Scope:** Untextured Material / Configuration Format / Default Values / Lighting

### Decision

Untextured Material Configuration Fileでは、`diffuse`のみを必須fieldとし、`ambient`、`specular`、`shininess`をOptional Fieldとする。

| Field | Requirement | Default |
|---|---|---|
| `diffuse` | Required | なし |
| `ambient` | Optional | `diffuse * 0.1f` |
| `specular` | Optional | `(0.0f, 0.0f, 0.0f)` |
| `shininess` | Optional | `32.0f` |

Optional Fieldのデフォルト値は`untextured_material_config_loader`がConfiguration Fileの読み込み時に適用する。

`untextured_material_config_t`および`untextured_material_t`には未設定状態を導入せず、常に4つのfieldが確定した完全なvalueとして扱う。

明示的に指定された値はデフォルト値で上書きしない。特に`ambient`、`specular`、`shininess`のゼロ値は、未設定を表すsentinelとして扱わない。

`specular`と`shininess`の指定は相互に独立させる。

- `specular`のみ指定した場合、`shininess`にはデフォルト値を使用する。
- `shininess`のみ指定した場合、`specular`にはデフォルト値を使用する。
- 両方を省略した場合、鏡面反射係数はゼロとなり、初期状態ではSpecularによる描画寄与を持たない。

このOptional仕様はExternal Material Configuration Formatの契約であり、Applicationが直接構築する`untextured_material_t`にOptional Fieldや暗黙のデフォルト処理を要求するものではない。

### Context

GLCEでは、Textureを使用しない3D Geometryの描画に向けて、`untextured_material_t`へPhong系の反射特性である`ambient`、`diffuse`、`specular`、`shininess`を導入した。

Material Registry、Material Pipeline、Material Config Loaderを実装した段階では、Configuration Fileに4つのfieldをすべて記述する仕様としていた。

その後、一般的なSTLビューワに適したLightingを検討した結果、STLの主要な表示目的はMaterialの写実的な再現ではなく、Geometryの形状や面の向きを把握しやすく表示することであると整理した。

この用途ではDiffuseを主体とするマットな描画が基本となり、Specularによる光沢は必ずしも必要ではない。

また、Ambientは直接光が届かない面の視認性を改善するために有用であるが、通常のMaterial設定ごとにAmbient反射係数を明示的に記述することは煩雑である。

そのため、Materialの表現能力を維持しながら、Configuration Fileの必須項目を削減する方針を採用した。

### Rationale

#### Diffuseのみを必須とする

Diffuseは、Materialの基本色とDirectional Lightによる拡散反射を定義する。一般的なSTL表示では、Diffuseを指定すれば基本的なMaterialとして成立する。

一方、Ambient、Specular、Shininessは描画の補助的な特性として扱えるため、すべてのConfiguration Fileに明示的な指定を要求する必要はない。

#### Ambientのデフォルト値をDiffuseから導出する

Ambientのデフォルト値は、固定のRGB値ではなく`diffuse * 0.1f`とする。

これにより、Materialの基本色に応じたAmbient反射係数を生成できる。例えば、赤いMaterialには赤系のAmbient反射係数が生成され、暗部においても基本色との関係を維持できる。

この`0.1f`はConfiguration FileでAmbientが省略された場合のデフォルト値生成係数であり、Shaderへ固定の`ambient_strength`を導入するものではない。

Ambient Lightそのものの色や強度、Directional Lightとの関係は別の責務として扱う。Ambient Lightの具体的な設計を、このDefault Value Decisionによって確定しない。

#### Specularをデフォルトで無効にする

Specularのデフォルト値をゼロとすることで、通常のSTL表示では光沢のないマットな外観を基本とする。光沢が必要なMaterialでは、Configuration FileでSpecularを明示的に指定できる。

これにより、単純なGeometry表示を優先しながら、Materialの表現能力を制限しない。

#### Shininessへ独立したデフォルト値を与える

Shininessのデフォルト値を`32.0f`とする。Specularがゼロの場合、Shininessは描画結果へ寄与しない。一方、Specularのみを指定した場合には、Shininessの追加設定なしで光沢を使用できる。

SpecularとShininessを必須の組として扱わないことで、Configuration Formatに不要なfield間依存を導入しない。

#### Optional処理をLoaderに閉じる

External Configuration File上の未指定状態は、Loaderによる解析とデフォルト値の補完によって解決する。

Loaderが成功した後の`untextured_material_config_t`には、すべてのfieldについて確定した値が格納される。Material Pipeline、Material Registry、Shaderは、Configuration Fileでどのfieldが省略されたかを意識する必要がない。

この構造により、External Trust BoundaryにおけるConfigurationの解釈と、Engine内部のMaterial表現を分離できる。

### Rejected Alternatives

- 4つのMaterial Fieldをすべて必須とする。
  - 通常のSTL表示では不要なSpecularやShininessの記述まで要求することになる。
- Ambientのデフォルト値を固定のRGB値とする。
  - MaterialのDiffuse Colorと無関係なAmbient Colorになり得るため、基本色から導出する方式を採用する。
- Optional Fieldの未設定状態を`untextured_material_t`へ持ち込む。
  - Materialの利用側で未設定判定やデフォルト補完が必要となり、完成済みvalueの契約が複雑になる。
- ゼロ値を未設定のsentinelとして扱う。
  - 明示的に反射係数をゼロへ設定する有効な操作と区別できない。
- SpecularとShininessを必須の組として扱う。
  - Specularだけを指定したい場合にもShininessの記述を要求し、デフォルト値の利点が失われる。

### Consequences

- `untextured_material_config_loader`のRequired Field判定を変更し、`diffuse`のみを必須とする。
- Optional Fieldが存在する場合は従来どおりParsingとSemantic Validationを実施する。
- Optional Fieldが存在しない場合は、定義済みデフォルト値を適用する。
- Required / Optionalの区別、重複Keyの禁止、Unsupported Keyの拒否など、External Format固有の規則はLoaderが管理する。
- 完成したConfigurationについては、引き続き`untextured_material_config_is_valid()`によるCanonical Validationを行う。
- `untextured_material_t`の4つのfield、およびそのCanonical Validityを変更しない。
- Loader HeaderのUntextured Material Configuration Format、Supported Keys、Required Field、Default Values、Exampleを新仕様に更新する。
- ShaderにおけるAmbient / Diffuse / Specularの計算方式、Ambient Lightの所有者と操作Interface、およびShininessがゼロの場合の具体的な描画semanticsは、Lighting実装の継続作業で確認・確定する。

### Revisit Conditions

- STLビューワ以外の用途が主要となり、Materialのデフォルト特性を変更する合理的な要求が生じた場合。
- Ambient LightやEnvironment Lightingの正式導入に伴い、Ambient反射係数のデフォルト値を再評価する必要が生じた場合。
- MaterialのShader ModelがPhong系から変更され、Ambient / Diffuse / Specular / Shininessというfield構成そのものを再検討する場合。
- Configuration Fileの用途が拡大し、Material Presetや継承、複数Materialの定義など、現在とは異なるデフォルト解決方式が必要になった場合。

### Related Documents

- `design_decisions.md` — Untextured / Textured MeshのMaterial構成とTexture参照lifetimeを分離する
- `design_notes.md` — Material and Mesh Rendering Architecture
- `glce_near_term_todo.md` — Untextured Mesh Material / Lighting Completion
- `untextured_material_config_loader.h` — Untextured Material Configuration Format

---

## Decision: Ambient LightとDirectional Lightを独立したLight Valueとして扱う

**Date:** 2026-10-14  
**Status:** Accepted / Implemented  
**Scope:** Engine Light / Untextured Mesh Lighting / Application State

### Decision

Untextured MeshのLightingでは、Ambient LightとDirectional Lightを独立したLight Dataとして扱う。

- `directional_light_t`はDirection（World Space）、Color、Intensityを保持する。Directionは物体表面から光源へ向かう向きを表し、初期化時に正規化する。
- `ambient_light_t`はColor、Intensityを保持する。特定のDirectionを持たず、Directional LightのColor / Intensityを借用しない。
- 双方は独立した非opaque value typeとし、それぞれInitializerとValidatorを設ける。
- ColorはRGB各成分について有限な`0.0f`〜`1.0f`、Intensityは有限な`0.0f`以上の値とする。Directional LightのDirectionには非零を要求する。
- 現段階ではApplicationが各Lightを1つ保持し、Rendererへの更新時に渡す。LightごとのRegistry、汎用Light System、複数LightのContainerは導入しない。
- Lightの値と`untextured_material_t`の反射係数を分離する。Material Ambientは環境光の反射係数であり、Ambient Light Intensityとは異なる。

### Context and Rationale

Diffuseは面のNormalとDirectional Lightとの向きの関係を表すため、方向を持つLightが必要となる。一方、直接光が届かない面の視認性を確保するAmbientは方向を持たない簡易的な環境光として表現できる。

AmbientをDirectional LightのIntensityから導出すると、直接光の明るさと暗部の明るさを独立して調整できない。STL Viewerでは形状の視認性を優先したいことから、両LightのColor / Intensityを独立させた。

また、現在必要なのは2種類のLight ValueをRendererへ渡すことだけであり、Light一般のRegistry / Lifecycle / Systemを構築する実需要はない。現在の一種類一要素という構成も、産業・ロボット・Embedded向けの将来要件を確認するまでの実装上の制約であり、普遍的な光源数上限の規定ではない。

### Considered Alternatives

- AmbientにDirectional LightのColor / Intensityを共用させる：Light Stateは小さくなるが、暗部と直接光を独立して調整できないため不採用。
- 先に全種類共通のLight Registry / Light Systemを作る：現行要件を超える責務とResource管理を先取りするため採用しない。
- 複数Light Sourceを最初からArrayで管理する：具体的なLight数、Shader転送方式、GPU上限の要件が固まるまで先行導入しない。

### Revisit Conditions

Spot Light、複数Directional Light、複数の同一種Light、Light Animation / External Control、SceneのLighting構成管理が具体的に必要になった場合は、State Owner、数の制約、Shaderへの転送方法を再検討する。その際もMaterial反射係数とScene Lightの分離を出発点とする。

---

## Decision: Untextured MeshのPhong LightingをView Spaceで計算し、Ambient・Diffuse・Specularを加算する

**Date:** 2026-10-14  
**Status:** Accepted / Implemented  
**Scope:** Untextured Mesh Shader / Material / Coordinate Space / Lighting Semantics

### Decision

Untextured MeshのFragment Shaderは、Ambient、Diffuse、Specularの3つの反射成分を計算し、加算して最終RGB Colorを求める。Shader ModelにはPhong Reflection Modelを採用する。

- **Ambient**：`material.ambient * ambient_light.color * ambient_light.intensity`。Normal、Light Direction、View Directionによらない。
- **Diffuse**：`material.diffuse * directional_light.color * directional_light.intensity * max(dot(N, L), 0)`。`N`と`L`は正規化されたView SpaceのNormalと物体表面から光源へ向かう方向である。
- **Specular**：`material.specular * directional_light.color * directional_light.intensity * pow(max(dot(R, V), 0), shininess)`。`R = reflect(-L, N)`、`V = normalize(-view_position)`とする。ただし、`dot(N, L) <= 0`または`shininess <= 0`の場合はSpecularの寄与をゼロとする。
- SpecularはDirectional Lightからの光に対して計算する。Ambient Lightの色や強度をSpecularへ混ぜない。
- `shininess == 0.0f`はSpecular無効を意味する。正値の`shininess`でもMaterial Specular Colorがゼロならハイライトは現れない。
- Vertex ShaderはView Spaceへ変換したPosition (`view_position`) とNormal (`view_normal`) をFragment Shaderへ渡す。Directional Light DirectionはApplication Renderer側でWorld SpaceからView Spaceへ変換してShaderへ設定する。
- View SpaceではCameraが原点にあるため、Fragment ShaderでCamera World Positionの追加Uniformを要求しない。
- Normal変換は当面、Model MatrixのX/Y/ZのScalingが均等であることを前提とする。非均等Scalingへの対応は別判断とする。

### Context and Rationale

従来のDiffuse-only表示では、光が当たらない面が黒くなり、STL形状の把握が難しい。また、Materialが保持していたAmbient、Specular、ShininessのFieldが描画計算に利用されなかった。

Ambientを追加して暗部に一定の明るさを残し、SpecularとShininessで必要に応じた光沢を表現する。Viewerの基本MaterialではSpecularをゼロとして、形状認識に適したマット表示を維持する。

SpecularにはCameraから見た方向と反射方向が必要になる。既存のView Matrix更新機構を再利用できるため、Fragment PositionとLight Directionを同じView Spaceへ揃えた。カメラ位置Uniformを新規導入したり、`view_dirty`へLight更新を結び付け直したりしない。Light Color / Intensityの更新はCamera View Matrixの変更有無と独立した処理とする。

`shininess == 0`の場合に`pow(x, 0)`の結果に描画Semanticsを依存させると、意図しない全方向への反射や`pow(0, 0)`の問題につながる。そのため、Specularを計算しない規則を明示した。また、光源が面の裏側にある場合のSpecularを抑制することで、Viewerにおける直感的な描画を優先した。

Material Configurationの`ambient`省略時のデフォルト`diffuse * 0.1f`、`specular = (0,0,0)`、`shininess = 32.0f`は、既存の「Untextured Material ConfigurationはDiffuseのみを必須とし、その他の反射特性にデフォルト値を適用する」Decisionに従う。今回のShader Semantics確定によって、Config Defaultの値を変更していない。

### Considered Alternatives

- Diffuse-onlyの維持：Materialの3つのFieldが実描画に寄与せず、暗部の視認性や光沢表示に制約があるため不採用。
- AmbientとDirectional Lightの同一強度化：別Decisionにより、それぞれ独立したLight Valueを採用。
- Specularで光源の裏面判定を省く：表面に直接光がない場合にもハイライトが生じ得るため採用しない。
- `shininess == 0`を通常の`pow(x, 0)`へ任せる：意図した無効化Semanticsを保証しないため採用しない。
- World SpaceでCamera Position Uniformを渡す：実装可能だが、既存View Matrixを利用したView Space方式の方が現構成で追加Stateを必要としないため採用しない。

### Verification and Limits

2026-10-14、ペンギンSTLを静止表示・Model Matrixで回転させた描画を通じて、Ambient、Diffuse、Specularの効果の変化を確認した。これは基本機能の動作確認であり、すべての数値境界・異常入力・GPU環境について網羅的なRegression Testが完了したことを意味しない。

非均等Scaling、Gamma / Linear Color Space、HDR / Saturation、Spot Light、複数Light Sourceは今回のLighting Decisionの対象外とする。

### References

- `design_notes.md` — Material and Mesh Rendering Architecture
- `untextured_material_config_loader.h` — Untextured Material Configuration Format
- `directional_light.h` / `ambient_light.h` — Light Value API


---

## Decision: Renderer専用ResourceをRenderer Domainへ統合し、Range AllocatorをMemory Domainへ移動する

**Status:** Accepted / Implementation Pending  
**Date:** 2026-10-15  
**Scope:** Renderer / Resource Domain / Directory Architecture / Memory Range Allocation

### Decision

従来の`engine/resource/`と`engine/systems/renderer/resources/`という二つのResource配置を統合し、**Renderer用途のResource表現・生成・登録・管理を`engine/systems/renderer/resources/`配下へ集約する**。

- `engine/resource/`にあるCPU Geometry、CPU Texture Resource、Resource Core、Loader、Geometry Conversion等をRenderer側Resource Domainへ移す。
- `renderer/resource_registries/`および`renderer/resource_pipelines/`も`renderer/resources/`配下へ移し、既存Shader、Material、GPU Texture、VBO Manager等と一つのResource Domainにまとめる。
- この移行が完了した時点で従来の`engine/resource/`を廃止する。
- `renderer/resources/allocators/range_allocator`は`engine/memory/low_level_allocators/range_allocator`へ移す。
- `engine/core/geometry_primitive`等のRenderer非依存な幾何PrimitiveやMath機能はResource移動の対象にしない。

このDecisionは**配置と責務領域の決定**であり、Shader、Registry、Pipeline、CPU/GPU Resourceの既存の公開APIやOwnershipをDirectory移動だけで再定義するものではない。Renderer FrontendへのOwnership移管は別Decisionに従い、独立した後続Workで行う。

### Context

これまでResourceは、おおむねCPU Resourceを`engine/resource`、GPU側ResourceやShaderを`renderer/resources`として分けていた。しかし、Resource PipelineはCPU側のLoad / CreateとGPU Uploadをまたぎ、RegistryもCPUとGPUのResourceを一体で所有する。Materialや新しいMesh / ModelもCPU/GPUの単純な二分法だけでは自然な置き場所を決められない。

また、`range_allocator`は現状GPU Buffer用として使われるものの、実GPU Buffer Object、base pointer、GPU APIやPixel / Vertex Dataの実体を所有せず、byte offsetの論理的なRange割当を担当している。そのAllocation Mechanismは`vbo_manager`のGPU Resource Managementとは別の責務である。

将来Collision / Physicsが必要になった場合にも、Renderer側GeometryをそのままCollisionのAuthorityとする必要はない。Renderer非依存のPrimitive / Intersection処理を共通基盤とし、Rendererは必要に応じてそれらを利用する構造が考えられる。

### Rationale

- CPU/GPUの保存先ではなく、Renderer用途ResourceのLifecycle / Identity / Consumptionというまとまった責務でResource Domainを説明できる。
- CPU Geometry・Material・Mesh・Model・Shader・Registry・Pipelineが同一Renderer Domain内に置かれ、同じResourceの管理責務を複数のトップレベル領域へ分割せずに済む。
- ShaderやGPU Textureが参照するRenderer Backendも同じRenderer System内にあり、Directory上のCross-domain Dependencyを減らせる。
- Range Allocation MechanismをMemory Domainに置き、GPU Buffer自体のOwnerであるVBO Managerとの区別を明確にできる。
- 旧Resource APIの移動と、後続Frontend移行でのOwnership再設計を別Workにすることで、回帰の原因を切り分けやすい。

### Considered Alternatives

- `engine/resource`へRenderer側Shader / Registry / Pipelineまで集約する。
  - General-purpose Resource Domainとして発展させやすいが、GPU依存を持つShader / TextureからRenderer BackendへのCross-domain Dependencyが増える。現時点のResource群の主目的はRendererにあるため採用しない。
- CPU/GPUの従来の二分割を維持する。
  - Resource TransactionとRegistry Ownershipが境界を横断し、Mesh / Modelの所属が曖昧なままとなる。
- `range_allocator`をGPU Buffer用という理由だけでRenderer内部へ残す。
  - GPU API操作を担当しない論理range管理の責務が配置から読み取りにくい。
- Renderer外のPhysics / Collisionの将来需要を理由として、現時点で共通Model / Scene Resource Domainを構築する。
  - 要件未確定の共通Authorityを先行導入することになり、現在のRenderer目的のModel設計を複雑化する。

### Consequences

- `renderer/resources/`はCPU Resource限定でもGPU Resource限定でもなく、**Rendererに属するResource全般のDomain**となる。
- Geometry CPU ResourceのOwned Vertex、Texture CPU / GPU ResourceのAuthority、Registry登録時のOwnership Commitなど、既存の内部Contractは配置変更だけでは変えない。
- `engine/memory/low_level_allocators/range_allocator`は論理Rangeのallocation / freeを提供する。GPU Bufferの生成・転送・破棄は引き続きRenderer内のVBO Manager / Backendが担う。
- 本DecisionのMemory Domain拡張はGPU Memory ObjectのOwnershipをMemory Domainに移すことを意味しない。既存のCPU allocation-level validation trust rootはそのまま維持する。
- Directory移動後のHeader、Build、Doxygen、設計文書の参照を整合させる。

### Revisit Conditions

- Renderer外で独立して所有・利用すべきAsset / Resource群が実装され、共通Resource Domainが実際に必要となった場合。
- Collision / Physics / Simulation等が同じ幾何データのAuthorityを共有する具体的要件を持つようになった場合。
- Range AllocationがMemory Domainより独立した汎用Resource Handle / Interval Domainへ属すると判断できる用途が増えた場合。

### Related Documents

- `glce_near_term_todo.md` — Renderer Resource Directory Consolidation
- `design_notes.md` — Renderer Resource Domain and Frontend Migration
- `memory_model.md`

---

## Decision: Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする

**Status:** Superseded（FrontendによるShader / Registryの直接所有とRender Resource全面廃止という構造を変更。Frontend全体Owner / Stateless Mesh Rendererは維持）  
**Superseded by:** 2026-10-19「Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する」  
**Date:** 2026-10-15  
**Scope:** Renderer Frontend / Ownership / Mesh Renderer / Resource Pipeline / API Boundary

### Decision

Renderer Frontendが、Renderer Backend Contextと、描画種別ごとに必要なShaderおよびRegistry群を**直接かつ長寿命に所有する**構造へ移行する。

- `renderer_frontend`はRenderer ResourceのLifecycle、Import / Release APIの入口、Render Packet処理と描画のOrchestrationを担当する。
- `mesh_renderers`はShader種別ごとの**StatelessなDraw Operation Module**とする。Shader / Registry等をOperation中にBorrowして描画を実行し、内部のPersistent StateやOwned Resourceを持たない。
- Mesh RendererはUniform設定、描画時のResource解決、BackendへのDraw要求等を扱うが、Resource Import / Releaseを担当しない。
- FrontendのImport APIは既存のStateless Resource Pipelineへ具体的なLoad / Create / GPU Upload / Registerを委譲する。Registryは登録後のResource Identity / Ownershipを維持する。
- ReleaseはFrontendを受付境界とし、Registry等の既存解放経路を利用する。
- 新しい`resource_set`等のOwnership Wrapper Objectは設けず、Frontendが各Shader / Registryを直接所有する。
- 新しいStateless Importerモジュール群は**今は作らない**。FrontendのImport / Releaseを実装して必要性が明確になった場合に分離する。
- 現行`render_resources`のImport / ReleaseはFrontend移行と同時に撤去し、OwnershipとDraw機能を新しい担当へ移管した後、旧`render_resources`自体を廃止する。`application_renderer`も不要なFacade / Compositionを整理する。

個々のShader / Registry間の独立性は維持し、共通の万能Material Registry、Mesh Registry、Resource Set Object等の導入を要求しない。

### Context

2026-09-12にはTopology別`render_resource`をShader / RegistryのOwnership Rootとして採用し、Import / Release / Drawを同一Objectに集約した。これは当時ApplicationへShader / Registry管理を露出させないために有効だった。

その後、新しく`renderer_frontend`、`xxx_mesh_t`、Model、Render Packetを導入する計画を進める中で、`render_resource`はResourceを所有する側面と、描画を実行する側面を兼ねていることが明確になった。

`mesh_renderer`という名称を採用する場合、そこへImport / Releaseを残すと、Resource Lifecycleと描画実行が再び混在する。また、各Render ResourceとFrontendの間にResource Set的なObjectを追加すると長寿命Ownerが余分に一層増える。現段階のShader / Registry数なら、Frontendによる直接所有の方が実装とOwnership Relationを追いやすいと判断した。

### Rationale

- Shader / Registryの長寿命OwnerをRenderer Frontend一箇所として見通しよく管理できる。
- Resource ImportのTransaction Responsibilityを既存Pipelineに維持し、Frontendに詳細処理を複製しない。
- Mesh RendererはDrawだけを担当するため、状態・Construction / Destruction・内部Ownershipを持たずに済む。
- Import / ReleaseとDrawを明確なAPI Boundaryへ分離できる。
- 既存RegistryのRegistration Commitと独立Vertical SliceのResource管理を維持できる。
- Resource SetやImporter等の追加のForwarding / Ownership層を設けず、実需要が現れた段階で抽出できる。

### Considered Alternatives

- 従来のTopology別Render ResourceをOwnership Rootとして残す。
  - Frontend導入後もImport / Release / Drawが同一Objectへ混在し、StatelessなMesh Rendererへ描画責務を分離する狙いと合わない。
- Stateless Mesh RendererにImport / Releaseも担当させる。
  - 「描画を実行する」という責務と、Resource Lifecycle / Registry Mutationが混在する。
- `xxx_mesh_resource_set_t`等の中間Stateful Ownerを新設する。
  - Vertical SliceごとのOwnershipを局所化できるが、今回の規模ではFrontendとの間に別Object Lifecycleが増え、構成が複雑になる。
- Renderer FrontendへImport / Releaseの具体的なLoad / Upload / Registration処理をすべて埋め込む。
  - 既存PipelineのOrchestrationを重複実装し、Frontendの責務が過大になる。
- 直ちにStateless Importer Layerを新設する。
  - 現在のPipelineと単純Forwardingになる可能性があり、Mesh / Modelの具体的Import Semanticsも未確定なので保留する。

### Consequences

- Renderer FrontendのConstructor / Destructorは複数Shader / RegistryとBackend Contextを整合した順序で生成・破棄する。特にRegistry / GPU Resourceが参照するShaderやBackend Contextを先に解放しない。
- 旧`render_resources`をOwnership Rootとする2026-09-12のDecisionは、**採用したTarget Architectureとしては本Decisionに置き換わる**。旧Sourceは移行が完了するまで歴史的Implementationとして残る。
- 2026-09-12「Backend ContextはRender Resourceがborrowする」Decisionの**Backend Contextを個別Resourceが所有しない／長寿命Ownerが支える**という原則は維持するが、Borrow関係の具体的なOwnerは移行後のFrontend / Shader / GPU Resource構造へ更新する。
- 2026-09-12「Resource Pipelineはstateless orchestrationとして維持する」DecisionのStateless / Temporary Ownershipは維持し、Pipelineへ渡す長寿命Shader / RegistryのOwnerだけをFrontendへ変更する。
- 2026-10-09「Untextured / Textured MeshのMaterial構成とTexture参照lifetimeを分離する」DecisionのMaterial Value・Vertical Slice・Texture Lifetime Contractは維持する。Render ResourceがShader / Registryを所有するとの記述だけを新Targetに合わせて置き換える。
- Material / Texture、Geometry / Material等のReference Lifetime保証は既存Contractを維持し、MeshやFrontendの導入だけでReference CountingやGeneration付きHandleを追加しない。
- ApplicationのResult Conversion、FrontendのOutput Contract / Validation、State Update、既存描画と同等のDrawを実装・Review時に確認する。

### Revisit Conditions

- Frontendが多数の独立Resource集合を管理し、Construction / DestructionやLifetimeの局所性を分ける明確な必要が生じた場合。
- FrontendにImport / Release APIが増大し、独立したStateless Importer / Resource Orchestratorの方が見通しがよくなる場合。
- Resource Streaming、Hot Reload、GPU Residency等によりResource LifetimeとFrontend Lifetimeを再定義する必要が生じた場合。
- Render Packetの実装結果からStateless Mesh Rendererという分割が不適切と判明した場合。

### Related Documents

- `glce_near_term_todo.md` — Renderer Frontend / Model / Mesh / Frame Allocator Integration
- `design_notes.md` — Renderer Resource Domain and Frontend Migration

---

## Decision: MeshはShader別Value Typeとし、当面ModelがCoordinateを所有する

**Status:** Accepted / Implementation Pending  
**Date:** 2026-10-15  
**Scope:** Mesh / Model / Resource Reference / Coordinate Ownership / Future Spatial Boundary

### Decision

- `untextured_mesh_t`や`ui_mesh_t`等、Shader / Rendering Vertical Sliceごとに独立した`xxx_mesh_t`を設け、**非opaque Value Type**とする。
- Meshは描画対象に必要なRegistry Resourceの**非所有参照の組合せ**を表し、Geometry / Material / Textureの実体およびそれらのRegistration IdentityのAuthorityを持たない。
- Meshは自分自身の配置姿勢とResourceの組合せを同一視しない。Model Matrix / Coordinateの管理をMeshに一律に組み込まず、Model側でのBindingを検討する。
- 今回構築するModelは複数のMesh参照とCoordinate配列を管理し、**当面Model自身がCoordinate Storage / LifetimeのOwner**となる。
- ModelのCoordinateはPenguin全体姿勢とBody / Head / Wing等の部位姿勢を表すために利用する。親子関係、Indexの保持場所、Local / Worldの具体SemanticsはModel実装時に確定する。
- Scene / Spatial SystemをRendererと同格に設け、BridgeでCoordinateをRendererへ提供する構想は将来の再検討候補とし、今回のWorkでは導入しない。

### Context

Shader別Meshを導入する際、既存`untextured_mesh_render_resource_draw()`等ではGeometry IDとMaterial ID（またはTexture ID）をDraw時に渡していた。Meshはそれらの組み合わせをひとつの描画要素として表現する自然なValue Objectとなる。

他方、ModelへCoordinate Arrayを持たせると、将来Physics / CollisionがRenderer内のModelをCoordinate Authorityとして参照しなければならない可能性がある。Rendererと同格のSpatial SystemとBridgeを設ける構想も検討したが、現段階ではCollision / Physicsを実装せず、Penguinの部位操作とSpot Lightへの発展をまず検証したい。そのため、早期のSubsystemとBridge導入は行わず、当面はRenderer ModelにCoordinateを持たせる判断とした。

### Rationale

- Meshが小さなResource参照の組み合わせである現要件に、Heap OwnershipやOpaque Object Lifecycleを強制せずに済む。
- Shader別の型によって、UI MeshにPhong Materialを要求する等の不要なCross-slice共通状態を避けられる。
- Resource OwnershipをRegistryへ残し、MeshがResourceの解放順序や実体Ownerを重複管理しない。
- MeshのResource Compositionと、Model内のCoordinate / Placementを分離して設計できる。
- 当面のModelに必要な部位別Transformを、別Scene / Spatial SystemやBridgeを先行実装せずに成立させられる。
- 将来独立したCoordinate Authorityが必要になれば、描画Resourceの構成とCoordinateのAuthorityを別境界へ切り出す余地がある。

### Alternatives and Open Design Details

- **ID参照とBorrowed Pointer参照：未決定。** Pointer BorrowはRegistry Lookupを一部省略できるが、Lifetime / address stabilityを強く要求する。既存Geometry描画はGPU Draw RangeをRegistryから解決する必要があり、CPU Geometry Pointerだけでは完全なDraw情報を得られない。この比較を行ってからField型を決める。
- Mesh Identity用の独立Registry / Stable ID：具体的な需要が確定していないため、今回のValue Type採用だけを理由に先行導入しない。
- Model内のMesh配置方式：種別別配列、Tagged Entry等は未確定。Mesh参照とCoordinate IndexをどのObject / Entryが持つかも未確定。
- CoordinateのLocal / World、親子Transform、Pivot、Transform Cache、Model Instance化：今回のPenguin操作に必要なContractから決定し、汎用Scene Graphは先行導入しない。

### Consequences

- Mesh Value TypeのValidator / Initializer、Borrow / ID Contractは具体的Field選択後に設計する。
- Geometry / Material / TextureのResource IdentityとOwnershipは引き続き既存Registryに属する。MeshやModelから参照する間のResource Lifetime / Release Orderを明文化する。
- ModelのCoordinateは当面Renderer内部のModelが保持するが、独立Scene / Spatial Systemへ一方向に抽出することも将来案として残す。
- 今回の実装検証は分割Penguin、部位AABB、頭部Keyboard操作、異種UI Mesh、Render Packet経路を対象とする。Collision / Physicsは実装しない。

### Revisit Conditions

- Physics / Collision / Animation等がRendererから独立してPosition / OrientationのAuthorityを必要とした場合。
- 同じMesh構成を持つ多数のModel InstanceでCoordinateだけを分ける具体的要件が生じた場合。
- Model内のCoordinate Ownershipにより、Renderer外部のLight / Simulation等との情報共有が複雑化した場合。

### Related Documents

- `glce_near_term_todo.md` — Renderer Frontend / Model / Mesh / Frame Allocator Integration
- `design_notes.md` — Renderer Resource Domain and Frontend Migration / Material and Mesh Rendering Architecture

---

## Decision: Renderer Resource Directory Consolidationの実装完了とResource Core廃止に伴うResult Domain再編

**Status:** Accepted / Implemented（Source配置・Result型の再編）  
**Date:** 2026-10-15  
**Scope:** Renderer Resource / Directory Architecture / Geometry・Loader・Texture Result / Migration History

### Decision

2026-10-15「Renderer専用ResourceをRenderer Domainへ統合し、Range AllocatorをMemory Domainへ移動する」の**配置判断を実装した**。加えて、実際のDirectory Consolidationで旧`Resource Core`の共通Result型の必要性を再検討し、次の追加判断を採用した。

1. 旧`engine/resource/`のうちRenderer用途のGeometry、Texture CPU Resource、Loader、Geometry Conversion等を、`engine/systems/renderer/resources/`配下へ移す。旧`engine/resource/`は廃止する。
2. 既存のResource Registry、Resource Pipeline、Shader、Material、GPU Texture、Buffer Manager等もRenderer Resource Domainの下へ整理する。
3. GPU用VBOと組み合わせていた`range_allocator`は、GPU API / GPU BufferそのもののOwnerではなく、offset / range allocation mechanismであるため、`engine/memory/low_level_allocators/range_allocator/`へ移す。
4. `engine/core/geometry_primitive/`のRenderer非依存なPrimitiveは、Renderer Domainへ移さない。
5. 旧`Resource Core`を廃止し、共通`resource_result_t`を廃止する。従来の共通Resultの使用箇所を**Geometry / Loader / Texture各Domainが所有するResult型**に再編する。
6. `resources/`自体には共通Result型を強制しない。これはRenderer用途の複数のResource関連Moduleを束ねる配置領域であって、各Module固有のError Semanticsを代行する単一のSemantic Ownerではない。
7. `resource_pipeline_result_t`、`resource_registry_result_t`、`render_resource_result_t`など、独立した既存DomainのResult型は維持する。
8. 今回の再編では、現行`render_resources`による長寿命Shader / Registry Ownership、Stateless PipelineのImport、RegistryへのOwnership Commit、`application_renderer`の位置付けを変更しない。Renderer Frontendへの移管は別Decision／後続R3で実施する。
9. `texture_min_filter_config_t`、`texture_mag_filter_config_t`、`texture_wrap_config_t`およびそのValidatorは、Texture Resource固有のMetadataではなくRenderer BackendとTexture Resourceが共有する設定契約であるため、`renderer/core/renderer_types.h`および対応する`.c`に配置する。`texture_resource_info_t`のSemantic Ownerである`resources/textures/core`とは分離する。

実装済みの主要な構造は次のとおり（`renderer/resources/`以下は複数形の実Directory名を表す）。

```text
engine/
├─ core/geometry_primitive/                  # Renderer非依存Primitive
├─ memory/low_level_allocators/
│   └─ range_allocator/                     # logical byte range
└─ systems/renderer/
    ├─ core/                                # BackendとResourceが共有する型・設定契約
    ├─ renderer_backend/
    ├─ render_resources/                    # 現行Ownership Root。後続R3で再編
    └─ resources/
        ├─ buffer_managers/
        ├─ geometries/
        │   ├─ core/                         # geometry_result_t等
        │   └─ conversion/
        ├─ loaders/
        │   └─ core/                         # loader_result_t等
        ├─ materials/
        ├─ pipelines/
        ├─ registries/
        ├─ shaders/
        └─ textures/
            └─ core/                         # texture_result_t、texture metadata
```

### Context — `engine/resource`と`renderer/resources`の比較から始まった

#### 1. 当初の問題はCPU / GPUによるResource配置の二分だった

従来のGLCEには、主にCPU側のGeometry、Loader、Texture CPU Resource等を置く`engine/resource/`と、Shader、GPU Texture、VBO Manager等を置くRenderer内部のResource領域が並存していた。ところがImport TransactionはCPUでのLoad / CreateからGPU Upload、Registry Registrationまでを横断し、ResourceのIdentity、Ownership、LifecycleもCPU / GPUの区分では完結しない。

MeshとModelを新設するに当たり、このままでは「どちらのResource領域がそれらを所有するのか」を明快に説明できない。そこで単なるパスの短縮ではなく、**Resourceが何に使用され、そのLifetimeとOwnershipを誰が統括するか**を基準に配置を検討した。

#### 2. 第一の候補は`engine/resource`を共通Resource領域として拡張する案だった

Geometryを将来のCollision / Physics等でも使う可能性を考慮すれば、CPUデータをRendererから独立させ、Renderer側のShader / Registry / Pipelineまで含めて`engine/resource`へ集約する案には合理性がある。一般用途のAsset / Resource Domainを先に作る構想も検討できる。

しかし現行のShaderやGPU Texture、VBO ManagerにはRenderer Backendへの強い依存があり、現行Resource Pipeline / RegistryもRenderer描画を成立させるための構成要素である。これらをEngine直下へ移してもRenderer依存が消えるわけではなく、**一般Resourceと名乗る層にRenderer固有の意味と依存を移すだけ**になりかねない。将来の再利用を根拠に、現在存在しない共有Authorityを先行して構築することは採用しなかった。

#### 3. 第二の候補はRendererの`resources`へ集約する案だった

現在のResource群の主要ConsumerはRendererである。Renderer用途のGeometry、Texture、Material、Shader、Registry、Pipelineを`renderer/resources/`にまとめれば、CPU/GPUの物理的な置き場所ではなく、**Renderer Resourceとしての目的・Lifecycle・Ownership**で境界を説明できる。

Renderer外で必要な共通機能まで移すわけではない。例えば`geometry_primitive`は依然Engine基盤に残す。将来、物理・衝突判定向けの独立したGeometry Authorityや共通Asset Domainの必要性が具体化したときは、その要件に基づいて再分割を検討できる。この条件付きの設計を採用した。

#### 4. Range AllocatorはResource用途ではなくMechanismで判断した

Range AllocatorはVBO用に使われていても、GPU Memory Objectそのもののallocation / destructionを所有しない。実GPU Buffer操作やRangeとGPU Bufferの結合はVBO Manager／Backendの責務であり、Range Allocator自身が提供するのは論理的なbyte offset rangeのallocation / freeである。そのため、Renderer内部に残すより`engine/memory`へ移す方が機構の責務を正確に表す。

#### 5. 当初はDirectory移動と機能変更を分離する予定だった

当初のR0はDirectory Consolidationを先に独立して完了し、API SemanticsやOwnership再編を後続Workへ分離する計画だった。この判断は、差分が大きくなったときの回帰要因を特定しやすくするためのものだった。

しかしDirectory再編に伴い、旧`engine/resource/core`に集約されていた`resource_result_t`とError Conversionをどこへ移すべきかが実問題となった。共通Coreを機械的に移設するだけでは、Geometry・Loader・Textureそれぞれの責務とError Vocabularyの不一致を固定化してしまう。そこで今回の実装では、**Directoryの移動に加えてResult Domainの切り分けを行う**方針へ拡張した。

この追加変更は、Renderer Frontend / Mesh / Model実装まで同時に始めたことを意味しない。既存Ownershipと描画経路は保ち、Result型とそのConversion Boundaryだけを整合させた。

### Result Domainの整理

| Domain | Result型 | 主な責務 |
|---|---|---|
| Geometry | `geometry_result_t` | CPU Geometry生成・変換・管理等のGeometry操作 |
| Loader | `loader_result_t` | 外部File / Formatの読み込み、Parsing、受理可否 |
| Texture | `texture_result_t` | CPU / GPU Texture Resourceの操作 |
| Resource Pipeline | `resource_pipeline_result_t` | Import Transaction / Orchestration |
| Resource Registry | `resource_registry_result_t` | Identity / Registration / Release / Ownership |
| Render Resource | `render_resource_result_t` | 現行Render Resource API / Ownership Root |

Geometry Domainに一般的なFile Read Errorを背負わせる必要はなく、Loaderには`UNSUPPORTED_FILE`、File Open / Read関連のFailureが必要である。TextureにはTexture Resourceとしての操作結果がある。元の共通Result型を場所だけ変えて維持するよりも、**Result Semanticsを責務境界に局所化する**方が説明可能である。

各Domainが必要な下位ResultのConversionを所有し、上位Pipeline / Render Resourceは自身のResult Domainへ明示的に変換する。Resource名を含む独立既存Domain（`RESOURCE_PIPELINE_*`、`RESOURCE_REGISTRY_*`、`RENDER_RESOURCE_*`）を、旧`RESOURCE_*`と混同しない。

### Renderer共有Texture設定型のSemantic Ownershipと依存方向

旧Resource Coreの廃止に際して、Textureに関係する型をすべて`resources/textures/core`へ移せばよいわけではないことも確認した。具体的には次の3型と各Validatorである。

- `texture_min_filter_config_t` / `texture_min_filter_config_is_valid()`
- `texture_mag_filter_config_t` / `texture_mag_filter_config_is_valid()`
- `texture_wrap_config_t` / `texture_wrap_config_is_valid()`

これらはPixel DataのWidth、Height、Channel Count等のTexture Resource Metadataを定義する型ではない。TextureをRendererでどのようにSampling / Filtering / Wrappingするかに関する設定値であり、現行の`renderer_backend_texture_create()`と`texture_gpu_resource_create()`の双方が使用する**Renderer内部の共有契約**である。

もしこれらを`resources/textures/core`に配置すると、Renderer BackendがResource配下の設定型に依存する。一方でTexture GPU ResourceはRenderer Backendを利用するため、Backend → Texture Resource側の型参照と、Texture Resource → Backendの利用が併存し、依存方向が双方向になりやすい。実際の循環Includeの発生を待つのではなく、**共通の契約を一方のConsumerに所有させない**構造を採用した。

そこでFilter / Wrapの型とValidatorは`engine/systems/renderer/core/renderer_types.h`および対応する`.c`をOwnerとし、Renderer BackendとTexture GPU Resourceの双方が同じ上位の共有契約を参照する。依存方向は次のように整理できる。

```text
renderer/core/renderer_types.h
     ↑                      ↑
renderer_backend/     resources/textures/
     ↑                      │
     └──────────────────────┘   (Texture GPU ResourceがBackendを利用)
```

これに対して`texture_resource_info_t`はGLCE内部のTexture Resource自体のMetadataであり、Texture DomainがField間RelationとValidatorを所有する。**同じ「Texture関連の型」であっても、共有Renderer設定とResource固有MetadataではSemantic Ownerが異なる**という判断である。

### Considered / Rejected Alternatives

- **共通`resource_result_t`を`renderer/resources/core`へそのまま移す。** Directory移動は容易だが、異なる責務のError Semanticsが共通型へ残るため採用しない。
- **Filter / Wrap設定型を`resources/textures/core`へ集約する。** Texture関連という名称上のまとまりは得られるが、Renderer BackendがResource Domainの型へ依存することになり、Texture ResourceからBackendへの依存と合わせて双方向依存を生みやすいため採用しない。
- **全Resource SubdomainのResult Conversionを共通Coreへ集中させる。** 一見重複を避けられるが、各Domainの依存・Failure Semanticsを把握しにくくし、共通CoreがすべてのSubdomainの依存を集めるため採用しない。
- **`engine/resource`へRenderer Resource全体を集約する。** 将来の一般Asset利用には余地がある一方、現状のRenderer依存をEngine共通Resourceとして見せることになるため今回は採用しない。
- **CPU側とGPU側を従来どおり分離する。** Pipeline / RegistryのTransaction・Ownershipが分割境界を横断し続けるため採用しない。
- **Renderer外のScene / Physics / Collisionを先行実装する。** 具体的ConsumerやCoordinate Authorityが未確定であり、現行R0の前提条件にしない。
- **今回Frontend / Mesh / Modelまでまとめて実装する。** Ownership移管・Draw Route変更とDirectory / Result変更を同時に行うと検証範囲が過大になるため、R1以降へ分離する。

### Consequences

- `renderer/resources/`はRenderer向けの各Resource Subdomainを束ねるが、それ自体が巨大な共通Resource型や万能Resultを提供することを要求しない。
- `renderer/core`はBackendとResourceが共有するRenderer設定契約を保持し、`resources/textures/core`はTexture Resource固有のMetadata / Result / Validatorを保持する。所属は名称やGPU/CPU分類ではなく、どのConsumerに対してどのContractを提供するかで決める。
- Geometry / Loader / Texture間の直接依存、LoaderとResource Metadataの境界は、実際のSemantic Ownershipに基づいて個別に判断する。必要な依存まで排除するために抽象型を機械的に追加しない。
- Existing `render_resources`、Registry／PipelineのTransaction / Ownership Contract、Texture Lifetime、Shader・Backend Borrowは、Frontend移行まで現行のままとする。
- 本DecisionのImplementation完了は、R1以降のMesh / Model / Frontend / Render Packet / Frame Allocatorが実装済みであることを意味しない。
- 実装変更と文書の更新は区別する。Source内の旧`Resource Core`へのコメント参照などは、別途Source Documentation整合として検証する。

### Revisit Conditions

- Renderer以外の独立したAsset OwnerやGeometry / Texture Consumerが生まれ、共通Resource Domainに実利が生じた場合。
- Physics / Collision / SimulationのAuthority・LifetimeがRendererと異なることが具体化した場合。
- Geometry / Loader / TextureのResult型の重複が、共通Result Contractを導入するに足る実問題となった場合。ただし単純な列挙子の重複だけでは理由としない。
- Renderer ResourceのAPI／Lifetimeが変化し、現行のSubdomain配置が責務を説明しなくなった場合。

### Related Decisions / Documents

- 2026-10-15「Renderer専用ResourceをRenderer Domainへ統合し、Range AllocatorをMemory Domainへ移動する」— **既存の配置判断を履歴として維持し、本Decisionが実装結果と追加変更を記録する**。
- 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」— Target Architectureは引き続きImplementation Pending。
- `design_notes.md` — Renderer Resource Domain and Frontend Migration
- `glce_near_term_todo.md` — R1以降のRenderer / Model track

---

## Decision: Texture Resource MetadataのSemantic OwnerをTexture Domainとし、Loaderからの直接利用を当面維持する

**Status:** Accepted / Implemented（現在のTexture Metadata／BMP Loaderの責務分担）  
**Date:** 2026-10-15  
**Scope:** Texture Metadata / Loader / External Trust Boundary / Dependency / Validation Ownership

### Decision

- `texture_resource_info_t`を`engine/systems/renderer/resources/textures/core/texture_types.h`に配置する。`texture_resource_info_is_valid()`もTexture Domainが所有する。
- Width、Height、Channel Count、Pixel Data Size等のMetadata FieldおよびField間RelationのSemantic OwnershipをTexture Domainに置く。
- `bmp_loader`はExternal BMP representationを読み込み、BMP固有のHeader / File Range / Pixel Formatを検証し、必要なNormalizationを実行して、GLCE内部で利用するPixel Dataと`texture_resource_info_t`を生成する。
- BMP LoaderはTexture Metadata Semanticを独自に再定義せず、受理するCandidateのFinal ValidationでTexture Domainの`texture_resource_info_is_valid()`を利用する。
- Texture CPU Resourceは有効なMetadataとOwned Pixel StorageとのRelation、Ownership / Lifetimeを管理し、Metadata固有のSemantic Validationを重複実装しない。
- **LoaderがTexture DomainのMetadata型およびValidatorに直接依存する構造を、当面意図的に許容する。** その依存を解消するためだけにBMP Loader固有のDecoded Typeや追加変換APIを先行導入しない。
- Loaderは引き続きExternal Trust Boundaryである。File由来のMalformed / Unsupported RepresentationをInternal `DATA_CORRUPTED`と機械的に混同しない。Result Conversion時も故障のProvenanceを勝手に再分類しない。
- 現行`bmp_loader`および`stl_ascii_loader`等は、汎用File DecoderではなくRenderer Resource用のLoaderとして`renderer/resources/loaders/`に置く。External FormatのParse / Decodeに加えて、現行のGLCE内部Resource表現へのNormalizationやCandidate生成を担うためである。
- GLCEが実装・保守するResource Loaderと、外部から取り込むThird-party Decoderを区別する。`vendors/`等のThird-party Code管理領域を、GLCE固有Loaderの配置先としては採用しない。汎用Decoder層の抽出と配置は具体的な再利用需要が発生した段階で判断する。

### Context — 旧Resource Coreの解体時に現れた依存の問題

従来、`texture_resource_info_t`とそのValidatorは旧`engine/resource/core`に位置していた。そのためBMP LoaderとTexture CPU Resourceは、共通Coreを介して同じMetadata Semanticsを参照できた。しかし旧Resource Coreを廃止すると、Metadataの正式なOwnerを決める必要がある。

Metadataの各Fieldを定義するのはBMP Formatそのものではない。BMPはExternal Source Formatの一つであり、GLCE内部のTexture Resourceが保持するNormalized Metadataを規定する主体はTexture Domainである。Loader専用の型として閉じ込めると、他のTexture生成経路（Solid Color等）から同じMetadata Semanticsを共有しにくくなる。

一方で、この型をTexture Domainへ移すとBMP LoaderからTexture Domainへの直接Include Dependencyが発生する。この依存を不自然と見なし、`bmp_decoded_image_t`等の中間型とLoader→Pipeline→Texture変換を必須にする案も考えられる。しかし、現状ではBMP LoaderはGLCE内部で使用できるTexture Pixel Representationへの変換まで担っており、追加型を設けてもField Copyと二重Validationが増えるだけになる可能性がある。

そのため、**現状の具体的なConsumerと責務に合う簡潔な直接依存を採用し、抽象化のためだけの抽象化を避けた**。

### Resource Loaderと汎用Decoderを区別した理由

「BMPやSTLのFileを読み取る」という機能だけを取り出せば、必ずしもRenderer固有ではない。この観点から、Loaderを`engine`直下の汎用入出力機能へ置く案や、Third-party Decoderと同じ領域に置く案も検討した。

ただし、現行のGLCE Loaderが実際に提供する契約は、単なるByte列のDecodeにとどまらない。BMP LoaderはExternal BMPを読み込み、必要なPixel Normalizationを行い、Texture Domainが定義する`texture_resource_info_t`を組み立てる。STL ASCII Loaderも、External Geometry FormatからGLCE内部のVertex Representationを構築し、受理に必要なValidationを行う。いずれも**Renderer Resourceを構成するためのExternal Trust Boundary**として振る舞っている。

責務の区別は次のとおりである。

| 区分 | 主な責務 | 現時点の判断 |
|---|---|---|
| 汎用Decoder / Parser | 外部Formatから一般的なDecoded Representationを得る。Renderer固有ResourceへのAdmissionは前提としない | 将来の独立利用が具体化するまで独立層を設けない |
| GLCE Renderer Resource Loader | 外部Formatを受理し、現行Renderer Resourceに適したNormalized Data / Candidateを生成する | `renderer/resources/loaders/`に配置する |
| Resource Pipeline | Load、CPU/GPU Resourceの構築、Registry Registration等のTransactionをOrchestrateする | `renderer/resources/pipelines/`に配置し、Loaderと役割を分ける |
| Third-party Decoder | 外部Projectから導入した実装そのもの | `vendors/`等のThird-party管理領域の対象候補。GLCE固有Loaderとは所有・保守の性質が異なる |

仮に`stb_image`や`tiny_obj_loader`等を採用しても、外部ライブラリ本体と、それを呼び出してGLCEのResource Contractへ接続するLoader / Adapterは別の責務である。`vendors/`という名称はThird-party Codeの管理先を例示するものであり、本Decisionが既存の`vendors/` Directoryや特定ライブラリの採用を宣言するものではない。

このため、**一般化の可能性だけで現行LoaderをRenderer外へ移したり、追加のDecoded Intermediate Typeを必須にしたりしない**。Renderer外でのDecode再利用、複数Format間の共有Normalization、Offline Asset Pipeline等の具体的な要件が成立したときに切り出しを再検討する。

### Rationale

- Semantic OwnerがTexture Domainであると明確にでき、BMP LoaderとTexture CPU ResourceでMetadata Conditionの別定義を作らずに済む。
- BMP LoaderはExternal Trust BoundaryのFormat受理、Decode / Normalize、Resource Metadata Candidate生成に集中できる。
- Texture CPU ResourceはOwned StorageのLifecycleと、成立済みMetadata Contractの保持を中心に設計できる。
- Texture MetadataのContractをSourceごとに複製せず、将来の別Format LoaderやProcedural Texture生成でも同じValidatorを利用できる。
- 追加の中間Representation、Conversion、Allocation / Copy、Validatorを現段階で増やさずに済む。

### Considered / Rejected Alternatives

- **MetadataのSemantic OwnerをLoaderへ移す。** GLCE内部ResourceのSemanticがExternal BMP Formatに従属し、別SourceのTextureで再利用しにくいため採用しない。
- **旧`resources/core`を残し、共通Resource型としてMetadataを保管する。** 実際にはTexture固有のSemanticsであり、旧Coreを形式上温存することになるため採用しない。
- **Loader専用のDecoded Image型を導入し、Pipeline側でTexture Metadataへ変換する。** 将来の再利用には有効だが、現状の単一利用では変換とValidationが重複するため先行導入しない。
- **LoaderとTexture CPU Resource双方でMetadataのField間Relationを検証する。** Semantic Ownerを複数化し、変更時の整合性維持を難しくするため採用しない。
- **Loaderの全責務をExternal Byte Parserだけに限定する。** 現行ではGLCE内部形式へのNormalizationを含むExternal Trust Boundaryとして自然に機能しており、全面再設計の実需がないため採用しない。
- **GLCE固有のLoaderを一律にEngine直下の汎用Decoder領域へ移す。** 既存LoaderはRenderer Resource Representationを直接構築しているため、汎用と称しても現在のSemantic Dependencyは消えない。独立した利用要件が生じるまで採用しない。
- **GLCE固有Loaderを`vendors/`等のThird-party Code管理領域へ置く。** External Libraryの原典実装と、GLCEが所有するResource Loaderの責務・保守境界を混同するため採用しない。

### Consequences

- `bmp_loader`はTexture Domainの型／Validatorを利用する。一方でTexture DomainはBMP File Format／Loader固有のDetailsを知る必要がない。
- GLCE固有のLoaderは現状Renderer Resource Domainに属するが、今後汎用Decoderを利用する実装へ変更しても、Third-party CodeそのものとResource Admission／NormalizationのGLCE側契約を混同しない。
- `stl_ascii_loader`等、Texture以外のLoaderも一律のDecoded Intermediate Typeへ強制移行しない。それぞれのExternal Formatと利用先に応じた責務を保つ。
- `loader_result_t`と`texture_result_t`は独立し、BMP LoaderのResultとTexture ResourceのResultの変換は必要な上位Boundaryで明示的に行う。
- Texture MetadataのValidityはTexture Domainが定義するが、File由来のCandidateをPublic Outputとして受理するTrust PromotionはLoaderの責務である。両者を混同しない。
- Dependencyの整理だけを理由に別途「Loader Architecture Refactoring」を今すぐ実装キューへ入れない。再検討条件が成立した際に独立Work化する。

### Revisit Conditions

- BMP DecoderをRenderer外の用途でも再利用する必要が現れた場合。
- 複数のLoader / Procedural Source間で、共通Decoded Image Representationや共通Conversionを設ける具体的利益が判明した場合。
- LoaderからTexture Domainへの依存により、現実にBuild・Test・Portability等の不利益が発生した場合。
- Decode／NormalizeとTexture Resource Admissionの責務を分離する必要が、Streaming／Offline Conversion／Asset Pipeline等の実装で具体化した場合。

### Related Decisions / Documents

- 2026-10-15「Renderer Resource Directory Consolidationの実装完了とResource Core廃止に伴うResult Domain再編」
- 旧Decision「Texture resource validation responsibilityをBMP Loader / Resource Core / Texture CPU Resourceへ分離する」— **責務の基本思想は維持し、MetadataのSemantic Ownerの所属のみ今回の判断へ更新した**。旧本文は当時の履歴として保持する。
- `glce_near_term_todo.md` — 次WorkではLoader全体の再設計を前提条件にしない
- `design_notes.md` — Texture Resource Lifecycle and Registration


---

## Decision: Mesh / Coordinate / Modelの責務を分離し、Model所有のPointer-based Coordinate Treeを採用する

**Status:** Accepted / Implementation Pending  
**Date:** 2026-10-16  
**Scope:** Renderer / Mesh / Model / Coordinate / Resource Reference / Tree Structure / Identity

### Decision

- Rendering Vertical Slice / Shader種別ごとの`line_mesh_t`、`point_mesh_t`、`ui_mesh_t`、`untextured_mesh_t`は、独立した非opaque Value Typeとする。当面はGeometry / Material / Texture等のRegistry Resourceの**IDによる非所有参照**を組み合わせる。`line_mesh_t`は描画色も保持する。Mesh自身はResourceの実体や登録IdentityのAuthority、Model Matrix / Coordinateを持たない。
- Mesh Value Typeは、現時点で独立したConstructor / Destructor / Getter / Setterを必要としない。型自身に検証すべき追加Semantic Invariantがなければ、`xxx_mesh_is_valid()`も機械的に設けない。Resource IDに対応する登録済みResourceの有無は、Registryを利用する側のOperation Contractで確認する。
- Mesh種別の独立性を保ち、`meshes/`配下に各`xxx_mesh.h`を配置する方向とする。対応する`.c`は処理を必要とするときだけ追加し、`meshes/core/`は共有される独立した責務が生まれるまで設けない。
- Modelは異なるMesh種別を混在させて構成し、**Model内のMeshの識別名`mesh_name`、Mesh参照群、およびCoordinateとのBindingを管理**する。Mesh Nameを`xxx_mesh_t`へ組み込まない。
- CoordinateはLocal Transformおよび親子関係を持つ空間的な基準点とし、**Meshを持たないCoordinateを有効とする**。一つのCoordinateへ複数Meshを関連付けられる。Coordinate NameはModel内での識別と外部からの参照に用いる。
- **ModelがCoordinate TreeのStorage / Lifetime Ownerとなる。TreeはPointer-basedとし、World Transformの階層合成にはDepth-First Traversalを使用する。** 兄弟関係は共通のParentから導出し、独立したSibling指定をConfigurationの前提条件にはしない。
- 当面はModel構築時にCoordinate Treeの親子構造を確定し、構築後はLocal Transformのみ変更可能とする。構築後のReparentingは未実装の将来機能とする。
- Coordinate Treeを持つ領域は、独立した汎用Spatial Systemを先行導入せず、当面`engine/systems/renderer/resources/model/`とする。`model/coordinate`と`model/model`のModuleを設け、複数種別のMeshを扱うModelを単一の責務領域としてまとめる。
- Camera / Light等は将来、Geometryを持たないCoordinateのWorld Transformを参照して配置を追従できる。CoordinateにCamera / Light ResourceのOwnershipを与えることは、このDecisionには含めない。

### Context

2026-10-15「MeshはShader別Value Typeとし、当面ModelがCoordinateを所有する」では、ModelがCoordinate配列を保持する候補とし、ID参照／Borrowed Pointer参照、MeshとCoordinateの関係、内部表現を未確定としていた。

その後、PenguinのBody / Head / Wingの階層操作に加え、Geometryを持たないCamera Anchor、Light Anchor、同一Coordinateへの複数Mesh関連付け、Model内部のMesh識別を必要とすることが明らかになった。また、Coordinate構造の可読性を優先し、配列で階層関係を表す案からPointer-based Treeへ方針を変更した。

Meshは描画Resourceの組合せ、Coordinateは空間的基準、Modelは両者の構成と関連付けという責務に分けることで、MeshのResource参照と空間的配置を一対一に固定しない設計とした。

### Rationale

- Pointerで親子関係を表現すると、Model内の階層構造とDepth-First Traversalを実装・観察しやすい。現段階では未計測のAllocation Costより構造の分かりやすさを重視する。
- Pointer-based TreeでもNodeごとの個別Allocationは必須ではない。将来Memory Layout / Allocation Strategyを変更する余地がある。
- 空Coordinateを許すことで、描画されない基準点もWorld Transformを持てる。Camera / Spot Lightの取り付けやロボット機構の基準点に利用できる。
- 同じCoordinateに複数Meshを結び付けられ、MeshのResource CompositionとModel内の配置を独立して変更できる。
- Mesh Name / Coordinate NameのAuthorityをModelへ集め、Geometry / Material / TextureのRegistration Identityを既存Registryへ残せる。
- 特定のMesh種別に共通のLifecycle / Validator / Configを強制せず、Vertical Sliceごとの実需に応じた機能追加ができる。
- 将来のSpatial System移管を想定した汎用Subsystemを、現在のModel構築の前提条件にしない。

### Considered / Deferred Alternatives

- **Array-based Coordinate Treeを最初から採用する。** Data LocalityやAllocation回数の面で利点があり得るが、現段階ではPointer-basedの構造的な明快さを採る。性能・Allocationが実問題となったら再検討する。
- **MeshとCoordinateを一対一に対応させる。** 空Coordinateや一つのCoordinateに複数Meshを結び付ける要求を表現できない。
- **Mesh自身にTransform / Mesh Nameを含める。** Resource CompositionとModel内のPlacement / Identityを混在させるため採用しない。
- **Model構築後のReparentingを今回実装する。** 現在のPenguin検証には不要であり、Transform維持方式、Cycle Prevention、Ownership / Lifetimeの契約が増えるため保留する。
- **`renderer/resources/coordinates/`をModelと同列のDomainとして設ける。** 現在のCoordinate TreeはModel-ownedであり、独立したSpatial Authorityを先行して仮定しないため、当面は`model/`に所属させる。
- **全Meshへ共通のMesh Registryや共通Configを必須化する。** Mesh Identityは当面Modelが管理し、既存Resource Registryとは異なる新たなOwnerの必要性が確認されていない。

### Consequences

- 2026-10-15の旧Decisionにある「Coordinate配列」および「Resource参照方式未決定」は、**当面のTarget Architectureについて本Decisionが更新する**。旧Decision本文は当時の検討履歴として保持する。
- Pointer-based Treeについて、Root / Parent / Child / Siblingの具体的なPointer表現、Node Storage、Construction / Destruction、World Transform Cache、TreeのCanonical Invariantは実装時に決定する。
- Meshの`uint16_t` Resource IDが未登録・解放済み・再利用済みかどうかは、Mesh Value自体の型だけでは判定できない。参照している間のResource Lifetime / Release Orderは、当該構築・描画経路のContractとして整理する。
- `line_mesh` / `point_mesh`は主にApplication由来の動的描画要求に用いるため、専用Mesh Configを当面設けない。`ui_mesh` / `untextured_mesh`のConfig型とValidatorはAsset利用経路の需要に応じて後から確定する。
- Tree構造の固定は今回の実装段階に限る。ロボットのリンク再接続等の具体的要求が生じれば、Runtime Reparentingを独立Workとして設計する。

### Revisit Conditions

- Coordinate NodeごとのAllocation、Traversal、Cache Localityが実際の性能・Memory制約となった場合。
- Runtimeで部位を取り外し、別の親へ接続するRobot / Assembly等のReparenting要求が具体化した場合。
- Physics / Collision / Scene等がRendererのModelから独立したCoordinate Authorityを必要とした場合。
- 複数Model間のMesh共有により、Modelを越えたMesh Identity / Lifetime管理が必要になった場合。

### Related Decisions / Documents

- 2026-10-15「MeshはShader別Value Typeとし、当面ModelがCoordinateを所有する」— CoordinateのRepresentationおよび当面のID参照方式を本Decisionで更新する。
- 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」
- `design_notes.md` — Mesh / Model / Coordinate / Render Packet Architecture
- `glce_near_term_todo.md` — R1 / R2、Coordinate ReparentingのDeferred項目

---

## Decision: Render PacketをShader別Draw Collectionとし、Model連携を先に検証してからOrchestrationを抽出する

**Status:** Accepted / Implementation Pending  
**Date:** 2026-10-16  
**Scope:** Renderer / Render Packet / Model Integration / Application Prototyping / Configuration / Module Extraction

### Decision

- Render Packetは**Shader / Mesh種別ごとのCollection**を基本構造とし、CollectionごとにCountと連続したDraw要素群を持つ方向とする。Renderer側はShaderを有効化してから、そのShaderに対応するDraw要素を順に処理する。
- Coordinate TreeのDepth-First TraversalによるWorld Transform導出と、Shader別のDraw順序は別責務とする。Render Packetは描画実行に必要なMesh Resource参照とModel / World Transformの対応を持てるようにするが、最終的なDraw EntryのRepresentation、Borrow / Copy、Transformの保持方法はR4で決める。
- 当面は半透明UI等のDraw Order要求のためにRender Pass一般化や複雑なSorting Mechanismを先行導入しない。必要になった時点でRender Packet周辺の描画順序・Pass設計を再検討する。
- Mesh / Coordinate / ModelのData Modelと連携を先に成立させ、**Application内で既存Geometry / Material / Texture Pipeline・Registryを使用してハードコーディングで構築・描画を検証**する。実際の責務・Contractを確認してから、必要な処理を適切なEngine Moduleへ抽出する。
- 新規`xxx_mesh_pipeline`とMesh Registryは当面設けない。ModelがMesh IdentityとBindingを扱い、Geometry / Material / Textureの実体・Registration Identityは既存Registryが管理する。新規Orchestration / Mesh Registryが必要かどうかは実際の処理から再評価する。既存Resource Pipelineを廃止する判断ではない。
- Mesh単体のConfiguration Fileは設けない。将来の**Model Configuration File**にMesh Resource参照、Mesh Name、Coordinate階層・Binding、Transform、Asset Path等を記述する構想とするが、Format / Loader / Config Value Typeの具体仕様はRuntime Modelの構築・描画検証後に決定する。
- 完成したRenderer / Model構造をもとに、`README.md`でModel / Mesh / Geometry / Material / Coordinateの概念・連携と、Vertical Slice、Model→Render Packet→Drawの関係を説明する独立したDocumentation Workを行う。READMEは実装済みのContractを説明し、未実装のTargetを実装済みとして扱わない。

### Context

当初はModel Configuration Fileの各`[mesh]`から`xxx_mesh_config_t`を生成し、専用Mesh Pipelineへ渡して下位Geometry / MaterialのPipelineを呼び出す構成を想定した。またMesh単位のConfiguration Fileは作らない方針を選んだ。

しかし、Line / Point Meshは外部FileのImportよりApplicationの動的要求から描画することが主用途である。さらに、Geometry / Material等のImport PipelineとRegistryは既に存在するため、Mesh Pipeline / Mesh Registryを先に設けると新しい責務を実装経験なしに固定することになる。

このため、まずPointer-based Coordinate Tree、空Coordinate、複数Mesh Binding、Penguinの部位操作を実際に動かし、ModelからDraw要求を得る経路を検証する順序へ変更した。

### Rationale

- Configuration FormatへRuntime Data Modelを従属させず、先に内部Contractと動作を確定できる。
- Applicationで構築手順を観察し、実際に必要なImport、Rollback、Resource再利用、Bindingの責務を発見できる。
- 既存のStateless Resource Pipeline / RegistryのOwnership Commitと、未確定のModel構築Orchestrationを混同しない。
- Shader別Collectionにより、Shader BindとDraw Loopをまとめられる。Treeの論理的な並びにGPU描画順を拘束されない。
- Modelの空間的StructureとFrame単位の描画最適化Representationを別々に設計できる。
- GLCEの中心概念が実際に成立してからREADMEへ明文化することで、理念を先行させず具体例でArchitectureを説明できる。

### Considered / Deferred Alternatives

- **初めから`xxx_mesh_pipeline`とMesh Registryを導入する。** Geometry / Material等の既存Resource管理との責務重複を確認できていないため保留する。
- **初めからModel Configuration File / Loader / Mesh Config Validatorを実装する。** Tree / Binding / Transform / Importの契約が未成熟な状態で外部Formatを固定するため後回しにする。
- **Model内のTree Traversal順をそのまま最終描画順とする。** Shader切替の集約とDraw Orderの独立性を損なうため採用しない。
- **全Render Pass / Alpha Sorting機構を先行実装する。** 現段階の描画検証に不要であり、半透明UI等の実需要発生時にPacket周辺から再検討する。

### Consequences

- 既存R1〜R5の段階的なIntegration計画を維持しつつ、R1 / R2ではModel構築・描画のApplication-side試行を優先する。R3では既存`render_resources`をFrontend OwnershipとStateless Mesh Rendererへ移管し、R4でShader別Render Packet / Frame Allocatorを接続する。
- Model構築時のResource Import順、既存Resource再利用の判断、途中Failure時のRollbackは、実装時にTransaction Boundaryを確認して設計する。
- Shader別Render Packetに必要なWorld Transformの転送形態、Mesh ValueのCopy / Borrow、Count / Capacity / Frame Lifetime、Alpha / UI Draw Orderは未確定のままとする。
- Asset RootのLocation Policyは既存どおりApplicationが所有し、Loader / Pipelineへ完成済みPathを渡す。Model Configuration File内のPath指定規則は後続Workで決める。
- READMEの中心概念・Vertical Slice / Rendering Flow説明は、R4〜R5の検証後に独立して着手する。Model Configuration Fileの完成はREADME着手条件にしない。

### Revisit Conditions

- Mesh Importが独立したTransactionを形成し、Application内での処理重複・Rollback Complexityが顕在化した場合。
- 複数Model間でMesh Value自身の独立登録・共有・Lifetime管理が必要になった場合。
- 半透明UI、Alpha Blending、Pass別描画、ShaderをまたぐStrict Ordering等が具体的に必要になった場合。
- Model / Packet間でDraw EntryのCopy Cost、Borrow Lifetime、Capacityが問題になった場合。

### Related Decisions / Documents

- 2026-09-12「Application↔Engineの当面の論理boundaryをapplication.h/.cとする」
- 2026-09-12「Resource Pipelineはstateless orchestrationとして維持する」
- 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」
- 2026-10-16「Mesh / Coordinate / Modelの責務を分離し、Model所有のPointer-based Coordinate Treeを採用する」
- `design_notes.md` — Mesh / Model / Coordinate / Render Packet Architecture
- `glce_near_term_todo.md` — R1〜R5、README Work、Deferred項目


---

## Decision: すべてのMeshを非共有のRender Modelへ所属させ、Coordinateに描画姿勢のAuthorityを集約する

**Status:** Accepted / Model Implementation Pending  
**Date:** 2026-10-17  
**Scope:** Renderer / Render Model / Mesh / Coordinate / Ownership / Instance / Visibility / Resource Reference

### Decision

- **すべての`xxx_mesh_t`は必ず一つの`render_model_t`に所属する。** Modelを経由しない独立Mesh描画を、Applicationから見た別の通常経路として設けない。`line_mesh_t`や`point_mesh_t`も例外としない。一時的なLine / Point描画が必要なら、Applicationが当該用途のRender Modelを構築・利用できる。
- `render_model_t`は、**Model Transform Treeと0個以上のShader別Meshを持つ、独立した描画用Runtime Object**とする。Coordinateだけを持つMeshなしModelも有効であり、Coordinate TreeにはMeshを持たないNodeが存在してよい。Rootを持つTree自体の必須条件は実装済み`model_transform_tree`のContractに従う。
- Model自身と、Modelに所属するMesh Instanceは**異なるModel間で同じ可変Objectとして共有しない**。同一の構築定義からModelを複数作る場合も、それぞれのModelが独立したCoordinate Tree / Mesh構成と表示状態を保持する。Modelを複数の利用ModuleがLifetime内でBorrowすることは、この「共有しない」の禁止対象ではない。
- MeshはShader別の非opaque Value Typeとして、Geometry / Material / Texture等の**既存Registry Resource IDによる非所有参照**、およびModel内で所属する**Coordinate ID**を持つ。Mesh自体は頂点Data / Resource実体を所有しない。Line Meshの現行Color表現等、描画種別固有の情報は別途保持してよい。
- Meshは`translate` / `rotation` / Model Matrixなどの独立したTransformを**持たない**。Local Translation / RotationはCoordinate Treeが唯一のAuthorityとなる。描画時のWorld Matrixは、Meshが参照するCoordinateから取得する。個別の位置や回転が必要なら別のCoordinateを設けて所属させる。
- 一つのCoordinate IDを複数Meshから参照してよい。**Mesh側がCoordinate IDを直接保持する**ため、以前検討したModel内の別個のMesh–Coordinate Binding Collectionは、現時点では設けない。Coordinate側もMeshの存在・種類を認識しない。
- Modelは自身が所有するMesh群とCoordinate Treeの構造的整合性（例：Meshの参照先Coordinateが存在すること）およびLifecycleを管理する。ただしGeometry / Material / Texture Resourceの登録Identityと実体Ownershipは既存Registryに残し、Modelはそれを移動・複製・解放しない。
- **参照中ResourceのLifetime / Release OrderはApplication側の利用Contract**とする。複数Modelが同じGeometry / Material / Texture IDを参照してよいが、利用中のResourceを先にReleaseしないことはApplicationが保証する。参照カウント、Model間のReverse Reference Tracking、Generation付きHandleを、このためだけに先行導入しない。
- Modelに必要なRuntime操作として、Coordinate Transform変更と**Meshごとの汎用的な表示／非表示の変更**を認める方向とする。AABBなど特定用途を表す専用操作をRender Modelへ追加しない。Mesh識別方式、VisibilityをMesh Field / Model-managed Stateのどちらで表すか、構築後のその他Mutation範囲は具体的APIを設計する際に決める。
- Meshの構成・参照・所属CoordinateとCoordinate Treeの親子構造は、**当面、構築後に固定する方向**とする。Runtime Reparenting、Mesh追加・削除、Resource参照の差替えを今回の必須機能にしない。ただし変更を永続的に禁止するDecisionではない。

### Context

当初、Shader別MeshはGeometry / Material等をまとめる非opaqueなValue Typeとして検討され、Coordinate IDとLocal TransformはMeshから切り離してModelが別のBindingを持つ構造を想定していた。さらに同一Meshを複数Modelで共有する可能性も残していた。

しかし、同じGeometry ID / Material IDを使用する別のMeshを複数Modelへ配置する場合でも、重いVertex / GPU ResourceはRegistryで共有できる。ModelをまたいでMeshの可変状態まで共有すると、所属Coordinate、表示状態、Lifetime / Mutationの意味が複雑になる。

また、Line / PointだけにModel外の描画経路を認めると、ApplicationがMeshとModelのどちらを描画単位として使うべきか用途別に判断する必要が生じる。Penguinの部位、RobotのLink、LiDARのPoint Cloud、Camera / Light用の空Coordinate、診断用Line等を同じ概念で扱うことを重視し、全Meshの所属をRender Modelに統一した。

一度はMeshへCoordinate IDに加えてLocal Translation / Rotationを持たせる案も検討した。しかしLocal Transformは既存`model_transform_tree`が保持しているため、Mesh Local Transformを追加するとSpatial TransformのAuthorityが二つになる。Coordinate TreeにTransformを集約する判断とした。

### Rationale

- Applicationにとって描画要素の所属関係を一種類の概念（Render Model）に統一できる。Temporary Line / Pointだけ別のMesh APIや別の描画Architectureを設けずに済む。
- Geometry / Material等を共有したまま、ModelとMeshの可変状態は各描画Objectへ閉じられる。RobotやPenguinの個体ごとに異なる姿勢を表現できる。
- ModelごとにCoordinate Treeが独立しているため、Application固有の姿勢を各Modelへ適用する際の干渉を避けられる。
- MeshへCoordinate IDを持たせることで、別のBinding Collectionを重複管理せず、複数Meshが同じCoordinateを参照できる。
- 空Coordinateを許すことで、描画Resourceを持たないCamera / Light AnchorをSpatial Referenceとして利用できる。ModelがCamera / Light自体を所有する必要もない。
- TransformのAuthorityをCoordinate Treeへ集中でき、Mesh別TransformとTree Transformの重複を避けられる。
- ResourceのOwnership AuthorityをRegistryに維持し、ModelをResource Registration / Import / ReleaseのOwnerへ変えずに済む。
- 未計測のModel / Coordinate Allocation Costを理由として共有Definition / Instance機構やFrame専用描画経路を先行導入しない。まず実装・検証・計測する。

### Considered / Rejected or Deferred Alternatives

- **Modelをまたいで同一Mesh Instanceを共有する。** Meshが特定ModelのCoordinate / Visibilityを参照する以上、共有Mutable Stateの意味とOwnershipが複雑になる。Registry Resourceだけ共有する。
- **`render_model_t`をDefinitionとInstanceの二層に分離する。** 個体ごとのCoordinate Treeを持つ独立Modelと、共通Resource参照 / 再利用可能なConstruction Configで現在の要求を表現できる。大量Instancing等の計測可能な問題が生じたら再検討する。
- **Line / Pointの一時描画のみModel外で行う。** Applicationが描画対象種別に応じて別の概念を選ぶ必要があるため採用しない。一時Model生成・破棄のAllocation Costは実測する。
- **MeshにCoordinateから独立したTranslation / Rotationを持たせる。** Treeが既にTransformを持つためAuthorityが二重化する。必要なら子Coordinateを追加する。
- **Modelが別個のMesh–Coordinate Binding Collectionを所有する。** MeshがCoordinate IDを直接持てるため、現在の要件では重複Representationになる。
- **Model自身が参照中Resourceを管理・自動延命する。** Registry OwnershipとApplicationの参照Lifetime Contractで当面成立するため、Reference CountやReverse Lookup等を先行導入しない。
- **Runtime Reparenting / Meshの追加・削除を最初から必須にする。** 現時点の利用例は固定されたSpatial / Mesh構造と、Transform / Visibility変更で表現できるため保留する。

### Consequences

- `render_model_config_t`は、独立したModelを繰り返し構築可能な入力定義として設計する。各Mesh Configと所属Coordinateを対応付ける具体的Field、Coordinate Root表現、Resource Name / IDの解決Boundaryは**未確定**とする。
- `render_model_t`がModel Transform Treeを所有するためのConstructor / Destructor / Failure Rollbackを設計する。`model_transform_tree`のStorage / Validation責務とModelのLifecycle Ownerを区別する。
- MeshのID / Nameによる識別、各種別のStorage、Visibility Field / API、ModelのCanonical ValidationとGetter APIは実装段階で決定する。
- 既存Shader別Render Packetの方向性は維持し、将来PacketへMesh参照と所属CoordinateのWorld Matrixを渡せるようにする。Tree Traversal OrderとShader別Draw Orderは区別する。
- Line / Pointに関する既存のApplication→Render Resource直接描画経路は移行完了までの既存実装であり、今後のApplication-facingな通常描画Modelでは必ずRender Modelを用いる。
- GeometryのVerticesやColor等の内容更新はGeometry Resourceの責務であり、既存Meshが同じGeometry IDを参照したまま更新できることを目指す。Geometryの動的更新APIは別の設計課題とする。

### Revisit Conditions

- 同じModel定義を大量配置する際のCoordinate Tree / Mesh Metadataの重複が計測可能な問題となった場合。
- Temporary Model生成・破棄の頻度によってAllocation / Draw Performance上の問題が明確になった場合。
- Animation / Simulation / Asset Streaming等でMeshの動的追加・削除やResource参照差替えが必要となった場合。
- Geometry / Material等のHot ReloadやCross-owner Resource SharingによりApplication単独のLifetime Contractでは管理が困難になった場合。
- Renderer外のPhysics / Scene等がTransform Authorityを必要とし、Spatial Ownershipの境界を再検討する場合。

### Related Decisions / Documents

- 2026-10-15「MeshはShader別Value Typeとし、当面ModelがCoordinateを所有する」— **MeshにCoordinate IDを持たせずModel側Bindingを検討する旧方針を変更する**。Shader別Value Type / Registry非所有参照という基本部分は維持する。
- 2026-10-16「Mesh / Coordinate / Modelの責務を分離し、Model所有のPointer-based Coordinate Treeを採用する」— **独立Binding Collectionを必要としないようMesh–Coordinate関係の表現を更新する**。Pointer-based Tree / 空Coordinate等は維持する。
- 2026-10-16「Render PacketをShader別Draw Collectionとし、Model連携を先に検証してからOrchestrationを抽出する」
- `design_notes.md` — Mesh / Model / Coordinate / Render Packet Architecture
- `glce_near_term_todo.md` — Renderer / Model Integration Work

---

## Decision: Application固有Modelが姿勢と診断描画の意味を管理し、Render Modelを汎用描画構成に限定する

**Status:** Accepted / Application Integration Pending  
**Date:** 2026-10-17  
**Scope:** Application Model / Render Model / Robot Posture / AABB Visualization / Line / Point Geometry / Debug Rendering

### Decision

- `render_model_t`は、Penguin、Robot、LiDAR、Camera等のApplication固有のSemanticを知る必要がない。**機種固有の構造、関節角・機構寸法・姿勢状態と、各Coordinateの対応**はApplication側のModel Moduleが管理する。
- **機種・種類によって異なるPostureのData StructureはApplication Domainに属する。** `penguin_posture_t`、建設機械のLink / Joint状態等を、汎用の`render_model_t`へ統合したり、全機種共通のPosture SchemaをRendererから要求したりしない。機構寸法のような比較的固定的なParameterと、角度等の変化するPostureは、機種固有Domainで必要に応じて区別する。
- **Render ModelのCoordinate Treeは、Application固有Postureの型やFieldを知らずに、Local Translation / Rotationと親子関係を汎用的な描画配置情報として保持できる。** Postureと無関係な配置や空Coordinateも扱える。ApplicationからPostureを反映して座標を更新する場合も、Render Model側の表現は汎用Coordinate Transformのままであり、Posture Structureを複製・保持しない。この「独立」は、描画位置がPostureの値に影響されないという意味ではなく、**描画配置のRepresentation / APIが機種固有PostureのSchemaへ依存しない**という意味である。
- Application側には用途 / 機種ごとのModel Module / Object（例：`penguin_model_t`）を設けてよい。`penguin_model_posture_apply(...)`相当のOperationがPenguin固有のPostureを解釈し、対象`render_model_t`のCoordinate Transformを更新する。`penguin_posture_t`やRobot PostureのFieldはRender Modelへ持ち込まない。
- **Application固有ModelはPosture反映だけでなく、機種に意味のある操作Commandを提供する。** 例えば`link1_swing_right`等のAPIで対象Linkの動作を要求できる構成とし、対応するPosture Stateの更新とRender Modelへの反映をApplication Domainの責務に置く。操作レバー / Keyboard等の物理入力とCommandの割り当てもApplication側で扱い、EngineのRender Modelへ入力装置や機種固有動作を認識させない。Commandの引数、角速度 / 増分 / 時間の扱い、機械的制限、Posture反映のタイミングは実装時に決める。
- 同じ機種を複数描画する場合、各個体は個別のApplication状態と個別の`render_model_t`を持ち得る。機種共通のGeometry / Material / Configは共有できる。Application固有Modelが`render_model_t`を直接所有するかBorrowするか、具体的なPublic API Signatureは実装時に決定する。
- **AABBはApplication側の診断・可視化上の意味であり、Render ModelにAABB専用の表示状態、作成API、`aabb_enable()`等の固有Operationを導入しない。** Render Modelにとっては他のLine Meshと同じ描画要素として扱う。
- Application固有Modelが、どのLine Mesh群がどの部位のAABBかを把握し、Render Modelの汎用Mesh Visibility操作を使って表示・非表示を切り替える。`penguin_model_body_aabb_enable(...)`等の部位別APIを設けることは許容するが、具体的な粒度・名称はApplication側の判断とする。
- AABBの表示検証では、Geometryの**Local AABBを対応CoordinateのWorld Transformで回転・移動したBoxとして描画してよい**。必ずしも再計算されたWorld Axis-Aligned Bounding Boxを表示する必要はない。両者の意味は混同しない。
- 点群表示でも、LiDARなどの**位置・姿勢はCoordinate**、点群の頂点数・頂点座標・頂点色情報は**Point Geometry Resource**、Point Geometry参照と所属Coordinateは**Point Mesh**として区別する。点群Dataの更新だけを理由にCoordinate TransformやMesh構成を更新しない。
- **Line Geometryへの頂点単位Color導入は有力な将来改善案とするが、今回の確定実装仕様にはしない。** Voxel Grid等で大量の色付きLineを一様ColorのLine Meshに分割するとMesh / Draw数が増えるため、Point Geometryと同様にGeometry側にColorを置く案を別Workで検討する。現行Line MeshのColor FieldやShaderを今回直ちに変更しない。

### Context

Render ModelにAABB表示を一律に管理させる案や、Applicationが表示要求時にGeometryからLine Meshを都度生成する案が出た。しかし、AABBはRendererの普遍的なModel Semanticではなく、PenguinのBody / WingなどApplication側で選択する可視化対象である。Engine側へAABB専用機能を足すと、通常のLine Mesh描画と二重の描画経路・APIを持つことになる。

また、Robotの姿勢状態には`link1_angle`、`link2_angle`等の機種固有情報があり、これらはRobot個体ごとに異なる。`link1_length`等は構造的・幾何的パラメータであり、実際の運動学とCoordinateの対応はApplication固有のModel Moduleだけが理解できる。機種ごとに関節数・動作方向・機構寸法・可動範囲が異なるため、共通のPosture表現をRender Modelへ組み込むことは適切でない。一方、描画用Coordinate Treeはそれらの差異に依存せず、同じTranslation / Rotation / Parent関係という汎用Representationで完成した描画配置を保持できる。

さらに、建設機械等の操作を想定すると、姿勢Snapshotを`posture_apply`で反映するだけでは、操作レバーから「Link 1を右へSwingする」といった機種固有の動作を直接要求しにくい。入力装置のSignalと機種固有のCommandをApplicationで対応付け、Postureの変化をRender ModelのCoordinateへ反映できる必要がある。

LiDAR Point CloudやVoxel Gridの可視化も検討した。点群のData更新をModel構造変更と同一視しないこと、Voxel毎に異なる線色を出すため大量のLine Meshが必要になる現行形式の制約を、将来のLine Geometry改善候補として把握した。

### Rationale

- Applicationの機種固有SemanticをRendererへ逆流させず、Render Modelを再利用可能な描画構成とする。
- Robot PostureをApplication側のAuthorityとし、Render ModelのCoordinate Transformはその可視化結果として更新できる。意味を持つ元状態と描画表現を重複した正本にしない。
- Render Modelが機種固有PostureのField / Typeを知らなくても配置情報を保持・照会できるため、特定機械の運動学に依存しない汎用Rendererを維持できる。
- Posture Snapshot反映と、操作レバーなどから発生する機種固有の動作Commandを、Application側の明示的な操作として提供できる。Input Mappingと描画用Coordinate変更を直接結び付けず、機械固有の動作SemanticsをApplication Modelへ閉じられる。
- AABBは通常のLine Meshと共通の描画経路を利用でき、専用Renderer Object / AABB Flag / 特殊Mesh Categoryを増やす必要がない。
- 部位単位の可視化操作をApplicationに置くことで、個別Geometryの診断意図を自然なApplication APIへ閉じられる。
- Local AABBを回転したBoxとして表示すれば、Transform Treeの動作確認を直接行える。
- LiDARの動的Point DataとCoordinate Treeの位置姿勢を別々に変更・管理でき、Modelを再構築する理由を減らせる。
- Voxel Gridで多数の色付きLineを扱う場合、頂点色をGeometryで保持すればMesh / Drawの過剰分割を避けられる可能性がある。まず将来のResource / Shader設計課題として分離する。

### Considered / Rejected or Deferred Alternatives

- **`render_model_t`へAABB専用表示Option / APIを設ける。** 特定Applicationの診断Semanticを汎用Engine Objectへ持ち込むため採用しない。
- **ApplicationへAABB専用の新規Geometry / Mesh生成を描画切替のたびに要求する。** 利用側APIが複雑になるため、既存Line MeshのVisibility制御を利用する方向とする。ただしAABB用Resourceをいつ用意するかは実装時に決める。
- **AABBのために必ずWorld AABBを再計算する。** 初期の姿勢検証にはLocal AABBをTransformした表示で足りる。厳密なWorld AABBが必要な用途は別途指定する。
- **Render ModelへRobot JointやPenguin部位のSemantic、機種共通のPosture Structure、`link1_swing_right`等の機械操作Commandを持たせる。** EngineがApplication固有の運動学 / Link識別 / 操作意味を知るため採用しない。
- **操作レバー等のInput SignalからRender ModelのCoordinateへ直接変換する。** 機種固有の動作・Postureの意味が入力処理とRender Model APIへ散在するため、Application固有ModelのCommand / Posture反映経路を利用する。
- **LiDAR点群更新時にMeshやCoordinateを必ず作り直す。** 更新対象はGeometry Dataなので不要とする。
- **現行のLine Mesh一様色方式をVoxel Grid用にそのまま大量Meshへ拡大する。** 性能と管理コストの懸念があり、頂点Color方式を今後比較する。

### Consequences

- Application側に`penguin_model`等の固有Model Moduleの導入を検討・実装する。`penguin_model_t`のField、`render_model_t`とのOwnership / Borrow、Posture / AABB API Signatureはまだ決まっていない。
- `posture_apply`に加えて、`link1_swing_right`等の**機種固有Command API**をApplication ModelのInterface候補とする。Commandは機種固有状態の変更を意味し、描画側には汎用Coordinate操作だけを要求する。実際にPostureをどのObjectが所有するか、CommandとApplyを同時 / 分離実行するか、動作量 / 制限 / 時間積分のSemanticsは今後設計する。
- Model Config / `render_model_t`の設計で、機種固有Postureの型・Fieldを必要条件にしない。`render_model_t`はGenericなCoordinate配置だけで有効に成立する。
- Render ModelはMeshの汎用Visibility操作と、Coordinate Transformの取得・変更に必要なAPIを提供する方向とする。どのMeshがAABBかという意味や部位別まとめ方はApplicationが保持する。
- AABB Geometry生成は既存の`geometry_conversion`等の責務に従い、異なるTopologyのRender Resource同士へ直接依存を設けない。AABBをどの段階でLine Geometry / Line Meshに変換するかはApplication Integration時に決める。
- Point Geometryの動的更新、Line Geometryの頂点Color対応、Voxel Grid RepresentationおよびDraw BatchingはModel Config / Constructorの完成条件にしない。
- Resource LifetimeのApplication Contractは、Modelが別個のResource Lifetime Authorityを作らない前提で維持する。

### Revisit Conditions

- AABBや他のDiagnostics描画に、Application固有Modelだけでは扱いにくい共通Operationが多数現れた場合。
- Application固有Model間で明確に同一のPosture-to-Coordinate変換機能が複数発生した場合。
- LiDAR / Voxel Grid等の大規模可変Geometry更新・Draw最適化要件が具体化した場合。
- 運動学のAuthorityがApplicationから独立したPhysics / Robotics subsystemへ移行する具体的要件が生じた場合。

### Related Decisions / Documents

- 2026-09-12「Application↔Engineの当面の論理boundaryをapplication.h/.cとする」
- 2026-09-12「異なるGeometry topology間の変換は独立conversion moduleへ置く」
- 2026-10-17「すべてのMeshを非共有のRender Modelへ所属させ、Coordinateに描画姿勢のAuthorityを集約する」
- `design_notes.md` — Mesh / Model / Coordinate / Render Packet Architecture
- `glce_near_term_todo.md` — Renderer / Model Integrationおよび将来のPoint / Line Geometry Work

---

## Decision: Render Model Configを独立Moduleに集約し、具体的なInitialize APIを設ける

**Status:** Accepted / Implementation In Progress  
**Date:** 2026-10-18  
**Scope:** Render Model Config / Config Lifetime / Module Boundary / API Simplicity

### Decision

- `render_model_config_t`、Coordinate Config、Shader別Mesh Config型とそれらの構築・解放APIを、`render_model/config/`配下の`render_model_config.h/.c`へまとめる。Runtime Modelの`render_model.h/.c`には、Configの具体的なStorage / Owned String操作を持ち込まない。
- `render_model.h`のConstructor等は`render_model_config_t*`をPointerとして扱うため、必要に応じてForward Declarationで型依存を表現する。
- Model Configにも明示的な`render_model_config_initialize()` / Deinitializeを設ける。Countと配列Pointer群を受け取るため引数は多いが、単なる引数削減のために`render_model_config_desc_t`等の追加Descriptor型は設けない。
- Resource Name等の`choco_string_t`を持つUntextured Mesh Configには、Stringを構築・破棄する独立したInitialize / Deinitializeを提供する。一律のAPI形状を揃えることを目的に全Configへ同じ処理を強制しない。
- `render_model_config`は外部Model Configuration Fileそのものではない。C側のOwned Config / Model構築経路を先に成立させ、File Parser / LoaderとExternal Formatは後続Workに残す。
- Configで受理するRotationは`0〜360`度とする。External File由来浮動小数点のNaN / Infの検証責務はLoader側のExternal Trust Boundaryに置き、Config Initializerへ機械的に重複させない。

### Context / Rationale

Sample Robotの構築で、各Untextured Mesh Configが複数のOwned Stringを保持し、初期化途中のCleanupも必要になることが明確になった。一方、`render_model.h`へConfig / Runtime型と両者のAPIを集約し続けると、独立したOwned ConfigurationのLifecycleとRuntime ModelのLifecycleが混在する。

そこでConfig Moduleを独立させる。ただし一つのConfigを構築するためにCount / PointerをまとめるだけのDescriptor Structureを別途導入すると、類似したField定義と型の往復が増えて見通しが悪くなる。GLCEでは意味を持たない新たなAbstractionを引数の数だけを理由に増やさず、Operation Contractの明確な通常の引数を採用する。

### Alternatives / Consequences

- `render_model_config_t`だけを`render_model.h`へ残す案は、Config自身のDeinitialize / Validation / Owned StorageのSemantic OwnerがConfig Moduleに属するため採用しない。
- Init専用Descriptorを追加する案は、Borrowed InputとOwned Configの区別を型で表現できる利点がある一方、現時点ではField重複とAPI構造の複雑化に見合う必要性がないため採用しない。
- Line / Point / UI Configの詳細InitializeやValidatorの実装は、まずUntextured MeshによるModel Import / Runtime確認後に進める。
- Config Constructor / DeinitializerのOwnership Transfer / Copy、Failure Cleanup、Output Contractの詳細は各Moduleの公開Contractと実装で定める。追加の共通Validation Policyを本Decisionに導入しない。

### Related Documents

- `glce_near_term_todo.md` — M1 / M4 / M5
- `design_notes.md` — Render Model Config / Model Import / Resource Lifecycle

---

## Decision: Model ImportをResource Dependency解決の入口とし、Model単位のReference LifecycleをRegistry Reference Countingで管理する

**Status:** Accepted / Implementation Pending（Model単位のReference Lifecycleは維持。Resource Managerを介するAcquire / Release、4種Geometryへの横展開と実装順序は2026-10-19 Decisionで更新）  
**Date:** 2026-10-18  
**Scope:** Model Import / Model Pipeline / Application Renderer / Geometry・Material Registry / Reference Counting / Resource Identity / Asset Location

### Decision

1. **Model ImportをResource Dependency解決の正式なOperationとする。** Applicationは`render_model_config_t`を用意してModel Importを要求するだけでよく、Modelが必要とするGeometry / Materialの一覧を事前に解析して個別Importする必要はない。Resource Nameを解決し、未登録のResourceのLoadを開始するのはModel Importの内部責務とする。これは登録済みResourceの組合せだけを生成する単なる`model_create`ではなく、実際に外部ResourceのImportを含むため、`import`という名称を採用する。
2. **正式な`renderer_frontend`を今は新設しない。** 現行`application_renderer`がModel Import / Model Releaseの公開窓口と、既存`render_resources`の長寿命Ownerを暫定的に担う。将来のRenderer Frontendへ移行した後もModel単位のAPI Semanticsを維持する。2026-10-15のFrontend直接Ownership / Stateless Mesh Renderer Targetは取り消さず、後続Workで実装する。
3. **Model PipelineをModel単位のStateless Orchestrationとして設ける。** Application Rendererから、Configに基づく各ResourceのAcquire、Model構築、途中失敗時のRollback、Model Release時の参照返却を委譲する。既存のGeometry / Material Resource Pipelineは各ResourceのLoad / Upload / Registerに引き続き使用する。Model Pipelineは長寿命RegistryやResource本体を所有しない。
4. **RegistryへのBorrowを公開しない。** Model Pipelineは、対応するRender ResourceのAcquire / Release APIからResource IDを得る。Render Resourceは自身が所有するRegistryと既存Resource Pipelineへ処理を委譲する。Registryの内部PointerやReference CountをApplication / Modelへ公開しない。
5. **ModelはResource実体ではなく参照権を持つ。** Mesh Valueは従来どおりGeometry / Material等のRegistry IDによる非所有参照を保持し、RegistryがResource Identityと実体Ownershipを保持する。Model Importの成功によってModel単位の参照権を確立し、Model Release時にすべて返却する。Applicationへ個別Geometry / MaterialのRelease列挙やConfigの再解析を要求しない。
6. **Untextured Geometry / Material RegistryにReference Countingを導入する。** 既存Resourceを取得した場合は参照数を増やし、未登録ならLoad / Registerしたうえで最初の参照を確立する。参照を返却し、Countが0になったとき、RegistryのOwnership / Resource Pipelineの既存解放経路で実Resourceを解放する。Countの値を判断するのはRegistry / Render Resource側であり、Render Model自体はCountを参照して解放要否を判定しない。
7. **Referenceの単位はModelごとの重複しないResource参照とする。** 同じModel内の複数Meshが同一Registry Resourceを使用しても、そのModelは1参照を持つ。別Modelが同じResourceを使用した場合はModelごとに1参照を持ち、最後のModelがReleaseされるまでResourceは存続する。内部の参照集合の表現・重複排除Implementationは未確定とする。
8. **Model ImportをTransactionalに扱う。** 途中で失敗した場合はそのImportで取得済みの参照を返却し、既存Modelの参照するResourceは維持する。新規ImportしたResourceも最終参照が消えれば解放される。Failure Semanticsは`validation_policy.md`に従い、`DATA_CORRUPTED`確定後の通常Rollbackは行わない。
9. **Resourceごとに相対Subdirectoryを指定する。** 1つのModel内で参照するGeometry / Materialが異なるSubdirectoryへ存在し得るため、Model Config全体またはModel Importへ単一のSubdirectoryを指定する方式は採用しない。Untextured Mesh ConfigではGeometryとMaterialそれぞれのResource NameとSubdirectoryを区別する。Asset Root / Deployment LocationのAuthorityは従来どおりApplicationに置く。
10. **Registry Keyには正規化した`Subdirectory + Resource Name`の複合Stringを使用する。** Registryの2 Field Key構造への変更は行わず、現在のStringによる検索・登録機構を再利用する。同じ正規化Keyは同じRegistry Resource Identityを表し、別Subdirectoryの同名Resourceは別Identityとする。Registry KeyとLoaderに渡す完成済みFile Pathを混同しない。

### Context

従来のApplicationは個別のGeometryやMaterialをImportし、Resource IDを取得して描画処理へ渡していた。この段階ではApplicationがResourceの利用関係を知っていたため、Release順序を明示的なApplication Contractとして成立させ、Reference Countingを追加しない判断が合理的だった。

しかし、Render ModelにCoordinate Treeと複数Meshを持たせ、Model ConfigだけをApplicationから渡してModelを構築する段階で事情が変化した。**ApplicationはModel Importの前にどのGeometry / MaterialをImportすべきかを知る必要がなく、知らないまま利用できることがModel抽象化の価値となった。** そのため、Applicationへ事前Import / 個別Releaseを要求すると、ApplicationはModelの内部Resource構成を再解析しなければならず、Config / Modelによる隠蔽が実質的に失われる。

さらに、複数Modelが同じGeometry / Materialを参照する場合、個別ModelのDestroyに連動したResource解放と、他Modelの参照存続を両立する必要がある。Renderer Lifetimeまで全Resourceを保持すれば解放時の衝突を避けられるものの、Modelが不要になってもResourceを保持し続け、Application側のModel単位Lifecycleと一致しない。Applicationが手動で利用関係を数える方式も同じ利用負担を残す。

また、Registryの従来のResource NameのみをKeyにすると、`sample_robot/part_a/base`と`robot_primitive/part_b/base`のような異なるLocationの同名Resourceを区別できない。反対に同一のAssetは複数Modelで重複Importせず共有したい。そこでSubdirectoryとResource Nameの組をLogical Identityとし、既存の単一String Key Registryを維持する方法を採用した。

### Rationale

- Applicationが必要Resourceの列挙・取得・Release手順を知る必要がなくなり、Model Import / Releaseの抽象化を保てる。
- 同じModel Configから独立した複数Modelを生成しつつ、Geometry / Materialの実体だけを共有できる。
- RegistryをResource Identity / Ownership / Reference CountのAuthorityとして維持し、Render ModelがRegistry内部情報やCPU / GPU解放の詳細を持たずに済む。
- Reference Countの0到達で共有Resourceを安全に解放でき、Renderer終了まで不要Resourceを保持し続ける必要がない。
- 既存Geometry / Material PipelineのImport Transactionと、Model単位の複数Resource Acquire / RollbackをOperationの粒度によって分離できる。
- `application_renderer`を暫定的なFacadeとして使い、将来のRenderer Frontend移行前にModelのConstruction / Lifecycleを実装・検証できる。
- Registry Entryと既存のName Search APIを複雑化せず、Subdirectory違いによるName Collisionを回避できる。
- Applicationが持つAsset Rootの配置Policyと、個々のModel Configが持つ相対的なResource Locationを分離できる。

### Considered / Rejected Alternatives

- **Renderer Lifetimeで全Resourceを保持する。** Modelを解放してもResourceを保持し続け、Model単位の自然なResource Cleanupが成立しない。Applicationが代わりに明示ReleaseするとModelの内部構成を知る必要がある。
- **Applicationによる明示的なResource Lifetime / Release順序の管理を続ける。** Model Configから依存Resourceを再走査して個別Releaseするような複雑な利用契約になり、Model Importの隠蔽を損なう。
- **登録済みIDを`id_get()`で問い合わせ、未登録を`BAD_OPERATION`とみなしてImportする。** 未登録はModel Importでは正常なCache Missであり、実際のOperation Failureと分離したい。通常の検索をエラーとして扱うAPIをそのまま流用しない。
- **`is_exist()`と`id_get()`を別々に呼ぶ。** 検索とID取得が重複するため、Reference Countを伴うAcquire Operationに統合する方が明確。Registry内の検索Helper等の実装分離は妨げない。
- **RegistryへのPointerをBorrowしてModel Pipelineから直接操作する。** Registryの内部構成とMutation / LifetimeをModel Pipelineへ露出するため採用しない。
- **ModelがReference Countを直接取得し、0なら個別Resourceを解放する。** Release可否のAuthorityをModelへ漏らし、Registryの参照状態を二重に管理するため採用しない。
- **Model Config全体またはModel Importへ単一のSubdirectoryを渡す。** 同じModelでResourceごとに別Subdirectoryが必要になるケースを表現できない。
- **Registry EntryへSubdirectoryとResource Nameを別Fieldで追加する。** 既存単一String KeyによるRegistryを変更せず、正規化Composite Keyで必要なIdentityを表現できる。
- **File Full PathそのものをRegistry Identityとする。** Application固有のAsset RootやDeployment Locationを論理的なResource Identityへ混入させる。相対Subdirectory + Nameと、Load時の完成済みPathを分離する。

### Consequences / Implementation Boundaries

- **まずUntextured MeshのGeometry / Material Registryから着手する。** Texture / UI / Line / Point等に新しいCount Semanticsを機械的に要求しない。MaterialからTextureへの参照Lifetimeは、当面既存の個別Contractを維持する。
- Render ResourceのAcquire APIは、既存Resourceの再利用と未登録時の既存PipelineによるImportを一つの正常Operationとして扱い、Resource IDを返す。ReleaseではModelが保持していた参照権を返却する。
- 既存の個別Import / Release APIが新しいReference Countを迂回してResourceを破壊しないよう、公開契約・移行順・Direct Importで取得する参照の扱いを再設計する。従来APIの廃止や改名はこのDecisionでは確定しない。
- Model Pipelineが同一Model内の参照を重複なくAcquire / Releaseできるよう、参照集合の格納場所、取得順、Cleanup順を実装時に決める。Model単位Release以外に`render_model_destroy()`だけを呼んで参照権が失われる経路を作らない。
- Referenced ResourceのIDがRegistryに再利用され得る既存モデルは維持するが、Model存続中のReference CountがそのIDの解放を防止する。Generation付きHandleは今回の前提条件にしない。
- Asset Root PolicyはApplicationに維持する。`fs_path`でのlexical path constructionと、Composite Keyの正規化規則を混同しない。`.` / `..`、空Subdirectory、区切り文字、case、Extension、Keyの衝突回避方式および入力ValidationはM1で定義する。一般用途のFilesystem Canonicalization / Symlink解決をこのDecisionで新設しない。
- Count Overflow、Reference Count 0での登録解除が失敗した場合、Importが途中で失敗した場合のRecoverable Cleanup、`DATA_CORRUPTED`に対するFail-stop、Registry Deinitialize時の未解放参照の扱いは該当ModuleのAPI Contract / Validation Policyに従い実装時に確定する。
- `render_model_config_t`およびUntextured Mesh Configの入力構築を先に完成させ、External Model Configuration File / Loaderは別Workに残す。Line / Point / UI Configの詳細もUntexturedでの動作確認後に扱う。

### Prior Decisionsとの関係

- **2026-10-17「すべてのMeshを非共有のRender Modelへ所属させ、Coordinateに描画姿勢のAuthorityを集約する」**の「参照中ResourceのLifetimeはApplication Contract」「Reference Countingを先行導入しない」を、**Model ImportでのGeometry / Material参照に関して変更する**。Model専属Mesh、Coordinate Transform Authority、RegistryのResource実体Ownershipは維持する。
- **2026-10-17「Application固有Modelが姿勢と診断描画の意味を管理し、Render Modelを汎用描画構成に限定する」**の機種固有Semantic / Command / AABBの責務分離は維持する。末尾のApplication Resource Lifetime Contractは、Model-managed Geometry / Materialについて本Decisionで置き換える。
- **2026-10-16「Render PacketをShader別Draw Collectionとし、Model連携を先に検証してからOrchestrationを抽出する」**の段階的導入を維持しつつ、Model Import Orchestrationの具体的必要性が判明したためModel Pipelineを導入する。汎用Mesh Pipeline / Mesh Registryを先行導入する判断ではない。
- **2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」**の将来Targetを維持し、現段階は`application_renderer`を暫定Frontendとする。旧Decisionの「Mesh導入だけでReference Countingを追加しない」は当時の前提であり、今回Model ImportのLifecycle要件が具体化したため局所的に変更する。
- **2026-09-12「Resource Pipelineはstateless orchestrationとして維持する」**のGeometry / Material Pipeline Contractは維持する。新Model PipelineはModel全体を扱う別粒度のOrchestrationである。
- **2026-10-09「Untextured / Textured MeshのMaterial構成とTexture参照lifetimeを分離する」**のMaterial→Texture直接参照に関するContractを、今回のUntextured Geometry / Material Model Acquireと同一視しない。
- **2026-08-31「Asset path policyはApplicationが所有し、Loaderへ完成済みpathを渡す」**と**2026-09-02「Geometry resource identityはRegistryが所有しCPU Geometryへ埋め込まない」**を維持する。

### Revisit Conditions

- Model内でResourceを差し替える、Streaming / Hot Reload / Shared Cache / GPU Residency管理を導入するなど、Acquire単位とRelease単位を再定義する必要が生じた場合。
- Model単位の重複排除とRegistryのReference Countingでは、大規模動的Resource管理の性能や表現要件を満たせなくなった場合。
- Asset Package / VFS / Archive / Mount Point等により、現在の相対Subdirectory + Resource NameというIdentityとLocationの関係を見直す必要が生じた場合。
- 新しい具体的なMesh種別のImportがGeometry / Material以外のResourceを含み、TextureなどのReference LifetimeもModel-managedへ移管する必要が生じた場合。

### Related Documents

- `glce_near_term_todo.md` — M1〜M5 / R2〜R5
- `design_notes.md` — Renderer Resource Domain and Frontend Migration / Mesh / Model / Coordinate / Render Packet Architecture
- `validation_policy.md` — Failure classification / Fail-stop
- `boundary_model.md` — Semantic owner / Trust Boundary / Ownership Transition


---

## Decision: Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する

**Status:** Accepted / Implementation Pending  
**Date:** 2026-10-19（設計議論: 2026-10-18〜19）  
**Scope:** Renderer Frontend / Resource Manager / Shader / VBO Manager / VAO / Mesh Renderer / Import Pipeline / Geometry・Material Registry / Model Resource Lifecycle / Ownership / Directory Architecture

### Decision

Model ImportとRegistry Reference Countingを実装するにあたり、既存の`*_render_resource`によるResource Ownership・Import / Release・Draw・Uniform設定の混在を解消し、以下の構造へ再編する。

1. **`*_render_resource`を`*_resource_manager`へ改名し、その役割をResourceの取得・解放およびResource Storageの管理に限定する。** Geometry / Material等の`acquire` / `release`はResource Managerを入口とする。Cache Hit / Missの判定、新規ResourceのImport依頼、Registryへの参照取得・返却、最終解放に必要な調整はResource Managerが統括する。ただしReference Count値のAuthorityはRegistryに置く。
2. **`renderer_frontend`をRenderer全体の長寿命Ownerとし、Backend Context、描画種別ごとのShaderおよびResource Managerを所有させる。** Frontendは個々のGeometry RegistryやVBO Managerまで直接所有する構成へはしない。Resource ManagerはShader別Vertical SliceのResource Ownership Domainである。
3. **VBO ManagerはShaderではなく、対応するResource Managerが所有する。** Resource Managerが自身のRegistryとVBO Managerを所有することで、GeometryのGPU Allocation / Upload / FreeがShaderを経由しなくても完結できるようにする。
4. **ShaderはShader Program、VAO、Uniform関連State / Operationを所有・提供し、VBO ManagerのOwnerにはならない。** VAOを構築するときはResource Managerが管理するVBOをBorrowして利用する。ShaderからResource Manager側への必要な依存を許容し、**Resource ManagerからShaderへの逆依存は設けない**。BorrowをObject内に保持するかConstructor時だけ使用するか、具体的Interfaceは実装時に定める。
5. **`*_mesh_renderer`はStatelessな描画専用Moduleとする。** ShaderおよびResource Managerを必要な描画Operationの間Borrowする。Draw RangeやMaterial等のResource解決はResource Managerから行い、Shader Program / VAO Bind、必要なUniform適用、Backend Drawを組み合わせる。長寿命Resourceを所有せず、Resource Acquire / Releaseを担当しない。
6. **Draw操作をResource Managerへ持ち込まない。** 旧`*_render_resource_draw()`はMesh Rendererへ移す。旧`view_matrix_set()`、`projection_matrix_set()`、Lighting関連の`xxx_set()`はResource Managerの役割ではない。Frame / View / Projection / Lighting等の上位設定窓口はRenderer Frontendに配置する方向とし、個々のUniform SetterはShaderへ残す。最終的なState保存場所、更新タイミング、Drawとの関係は実装時に確定する。
7. **既存Resource PipelineをImport目的に限定した`Import Pipeline`として維持し、Resource Managerの下位に配置する。** Geometry / Material等の新規Resourceについて、File Load、CPU Resource構築、GPU Upload、RegistryへのOwnership Commitに至る短命なImport Transactionを組み立てる、**Stateを持たないOperation Module**である。Import PipelineはShader / Registry / Resource Manager等の長寿命Objectを所有しない。現行PipelineのRelease操作はResource Manager側のRelease経路へ移す方向とする。
8. **Resource ManagerのAcquireは、新規Importと既存Resourceの参照取得を同一の上位Operationとして扱う。** RegistryはResource Identity、実Resourceの長寿命Ownership、およびReference Countの唯一のAuthorityである。新規登録時は最初の参照を成立させ、既存取得時は参照数を増やし、Releaseで0になれば実Resourceを解放する。具体的な内部API SignatureとCommit / Rollback順序は未確定とする。
9. **Geometry RegistryのReference Countingは4種類（Line / Point / UI / Untextured）すべてへ適用する。** Untextured Material RegistryもModel Importに必要な対象とする。これは「Model単位のReference Right」を導入する上位判断の具体化であり、Texture Registryへの同時導入は自動的には要求しない。
10. **Model PipelineはResource Managerの下位へ移動させない。** Model単位のResource Dependency解決、同一Model内での重複排除、Acquire済みReferenceのRollback、Model構築、Release調整を行う別粒度のStateless Coordinatorとする。Application / ModelからGeometry RegistryのPointerを直接利用する構造にはしない。
11. **本DecisionはTarget Architectureであり、2026-10-19時点で実装完了したという意味ではない。** 既存`application_renderer` / `*_render_resource` / Shader / Pipeline / Registryの変更は段階的に行い、各WorkでOwnership / Borrow / Result / Validation / 既存Drawの動作を確認する。

#### Target Ownership / Dependency（Untextured Meshの例）

```text
Renderer Frontend                         [Owner of renderer-lifetime objects]
│
├── Renderer Backend Context              [Owned]
│
├── Untextured Mesh Resource Manager      [Owned]
│   ├── VBO Manager                       [Owned]
│   ├── Geometry Registry                 [Owned]
│   └── Material Registry                 [Owned]
│       
│   Import Pipelines                      [Stateless subordinate operations]
│       ├── Geometry Import Pipeline
│       └── Material Import Pipeline
│
└── Untextured Mesh Shader                [Owned]
    ├── Shader Program                    [Owned]
    ├── VAO                               [Owned]
    └── Uniform-related state
          │
          └── Resource ManagerのVBOへアクセス [Borrow / operation dependency]

Untextured Mesh Renderer                  [Stateless]
    ├── Shader                            [Borrow]
    └── Resource Manager                  [Borrow]

Model Pipeline                            [Stateless model-level coordinator]
    └── Resource ManagerのAcquire / Releaseを使用
```

図のImport Pipelineは長寿命Owned Fieldではなく、**Resource Managerの下位に属するStatelessな処理Module**を示す。`Import Pipeline`がRegistryへ直接Registerするか、Resource ManagerにCandidateを返してRegisterを委譲するかは、Ownership TransferのAPI設計時に判断する。

この構造での基本的な依存方向は以下となる。

```text
Renderer Frontend → Resource Manager → Registry / VBO Manager / Import Pipeline
                  → Shader → (Resource Managerが管理するVBOへの限定的Borrow)

Mesh Renderer → Shader + Resource Manager
Model Pipeline → Resource Manager

Resource Manager -X→ Shader
Registry         -X→ Shader
Import Pipeline  -X→ 長寿命StateのOwnership
```

このため、**ShaderがResource Managerを利用することや、Mesh RendererがResource Managerを利用すること自体は不自然な依存ではない**。不適切なのは、Resource ManagerにDraw / Shader Bind / VAO Bindのような描画Operationまで混入させることである。

### Context / 議論の経緯

本Decisionは初めから一度で決まった構成ではない。Model Importに対応するResource Lifetimeを検討する過程で、RegistryのAPI、Pipelineの役割、Render ResourceのOwnership、ShaderとVBO / VAOの関係を順に再評価した。以下は、その議論で採用・修正・却下された考えを時系列で記録する。

#### 1. 出発点: Model Importによって、Resourceの参照取得と実体登録を分ける必要が生じた

従来、ApplicationはGeometry / Materialを個別にImportし、必要な間保持し、不要になったら個別Releaseできた。この構造なら、利用側がResource Lifetimeを直接管理できる。

しかしRender Modelは複数のMeshとCoordinate Treeを所有し、Model Configから依存するGeometry / Materialが決まる。Modelの利用者が毎回Configの内部Resourceを列挙して事前Importしたり、Model Releaseの際に個別解放したりする方式では、Model Importの抽象化が成立しない。

そこで、同一Modelが同じResourceを複数Meshから参照しても1つのReference Rightを持ち、別Modelはそれぞれ独立のRightを持つ方式を採用した。RegistryがReference Countを管理する方向は、2026-10-18「Model ImportをResource Dependency解決の入口とし、Model単位のReference LifecycleをRegistry Reference Countingで管理する」で先行して確定している。

この時点では、Render ResourceとGeometry / Material PipelineおよびRegistryに`acquire` / `release`を追加する方法を検討していた。

#### 2. Registry `register/unregister`をPrivate化し、`acquire/release`へ統一する案

最初は、`acquire_existing`と`acquire_new`を分けて呼び出す案も検討した。しかし、この設計では上位Callerが「既存か新規か」を判定する必要があり、`acquire_new`が実質的に`register`の改名となる危険があった。

これに対して、Geometry PipelineとGeometry Registryでは`register` / `unregister`をPrivateまたはInternal Operationとし、上位からは`acquire` / `release`で利用する方が分かりやすい、という提案が出た。

この議論で得た重要な判断は、**Resourceの利用者にCache Hit / Missを判断させないこと**である。一方で、Registry内部には「既存Entryへの参照追加」と「未登録ResourceのOwnership Transfer」という異なる内部操作が残る。これらをどのFunction Signatureで表すかは別問題であり、上位APIを一つにするためにすべての内部操作を無理に一関数へ統合することは要求しない。

また、別Translation UnitのImport Pipelineから呼ぶ`register`はCの`static`なPrivate Helperにできない場合がある。**Applicationから公開しないことと、Module内部専用APIとして利用できることは区別する**。最終的なInternal Header / Function Scopeは後続Workで決める。

#### 3. 「Geometry Pipelineは実は不要ではないか」という疑問

`render_resource`は既にShaderとRegistryを所有していた。Geometry Pipelineが行っている処理の多くは、その2つのObjectを受け取り、LoaderでGeometryを作り、Shader経由でVBOへUploadし、Registryへ登録するものであった。

したがって一度は、Geometry Pipelineを廃止してImport TransactionをRender Resourceへ内包する案が有力になった。実際に、独立したModuleが必要かどうかは、処理量よりも責務と依存関係で決まる。

しかし検討を進めると、Geometry Pipelineは単なるForwarderではないことが確認された。

```text
Load
  ↓
CPU Geometry Construct
  ↓
GPU Upload
  ↓
Registry Ownership Commit
  ↓
Failure時のTemporary Resource Cleanup / Rollback
```

Import完了前のTemporary Owner、GPU Uploadの失敗、RegistryへのOwnership Transfer等を扱う**短命なTransaction**としての責務は存在する。このため、Pipelineを廃止することだけを目的とした統合は行わず、最終的にImport専用Moduleとして存続させる方向になった。

ここでの判断は「すべてのPipelineを永久に独立Moduleとして維持する」ことではない。現在のResource Importでは、独立したTransactionを持つだけの意味があるということである。今後、実処理が消えて単なるForwarderになった場合は見直し得る。

#### 4. 問題はPipelineの有無だけではなく、Render Resource内で異なるOperationが混在していることだった

既存`untextured_mesh_render_resource`は概ね次のOwned Fieldを持っていた。

```c
struct untextured_mesh_render_resource {
    untextured_mesh_shader_t* shader;
    untextured_mesh_geometry_registry_t* geometry_registry;
    untextured_material_registry_t* material_registry;
};
```

一方、API群はImport / Release、Draw、`view_matrix_set` / `projection_matrix_set` / Lighting関連Setters、Create / Deinitialize / Validation等の異なる責務を含んでいた。

ShaderとRegistryを同一Objectが所有することは当時のVertical Sliceとして合理的だった。しかし、Model単位のAcquire / Releaseを増やしていくと、Storage管理と描画Operationが同一Moduleに置かれている違和感がより明確になった。

ここで**Render Resourceをただ廃止してFrontendへすべての処理を移すのではなく、Resourceの管理責務を独立させる**案が出た。

#### 5. Render ResourceをResource Managerとして残す案

`render_resource`が現にGeometry RegistryとMaterial Registryを所有しているなら、同一DomainのResource Acquire / Releaseを担当させることは自然である。

そこで、旧`*_render_resource`を廃止するのではなく、**`*_resource_manager`へ役割を再定義して残す**方針を採用した。これは旧Objectを名前だけ変えるという意味ではない。

```text
旧 Render Resource:
    Shader Ownership
    Geometry / Material Registry Ownership
    Import / Release
    Draw
    View / Projection / Lighting Set

新 Resource Manager:
    Geometry / Material Registry Ownership
    VBO Manager Ownership
    Resource Acquire / Release
    Resource Lookup / Storage Operation
    Import Pipeline Orchestration

別のOwner:
    Renderer Frontend → Shader Ownership / Frame-level Orchestration
    Mesh Renderer    → Stateless Draw
    Shader           → Program / VAO / Uniform
```

これにより、2026-10-15の「Renderer FrontendがShader / Registryをすべて直接所有し、中間の長寿命Ownerを持たない」という方針を修正することになった。

#### 6. ShaderをFrontendが直接所有し、Resource ManagerがShaderをBorrowする初期案

Resource ManagerからShader Ownershipを外し、FrontendがShaderを所有する案をまず検討した。既存ではVBO ManagerがShaderにあるため、Resource ManagerはGeometryのUpload / FreeにShaderを必要とする。

この段階では、次の依存関係を想定していた。

```text
Frontend
    ├── Shader [Owned]
    └── Resource Manager [Owned]
           ├── Shader [Borrowed]
           ├── Geometry Registry [Owned]
           └── Material Registry [Owned]
```

この方式も成立する。LifetimeはFrontendが統括すればよく、Resource ManagerがShaderをBorrowしてVBO操作を行える。しかし、**Resource Storageの管理がShaderの存在に依存する**という本質は残る。Geometry Registryが最終解放時にShaderを必要とする従来の依存も残る。

このため、VBO Managerそのものをどこが所有すべきかを再検討した。

#### 7. VBO ManagerとVAOを両方Resource Managerへ移す案

VBO ManagerはGPU Bufferの確保・Upload・解放に関わり、Geometry Resource StorageのManagerとして自然である。ならばVBO Managerだけでなく、密接に関係するVAOもResource Managerへ移すことができるのではないかと考えた。

この案にはShader ProgramとGeometry Storageを分離できる利点がある。しかし、**`shader_vao_bind()`がResource Manager側へ移る**ことが問題になった。

VAOはVertex Attribute / VBO Bindingの情報を管理するが、VAO BindはResourceのImportやReleaseで使うだけの操作ではない。実際にはDraw時のGPU State切替に直接使用する。

Resource Managerを「Resourceを取得・解放・操作する管理Module」と定義したのに、そこへ描画のための`vao_bind()`だけが残るのは責務として異質だった。

この観察から、**GPUに関係するObjectをすべてResource Managerへ入れることが正しいわけではない**と判明した。Owned Objectの分類は、実際にそれを利用するOperationの意味に従わせるべきである。

#### 8. VBO ManagerだけResource Managerへ移し、VAOはShaderに残す最終案

次に、VBO ManagerをResource Managerに所有させ、ShaderからはVBO ManagerのOwner Fieldを除き、VAO / Program / Uniformを残す案を検討した。

これはVBO ManagerとVAOの間に依存があるから所有者も同じでなければならない、という前提を外したものである。

VAO初期化では、対象VBOをBindした状態でVertex Attributeの設定を行う必要がある。OpenGL 3.3の現在のBackendでは、`glVertexAttribPointer()`がその時点のArray Buffer BindingをVAOのAttribute Stateへ取り込む。そのため、ShaderがVAOを構築するときにResource ManagerのVBOを一時Borrowすれば、ShaderがVBO Managerを所有しなくても成立する。

```text
Resource Manager create
    └── VBO Manager create
              ↓
Shader create
    ├── Shader Program create
    └── VAO create
          ├── VAO Bind
          ├── Resource ManagerのVBOをBind
          ├── Vertex Attribute Setup
          └── Unbind
```

その後のDrawではShaderが保持するVAOをBindすればよく、Resource Managerへ`vao_bind()`を押し込む必要がない。

一方、Resource Managerは自ら所有するVBO Managerを使ってUpload / Freeを実施できる。**Resource ManagerがShaderをBorrowしなければならない理由がなくなり、依存方向が単純になる**。

この時点で「ShaderがResource Managerに依存する」「Mesh RendererもResource Managerを利用する」という構造を採用した。Resource ManagerはResourceへの正規アクセス窓口であるため、それを描画側が利用すること自体は自然と判断した。

#### 9. Geometry Pipelineを`Import Pipeline`と呼び、Resource Manager配下へ移設する案

最後に、Pipelineの位置付けを再検討した。従来の`resource_pipelines`はGeometry / Material / TextureのResource構築を行っていたが、その名前だけでは、どの長寿命Ownerの下でどのOperationを担当するのかが分かりにくい。

新しい分割では、Resource Managerに外から見えるAcquire / Releaseの入口があり、RegistryにResource Identity / Reference CountのAuthorityがあり、Import Pipelineは未登録Resourceの構築だけを担当する。したがって、**Pipelineを`Import Pipeline`へ改名し、Resource Managerの下位へ配置する**のが自然であると判断した。

```text
Resource Manager.acquire()
    │
    ├── Registryに存在する
    │     └── Reference Count増加、既存ID返却
    │
    └── Registryに存在しない
          └── Import Pipeline
                ├── Load / Generate
                ├── CPU Resource Construct
                ├── GPU Upload（必要な場合）
                └── RegistryへのOwnership Commitに必要な結果を成立させる
```

このときImportは「Fileから読み込む」という狭い意味だけには限定しない。Vertices、AABB、Solid Color等から新しいResourceを構築し、Resource Manager管理Domainへ取り込むOperationも含み得る。各Import Methodの正確な対象はModuleごとに決める。

Import Pipelineに既存Release経路を残してAcquire / Releaseの入口を分散させず、上位からのReleaseはResource Managerで受け付ける。Registryによる実体解放とTemporary Import CandidateのCleanupは別Operationである。

**Directory配置とOwnershipは同じ意味ではない。** Import PipelineはResource Managerの下位Moduleとして配置するが、長寿命Ownerとして保持されるObjectではなくStatelessなFunction群のまま扱う。

### Rationale

#### Resource ManagerをShader別Resource Domainの管理単位とする

- Shader別Vertical SliceごとのRegistry / VBO Managerを一つの管理Domainへまとめられる。
- Renderer FrontendがすべてのRegistry / VBO Manager / Import処理の具体構造を直接知る必要がなくなる。
- Resource Acquire / Releaseの正常なCache Hit / Missと参照の管理を、Application / Modelから隠蔽できる。
- Model Importが個別Registry Pointerや内部Reference Countを知らずにResource参照権を取得・返却できる。

#### VBO ManagerのOwnershipをShaderから分離する

- VBO AllocationはGeometry Storageの一部であり、Resource ManagerがLifecycleを管理するResourceと整合する。
- Registryの最終解放にShader Pointerを要求しない構造を実現できる。
- ShaderのCreate / Destroy、Program / Uniform設定と、GeometryをUpload / FreeするOperationを分離できる。
- ShaderにはVAOを残し、Draw時のVertex Input State Bindingを自然なAPIへ配置できる。
- OwnershipとBorrowを区別することで、VBOとVAOに関係があっても一方が他方を所有する必要はない。

#### Resourceの使用とLifecycleを別Moduleへ置く

- Resource ManagerはResource Lifecycle、Mesh RendererはDraw、ShaderはProgram / VAO / Uniform、Frontendは全体のCompositionという責務をそれぞれ持てる。
- `resource_manager_vao_bind()`のような、管理Moduleの意味と異質なAPIを生まずに済む。
- Resource ManagerからShaderへの循環・逆依存を避けられる。
- Mesh Rendererは描画に必要なResource LookupをRMから受け、実体Ownershipを持たない。

#### Import Pipelineを下位のStateless Transactionとする

- Load / CPU Resource生成 / GPU Upload / Failure Cleanupという独立したTransactionを残せる。
- Pipelineを削除しただけで同じTransactionをResource Managerの大きな関数へ埋め戻すことを避けられる。
- ImportとReleaseの意味を分離し、PipelineをResourceの取得・解放の公開窓口にしなくて済む。
- 将来Import方法が増えても、Resource Managerの長寿命StateとImporterの一時作業を分けられる。

### Considered / Rejected or Revised Alternatives

#### A. RegistryとPipelineの両方へ一律に`acquire/release`を設ける

初期案として、PipelineとRegistryの双方に`acquire` / `release`を追加し、`register` / `unregister`を内部化する方法を検討した。Reference CountをRegistryに置く点は適切だったが、上位のAcquire / Releaseを誰が統括するかを先に決めないと、複数の公開入口ができ、Resourceの存在判定やLifetime Authorityを利用者が意識することになる。

採用したのは**Resource Managerが上位Operationの入口となり、RegistryがCountを管理する**という役割分担である。Registry内部にどの補助関数を設けるかは別判断とする。

#### B. Geometry Pipelineを廃止してRender Resourceへ全面統合する

Import Transactionには独立した処理責務があり、すべてをOwner Moduleへ集約することは必ずしも単純化にならない。最終的にImport専用Pipelineとして残す方針とした。

#### C. FrontendがShader / Registry / VBO Managerをすべて直接所有する

2026-10-15には、余計な長寿命Wrapperを増やさないことを重視していた。しかしModel単位のReference Lifecycleが具体化すると、Geometry / MaterialのAcquire / Releaseを担う管理Domainに意味が生まれた。`Resource Manager`は単なるWrapperではなく、RegistryとVBO ManagerのOwnerでありLifecycle Operationの入口であるため、Frontendが直接すべてを所有する構造は修正する。

#### D. Resource ManagerがShaderをBorrowし、ShaderがVBO Managerを所有し続ける

OwnershipだけFrontendへ移し、GPU Storage管理のためにRMがShaderを借用する案も成立する。しかしGPU GeometryのUpload / FreeがShaderに依存し、Registryの解放処理もShaderと結びついたままになるため、今回のTargetとしては採用しない。

#### E. VBO ManagerとVAOを両方Resource Managerへ移す

VAO BindがDrawに属するため、管理Moduleへ描画State Operationが混入する。VBOとVAOの物理的な関係だけでOwnerを一致させる必要はなく、VBOはRM、VAOはShaderに分ける方針とした。

#### F. VBO ManagerをShaderへ残す

ShaderがVAO / VBOの両方を所有する構造は局所的で、依存関係がModule内に閉じるメリットがある。しかしReference Countに伴うVBO Allocationの最終解放をRMで管理するにはShaderへ依存する必要が残る。Shader→RMの限定依存を受け入れてStorage Ownerを揃える方が、今回の責務分担と一致する。

#### G. Mesh RendererをStateful Objectにする、またはAcquire / Releaseを担当させる

描画中に借用できるShader / Resource Managerがあれば、現時点のDraw Operationは長寿命Stateを持たずに成立させられる。Resourceの取得・解放を加えると描画とStorage Lifetimeが混在するため採用しない。

#### H. Model PipelineをResource Manager内部へ取り込む

Model Pipelineは複数Resourceの依存解決とModel全体のRollbackを担当するため、Geometry / Material単位のImport PipelineとはOperation粒度が異なる。一つのRMに属するImport処理ではないため採用しない。

### Consequences / Migration Contract

#### 1. Create / DestroyとBorrow Lifetime

概念的な生成順序:

```text
Renderer Backend Context create
       ↓
Resource Manager create
       └── VBO Manager / Registry create
       ↓
Shader create
       └── Resource ManagerのVBOを利用してVAO構築
       ↓
Renderer Frontendとして利用可能な状態を成立させる
```

概念的な破棄順序:

```text
Shader / VAO destroy
       ↓
Resource Manager deinitialize
       ├── Registry内のOwned Resource解放
       └── VBO Manager destroy
       ↓
Renderer Backend Context destroy
```

Resource ManagerはShaderがVAOを保持する間、必要なVBOのBackend Object IdentityとBorrow Contractを維持する。GPU BufferのRecreate / Hot Reload時にVAOを再構築するか、Buffer Bindingがいつ無効になるか等の詳細は今回の決定に含めない。

Shaderが`resource_manager_t*`を長期Borrowするか、VAO初期化中にVBO Manager等へ一時的にアクセスするかは未決とする。**ShaderがVBO Managerを所有しないこと、Resource ManagerがShaderへ依存しないこと**が確定要件である。

#### 2. Resource ManagerのAPI責務

- `geometry_acquire` / `geometry_release`、必要なMaterialの`acquire` / `release`に相当する上位APIを提供する。
- 既存ResourceではCountを増やして同じResource IDを返す。未登録ならImport Pipeline等によって新規Resourceを作り登録する。
- Resource IDに対するGeometry Draw Range / Material等のGetterを描画用に提供する。ただしRegistry Pointerそのものを上位へ露出しない。
- Import Pipelineが内部的にRegistryへ直接登録する方式と、RMが登録を実行する方式は引数・Ownership Contractと合わせて決める。
- `register` / `unregister`のPrivate / Internal Scopeは実際のTranslation Unit構成と一致させる。
- 旧`render_resource`のDraw / View / Projection / Lighting SetterをそのままResource Managerに残さない。

#### 3. Reference Countingの適用範囲

- **Line / Point / UI / Untextured Geometry Registryの4種類**にReference Countingを導入する。
- **Untextured Material Registry**にもReference Countingを導入する。
- Reference CountのAuthorityはRegistry。Modelは自身が保持するResourceごとの参照権を管理するが、Registry内Count値を複製しない。
- 参照中のResourceをFinal Releaseで破壊しないため、ReleaseはCounterとResource RegistrationのState Transitionとして設計する。
- Texture Registry、将来のTextured Materialに同じCountを直ちに要求しない。必要になったとき、参照関係ごと判断する。

#### 4. Import PipelineのDirectory / API

- `Import Pipeline`をResource Managerの下位として表現できるDirectory構成へ移す。
- Geometry / Material / Texture等のPipelineは、実際にそれを所有・管理するVertical Sliceとの対応に従って配置する。UIのTexture Pipeline等で複数の管理Domainから利用される具体的関係があれば、Source Auditで確認して配置を決める。
- PipelineはStatelessであり、Object Create / Destroyを持つ長寿命Ownerにはしない。
- `import_from_file`、`import_from_vertices`、`import_from_aabb`等、入力方式ごとのMethod差は維持してよい。ImportをFile Parsingだけに限定しない。
- `release`は上位RMに集約する。Import完了前Temporary CandidateのRollbackと、登録後ResourceのFinal Releaseは明確に区別する。

#### 5. 描画Operationの移設

- 旧`*_render_resource_draw()`は対応するStateless Mesh Rendererへ移す。
- Mesh RendererはShader / RMをBorrowし、Geometry Draw Range、Material等を解決してDrawする。
- Shader Program / VAOのBindと個別Uniform SetterはShader側で提供する。
- FrontendはView / Projection / Lighting等の上位操作の入口となる方向。ただしUniformの更新時点・Frame Stateの具体的Owner・FrontendとMesh Renderer間のInput Contractは後続設計で確定する。
- Geometry→AABBのようなGeometry ConversionはDrawでもResource Lifetimeでもないため、責務の所有先を個別検討する。単純にRMへ移設しない。

#### 6. 段階的実装と既存Decisionの扱い

- 既存`application_renderer`や`*_render_resource`が現に所有しているResourceを移行途中で二重所有しない。
- Frontend / Resource Manager / ShaderのOwner移管、VBO Manager移動、Draw / Setter分離、Refcount追加、Model Import接続は、レビュー可能な小さなWorkに分割する。
- 4種類のGeometryは対象とするが、一つのCommitで4種類を無条件に同時変更することまでは要求しない。
- 各WorkでCreate / Destroy、Error Conversion、Partial Failure Cleanup、Validation、既存描画の回帰を確認する。
- 参照Countの更新、Registry登録時のOwnership Commit、VBO AllocationのFinal Free、Model ImportのRollback等は`validation_policy.md`と既存のFunction Phase Contractに従って詳細化する。
- Renderer Frontend導入の前倒しによって、2026-10-18の旧Model Import Decisionにある「Frontendは今は新設せず`application_renderer`を暫定Ownerとする」「Untexturedから先にRefcountを入れる」という作業順の判断を更新する。Model単位の参照権と自動Resource解決という本質的Semanticsは維持する。

### Prior Decisionsとの関係

#### 2026-09-12「TopologyごとのRender ResourceをRenderer resource ownership rootとする」

この当時は、Topology / Shader単位にShaderとRegistryをまとめ、Applicationから内部Resource Managementを隠すことが主目的だった。**Shader別Vertical Sliceの局所性は維持する**が、当時の`render_resource`をImport / Release / Drawの混在したObjectとして維持することはしない。2026-10-15の再編を経て、今回の`resource_manager`として責務を再定義する。

#### 2026-09-12「Backend ContextはRender Resourceがborrowする」

Backend Contextの長寿命Ownerを上位に置き、下位がBorrowする原則は維持する。Borrowする具体的Objectが旧Render ResourceからFrontend / Shader / Resource Manager等へ変わるため、旧Class名に依存したContractは読み替える。

#### 2026-09-12「Resource Pipelineはstateless orchestrationとして維持する」

Pipelineが長寿命Stateを持たず、Import Transaction中のTemporary Ownerとなる原則は維持する。一方、**名前をImport Pipelineへ変更しResource Manager下位へ配置、ReleaseをRMへ集約する**ことで、旧PipelineのOperation Scopeを更新する。

#### 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」

FrontendがRenderer全体のOwnerであり、Mesh RendererがStateless Draw Moduleである基本は維持する。一方、**FrontendがRegistryを直接所有し中間Ownerを置かない方針はSuperseded**とする。Frontendは各Resource Managerと各Shaderを所有し、Resource ManagerがRegistry / VBO Managerを所有する。

#### 2026-10-15「Renderer専用ResourceをRenderer Domainへ統合し、Range AllocatorをMemory Domainへ移動する」

RendererのResource Domainを一つにまとめ、CPU / GPUという区別だけでResourceを分割しないDecisionは維持する。今回のImport Pipeline / Resource Managerの配置は、その内部Module構成を明確にする追加の再編である。Range Allocatorは引き続きMemory Domainのlow-level mechanismとする。

#### 2026-10-09「Untextured / Textured MeshのMaterial構成とTexture参照lifetimeを分離する」

Untextured / Textured Materialの区別、Material Value、Vertical Sliceの独立性は維持する。旧Decisionに書かれた「Render ResourceがShaderとRegistryを所有する」という実装前提だけを新Targetに合わせて更新する。TextureのReference Model全体を今回のGeometry / MaterialのRefcount導入と同一視しない。

#### 2026-10-17「すべてのMeshを非共有のRender Modelへ所属させ、Coordinateに描画姿勢のAuthorityを集約する」

MeshのModel所属、Coordinate TransformのAuthority、Resource IDを非所有参照とする判断は維持する。Applicationが参照Resourceをすべて個別に延命する旧判断は、2026-10-18のModel Import Decisionですでに更新されている。今回のResource Manager再編は、そのReference Rightを受け渡す正式な窓口を具体化する。

#### 2026-10-18「Model ImportをResource Dependency解決の入口とし、Model単位のReference LifecycleをRegistry Reference Countingで管理する」

Model ImportによるDependency解決、Model単位の重複排除、RegistryのReference Count、失敗時Rollback、Composite Resource Keyの決定は維持する。**Resource Acquire / Releaseの上位OwnerをRender ResourceからResource Managerに変更し、Reference Countingの実装対象を4種類のGeometryに拡張し、FrontendへのOwner移管をModel Import完成より前に進める**点を更新する。

### Revisit Conditions

- Shader側にVAOを保持したままResource ManagerのVBOをBorrowする方式が、別のRenderer Backendで成立しない、またはVertex Input StateのOwnerを再定義する必要が生じた場合。
- Renderer Backend / Shader / Resource Managerの生成・破棄、GPU Bufferの再生成、Context Loss、Hot Reload等で現行Borrow Lifetime Contractでは不十分となった場合。
- Resource ManagerがDraw・Frame Policy・Shader Stateまで扱い始め、Resource Lifecycle Moduleとしての責務境界が崩れた場合。
- Import Pipelineが実処理をほとんど持たないForwarding Layerになった場合、またはImportが長寿命State / Async Streaming / Residency管理を必要とするようになった場合。
- Geometry / Materialの参照単位がModel単位だけでは表現できない、Runtime差替え・Streaming・跨Domain共有等の具体的要件が生じた場合。
- Resource Managerが保持するRegistry / VBO Managerが大規模化し、Shader別Vertical Sliceの境界より別のLifetime / Ownership Domainが自然と判明した場合。
- Textured MeshやTexture Registryへの参照管理導入で、現在の「Geometry / MaterialはRM、Textureは既存独立Contract」という分担を再評価する必要が生じた場合。

### Related Documents

- `glce_renderer_resource_model_workplan.md` — 本Decisionの具体的移行Work（長期Project管理文書）
- `design_notes.md` — Renderer Resource Domain / Model / Frontend移行の設計メモ
- `glce_near_term_todo.md` — 全体の直近Queue、Renderer / Model Integrationの位置付け
- `validation_policy.md` — 参照更新・Ownership Commit・Cleanup / Fail-stop
- `boundary_model.md` — Registry / RM / FrontendのOwnership・Authority / Borrow関係
- `coding_style.md` — Public / Internal APIと命名・Lifetime・Result Conversion

---

## Decision: VBO ManagerをRenderer Frontendの共通基盤とし、Shader / Resource Managerから引数でBorrowする

**Status:** Accepted  
**Date:** 2026-10-19  
**Scope:** Renderer / VBO Manager / Shader / Resource Manager / Ownership / Borrow / Transitional Migration

### Decision

Renderer Frontendの完成後、**各Mesh種別のVBO Managerの長寿命OwnerをRenderer Frontendとする**。

Renderer Frontendは、対応するResource ManagerおよびShaderも所有する。Resource ManagerとShaderは相互に依存せず、それぞれVBO Managerを必要なOperationの引数で一時Borrowする。

```text
Renderer Frontend [long-lived owner]
│
├── Renderer Backend Context [Owned]
│
├── Untextured Mesh VBO Manager [Owned]
│
├── Untextured Mesh Resource Manager [Owned]
│    ├── Geometry Registry [Owned]
│    ├── Material Registry [Owned]
│    └── Import Pipeline [Stateless / Operation module]
│
└── Untextured Mesh Shader [Owned]
     ├── Shader Program [Owned]
     ├── VAO [Owned]
     └── Uniform Locations / Setters

Resource Manager / Registry  ──(Operation parameter)──> VBO Manager [Borrow]
Shader / VAO initialization ──(Constructor parameter)─> VBO Manager [Borrow]

Untextured Mesh Renderer [Stateless]
    ├── Shader [Borrow]
    └── Resource Manager [Borrow]
```

上図はUntextured Meshの例である。Line / Point / UIも各Vertical Slice固有のVBO ManagerをFrontendが所有する構成を基本とする。**共通基盤とはShaderとResource Managerの双方が利用できる基盤であるという意味であり、全Mesh種別で一つのVBO Manager instanceを共有するという意味ではない。**

Borrowは必要なAPIの引数によって行い、ShaderやResource ManagerのObject fieldにVBO Manager Pointerを恒久的に保存する設計は、今回のTargetでは採用しない。全APIに機械的に渡すのではなく、VBOへアクセスするOperationだけに明示する。

**暫定実装:** Renderer Frontendがまだ存在しないため、現在の`*_render_resource`（将来の`*_resource_manager`）が、対応するVBO Managerを一時的に所有することを許容する。**FrontendがVBO Managerの正式なOwnerとなる段階で、Resource Managerから当該Ownershipを移管する。** この暫定配置を最終Architectureとして固定しない。

Registryは登録ResourceのIdentity、CPU Geometry、VBO Allocation DescriptorとReference CountのAuthorityを持ち続ける。VBO ManagerはGPU Bufferとその内部Allocation操作のAuthorityを持つ。Resource ManagerはAcquire / ReleaseとResource Domainの統括を担当し、Backing Storage全体のOwnerである必要はない。

`vbo_range_t` / `draw_range_t`の定義位置の変更は別Workへ送る。型の移動を本Decisionの導入条件にしない。

### Context — 今回の再検討に至った経緯

2026-10-19の先行Decision「Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する」では、VBO ManagerをResource Managerが所有し、ShaderはVBOをBorrowしてVAOを構築する方針を採用していた。これはShaderがVBOを所有するとRegistryのVBO解放にShaderへの依存が残ること、およびResource ManagerとShaderを分離したいことから導いた構造だった。

API移動・Constructor / Deinitializerの整理を始める中で、Geometry Registryの`deinitialize()`、`unregister()`等がShaderではなくVBO Managerを受け取る方針を検討した。VBO ManagerからRegistryへの依存はなく、RegistryからVBO ManagerへのBorrowで循環Dependencyは生じないことを確認した。このとき、VBO ManagerはRegistryにとってもShaderのVAO構築にとっても必要なResourceである点が改めて明確になった。

その後、VBO Managerの所属を決めるため、ShaderとResource ManagerのOwnership自体を再検討した。

### Discussion History — 検討したOwnership構造

#### 1. ShaderをFrontendが所有する案

Shader Program、VAO、Uniformの管理は描画側の責務であり、Renderer FrontendがShaderのLifetimeを統括する構造は自然である。ShaderをResource Managerから分離すれば、Geometry / Materialの管理DomainがShader Stateを直接所有しなくて済む。

ただし、VAO初期化にはVBO Managerが必要であり、VBO ManagerをResource Managerが所有した場合はShaderがResource Manager側のResourceを参照する。この依存は成立するが、VBO Managerが両者にまたがる立ち位置であることは残る。

#### 2. ShaderもResource Managerへ所有させる案

ShaderとVBO ManagerをResource Manager配下へまとめると、VAO構築に必要なVBOとの関係を同一Ownership Boundary内に閉じられる。既存Render ResourceもShaderとRegistryをまとめていたため、この構成自体が不自然というわけではない。

一方、Resource ManagerはGeometry / MaterialのAcquire / ReleaseとRegistry管理に加え、Shader Program、VAOのLifetimeまで統括することになる。Shader実装を別Moduleへ置いたままでも、Resource Managerの論理責務が拡大する。またShaderだけを再生成する場合の窓口やLifetime ContractもResource Manager側で扱うことになる。

**ShaderをResource Managerへ戻す案を最終採用する理由はない**と判断した。ShaderがResource Managerに所有されなくても、必要なVBOを安全にBorrowできればよい。

#### 3. VBO ManagerをShaderが所有し続ける案

VAOとVBOの使用関係をShader側に閉じられる反面、Registry / Resource ManagerがGeometry Upload / Final Freeを実行するためにShaderへ依存する構造が残る。GPU StorageとShader ProgramのLifetimeも不必要に結びつくため、今回の再編目的に合わない。

#### 4. VBO ManagerをResource Managerが所有する案（直前までの採用案）

Resource ManagerがGeometry RegistryとVBO Managerを所有すれば、Geometry Resourceの取得・解放とそのBacking Storageを同じ管理Domainに収められる。ShaderはResource Managerが保有するVBOを借用してVAOを構築できるため、ShaderからResource Managerへの片方向Dependencyとして成立する。

しかしVBO Managerの役割は、ResourceのUpload / FreeだけでなくShaderのVAO初期化に必要なBuffer Bindにも及ぶ。実際のOperation責務から見ると、VBO ManagerはResource Manager固有の内部部品というより、**ShaderとResource Managerの双方から利用される共通基盤**である。

この認識により、Resource ManagerをVBO Managerの恒久Ownerとする前提を再検討した。

#### 5. VBO ManagerをRenderer Frontendが所有する案（採用）

Renderer FrontendはShaderとResource Managerをともに所有する上位Composition Rootである。VBO Managerも同じFrontendが所有すれば、三者のLifetime関係を上位で保証しつつ、下位では対等な兄弟Moduleが相互に依存せずにVBO Managerを利用できる。

```text
Frontend owns: Backend Context / VBO Manager / Resource Manager / Shader

Resource Manager ─┐
                  ├──> VBO Manager [Operation-scoped Borrow]
Shader           ─┘

Resource Manager ─X─> Shader
Shader           ─X─> Resource Manager
VBO Manager      ─X─> Geometry Registry / Shader
```

これを採用した。

#### 6. Borrow方法としてPointer保持と引数渡しを比較

ShaderとResource Managerの双方がVBO Managerを利用することを理由に、長寿命ObjectへBorrowed Pointerを保存する必要はない。利用が必要となるConstructor / Deinitializer / Import / ReleaseなどのOperationで明示的に引数として渡せば成立する。

このため、**VBO ManagerのBorrowは原則としてOperationの引数で渡し、下位Objectに長期保持しない**ことを採用した。

なお、Shaderが生成したVAOにはOpenGLのBuffer Binding関係が保持され得る。これは`vbo_manager_t*`というC側Object Pointerを保持することとは区別する。VBO Backend ObjectのLifetime / IdentityとVAOとの整合性は引き続き保証が必要である。

#### 7. 直ちにFrontend所有へ移行せず、Resource Managerを暫定Ownerとする案（採用）

議論時点で正式なRenderer Frontendは未完成であり、VBO Managerを最終Ownerへ直接移す場所がない。これを理由にFrontendの実装を拙速に開始したり、既存OwnerからVBO Managerを取り除いたまま所在不明にしたりすることは避ける。

まずResource Manager（移行中は旧名`render_resource`）がVBO Managerを一意に所有し、既存APIの移動、RegistryのShader非依存化、ShaderからVBO操作責務の除去を段階的に進める。Frontendが完成した時点でVBO ManagerのCreate / DestroyおよびOwned FieldをFrontendへ移す。

この際、一時的にResource ManagerとFrontendが同じVBO Managerを二重所有する状態を作らない。移管に伴って必要なConstructor / Cleanup / Validation / Call Site / VAO初期化を同一Workで整合させる。

### Rationale

- **Semantic Ownerに沿った配置:** VBO ManagerはGPU Buffer / Allocationを管理する共通基盤であり、Shader ProgramにもGeometry Registryにも専属ではない。
- **一方向のDependency:** FrontendがOwnerとなり、ShaderとResource Managerは同じVBO Managerだけを必要時に利用する。Shader / Resource Manager間の相互参照が不要になる。
- **最小の長期State:** Operation引数で借りられるVBO Manager Pointerを各ModuleのFieldへ重複保持しない。
- **OwnershipとResource Referenceの分離:** FrontendがVBO Managerを所有しても、個々のGeometryの参照数やFinal ReleaseをFrontendへ移さず、Registry / Resource ManagerのAuthorityを維持できる。
- **Vertical Sliceの維持:** VBO Managerの一般的な役割を共通基盤と捉えながら、各Mesh種別の異なるVertex Format / Buffer Configurationを一つのInstanceへ強制統合しない。
- **安全な段階移行:** Renderer Frontend完成までの暫定Ownerを明示することで、Ownershipが未定の期間や二重Ownerを作らず、作業を小規模なWorkに分割できる。

### Consequences / Implementation Contract

#### 最終OwnershipとDependency

- Renderer FrontendはBackend Context、各Mesh種別のVBO Manager、Resource Manager、ShaderのLifetimeを統括する。
- Resource ManagerはGeometry / Material等のRegistryとResourceのAcquire / Releaseを担う。VBO Managerの長寿命Ownershipを持たない。
- Geometry RegistryはVBO Allocation Descriptorを保持し、個別Resourceの最終解放時に、該当Allocationを発行したVBO Managerを引数でBorrowしてFreeする。RegistryがVBO Managerを所有するわけではない。
- ShaderはProgram / VAO / Uniformを所有し、VAO構築に必要なVBO ManagerをConstructor / Initialization Operationの引数でBorrowする。
- ShaderとResource Managerの間には互いへの直接Dependencyを設けない。Mesh Rendererは描画中だけ両者をBorrowする。
- VBO ManagerのBackend Buffer Identityは、VAOがそのBufferを参照している間に無断で破棄・差し替えない。詳細なRebuild / Hot Reload Policyは将来の検討対象とする。

#### Lifecycle

```text
[Target Create]
  Renderer Backend Context
      ↓
  VBO Manager(s)
      ↓
  Resource Manager(s) / Registry(s)
      ↓
  Shader(s) / VAO(s)  (VBO Managerを一時Borrow)
      ↓
  Frontendを公開

[Target Destroy]
  Shader(s) / VAO(s)
      ↓
  Resource Manager(s) / Registry(s)
      └── RegistryのVBO Allocation解放時にVBO Managerを借用
      ↓
  VBO Manager(s)
      ↓
  Renderer Backend Context
```

Resource ManagerとShaderの相互の厳密な生成順は、FrontendのComplete Constructor / Borrow条件を満たすよう実装時に定義する。ただし、ShaderによるVAO生成より前に対応するVBO Managerが利用可能であり、VBO Managerの破棄より前にそれを参照するVAOおよびAllocationのLifetimeが終わることを保証する。

#### 暫定Ownerの解消条件

- Frontend未実装の間はResource ManagerにVBO ManagerのOwned Field、Create / Destroyが存在してよい。
- Frontend導入時に、対応するVBO ManagerのOwnerをFrontendへ移し、Resource Managerに残るOwned FieldとCreate / Destroyを削除する。
- Resource Manager、Import Pipeline、Geometry Registry、Shaderへは必要なOperation引数で同じVBO Managerを渡す。全APIに機械的に追加するわけではない。
- 途中の移行が完了した時点で、VBO ManagerのOwnerを二重化していないこと、登録済みAllocationの解放元が正しいこと、VAOが有効なVBOを参照することを確認する。
- Reference CountingのAcquire / Releaseがまだ未実装の段階では、既存VBO Write / Free APIを暫定的に公開してよい。Acquire / Release完成後にその公開範囲を再検討する。
- `vbo_range_t` / `draw_range_t`の型所属とHeader移動は独立した後続Taskとし、Ownership移管に混ぜない。

#### 既存のDesign Decisionとの関係

- **2026-10-19「Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する」**: Resource Manager / Shader / Stateless Mesh Renderer / Import Pipelineの責務分離とRegistry Reference CountのAuthorityは維持する。一方、**VBO Managerの恒久OwnerをResource Managerとした部分、およびShader→Resource ManagerのBorrow前提は本Decisionにより置き換える**。旧判断の議論経緯は削除せず、その後の再検討として本Decisionを追記する。
- **2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」**: Frontendが上位Ownership / Composition Rootとなる原則は維持する。FrontendがRegistryを直接所有する構成へ戻すのではなく、Resource ManagerがRegistryのOwnerとなる。
- **2026-09-12「Resource Pipelineはstateless orchestrationとして維持する」**: Import Pipelineは引き続きStatelessなOperation Moduleとし、長寿命VBO Manager PointerのOwnerにはしない。

### Revisit Conditions

- ShaderとResource Managerの双方が同じVBO Managerを必要とするという前提が変わり、別のLifetime Domainが自然になった場合。
- VBO ManagerのPer-Slice分離が、実測性能や具体的な跨Slice共有要件を満たせなくなった場合。
- VAO / VBOのRebuild、Backend切替、Context Loss、Hot Reload等により、Operation引数だけのBorrow / Lifetime Contractでは安全性が不足する場合。
- Renderer FrontendのComposition責務が肥大化し、VBO Manager群の専用上位Ownerを設けた方が合理的となった場合。
- GPU Buffer StorageのLifetimeとGeometry Reference Lifecycleが将来のStreaming / Residency管理等によって分離され、現在のRegistry / VBO Manager境界を変更する必要が生じた場合。

### Related Documents

- `glce_renderer_resource_model_workplan.md` — 暫定Resource Manager OwnerからFrontend最終Ownerへの段階的移管
- `design_notes.md` — Renderer Domain / VBO Managerの位置付け
- `validation_policy.md` — Ownership移管、解放とFail-stop
- `boundary_model.md` — Lifetime Authority / Borrow Boundary
- `coding_style.md` — Create / Destroy、Operation Parameter、Public / Internal API

---

## Decision: Renderer Frontendを先行確立し、Untextured Meshの描画経路をFrontendへ直接集約する

**決定日:** 2026-10-20〜21  
**Status:** 採用・Untextured Meshの描画動作を確認済み（2026-10-21時点）。関連Cleanupと追加Validationは未完了。  
**関連する過去Decision:** 2026-10-15「Renderer FrontendをResource Ownership Rootとし、Mesh RendererをStateless Draw Moduleとする」、2026-10-19「Renderer Resource LifecycleをResource Manager / Shader / Mesh Renderer / Import Pipelineへ再編する」、2026-10-19「VBO ManagerをRenderer Frontendの共通基盤とし、Shader / Resource Managerから引数でBorrowする」。これらは検討当時の履歴として保持し、本Decisionが**実装方式に関して後から更新した部分**を優先する。

### Context / Background

旧`application_renderer`はBackend Contextと各`*_render_resource`を所有し、Render Resource側ではShader / Registry / Draw / Import・Releaseの操作が集まっていた。Model単位のResource参照権、将来のAcquire / Release、GeometryとShaderの独立したLifetimeを整えるため、Ownership RootとしてのRenderer Frontendが必要だった。

当初は旧Render ResourceをResource Managerへ移す方式や、Frontendと独立したStateless Mesh Rendererを新設する方式を検討した。しかし、Resource Managerの独立責務はReference Counting / Acquire・Releaseの実装前には確定できず、一時的なOwnerを作ってから移管することは二重作業になり得た。また、Draw操作をFrontendから単にMesh RendererへForwardするだけなら、実質的な抽象化が一段増えるだけになる。

そこで、Frontendを先に構築して直接最終Ownerにする方針に変更した。Untextured MeshのSource移行中に、Drawの責務とPublic APIを実際に検討し、独立Mesh Rendererを取り下げた。

### Accepted Decisions

1. **Frontend先行 / Resource Manager導入保留。** `renderer_frontend_t`はRenderer Backend Context、Untextured Mesh用VBO Manager、Geometry Registry、Material Registry、Shaderを直接所有する。`resource_manager`という新Objectを前提にしない。Acquire / ReleaseやModel Pipelineの実装で独立責務が明らかになった場合に改めて検討する。
2. **BorrowをOperationへ限定。** Geometry Pipeline・Geometry Registryの解放処理・ShaderのVAO構築などは、必要なOperationの引数で該当VBO ManagerをBorrowする。ShaderはVBO ManagerやGeometry Registryを所有しない。RegistryもShaderへ依存しない。複数Sliceを一つのVBO Manager Instanceへ強制統合しない。
3. **Untextured MeshのDrawはFrontend Public APIへ直接実装。** `renderer_frontend_untextured_mesh_draw()`がGeometry ID / Material IDからResourceを解決し、ShaderにMaterial・Model Matrixを設定し、VAOをBindして描画する。中間の`untextured_mesh_renderer`専用Moduleや独自Result Domainは作らない。
4. **ShaderはShaderの仕事に限定。** `untextured_mesh_shader`はProgram / VAO / Uniform Operationを担当する。過去の`untextured_mesh_render_resource_draw()`をShader Sourceへ移した暫定実装は不適切であり、Frontend Drawへ移す。Raw OpenGL Draw CallをRenderer Backendへ移す作業は別Trackとする。
5. **Import / Releaseの入口をFrontendへ置く。** Geometry / MaterialのImport / ReleaseはFrontendから既存のStateless Pipelineへ直接委譲し、Pipeline / Registryの責務を再実装しない。Resource IDの取得・解放とRegistry内部のStorage / Transactionを混同しない。現行のImport / Releaseは将来のReference Countingを備えたAcquire / Releaseと同義ではない。
6. **移行では動作優先の局所的な切替を採用。** ApplicationのUntextured Drawは新Frontend経由へ切替済み。旧`application_renderer`と`render_resources`のSourceは、後続のLine / Point / UI移行の参照基準として残す。現時点で無理に削除しない。

### Renderer Updateと個別Setterの再評価

FrontendにView、Projection、Directional Light、Ambient Lightの個別Public Setterを追加したが、描画に必要なUniformを一括更新する`renderer_frontend_untextured_mesh_render_update()`を実装した結果、**Frontend側の個別Setterは不要**と判断して撤去した。Shader Module内の低位SetterはUpdate内部で必要なため維持する。

Updateは対象Shader ProgramをBindしてからProjection / ViewとLighting Uniformを設定する。Directional LightのDirectionはWorld Spaceの方向ベクトルを`w = 0`としてView Matrixで変換し、View SpaceでShaderへ渡す。DrawはGeometry / Material / Model Transformの描画対象ごとの処理を担当する。この分担により、ApplicationはShaderの個々のUniform APIを呼び分けずに済む。

UpdateとDrawにより描画機能が明確になったため、`renderer_frontend_t`の構造体定義を`renderer_frontend_internal.h`に移し、Geometry / Material APIとShader Operationを別Translation Unitへ分割する案も**採用必須とせず保留**した。現時点ではOpaqueなFrontendの内部構造体を`renderer_frontend.c`だけで管理し、今後Line / Point / UIの追加により実際に肥大化・変更干渉が生じたときに再検討する。

### Error / Validation / GPU Boundaryで確認した事項

- `size_t`で保持するDraw RangeをOpenGL `GLint` / `GLsizei`へ変換する前に、各値が`INT32_MAX`以下であることを確認する。`size_t`の加算が必要な箇所では`SIZE_MAX`に基づくOverflow検証を別目的として扱う。Draw Rangeの実際の有効領域検証は、Registry / VBO Managerの既存Contractとの重複に注意して検討する。
- Import Pipelineで`DATA_CORRUPTED`が確定したときは、通常Rollback / Cleanupを続行しない。`untextured_mesh_geometry_pipeline_import_from_file()`ではCleanup冒頭およびVBO Rollback失敗時のEarly ReturnでFail-stopを明示する。これを全Moduleへ機械的に広げること自体は今回のDecisionではない。
- Result Domainを増やすことが目的ではない。Frontendは`renderer_frontend_result_t`を返し、Pipeline / Shader等からのResult Conversionは必要な境界に限る。不要になったConversion・Include・中間Helperは移行が安定した後にAuditする。
- OpenGL State（Program / VAO / Draw、Framebuffer Clear、Depth / Viewport、Swap）の操作順序を区別する。Application側に残るRaw `gl*`操作をFrontend / Backendへ統合するかは、独立したBackend Boundary Cleanupで扱う。

### Current Evidence / Completion Boundary

2026-10-20 23:58のSource Snapshot `glce_source_context(20261020-235854).txt`では、ApplicationがFrontendをCreate / Deinitializeし、Untextured Geometry / MaterialをFrontend APIでImportし、Update / Drawを呼び出している。2026-10-21、ユーザーにより**ペンギンの描画成功（初回の実行で成功）**が報告された。これはRuntime描画の成立を裏付けるが、異常系、Release単体、Canonical Validation、TEST_BUILD・Sanitizerの網羅的完了を意味しない。

次Chatでは描画経路の局所的なBug Auditと不要コード整理を実施する。ただし、旧`application_renderer`と`render_resources`のSourceは保存する。移行完了扱いを旧実装全撤去と同義にしない。Line / Point / UI、Reference Counting、Model Pipeline、Sample Robot End-to-Endは後続Workの対象である。

### Rejected / Deferred Alternatives

- **先にResource ManagerをOwnerとして作る:** 独立責務が未確定で二度のOwnership移管になり得るため、今回の手順では採用しない。
- **専用`untextured_mesh_renderer` ModuleとResult Domain:** Frontend APIとの一対一Forwardingが増え、現行Draw規模で意味のある境界を作らないため採用しない。
- **Frontend側の個別Uniform Setter群を維持:** Render Updateへ集約できるため採用しない。Shader自体のSetterは維持する。
- **Internal Headerを先行新設:** 分割理由がまだ小さく、内部状態の共有範囲を広げるため保留する。
- **旧Renderer / Render Resourceをすぐ削除:** 他Sliceの移植元を失うため保留する。

### Revisit Conditions

- Frontendに複数Sliceを移行した結果、Source分割なしでは責務把握・レビュー・変更が困難になったとき。
- Import / Release / Acquire・Release / Model Pipelineの実装から、独立したStateless Resource OperationまたはStateful Resource Managerが実質的な責務を持つと判明したとき。
- Render Packet / Frame Allocator・複数Shader・Passを導入し、個別DrawとUpdateの適用タイミングが変化したとき。
- Mesh / Model側の構造によってGeometry ID / Material ID / Model Matrixを個別に渡す暫定Draw Signatureを変更すべきとき。

### Related Documents

- `design_notes.md` — Renderer FrontendのCurrent Architectureと保留事項
- `glce_near_term_todo.md` — 直近Cleanupと後続Workの依存順
- `glce_renderer_resource_model_workplan.md` — 長期Migration / Registry Refcount / Model Integration計画
- `validation_policy.md` / `boundary_model.md` — Fail-stop / Ownership / Borrow Contract

---

## Decision: Untextured MeshのVBOとGeometry ResourceのLifecycleをShaderから分離する

**Status:** Accepted（Untextured Meshに実装済み。Runtime正常描画を確認済み。異常系・全Build構成の検証完了を意味しない）  
**Date:** 2026-10-21  
**Scope:** Renderer / Untextured Mesh / VBO Manager / Geometry Registry / Geometry Pipeline / Resource Lifetime

### Decision

Untextured Meshでは、GPU頂点Bufferの所有、Geometry Resourceの所有、ShaderのGPU State管理を次の責務へ分離する。

```text
Renderer Frontend [Composition / Lifetime Owner]
    ├── VBO Manager [GPU VBO本体とAllocation操作のAuthority]
    ├── Geometry Registry [Resource Name / CPU Geometry / Allocation DescriptorのOwner]
    ├── Material Registry
    └── Untextured Mesh Shader [Shader Program / VAOのOwner]

Geometry Import Pipeline [Stateless]
    CPU Geometry生成 → VBO ManagerへのUpload → Geometry Registryへの登録

Geometry Release
    Registry → VBO Managerから該当AllocationをFree
             → CPU Geometry / Resource Nameを解放
```

VBO Managerを使用する際は必要なOperationの引数でBorrowする。Geometry Registryの`unregister` / `deinitialize`は、Shader Objectを経由せず、渡されたVBO Managerを使用してAllocationを解放する。RegistryはVBO本体を所有しない。

Shaderの責務はShader Program、VAOおよびUniform Operationに限定する。VAOが保持するGPU Buffer Bindingと、Cの`vbo_manager_t*`への長期Borrowは区別する。

### Context

旧Untextured Mesh構造では、ShaderがVBOを所有・操作することを前提とした説明やAPI関係が残っていた。これではGeometry Resourceの解放がShaderのLifetimeやInterfaceに結び付き、Shader、Geometry Registry、GPU Storageの責務を独立に扱いにくい。

2026-10-19の「VBO ManagerをRenderer Frontendの共通基盤とし、Shader / Resource Managerから引数でBorrowする」と、2026-10-20〜21の「Renderer Frontendを先行確立し、Untextured Meshの描画経路をFrontendへ直接集約する」で最終OwnerとDependency Directionは決定済みだった。本Decisionはそれらを改めて置き換えるものではなく、Untextured Meshへ適用した結果を具体的なResource Lifecycleとして確定する。

### Rationale

- GPU StorageのAuthorityをShader Program / VAOのAuthorityから分離できる。
- Geometry Registryは自身が所有するAllocation Descriptorを基に、Shaderに依存せず登録済みGeometryを解放できる。
- Import Pipelineを一時的な構築・転送・登録Transactionのまま維持できる。
- Frontendが各ObjectのLifetimeを一元管理しながら、Registry内部のResource Identityや解放処理まで所有する必要がない。
- Source内のOwnership説明、Release API Contract、コメントを同じLifecycleに一致させられる。

### Consequences / Implementation Contract

- Shaderの破棄はShader ProgramとVAOを解放する。登録済みGeometryのCPU ResourceやVBO AllocationはShaderの破棄責務ではない。
- Geometry RegistryはResource Name、CPU Geometry、VBO Allocation Descriptorを管理し、登録済みResourceの解放を調整する。VBOそのもののOwnerにはならない。
- Geometry PipelineはVBO ManagerへのWriteを行い、Registry登録成功後の長期Resource Ownershipを保持しない。
- Registryの`deinitialize`は登録済みEntryの終了処理とLogical Lifetimeの終了を担当するが、Subsystem Allocatorから得たRegistry本体・Entry配列のBacking Storageを個別にFreeするものではない。
- VBO ManagerおよびBackend Contextの破棄は、それらを使用するShader / Registry等の終了後とする。
- 既存Doxygen / コメントについては旧Shader-owned VBOや旧Release順序を前提とする記述を整合させる。これは既存Contractの修正であり、新しいDocumentation Policyの導入ではない。
- Geometry / MaterialのReference CountingおよびAcquire / Releaseは未実装の後続Workとし、今回の`unregister`をReference Count付きの最終Releaseと同一視しない。

### Related Decisions / Revisit Conditions

2026-10-19のVBO Manager共通基盤Decisionと、2026-10-20〜21のFrontend先行Decisionを優先して参照する。VBOの再生成、VAOのBuffer Binding更新、Context Loss、Streaming / Residencyなどで現在のLifetime関係を維持できなくなった場合は再検討する。

### Related Documents

- `design_notes.md`
- `glce_renderer_resource_model_workplan.md`
- `glce_near_term_todo.md`

---

## Decision: Untextured Mesh ShaderのBackend ContextはOperation単位でBorrowし、長期保持しない

**Status:** Accepted（Untextured Mesh Shaderに実装済み。他のShaderには未展開）  
**Date:** 2026-10-21  
**Scope:** Renderer / Shader / Backend Context / Borrow / API Boundary

### Decision

`untextured_mesh_shader_t`は`renderer_backend_context_t*`を内部Fieldとして保持しない。

Shader Program / VAOの生成、破棄、Bind、Uniform設定など、Backend Contextを必要とするAPIは、実行するOperationの引数としてBackend ContextをBorrowする。Private Helperも必要な場合に引数を受け取り、長寿命ObjectへPointerを保存しない。

Backend ContextのOwnerはRenderer Frontendのままとし、ShaderがContextを生成・破棄したりOwnershipを取得したりすることはない。

```text
Renderer Frontend
    ├── Backend Context [Owned / unique within current GLCE Renderer]
    └── Untextured Mesh Shader [Owned]
            └── Shader Program / VAO [Owned]

Renderer Frontend Operation
    └── Untextured Mesh Shader API
            ├── Shader Resource [Borrow]
            └── Backend Context [Operation-scoped Borrow]
```

### Context / 旧Decisionの位置付け

2026-09-12の「Backend ContextはRender Resourceがborrowする」では、各Render ResourceのConstructorでBackend Contextへの長期依存を確立し、各OperationでContextを渡し直さない構造を採用した。

この判断には、Backend Contextを上位で一元所有することに加え、CallerへBackend Contextを意識させず、Render Resource単位でAPIを完結させる狙いがあった。当時はRenderer Frontendが現在のようにShaderとBackend Contextを直接所有・統括する構造ではなかったため、長期Borrowは合理的だった。

その後、Renderer FrontendがBackend ContextとUntextured Mesh Shaderをともに所有し、ApplicationにはFrontendの高位APIだけを公開する構造が成立した。Frontend内部で必要なContextをShader Operationへ渡しても、Application側へBackend Contextを露出させずに済む。

また、VBO ManagerはすでにVAO初期化等のOperationで引数Borrowする方式を採用している。Backend Contextについても、Shader Object自身に非所有Pointerを保持する必要性を再評価した。

### Rationale

- Backend Contextへの依存が、Shader Objectの隠れた長期Fieldではなく、実際にBackendを使用するOperationの引数として明示される。
- Contextを渡す責務はRenderer Frontend内部に閉じるため、Application向けFrontend APIの抽象化境界は変わらない。
- Shaderの長寿命StateをShader Program / VAO / Uniform関連情報へ限定できる。
- Frontendが唯一のBackend Contextを所有・管理する現在のGLCEでは、異なるContextのIdentityを照合する仕組みを今回新設する必要はない。
- VBO ManagerのOperation-scoped Borrowと同じLifecycleの考え方で下位Resourceを扱える。

### Rejected / Superseded Alternatives

- **ShaderがBackend Contextへの長期Borrow PointerをFieldとして保持する:** 旧Render Resource境界を前提としたAPI簡素化の利点はあったが、現在はFrontendからOperation引数として渡せるため採用しない。
- **各ShaderがBackend Contextを所有する:** Backend ContextはSystem-level Resourceであり、ShaderごとのOwnerへ分散させない。
- **Global SingletonをShader内部から暗黙参照する:** OperationのDependencyがAPIから見えなくなるため採用しない。
- **複数Backend Contextを取り違えた際のIdentity照合を追加する:** 現行GLCEは唯一のBackend Contextを扱う設計であり、今回の変更の要件とはしない。

### Consequences / Implementation Contract

- `untextured_mesh_shader_create()`では従来どおりBackend Contextを受け取り、初期化時にのみ使用する。作成後のShader Instanceには保存しない。
- `destroy`、`use`、`vao_bind`、Matrix / Lighting / Material Uniform Setterは、必要なBackend Contextを引数で受け取る。
- Renderer Frontendは生成、通常破棄、Constructor失敗時のRollback、Render Update、Drawの各経路から同じOwned Contextを渡す。
- Shaderが利用するBackend Contextは、Shader Program / VAOの生成から破棄まで有効である必要がある。Renderer FrontendはShaderの終了後にContextを終了させる。
- この変更はObject Ownershipそのものを変更するのではなく、**長期BorrowをOperation-scoped Borrowへ変更する**ものである。
- `line_mesh_shader`、`point_mesh_shader`、`ui_mesh_shader`の同種変更は未実施であり、後続のSlice移行時にそれぞれの実装・API Contractと併せて検討・適用する。Untexturedの完了をもって全Shaderが対応済みとは扱わない。
- Backend Contextの一意性という現行前提が将来変わった場合は、その時点で適切なIdentity / Capability Contractを別途検討する。

### Supersession / Related Decisions

2026-09-12の「Backend ContextはRender Resourceがborrowする」は**上位によるContext所有と下位非所有Borrowの原則を維持する**。一方、Untextured Mesh Shaderに対する「生成時にContextへの長期依存を固定し、各Operationでは渡し直さない」という部分は、本Decisionにより更新する。旧Decisionの検討経緯はそのまま残す。

2026-10-20〜21の「Renderer Frontendを先行確立し、Untextured Meshの描画経路をFrontendへ直接集約する」と併せて適用する。

### Related Documents

- `design_notes.md`
- `glce_renderer_resource_model_workplan.md`
- `glce_near_term_todo.md`
- `boundary_model.md`

---

## Decision: Untextured ModelのEnd-to-End完成を他Mesh Sliceへの横展開より優先する

**Status:** Accepted（作業順序の決定。Model End-to-Endの実装完了ではない）  
**Date:** 2026-10-21  
**Scope:** Renderer Migration / Model Lifecycle / Resource Acquire・Release / Work Sequencing

### Decision

Untextured MeshのFrontend単独描画が成立した後、ただちにLine / Point / UIのFrontend移行へ進まず、**Untextured ModelのResource Lifecycleと描画経路を縦方向にEnd-to-Endで完成**させる。

直近のUntextured Frontend Cleanupが完了した後の作業順序は次のとおりとする。

```text
Untextured FrontendのBug Audit / Cleanup / 検証
    ↓
Untextured Geometry Registry Reference Counting
    ↓
Untextured Material Registry Reference Counting
    ↓
Untextured Geometry / Material Acquire・Release
    ↓
Untextured Model Config / Model Pipeline
    ↓
Sample Robot End-to-End / Sharing / Rollback
    ↓
実装結果に基づくRenderer / Model / Resource Boundary Review
    ↓
Line / Point / UIをSliceごとに移行
```

既存Work Planの識別子では、`C1–C3 → W6 → W7 → W8 → W9 → W10 → W5 → W4`の順である。W番号は履歴参照のため再採番しない。

### Context

Untextured MeshのFrontend経由の正常描画は、Backend Context / VBO Manager / Registry / ShaderのOwnershipと描画連携を一定程度確認できる。しかしModelを導入したときに必要な、ResourceのCache Hit / Miss、Reference Count、Model単位のReference Right、複数Modelでの共有、Coordinate Tree、Posture、失敗時Rollbackまで成立したことは意味しない。

ここでLine / Point / UIへ同じOwnership形状だけを先に複製すると、Untextured Modelの実装で新たに判明したResource Boundaryの変更を、複数Sliceへ再適用する可能性がある。

### Rationale

- 一つのVertical SliceでImportからModel単位の最終Releaseまで検証してから、他Sliceへ設計を展開できる。
- Reference Counting、Acquire / Release、Model PipelineとのDependencyが実装によって具体化するため、Resource Managerを先行して固定する必要がない。
- 複数Sliceへ広げた後の大規模なAPI / Ownership再変更を抑えられる。
- 各Slice固有のGPU StateやTexture Lifecycleを、未確認の共通化へ強制せずに済む。
- Sample Robotを使い、複数Geometry、階層Transform、Resource Sharing、Rollbackという実際の利用形態で検証できる。

### Rejected Alternative

Untexturedの単独Drawが成功した時点でLine / Point / UIを一括してFrontendへ横展開し、その後で全SliceのReference Counting / Model連携を追加する方式は採用しない。初期段階の描画成功だけではModel境界の設計を十分に検証できず、同じ変更が複数Sliceへ波及するためである。

### Consequences / Completion Boundary

- `W6`および`W7`はまずUntextured Geometry / Material Registryを対象とする。他3種類のGeometry RegistryのReference Countingは、それぞれのSliceを移行する`W4`で必要に応じて扱う。
- `W8`ではResource Managerの新設を必須としない。実際に必要なAcquire / Release境界から、Frontend内Operation、Stateless Module、独立Resource Manager等を比較する。
- `W9`のModel PipelineではFrontendへの循環依存を作らず、Modelが取得・保持するResource Reference Rightを明示する。
- `W10`では同一Model内の重複Acquire抑制、複数Modelの共有、一方だけのRelease、最後のRelease、Posture反映、失敗時Rollbackを検証する。
- `W5`のArchitecture Reviewは、Model利用実績が得られた後に行う。
- `W4`でLine / Point / UIを一種類ずつ移行する際、ShaderのBackend Context長期Borrow廃止を含むUntexturedで得た知見を適用対象として確認する。
- 旧`application_renderer`と`render_resources`のSourceは、後続Sliceの移植元として当面保持する。Untextured Modelの完成とLegacy Sourceの全撤去を同義にしない。
- 最新の進捗、未完了項目、詳細Done条件はWork PlanとNear Term TODOを正本とし、このDecisionでは採用した**順序と理由**を固定する。

### Revisit Conditions

- Untextured Modelを完成させる前に他Sliceの移行が必要となる、具体的で優先度の高い要件が生じた場合。
- Model Pipelineの実装から、他Sliceの共通基盤を先に導入した方が依存関係や検証を明確に改善できることが判明した場合。

### Related Documents

- `glce_renderer_resource_model_workplan.md`
- `glce_near_term_todo.md`
- `design_notes.md`
