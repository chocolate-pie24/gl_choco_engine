<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Validation Policy

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

最終更新: 2026-10-03

## 1. 文書の位置づけ

本書は、GLCEにおけるvalidationの責務、検証範囲、自動実行方針、stable boundary、failure時の扱い、および性能とのバランスについて、project-wideで適用する正式方針を定める。

validation固有の規則については本書を正本とする。`design_notes.md`、`design_decisions.md`、`glce_coding_style.md`等と内容が重複または矛盾する場合、validationに関しては本書を優先する。

Boundaryの分類、Boundary Owner、Outside / Inside、trust / authorityの切り替わり等のarchitecture modelは`boundary_model.md`を正本とする。本書では、そのBoundary Modelを前提として、validation responsibility、trust promotion、およびfailure classification上の帰結を定める。

本書では、次を分離して扱う。

- operationがどのsemantic contractに責任を持つか
- contract違反をruntimeで検査するか、callerが成立させるtrusted contractとして扱うか
- validatorそのものが何を意味するか
- public APIが通常実行時にどのconditionをautomaticに検査するか
- DEBUG／TESTでどのinternal validationを自動実行するか
- validation failureをどのresult／system behaviorへ変換するか

contract responsibilityを持つことと、そのconditionを毎回runtimeで検査することは同義ではない。

canonical validatorの意味はBUILD_MODEによって変更しない。BUILD_MODEによって変えるのは、主としてautomatic validationの実行位置とdepthである。

Runtime Status ReportとValidation Reportは異なるdiagnostic responsibilityとして分離する。Runtime Status Reportは現在のruntime stateを観測するためのものであり、Validation Reportはvalidatorを明示的に実行してcontract上のvalidityを診断するためのものとする。

Validation Reportの具体的なstructured representation、system-wide self-diagnosticsの実行方式、共通validation macro、およびFATAL state controllerの最終仕様は今後設計する。

---

## 2. 目的

GLCEのvalidationは、次を目的とする。

- GLCE自身の実装bugによってinternal invariantが破壊されたことを検出する。
- operationが安全かつ正しく成立するために必要なcontractを明確化し、必要に応じてruntime validationする。
- public APIの正常終了後に、documented preconditionが成立している範囲で、対象objectが有効なstable stateにあることを保証できるようにする。
- unit test、DEBUG時の診断、および将来のRELEASE self-diagnosticsで共通利用できるvalidatorを提供する。
- objectのdata structure、ownership、lifecycleおよびsemantic contractを明確化する。
- corruption検出後に追加operationを抑止し、二次破壊を避けるためのfail-stop判断材料を提供する。

すべてのcontractを通常実行経路で常時再検査すること、またはすべての潜在的bugを通常実行経路で検出することは目的としない。validationによってengineが実用不能になることを避け、検出能力、memory safety、計算量および実行頻度のバランスを取る。

validatorは、corruption状態に対しても、可能な限りmemory-safeかつboundedに検査を行い、不正なmemory accessを起こす前にfailureとして検出することを目指す。

pointer、size、count、capacity、allocation metadata、ownership情報等を利用してdereference前に安全性を確認できる場合は、その確認を行ってから対象memoryへアクセスすることを候補とする。ただし、そのconditionがtrusted／hard preconditionとして定義されている場合は、RELEASE_BUILDでのruntime checkを一律には要求しない。

C言語および現在利用可能なruntime informationだけでは安全性を確認できない場合、またはその確認機構を構築・維持するための複雑性や実行コストが得られる安全性に対して過大となる場合は、完全なmemory-safe validationを要求しない。この判断は一般則として機械的に適用せず、対象となるdata structure、operation semantics、corruption時の影響、実装複雑性および実行コストを踏まえて、その都度検討する。

そのような領域については、sanitizer、unit test、failure injection、fuzzing、およびmemory allocatorのallocation metadata validation等の補助的な診断手段と組み合わせて検出能力を高める。

---

## 3. Validation model

GLCEでは、contract responsibilityとvalidation depthを別軸として扱う。

```text
operation semantics
        ↓
semantic consumption
        ↓
contract responsibility / semantic ownership
        ↓
checked precondition or trusted / hard precondition
        ↓
必要な場合のみautomatic validation
        ↓
direct condition check / shallow validation / canonical validation
```

API contract、shallow validation、canonical validationは単純な強弱関係ではない。

- API contractは、callerとcalleeの間で何が成立している必要があるかを定義する。
- shallow／canonicalは、objectまたはvalueのvalidityをどのdepthで確認するかを定義する。
- contractが存在することは、そのcontractを毎回automaticにruntime validationすることを意味しない。

### 3.1 Semantic consumptionとoperation responsibility

各operationは、その処理が実際にconsumeするsemanticについてcontract responsibilityを持つ。

ここでいうconsumeとは、その値、state、relationまたはstructureの意味がoperation結果、安全性、有限実行、resource operation、indexing、allocation、mutation等へ実際に影響することを指す。

次のことだけを理由として、対象全体のsemantic validityを現在のoperationのvalidation責務としない。

- 引数として受け取る
- 別moduleへ転送する
- 単純にcopyする
- storageへ保存する
- source representationから別representationへ変換する際、source側semanticそのものを再認証する必要がない
- 既存stateを読まず、既知のstateへ全面上書きする

したがって、valueまたはobjectを受け取ったAPIが、その型のcanonical validatorを機械的に実行することは要求しない。

producer／converterは、source representationがdocumented preconditionを満たしていることを前提として、そのoperation自身の変換責務を果たす。source型のcanonical validityを再認証することは、その変換operationがsource semanticを検査する責務を明示的に持つ場合を除いて要求しない。

consumerは、対象semanticを実際に利用する場合、そのoperationに必要なcontractを定義する。ただし、そのcontractをchecked preconditionとしてruntime validationするか、trusted／hard preconditionとしてcallerへ要求するかは別途決定する。

value type、temporary object、stateful object等の分類だけでautomatic validation方針を決定しない。判断基準はoperation semanticsとsemantic consumptionである。

### 3.2 API contract

API contractは、callerとcalleeの間でoperationを成立させるために要求するconditionを定義する。

代表例:

- pointerの存在条件
- output pointer／out slotの初期状態
- API自身が直接必要とする初期化、登録、ロード等の操作可能状態
- API自身が直接必要とするownership／lifecycle contract
- size、count、index、ID、enum、flagおよび値に関するoperation-specific condition
- range condition
- arithmetic overflow／underflowを避ける条件
- source representationまたは入力valueが、そのAPIのdocumented preconditionを満たしていること

API contractは、runtime validationの方式によって次のように区別する。

#### Checked precondition

違反が通常利用時にcaller-controlled input、external factor、runtime state等から発生し得て、APIがrecoverable failureとして扱うconditionである。

checked preconditionは、RELEASE_BUILDを含む通常buildでruntime validationする。

代表例:

- recoverable inputとして扱うNULL pointer
- 0を許可しないcount／size
- operationとして無効なID／enum／range
- 現在stateでは許可されないoperation
- external APIから返されるEOF／error／status

#### Trusted / hard precondition

正しいcallerが成立させるprogramming contractであり、違反時のrecoverable behaviorをAPI contractとして提供しないconditionである。

trusted／hard preconditionは、DEBUG_BUILD／TEST_BUILDでassertionまたはdiagnostic validationを行ってよいが、RELEASE_BUILDでのruntime validationを一律には要求しない。

trusted／hard preconditionを採用する場合、そのconditionはheader、module boundary contract、またはAPI documentationでcallerから確認可能にする。

trusted／hard precondition違反時のbehaviorは、正常系contractおよびrecoverable failure contractの対象外とする。ただし、明示的にundefined behaviorという用語を使用するかどうかはmodule/APIごとに決定する。

同一種類のconditionをproject-wideで必ずcheckedまたはtrustedに固定しない。例えばpointer non-NULL conditionであっても、public recoverable APIではchecked、low-level utilityではtrustedとして扱うことを許容する。

### 3.3 Semantic ownership

semantic conditionは、その意味を所有するmoduleまたはoperationが一元的に定義する。

- forwarding／facade／orchestration layerは、下位semantic ownerが扱う条件を重複して検査しない。
- 値を引数として受け取ることだけを理由に、その値のsemantic validityを現在のmoduleの責務としない。
- operationがsemanticを実際にconsumeする場合、そのoperationは必要なconditionについてcontract responsibilityを持つ。
- contract responsibilityを持つこととruntime checkを実行することは別であり、Section 3.2に従ってcheckedまたはtrustedとして扱う。
- semantic ownerから返されたfailureは、上位layerで不必要に別の意味へ潰さず、適切なresult codeへ変換して伝播する。

この原則はBUILD_MODEに依存しない。DEBUG／TESTであってもforwarding layerへsemantic validationを機械的に重複追加しない。

caller-providedなcontainer／arrayについては、containerとしてのstructureと、格納elementのsemanticを分けて扱う。

- pointer existence、element count、size calculation、indexing range等、containerを安全かつboundedに走査するために必要なstructural conditionは、そのcontainerを実際に走査するoperationがcontract responsibilityを持つ。
- element固有のsemantic validityは、そのsemanticを実際にconsumeするoperationがcontract responsibilityを持つ。
- element固有のsemantic operationを別moduleのAPIへ委譲する場合は、そのsemantic validationもsemantic ownerである委譲先moduleへ委譲し、呼び出し元では同一semanticを重複して検査しない。

ここで「elementを読むこと」と「elementのsemanticをconsumeすること」は区別する。単純なcopy、storeまたはforwardはelement dataをreadする場合があるが、その型固有のsemantic validityにoperation結果、安全性または成立条件が依存しない限り、それだけを理由としてsemantic consumptionとはみなさない。

### 3.4 Shallow validation

shallow validationは、対象objectを取り扱うために必要なroot fieldおよび局所的なstructural invariantだけを確認する。

原則としてO(1)で完了し、owned array、linked structure、可変長collection、およびowned child全体の走査は行わない。

代表例:

- size、count、capacityの基本関係
- NULL／non-NULLのfield間関係
- 後続処理で使用する加算や乗算がoverflowしないこと
- root objectだけで判定できるstate関係
- canonical validationを開始するためのstructural precondition
- compile-timeまたはtype contract上で小さな固定値に限定されるfield／fixed-size arrayの局所検査

固定長の小配列など、要素数が型として固定され実質的にO(1)である検査については、実装上loopを使用してよい。

### 3.5 Canonical validation

canonical validationは、対象object自身の責務とownership closureについて、その型が保証するcanonical model全体を検査するdeep／full validationである。

ここでいう「対象object自身」は、そのobjectが定義するinternal representation、semantic state、およびfield間relationを指す。

validatorへroot argumentとして渡されたobject自身のstorage validity、allocation livenessまたはallocation identityは、そのobject自身のownership closureには含めない。

root objectをowned pointerとして保持するownerが存在する場合、そのstorage validityはowner側のownership closureとして扱う。独立したownerを持たずcaller-provided storage上に存在するobjectについては、root objectをdereference可能なlifetime中のstorageとして渡すことをcaller contractとして扱う。

本書ではdeep validationとfull validationを同じ意味で使用する。

代表例:

- owned arrayの使用中要素
- linked structureのnodeと接続関係
- 管理countと実際の要素数
- rangeの連続性、重複および合計値
- objectが所有する子object
- aggregate invariant
- stable stateにtransient／transitioning stateが残っていないこと

一般にO(n)以上のコストを持ち、対象構造によってはO(n^2)となる場合もある。

ここでいうfullはprocess内の全到達objectを検査するという意味ではない。「対象object自身の責務とownership closureについて完全に検査する」という意味である。

canonical validatorの存在は、次を意味しない。

- その型を受け取るすべてのAPIがcanonical validatorをautomaticに実行すること
- constructor／initializerが完成valueを毎回self-validationすること
- private shallow validatorが必ず存在すること

private temporaryやderived processing contextについて、成功したchecked constructor／initializer／transformationだけを生成経路とし、独立したpublic lifecycleや任意mutationを持たない場合は、standalone canonical validatorを設けずvalid-by-constructionとして扱ってよい。

この場合、downstream private operationはconstruction成功によって成立したinternal contractを信頼し、自身が直接consumeするoperation-specific conditionだけを必要に応じて検査する。private typeが存在することだけを理由としてcanonical validatorを追加しない。

### 3.6 可変長buffer／arrayの全要素validation

caller-controlledな可変長bufferやarrayについて、element validatorが存在することだけを理由として全要素validationを自動実行しない。

array全体のstructural validationと、各elementのsemantic validationを区別する。arrayを走査するoperationがpointer、count、size relation等を検証することは、各elementについてcanonical validatorを実行することを意味しない。element validationの要否はsemantic consumption、semantic ownership、trust boundaryおよびvalidation costから個別に決定する。

RELEASE_BUILDでは、diagnosticを目的とした可変長buffer／arrayの全要素validationを自動実行しない。

DEBUG_BUILD／TEST_BUILDで全要素validationを自動実行するかどうかは、そのoperationが各要素のsemanticをどのようにconsumeするか、semantic owner、trust boundary、およびvalidation costを考慮して判断する。

判断時には、必要に応じて次を考慮する。

- operationの意味上、全要素の妥当性確認が必要か
- source representationをtrusted internal representationとして受理するboundaryか
- APIの実行頻度
- 要素数
- validatorの計算量
- validationによる漸近計算量の悪化

ただし、operation自体の成立、安全な実行、またはexternal inputをtrusted internal representationとして受理するために全要素の処理・検査が本質的に必要な場合は、ここでいうdiagnostic目的のautomatic validationとは区別する。

pointer + countで表現されるstorage extentについて、allocator metadata等からruntime verificationできる場合がある。ただし、そのvalidationを可能にするためだけに、特定allocator由来であることをAPI contractへ追加してcallerが利用可能なstorage provenanceを不要に制限しない。allocation provenance自体がmodule boundary contractとして必要な場合に限り、allocator authorityを利用したallocation identity／range validationを検討する。

### 3.7 Trust boundaryとinput classification

Boundaryの分類そのものは`boundary_model.md`を正本とする。本節では、External Trust BoundaryおよびEngine API Trust Boundaryをvalidation上どう扱うかを定める。

validation failureの意味を判断するときは、型のrepresentationがpublicかopaqueかだけでは決定しない。主要な判断軸は、対象が**caller-controlled input**なのか、正規のlifecycleを経て成立済みの**established internal state**なのかである。

```text
caller-controlled input
    ↓ validity failure
recoverable input failure

established internal state
    ↓ integrity failure
DATA_CORRUPTED candidate
```

Engine内部private moduleの型について、storage配置やmodule間利用の都合でstruct definitionがheader上に可視であっても、それだけを理由としてcaller-controlled valueとはみなさない。正規のinitialize / create後にmodule contract上validであることを前提として管理されるstateはestablished internal stateとして扱える。

#### 3.7.1 External Trust Boundary

External Trust Boundaryから受け取った値は無条件にtrusted dataとして扱わない。ただし、boundaryで全semantic validationを一括実行することも要求しない。

```text
untrusted external representation
    ↓
acquisition / parse / loader
    ↓ validation / normalization
accepted internal representation
```

- operationがelement semanticを直接consumeしない場合でも、external representationをinternal representationとして受理するboundaryであれば、boundary以降で成立済みとみなすsemantic conditionについてvalidationを行うことがある。
- trust promotion前のcandidateがrequired validityを満たさない場合、それはinternal state corruptionではなく、原則としてそのboundaryに対応するrecoverable rejectionとして扱う。file loaderであれば`UNSUPPORTED_FILE`等、external API wrapperであれば対応するruntime / I/O error等を使用する。
- malformed file、unsupported format、external device dataの不正等は、それ自体を理由として`DATA_CORRUPTED`へ分類しない。
- 一度validated internal representationとして受理されたdataについては、downstream operationがcopy、storeまたはforwardすることだけを理由として同一semantic validationを機械的に繰り返さない。
- downstream operation自身が型固有semanticをconsumeする場合は、upstreamでtrustが確立済みであることとは別に、そのoperation自身のcontract responsibilityに従う。

trust boundaryとsemantic consumptionは別の判断軸として扱う。

#### 3.7.2 Engine API Trust Boundary

Application、Frontendその他のEngine外側のcallerがEngine APIへ渡すconfig、descriptor、value、array等は、Engine API entryではcaller-controlled inputとして扱う。

そのinputがAPIのchecked validityを満たさない場合、原則として`INVALID_ARGUMENT`、`BAD_OPERATION`その他のrecoverable resultへ分類する。callerがそのvalueをどこから取得したか、またはEngine内部producerが過去に生成した可能性があるかをcallee側で推測し、provenanceだけを理由として`DATA_CORRUPTED`へ再分類しない。

```text
Application / Frontend controlled value
    ↓ Engine API Trust Boundary
validation / normalization / copy
    ↓
Engine-owned established state
```

Engineがcaller-controlled inputをvalidationし、必要に応じてcopy / normalize / constructした後、その値をEngine-owned established stateへ取り込むことでtrust promotionが成立する。

#### 3.7.3 Engine内部module間のinput

Engine内部module Aが生成したvalueをmodule Bへinputとして渡す場合でも、Bはそのvalueのprovenanceを推測してerror categoryを変更しない。Bから観測できる事実が「API inputが要求validityを満たさない」であるなら、Bは自身のcontractに従って`INVALID_ARGUMENT`等を返してよい。

```text
Module A
    ↓ invalid value
Module B
    ↓ input validation failure
INVALID_ARGUMENT
```

この場合、Aのimplementation bugがその場で`DATA_CORRUPTED`として分類されない可能性は許容する。Aがestablished stateを破壊している場合はA自身のcanonical validation、Commit eligibility、Postcondition validationまたは明示的diagnostic validationがcorruption detection pointとなり得る。Aが一時的temporary valueを誤生成しただけでestablished stateを破壊していない場合、通常operation failureとして局所化されることを許容する。

この方針は、result propagationを単純に保ち、caller contextやprovenanceに基づく複雑な再分類を避けるためのものである。

#### 3.7.4 Established internal state

正規のcreate / initialize / commit等を経て成立し、module contract上validであることを前提としてmoduleが管理しているobject、registry、allocator state、owned resource等はestablished internal stateとして扱う。

このstateについてcanonical invariantの破損が確認された場合は、`DATA_CORRUPTED`へ分類する根拠になり得る。

ただし、`is_valid() == false`という事実だけで機械的に`DATA_CORRUPTED`へ分類しない。そのvalidatorがcaller-controlled input、trust promotion前candidate、established internal stateのどれを検査していたかを確認する。

#### 3.7.5 External opaque resource

OS、standard library、external library、device等が所有するopaque resourceをGLCEのwrapper objectから参照する場合、external resource内部のstateはwrapper objectのcanonical invariantには含めない。GLCE側のcanonical validatorは、wrapper自身が所有するstate、handle / pointerの存在、およびGLCE側で定義するownership relationを検証する。external resource内部をGLCE独自のcanonical validationで診断しない。

external resourceに対するoperationの成否、EOF、error、status等は、external APIが提供するreturn valueまたはstatus mechanismに基づいて判定する。external factorから発生し得るfailure handlingは、RELEASE_BUILDを含む通常buildにも残す。

通信、外部file、device、external process等から取得した浮動小数点値は、GLCE内部のtrusted representationとして受理するacquisition / parse boundaryで、必要に応じてfinite性を含むvalidationを行う。

一度validated internal representationとして受理した浮動小数点値について、setter、getter、内部演算等でgenericな`isfinite()` checkを機械的に繰り返さない。

一方、あるvalue typeがfinite性そのものをcanonical contractとして明示的に保証する場合、そのcanonical validatorがfinite性を検査することは許容する。これはgenericなinternal revalidationとは区別する。

GLCE内部の演算では、genericなfinite性ではなく、そのoperation固有の成立条件を検査する。

代表例:

- 除算: divisorが0でないこと
- normalize: 対象vectorがzero vectorでないこと
- matrix inverse: matrixがsingularでないこと
- perspective frustum: `aspect > 0`、`0 < fovy < 180`、`0 < near < far`

有限な入力同士の内部演算でもoverflow等によってInf / NaNは生成し得るため、「GLCE内部のすべての浮動小数点状態は常にfiniteでなければならない」という一般contractは設けない。

### 3.8 Runtime StatusとValidation Diagnosticsの分離

Runtime Status ReportとValidation Reportは別の責務として扱う。

Runtime Status Reportは、対象moduleまたはsystemが現在保持しているruntime stateを観測するためのdiagnosticsである。

代表例:

- memory usage
- allocation count
- resource count
- capacity / usage
- subsystem固有のruntime state

Runtime Status Reportは、そのstateがcontract上validであるかどうかを表すものではない。

Validation Reportは、shallow validator、canonical validator、または将来追加されるdiagnostic validationを明示的に実行し、その結果を診断情報として提示する責務を持つ。

このため、runtime statusを表すstatus structureへ`is_valid`等のvalidation resultを組み込まない。

また、public API内部で行うautomatic validationと、Validation Report等から明示的に行うdiagnostic validationを分離する。

automatic validationは、Section 3およびSection 6の判断規則に基づき、そのoperationでruntime checkすることを選択したcontractまたはinternal invariantだけを対象とする。

一方、明示的なdiagnostic validationは、通常operationの成立条件を検査するためではなく、要求された時点で対象stateのintegrityを診断するために実行する。

したがって、status取得APIがDEBUG_BUILD／TEST_BUILDでautomatic canonical validationを内部実行する場合でも、そのvalidation resultをRuntime Status Reportの一部として扱わない。

---

## 4. Validator contract

### 4.1 Canonical validator

validationが必要な型には、原則として一つのcanonical validatorを設ける。

canonical validatorは、その型のdata structure、ownership、およびpublic lifecycleが確定したstable object modelに対して定義する。value typeでは、その型が明示的に保証するsemantic modelに対して定義してよい。

近い将来に削除・移動するfieldや、廃止予定の中間stable stateへ一時的に合わせるためだけのvalidatorを先に作らない。

基本規則:

- boolを返す妥当性判定は`*_is_valid`を基本とする。
- 対象は原則として`const`で受け取る。
- NULL-safeとし、対象pointerがNULLの場合はfalseを返す。
- validatorの意味はBUILD_MODEによって変更しない。
- runtimeのbool引数によってshallow／canonicalを切り替えない。
- validator自身は対象を変更しない。
- repairを行わない。
- 原則としてdynamic memory allocationを行わない。
- 文章ログを直接出力しない。
- assert／abortを直接行わない。

canonical validatorが検査する条件は、その型のdata structure、ownership、lifecycleまたはsemantic contractとして実際に保証されるinvariantに限定する。

「検査した方が安全そう」「異常値を早期発見できる」といった一般的な防御上の理由だけで、新しい条件をcanonical invariantへ追加しない。validatorは防御checkの一覧ではなく、その型が保証するcanonical modelの判定である。

浮動小数点値がfiniteであることを型の明示的contractとして保証していない場合、`isfinite()`等によるgenericなNaN／Inf checkをcanonical validatorへ機械的に追加しない。external trust boundaryでのfinite validationは3.7に従う。

APIごとに`is_valid_for_*`形式のvalidatorを機械的に増やさない。特定operationだけに必要な条件はAPI contractまたは意味の明確なstate predicateとして扱う。

### 4.2 Shallow validator

private shallow validatorは、module内にshallow validation depthを必要とする実際のcall siteが存在する場合に設ける。

canonical validatorを持つことだけを理由として、private `*_is_valid_shallow`を機械的に追加しない。

対象型に対するoperationが存在していても、そのoperationがautomatic shallow validationを必要としない場合はshallow validatorを省略してよい。

逆に、現在のcanonical validatorとshallow validatorの検査内容が実質同一であっても、operation call siteがshallow depthを意味として必要とする場合は分離して保持してよい。これはcall siteが要求するvalidation depthをコード上で明示するためである。

shallow validatorが存在する場合でも、canonical validator内でshallow validatorを最初に実行することはproject-wideには要求しない。validationの実行順序は、各validationが安全に実行できるためのdependency、dereference前に必要なallocation-level validity、およびmodule固有のmemory safety requirementに従って決定する。

shallow validationとownership closure validationの間に固定した順序は設けない。例えばowned childのallocation validityを確認した後にshallow validationを行う構成も、shallow validationによってchild pointerを安全に参照できることを確認した後にownership closureを検証する構成も、各validatorのdependencyが満たされていれば許容する。

shallow validatorが存在しない場合、canonical validatorが対象型のcanonical conditionを直接検査する。

`*_is_valid_shallow`をpublic APIとして公開する必要はない。public validatorはcanonicalな`*_is_valid`を基本とする。

shallow以外のprivate validator／validation helperを設けるかどうか、その粒度、名称および分割方法はmoduleごとのdata structureとoperation semanticsに応じて決定する。project-wide policyでは固定しない。

### 4.3 Ownershipとborrow

parent objectのcanonical validationは、そのparentがlifetimeを所有するchildをownership closureとして検査する。

owned childをpointerとして保持し、そのpointerについてMemory moduleからallocation-level validityを確認できる場合、parent canonical validatorはchildをdereferenceまたはchild canonical validatorへ渡す前に、そのallocation validityを確認することを候補とする。

owned childが独立したcanonical validatorを持つ場合、parentは必要なallocation-level validityを確認した後、child内部のcanonical validationをchild canonical validatorへ委譲してよい。

child canonical validatorは、自身へroot argumentとして渡されたobjectのallocation validityを重複して検証することを原則として要求しない。そのobject自身のstorage validityは、そのstorageを所有するowner側の責務とする。

embedded valueのように独立したallocationを持たないchildについては、allocation-level validationを要求しない。

owned arrayやowned collectionについて、canonical validator内部をどのprivate helperへ分割するかはmodule実装に委ねる。project-wide policyではraw array専用validatorの有無やhelper構成を規定しない。

borrowed pointerの参照先全体はborrowerのownership closureに含めない。borrowed objectのfull validationは、そのobjectのownerまたはsystem側validatorが担当する。

borrower側では、必要に応じて次のようなborrower自身から判定できるconditionについてcontractを定義する。

- pointerの存在が必須か
- optionalならNULL／non-NULLの組み合わせがborrowerのstateと整合するか
- borrower自身が所有するrelation／stateの整合性

これらをruntimeでchecked validationするか、trusted contractとして扱うかはSection 3.2に従う。

borrowerのcanonical validatorは、pointer値だけから借用先のlivenessやlifetime順序を保証しない。borrow lifetimeはownerおよび上位lifecycle contractの責務とする。

複数object間のrelationは、そのrelationをconsumeするoperationが必要な範囲で扱う。

### 4.4 Memory-safe and bounded validation

canonical validatorは、corruption状態でも可能な範囲でmemory-safeかつ有限時間でfalseを返すように設計する。

基本方針:

- NULL、count、size、capacity、overflow等、dereference前に確認できるconditionを先に検査する。
- traversalには管理上限または明確な終端条件を設ける。
- cycleが存在し得るstructureでは無限loopを防止する。
- 対象objectが所有するraw pointerについてallocation metadata等から安全性を確認できる場合は、dereference前に確認する。
- validatorへroot argumentとして渡されたobject自身のstorage validityは、そのvalidator自身ではなく、そのstorageを所有するownerまたはcaller側のcontract responsibilityとして扱う。
- linkage pointer等のmetadataが破損し得る場合、そのpointerを次のdereference先として無条件に使用しない。
- 既知の安全なrangeやphysical layoutから検査位置を導出できる場合は、その方法を優先してよい。

完全なmemory-safe validationを実現できない場合の扱いは、2章で定めた原則に従い、その都度検討する。

### 4.5 Memory validation trust boundary

`engine/memory`は、GLCE内部におけるallocation-level validationのtrust rootとする。

上位moduleのvalidatorは、対象pointerを管理するMemory moduleが提供するallocation query結果をauthorityとして扱い、そのqueryを成立させているallocator metadataのvalidityを再帰的に検証しない。

Allocatorごとに保持するmetadataとallocation semanticsは異なるため、利用可能なvalidation capabilityも異なる。

```text
Free List based allocation
    per-allocation metadataを利用可能
    allocation payloadの先頭やcurrent allocation stateを判定できる

Linear based allocation
    per-allocation metadataを保持しない
    current in-use range等、allocatorが提供するcoarser informationを利用する
```

上位validatorは、Memory moduleが提供していないallocation identity、allocation boundary、historical liveness等を独自に仮定しない。

一方、`engine/memory`自身のvalidatorは、allocator metadataが完全に無傷であることを別のallocation authorityによって証明するものではない。各Memory moduleは、既知のbacking memory range、capacity、physical layout、および局所metadata間の整合性から、検出可能なcorruptionを検査する。

`engine/memory`内部では、各low-level allocatorが自身の管理するmemory poolについてallocation stateのauthorityを持つ。General Allocator / Subsystem Allocator等の上位Allocatorは、owned low-level allocatorのcanonical validityやownership relationを必要に応じて検査するが、さらに別のGLCE allocatorへmetadata validityの証明を再帰的に要求しない。

このtrust boundaryは、allocator metadataを検証するために別のallocator metadataを要求し続ける無限後退を避け、GLCE software architecture内部でallocation-level validityの停止点を明確にするために設ける。

この方針はOS、libc、linker、virtual memory、MMU、hardware等を含むsystem全体の完全性を保証するものではない。guard page、sanitizer、MPU等の独立したdiagnostic / protection mechanismを将来追加する場合も、それらはこのsoftware trust boundaryを補強する別mechanismとして扱う。

---

## 5. Stable state and lifecycle

### 5.1 Stable state

documented preconditionが成立した状態で、lifetimeを維持するpublic APIが正常終了した場合、対象objectまたは生成されたvalueは、その型が定義するvalidなstable state／canonical modelを満たさなければならない。

この保証は、trusted／hard precondition違反時には適用しない。

stable stateまたはcanonical modelを保証することと、その保証をreturn前にcanonical validatorでautomaticに再確認することは別である。

private処理の途中で一時的に現れるtransient／transitioning stateは、canonical validatorが受理する必要はない。

constructor実装上だけ必要なpartial stateはprivate temporaryへ閉じ込め、public stable stateとしてcanonical validatorに受理させない。

empty、closed、unloadedまたは初期化前の状態をvalidなstable stateとするのは、その状態に独立した利用価値があり、許可操作、禁止操作および遷移条件をpublic contractとして定義する場合に限る。validatorを実装しやすいことだけを理由に不要なstable stateを増やさない。

validは必ずしもreadyを意味しない。validとreadyの関係は型ごとのpublic lifecycle contractで定義する。

### 5.2 Recoverable failure

public APIがchecked precondition violationまたはその他のrecoverable failureを返し、対象objectのlifetimeが継続する場合、対象objectはvalidなstable stateを維持または回復しなければならない。

- semantic Commit開始前に失敗した場合、原則としてoperationのsemantic effectを成立させず、既存のpublic state、ownershipおよびlifecycle relationを変更しない。
- Commit開始後にrecoverable failureが発生し得るoperationでは、可能な範囲でrollbackし、仕様化されたstable stateおよびownership / lifecycle relationへ戻す。
- constructor中のprivate partial stateはrecoverable failure時にrollbackしてよい。
- rollback不能なinternal corruptionは通常のrecoverable errorと区別する。

trusted／hard precondition違反はrecoverable failure contractの対象外である。

`DATA_CORRUPTED`はこのrecoverable failure規則の例外である。

### 5.3 Stable boundary

automatic Postcondition validationを行う場合は、operationのsemantic effectがCommitされ、対象objectのstate、ownership、lifecycle等が最終的に確定したstable boundaryで行う。

「return直前」を機械的な規則とはしない。temporary object、replacement object、ownership move、public publication等を伴う場合は、どのstate transitionをCommitとするかを先に明確化し、そのCommit完了後にPostcondition validationのboundaryを決定する。

Commit前に完成済みcandidate representationを検査し、public publication、ownership transitionまたはその他のsemantic transitionを実行してよいか確認するvalidationはPostconditionではなくCommit eligibilityとして扱う。詳細はSection 5.4に従う。

一般原則:

- temporaryを生成してpublic lifetimeへ公開するoperationでは、temporary完成後かつpublic Commit前のcandidate validationはCommit eligibilityである。Postcondition validationを行う場合は、publicationやownership transitionを含むCommit完了後に行う。
- 既存objectを変更するoperationでは、automatic Postcondition validationを行う場合、対象への最後のmutationまたはrollback完了後がstable boundaryとなる。
- ownership relationまたはlifecycleを変更するoperationでは、そのoperationのcontractを構成するstate transitionがすべてCommitされた時点をstable boundaryとする。
- getter／read-only operationでは、通常stable-exit canonical validationを行わない。
- external opaque resourceのstateだけが変化し、GLCE-owned canonical stateが変更されないoperationでは、external resourceのstate changeだけを理由としてGLCE objectのcanonical Postcondition validationを機械的に行わない。
- destroy／deinitializeでは、logical lifetime終了後のcanonical validationを要求しない。

value construction／conversionでcanonical-validなresultをcontractとして保証していても、そのことだけを理由としてautomatic canonical validationを要求しない。pure Outputだけを行いsemantic Commitを持たないoperationにはCommit eligibilityは存在しない。

### 5.4 Commit eligibility

operationがmutation、ownership／lifecycle transition、registration、public lifetimeへのpublication、external side effect等のsemantic Commitを行う場合、必要に応じてCommit直前にCommit eligibility validationを設けてよい。

Commit eligibilityは、Commit対象がそのsemantic transitionを安全かつcontractどおりに成立させられることを確認するために使用する。対象は複数値の組み合わせに限定しない。代表例は次のとおりである。

- 個別にvalidityが確立された複数の値、state、resourceまたはderived valueの組み合わせ整合性を確認する。
- Prepareで完成したcandidate representationが、public lifetimeへ昇格またはownership transferされる前に、その型のcanonical contractを満たしていることをDEBUG／TESTのdiagnosticとして確認する。
- Commit helperが失敗しないために必要なrelationやcapacity等を、Commit開始前に確認する。

completed candidateに対するcanonical validationをCommit前に行う場合、それはCommit後のstable stateを検査するPostconditionではなくCommit eligibilityである。

Commit eligibilityは、Preconditionsやupstream trust boundaryですでに成立しているconditionを機械的に再検査するための仕組みとはしない。また、semantic Commitを持たないpure Output operationへ機械的に設けない。

Commit eligibility failureのresult classificationはcandidateのtrust stateによって区別する。

- external / caller-controlled representationをtrusted internal representationへ昇格させる前のcandidateがrequired validityを満たさない場合は、trust promotionを拒否するrecoverable failureとして扱う。file loaderであれば`UNSUPPORTED_FILE`等を使用し、candidateがinvalidであることだけを理由として`DATA_CORRUPTED`へ分類しない。
- 一方、validated / trusted inputと成立済みinternal contractを前提としてmodule自身が構築したinternal candidateが、public Commit直前のdiagnostic canonical validationに失敗した場合は、module implementationまたはinternal invariantの破損を示す可能性があるため`DATA_CORRUPTED`候補となる。

Commit eligibilityを設けるか、どのvalidation depthを使用するか、どのhelperへ分離するかはmodule／operationごとに決定する。

---

## 6. Automatic validation policy

### 6.1 基本原則

canonical validator、shallow validator、またはその他のvalidation helperが存在することと、それを通常API内でautomaticに実行するかどうかを分離する。

automatic validationはAPI categoryから機械的に決定しない。次の順序で判断する。

1. operationが実際にconsumeするsemantic、state、relation、structureを特定する。
2. そのconditionのsemantic owner／contract ownerを特定する。
3. そのcontractをchecked preconditionとしてruntime validationするか、trusted／hard preconditionとして扱うかを決定する。
4. checked preconditionとは別に、DEBUG／TESTでinternal invariantのautomatic diagnostic validationを行う価値があるかを判断する。
5. validationを行う場合、direct condition check、shallow、canonicalのうち必要なdepthを選択する。

判断時には、必要に応じて次を考慮する。

- operation semantics
- semantic consumption
- semantic ownership
- checked / trusted contract classification
- 実際にdereference、traversal、mutationする範囲
- memory safety
- bounded execution
- lifecycle / ownership transition
- APIの実行頻度
- validatorの計算量
- 対象要素数
- API自身の計算量
- validationによる漸近計算量の悪化
- bug局所化の価値
- trust boundary

値またはobjectを単に受け取ること、転送すること、copyすること、または既存stateを読まず全面上書きすることだけを理由としてshallow／canonical validationを追加しない。

ただし、そのoperationがuntrusted representationをtrusted internal representationとして受理するtrust boundaryを形成する場合は、copy、storeまたはforwardのみを行うoperationであっても、boundary以降で成立済みとみなすsemantic conditionについてvalidationを行ってよい。したがって、automatic validationの要否はsemantic consumptionだけでは決定せず、trust boundaryとしての役割も独立して考慮する。

型のstruct definitionがpublic headerに存在するか、opaque declarationであるかだけを理由としてautomatic validationやfailure classificationを決定しない。caller-controlled inputか、established internal stateか、またそのoperationがどのsemanticをconsumeするかを基準とする。

producer／converterがdocumented preconditionを前提としてvalidなoutputを生成する場合、そのoutput guaranteeを確認するためだけにcanonical validatorをautomaticに実行しない。

一方、operationが対象型のsemanticを実際にconsumeし、そのvalidityがoperationの意味または安全性に必要である場合は、direct condition check、shallowまたはcanonicalを必要に応じて使用する。

### 6.2 BUILD_MODE

#### RELEASE_BUILD

次はRELEASE_BUILDでも通常のruntime validation／error handlingとして残す。

- checked precondition
- external factorから通常発生し得るfailure
- external APIが返すEOF／error／status
- recoverable runtime state violation

次はRELEASE_BUILDでのautomatic validationを原則として要求しない。

- trusted／hard preconditionの再検査
- 正規APIを通して成立しているinternal invariantのdiagnostic validation
- diagnostic目的の可変長buffer／array全要素validation
- producer／converterが自身のoutput guaranteeを再確認するためだけのcanonical validation

ただしmodule boundary、safety requirement、security requirement、external trust boundary等からRELEASE_BUILDでも必要と判断したvalidationは残してよい。

#### DEBUG_BUILD／TEST_BUILD

checked preconditionはRELEASE_BUILDと同じsemanticで検査する。

trusted／hard preconditionおよびinternal invariantについて、bug局所化やsafety上の価値がある場合はassertion、direct check、shallow、canonical等をautomaticに実行してよい。

DEBUG_BUILD／TEST_BUILDであることだけを理由に、対象value／objectのshallowまたはcanonical validationを機械的に追加しない。

TEST_BUILDであることだけを理由に、DEBUG_BUILDより機械的に深いprecondition validationを追加しない。

### 6.3 Common patterns

以下は典型的な傾向であり、project-wideの固定tableではない。最終判断はSection 6.1に従う。

- stateful objectのcreate／initializeでprivate candidateを完成させてからpublic lifetimeへCommitする場合、DEBUG／TESTでcandidateへcanonical Commit eligibility validationを行う価値が高いことがある。
- create／initializeがcaller-provided object等へ直接Commitした後、そのstable stateを検査する場合はPostcondition validationとして扱う。
- value construction／conversionでは、documented preconditionからvalid resultを生成できることが実装contractとして明確なら、そのoutput guaranteeを再確認するためだけのautomatic canonical validationを省略してよい。pure Outputだけを行うoperationにはCommit eligibilityを機械的に追加しない。
- getter／read-only operationでも、対象semanticを実際にconsumeする場合はcanonical validationが適切なことがある。
- mutatorでも、既存semanticを読まず既知stateへ全面上書きする場合はprecondition object validationが不要なことがある。
- destroy／deinitializeでowned resourceをtraverseする場合、suspect objectを辿る前のcanonical validationが有用なことがある。
- forwarding／facade／orchestration layerは、下位operationが所有するsemantic validationを重複して行わない。

#### Low-level Allocatorのpublic boundary

`engine/memory/low_level_allocators`に属するAllocator moduleは、自身が管理するphysical memory rangeおよびallocation stateについて、GLCE内部におけるallocation-level validationのtrust rootとなる。

現在、この規則の対象はFree List AllocatorおよびLinear Allocatorである。

このため、DEBUG_BUILD / TEST_BUILDでは、initialized allocator stateを受け取り、そのstateに依存するPublic APIについて、API contract validationを行った後、state-dependentなoperationへ入る前にcanonical validatorを使用して現在のallocator state全体を確認する。

これは一般的なoperation semanticsだけでは要求されない場合でも、allocation-level trust rootとしてbug局所化とmemory safetyを優先するmodule-specific policyである。

この方針を採用する理由は次のとおりである。

- Low-level Allocatorは上位moduleのallocation validityおよびmemory safetyの土台になるため、corruptedなallocator stateを前提として処理を続行するより、Public API boundaryでcanonical stateを確認して早期にfailureとすることを優先する。
- low-level allocatorではcanonical validationのcostが十分小さい場合が多い。
- 将来canonical validatorへ検査項目が追加された場合も、Public API側を変更せず「Low-level AllocatorのPublic API boundaryではcanonical validationを行う」という一貫したboundary policyを維持できる。

General AllocatorやSubsystem Allocator等のhigh-level Allocatorには、この特例を適用しない。これらのmoduleではSection 6.1の一般policyに従い、各operationが自身でconsumeするstateについて必要なvalidationを行い、owned low-level allocator固有のsemanticは対応するAPIへ委譲する。

この規則は、初期化前objectを受け取るConstructor / InitializerのPreconditionsには適用しない。Constructor / Initializerでは、既存allocator stateをconsumeしないため、initialize前objectのcanonical validatorを使用しない。

Low-level Allocatorのconstructor／initializer stable exitでcanonical postcondition validationを行うかどうかは、各moduleの既存module-specific policyに従う。

canonical validator自身、およびinitialized allocator stateを受け取らないutility APIについても、このprecondition ruleの対象外とする。

### 6.4 Hot pathとvalidation cost

毎frame、objectごと、elementごと、またはtight loop内で呼ばれるAPIでは、validation costと実行頻度を明示的に検討する。

checked preconditionまたはoperationを安全に実行するために必要と判断したvalidationを、hot pathであることだけを理由に機械的に削除しない。

validation costが問題になる場合は、operation semantics、実行頻度、対象規模、実測performance等を踏まえてその都度検討する。

検討の結果、現時点では強いvalidationを維持し、将来的にperformance問題が顕在化した場合にdata structure／traversal方式を見直す、またはvalidationを弱めることをTODOとして残してよい。

安全な実行には不要なdiagnostic validationについては、stable batch boundary、unit test、failure injection、sanitizer、明示的self-diagnostics等へ移すことを検討してよい。

---

## 7. Failure handling

### 7.1 Validator failure

現段階のcanonical validatorはboolで結果を返す。

validatorはfailure時に次を直接行わない。

- log出力
- assertまたはabort
- application stateの変更
- repair
- resource解放

falseを受け取った呼び出し側が、API result、diagnostic log、test failure、将来のFATAL transitionまたはdiagnostic reportへ変換する。

validator failureのresult classificationは、`is_valid() == false`という事実だけでは決定しない。**何を検査していたか、誰がそのstateを制御しているか、trust promotionの前後どちらか**を確認する。

原則として次のように分類する。

- caller-controlled inputをchecked preconditionとして検査し、required validityを満たさない場合は、通常`INVALID_ARGUMENT`、`BAD_OPERATION`その他のrecoverable resultへ分類する。
- External Trust Boundaryでtrust promotion前のcandidateがrequired validityを満たさない場合は、`UNSUPPORTED_FILE`等、そのboundaryに対応するrecoverable rejectionへ分類する。
- 正規APIを通して成立済みで、moduleがvalidであることを前提として管理しているestablished internal stateのcanonical invariantが破損している場合は、`DATA_CORRUPTED`へ分類する根拠となる。
- validated / trusted inputからmodule自身が構築したinternal candidateについて、Commit eligibilityまたはPostconditionのdiagnostic canonical validationが失敗し、そのfailureがmodule implementation / internal invariantの破損を示す場合は`DATA_CORRUPTED`へ分類してよい。
- trusted / hard preconditionをDEBUG / TESTで診断してfailureした場合の扱いは、そのAPIのdiagnostic policyに従う。recoverable contractとして定義されていないconditionを、機械的に通常errorへ変換しない。

Engine内部producerが生成したvalueを別moduleがinputとして受け取る場合、consumerはproducer provenanceを推測して`INVALID_ARGUMENT`を`DATA_CORRUPTED`へ再分類しない。consumerは自身が観測できるinput contractに従ってresultを返し、producer側のinternal bug detectionはproducer自身のstable-boundary validationまたはdiagnosticsへ委ねてよい。

### 7.2 `DATA_CORRUPTED`

`DATA_CORRUPTED`は、GLCE内部で成立済みと信頼していたstateまたはinvariantについて、そのintegrityを維持できていないと判断する明確な根拠があり、通常operationへ安全に復帰できないことを表すpanic-class signalとして扱う。

`DATA_CORRUPTED`はvalidator failureの一般的なfallback resultではない。特に、caller-controlled input、external representation、trust promotion前candidateのinvalidityを、canonical validatorがfalseを返したという理由だけで`DATA_CORRUPTED`へ分類しない。

代表例:

- established internal object / stateのcanonical invariant破損
- established internal objectのfield relation破損
- owner管理情報の矛盾
- validated / trusted inputからmodule自身が構築したinternal candidateに対するdiagnostic Commit eligibility failureで、module implementationまたはinternal invariant破損を示す場合
- established internal stateに対するPostcondition validation failure
- recoverable cleanup / rollback中に、成立済みinternal stateの安全性を確定できなくなった場合

次は、それ自体を理由として`DATA_CORRUPTED`へ分類しない。

- Application / Frontendその他のcaller-controlled valueがAPI validityを満たさない
- malformed fileまたはunsupported format
- external device / OS / libraryから取得したdataやstatusの異常
- trust promotion前candidateがinternal representationとして受理できない
- Engine内部producer由来である可能性があるvalueをconsumerがinputとして受け取り、そのinput validity checkに失敗しただけの場合

caller-controlled inputのinvalidityを`DATA_CORRUPTED`へ過剰分類すると、Application側のbugやrecoverable input failureをFATAL方向へ誤って伝播させることになる。`DATA_CORRUPTED`がfail-stopを伴う重いsignalであることを踏まえ、分類には「成立済みinternal stateのintegrityが失われている」という追加根拠を要求する。

一方、internal producer bugがconsumer側では`INVALID_ARGUMENT`として観測され、`DATA_CORRUPTED`として即時分類されない場合があることは許容する。producer自身がestablished stateを破壊している場合はproducer側canonical validation等で検出できる可能性があり、temporary valueの誤生成だけでestablished stateが破壊されていない場合はoperation failureとして局所化される。

DEBUG / TESTのstable-boundary canonical validationによって、完成したはずのestablished internal objectがcanonical stateを満たさないことが判明した場合は、stable-boundary failureとして`DATA_CORRUPTED`へ分類してよい。

value construction / conversionでsource側のtrusted / hard preconditionが違反していたことに起因するinvalid outputは正常系contract外である。これを一律に`DATA_CORRUPTED`へ分類する規則は設けない。

### 7.3 Fail-stop

Fail-stopは`DATA_CORRUPTED`が確定した後の規則である。caller-controlled input failureやexternal rejectionを誤って`DATA_CORRUPTED`へ分類し、不要なFATAL遷移やresource leakを発生させないことが前提となる。

`DATA_CORRUPTED`を検出した後は、そのoperationにおける通常のrollback／destroy／deinitialize／release／free／close／unbindその他のcleanupまたは追加resource operationを行わない。

suspect objectのfieldを参照するcleanupだけでなく、construction中に別local variableで追跡していたtemporary resourceについてもcleanupを行わない。

下位APIから`DATA_CORRUPTED`を受け取った上位layerも同じ規則に従う。

```text
lower layer DATA_CORRUPTED
    ↓
current operationのcleanup停止
    ↓
upper layerへDATA_CORRUPTED伝播
    ↓
upper layerもcleanup停止
    ↓
resource leakを許容
    ↓
FATAL方向へ伝播
```

resource leakはcleanup failureではなく、corruption検出後に追加operationを行って二次破壊を起こすことを避けるための意図的なfail-stop behaviorである。

recoverable failureからcleanupへ入った後、cleanup operationの失敗によって`DATA_CORRUPTED`へ昇格した場合も、その時点で残りのcleanupを停止する。

元のfailureが`DATA_CORRUPTED`である場合は、それをcleanup failureで上書きしない。

`DATA_CORRUPTED`を受けた上位layerは通常運転継続を前提にせず、FATAL方向へ伝播する。FATAL state transition APIと即時abort／graceful terminationの最終境界は別途設計する。

### 7.4 Recoverable cleanup

recoverable failure時のrollback／cleanup方法、private unchecked cleanup helperの有無と構成はmoduleごとに決定してよい。

ただし、次を満たすこと。

- recoverable failure後に継続するobjectはvalidなstable stateへ戻す。
- private partial stateのcleanup手段をpublic destroyのvalidation迂回手段として使用しない。
- `DATA_CORRUPTED`確定後はunchecked cleanupを含む通常cleanupを行わない。

trusted／hard precondition違反はrecoverable cleanup contractの対象外である。

---

## 8. 今後決定する事項

次は本書では未確定とする。

- Validation Reportのstructured representation
- system-wide self-diagnosticsの具体的な実行方式
- diagnostic validationで使用するvalidation depthと実行対象の選択方式
- 共通validation macro／helperの最終形
- trusted／hard preconditionをDEBUG／TESTで表現するassertion／diagnostic mechanismの統一方針
- FATAL state transition API
- FATAL後のgraceful shutdownと即時abortの境界
- subsystem stateとapplication stateの関係
- 独立した`INVARIANTS`相当build optionの要否

個別moduleへのvalidator実装状況、適用順序および保留事項は`glce_near_term_todo.md`を正本とする。
