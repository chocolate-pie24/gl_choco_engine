<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Validation Policy

## このドキュメントの使い方

本書はGLCE全体のValidation Architectureについて、責務、検証範囲、自動実行方針、stable boundary、failure handling、および性能とのバランスを定める正本文書である。

新規実装、レビュー、リファクタリング時には、本書の規則と、対象の最新ソースコードを照合する。個々のAPIの検査手順は、古くなる可能性のある説明コメントではなく、実装を直接解析して確認する。公開APIの利用者が守るべき契約は`.h`のDoxygenを参照する。

本書は実装の解説書ではなく、実装とレビューに適用するproject-wideな規範である。設計理由を復元するために必要な情報は記録するが、API別の実装内容を網羅的に転記しない。

AIによる提案・生成内容は、それ自体を採用済み方針とはみなさない。GLCEの目的、制約、既存実装、設計判断との整合を確認して採用する。

最終更新: 2026-11-22

---

## 1. 文書の位置づけ

本書はValidation固有の規則の正本とする。関連文書との責務分担は以下のとおり。

- `boundary_model.md`: External Trust Boundary、Engine API Trust Boundary、Internal Integrity Boundary、およびtrust / authorityの境界。
- `coding_style.md`: 関数・ファイル構成、命名、関数内の処理phase等。
- `documentation_guide.md`: Doxygenとソースコメントの記述方針。
- `design_decisions.md`: 重要な設計判断と採否の履歴。
- 最新ソースコード: 現在実行されるValidation処理の事実。

次の概念はそれぞれ区別する。

1. Operationがどのsemantic contractを所有するか。
2. そのcontractをchecked preconditionとしてruntime検証するか、trusted / hard preconditionとして扱うか。
3. Shallow / Canonical Validatorが何をvalidとみなすか。
4. Public API内でどの検査をautomaticに実行するか。
5. DEBUG / TEST / RELEASEでautomatic validationをどう選択するか。
6. Validation failureをどのresultとsystem behaviorへ変換するか。

**契約に責任を持つことと、毎回runtimeで検証することは同義ではない。** Validatorの意味はBUILD_MODEで変更しない。BUILD_MODEによって変更するのは、主として自動実行位置とdepthである。

### 1.1 ソースコードへの記述方針

公開APIの入力契約、Ownership / Lifetime、成功時・失敗時の保証は、その利用者が参照できる`.h`のDoxygenに記述する。Module内のValidationの実行位置、検査順序、Shallow / Canonicalの選択、private helperへの委譲はソースコードを正本とする。

以下の独立したドキュメントブロックは設けない。

- Module Boundary Contract
- Module Internal Contract
- Module Validation Policy
- API-specific Validation Policy

上記の廃止は、**Validationの実施要否、depth、failure classification、Fail-stop等の実行規則を変更するものではない。** Private helperやprivate validatorについては、関数の目的と、実装から読み取れない注意事項だけを必要に応じてコメントする。非自明な意図や判断理由は該当する処理の近くへ短く記録する。

Runtime Status ReportとValidation Reportは分離する。前者は現在のruntime stateの観測、後者は明示的なvalidator実行によるintegrity診断を担当する。具体的なreportのstructured representation等は今後設計する。

---

## 2. 目的

GLCEのValidationは、GLCE自身の実装bugに起因するinternal invariant破損を検出し、operationの安全性と成立条件を明確化し、成功後のstable stateを保証できる構造を目指す。また、unit test、DEBUG diagnostics、将来のRELEASE self-diagnosticsで共通利用できるvalidatorを提供し、corruption検出後の二次破壊を抑止する判断材料を与える。

通常実行経路で全contractを再検証することや、あらゆる潜在bugを検出することは目的としない。Memory safety、bounded execution、diagnostic value、実行頻度、計算量、実装複雑性のバランスを取る。

- NULL、size、count、capacity、allocation metadata、ownership等を用いてdereference前に安全性を検査できる場合は、その検査を候補とする。
- Trusted / hard preconditionとして定義された条件について、RELEASE_BUILDでの再検査を一律に要求しない。
- C言語と利用可能なruntime情報だけでは完全なmemory-safe validationが実現できない場合、必要な複雑性や実行コストを含めて個別に判断する。
- Sanitizer、unit test、failure injection、fuzzing、Allocatorのallocation metadata validation等を補助的な診断手段として利用する。

---

## 3. Validation model

GLCEでは、contract responsibilityとvalidation depthを別軸として扱う。

```text
operation semantics / semantic consumption / trust boundary
        ↓
semantic ownership / contract responsibility
        ↓
checked precondition または trusted / hard precondition
        ↓
必要な場合のみ automatic validation
        ↓
direct condition check / shallow / canonical
```

API contractはcallerとcalleeの間で成立している必要がある条件を表す。Shallow / Canonicalはobjectやvalueをどのdepthで検査するかを表す。これらは単純な強弱関係ではない。

### 3.1 Semantic consumptionとoperation responsibility

各operationは、その処理が実際にconsumeするsemanticに責任を持つ。Consumeとは、値、state、relation、structureの意味がoperation結果、安全性、有限実行、resource operation、indexing、allocation、mutation等へ影響することを指す。

引数として受け取る、forwardする、単純copy / storeする、既存stateを読まず全面上書きする、といった事実だけを理由に、対象型全体のsemantic validityを再検査しない。

- Producer / converterはdocumented preconditionを前提に、自身の変換責務を果たす。Source型のcanonical validityの再認証を機械的に要求しない。
- Consumerは実際に利用するsemanticについて必要なcontractを定め、runtimeで検査するかtrustedとするかを別途判断する。
- Value type、temporary object、stateful object等の分類だけでautomatic validationを決めない。
- ただし、そのoperationがuntrusted representationをtrusted internal representationとして受理するtrust boundaryを形成する場合は、semanticを直接consumeしないcopy / store / forwardでも、境界以降で成立済みとみなす条件を検査してよい。

### 3.2 API contract

API contractの代表的な条件には、pointerやout slotの状態、lifecycle、ownership、size / count / index / ID / enum / flag、range、arithmetic overflow、source representationのvalidity等がある。

**Checked precondition**は、caller-controlled input、external factor、runtime state等から違反が通常発生し得て、APIがrecoverable failureとして扱う条件である。RELEASE_BUILDを含む通常buildで検査する。NULL、非ゼロcount、無効なID / enum / range、操作不可能なstate、external APIのEOF / error / status等が例となる。

**Trusted / hard precondition**は、正しいcallerが成立させるprogramming contractであり、違反時のrecoverable behaviorを提供しない条件である。DEBUG / TESTでassertionやdiagnostic validationを行ってよいが、RELEASEでの再検査を一律に要求しない。利用者が守る必要のある条件は`.h`のAPI Doxygen、必要ならheader先頭の共通説明から確認できるようにする。

Trusted / hard precondition違反は正常系およびrecoverable failure contractの対象外である。ただし、これを明示的にundefined behaviorと呼ぶかどうかは各module / APIが決定する。

同じ種類の条件を全APIで一律にcheckedまたはtrustedへ分類しない。Pointer non-NULLでも公開recoverable APIとlow-level utilityでは扱いが異なり得る。

### 3.3 Semantic ownership

Semantic conditionは、その意味を所有するmodule / operationが一元的に定義する。

- Forwarding / facade / orchestration layerは、委譲先が所有するsemantic validationを重複して実施しない。DEBUG / TESTでも同様とする。
- 引数を受け取ることだけで、その値のsemantic ownershipを引き受けたとはみなさない。
- Operationがsemanticをconsumeする場合は必要なcontractを定義するが、checked / trustedの区分は別途決定する。
- 下位moduleから受け取ったfailureを上位の公開操作に沿った意味へ変換し、不要に潰さない。

Caller-providedなcontainer / arrayでは、containerを安全かつboundedに走査するためのstructural conditionと、各elementのsemantic validityを分ける。前者は走査するoperation、後者はsemanticをconsumeするoperationが責任を持つ。別moduleへelement固有の処理を委譲する場合は、そのsemantic validationも委譲する。Element dataを単純にread / copy / storeすることは、それ自体でsemantic consumptionとは限らない。

### 3.4 Shallow validation

Shallow validationはroot fieldと局所的なstructural invariantを検査する。原則O(1)とし、可変長array、linked structure、owned child全体の走査を行わない。

対象例は、size / count / capacity、NULL / non-NULL relation、演算overflow、root state relation、canonical validation開始に必要な局所条件等である。型として固定された小配列の実質O(1)のloopは許容する。

### 3.5 Canonical validation

Canonical validationは、対象objectが所有するinternal representation、semantic state、field relation、およびownership closureについて、その型が保証するcanonical modelを検査するdeep / full validationである。Fullとはprocess内の全到達objectではなく、対象object自身の責務とownership closureに対する完全性を意味する。

対象例はowned arrayの使用中要素、linked nodeとlink relation、countと実要素数、rangeの連続性・重複・合計、owned child、aggregate invariant、stable stateにtransient stateが残っていないこと等である。一般にO(n)以上、構造によってO(n²)となり得る。

Root argumentとして渡されたobject自身のallocation identity / storage livenessは、当該objectのownership closureに含まれない。Ownerがいる場合はowner、caller-provided storageの場合はcaller contractの責務となる。

Canonical validatorが存在することは、すべてのAPIでのautomatic canonical実行、constructor出力の毎回のself-validation、private shallow validatorの必須設置を意味しない。

独立したpublic lifecycleや任意mutationを持たず、checked construction / transformationだけで生成されるprivate temporary / derived contextは、standalone canonical validatorなしでvalid-by-constructionとしてよい。Downstream private operationは構築成功によるinternal contractを信頼し、自身が直接consumeするoperation-specific conditionだけを必要に応じて確認する。

### 3.6 可変長buffer / arrayの全要素validation

Element validatorの存在だけを理由に全要素をautomatic validationしない。Arrayのstructural validityと各elementのsemantic validityは別である。

- RELEASEではdiagnostic目的の可変長array全要素validationを自動実行しない。
- DEBUG / TESTではsemantic consumption、semantic owner、trust boundary、API実行頻度、要素数、漸近計算量の悪化等を基準に判断する。
- Operationの成立、安全な実行、external inputのtrust promotionのために全要素処理が本質的に必要な場合は、diagnostic目的のautomatic validationとは区別する。
- Runtime verificationのためだけに、任意storageを受け取るAPIへ特定Allocator由来という新しいprovenance条件を追加しない。Allocation provenanceが公開API契約として必要な場合に限り、Allocator authorityによるidentity / range validationを検討する。

### 3.7 Trust boundaryとinput classification

Boundaryの分類は`boundary_model.md`を正本とする。Inputの分類は型がpublicかopaqueかではなく、**caller-controlled inputか、成立済みのestablished internal stateか**を基準とする。Engine内部のstructがheaderに可視であることだけでcaller-controlledとはみなさない。

**External Trust Boundary:** 外部表現を無条件にtrustedとして扱わない。一方、すべてのsemantic validationを入口に一括集約する必要もない。Acquisition / parse / loaderが必要なvalidationとnormalizationを行い、accepted internal representationへtrust promotionする。Promotion前のmalformed / unsupported candidateはrecoverable rejectionとし、invalidであるだけで`DATA_CORRUPTED`とはしない。受理後の単純copy / forwardで同じ検査を機械的に繰り返さない。

**Engine API Trust Boundary:** Application、Frontend等のcaller-controlled config / descriptor / value / arrayをAPI entryで受け取るとき、checked validityの違反を`INVALID_ARGUMENT`、`BAD_OPERATION`等のrecoverable resultとして扱う。過去にEngine内部producerが生成した可能性だけで`DATA_CORRUPTED`へ再分類しない。Validation / normalization / constructionを経てEngine-owned established stateへ取り込むことでtrust promotionが成立する。

**Engine内部module間のinput:** Module Aの生成valueをBが受け取り、Bから観測できる事実が入力のinvalidityだけであれば、Bは自己のAPI契約に従って`INVALID_ARGUMENT`等を返してよい。Producerのbugをconsumerでprovenance推測して`DATA_CORRUPTED`に変更しない。Producerのestablished state破損はproducer側のstable-boundary validation等で検出し得る。一時的な誤生成だけならoperation failureへ局所化されることを許容する。

**Established internal state:** 正規のcreate / initialize / commit等を経て成立し、moduleがvalidであることを前提に管理するobject / registry / allocator / resourceが該当する。Canonical invariant破損は`DATA_CORRUPTED`の根拠になり得るが、単に`is_valid() == false`だけでは判断せず、検査対象のtrust stateを確認する。

**External opaque resource:** OS、libc、外部library、device等が所有するopaque resourceの内部状態を、GLCE wrapperのcanonical invariantに含めない。GLCE側ではwrapper自身のstate、handleの存在、所有relationを検査する。外部resourceの成否、EOF、statusは外部APIの仕組みで判定し、RELEASEでも必要なerror handlingを残す。

外部浮動小数点値はacquisition / parse boundaryで必要に応じてfinite性を検査する。一度受理した値についてgenericな`isfinite()`をsetter / getter / 内部演算で機械的に繰り返さない。ただしfinite性がその型の明示的なcanonical invariantならvalidatorで検査してよい。内部演算ではdivisor非ゼロ、nonzero normalize、nonsingular inverse、perspectiveの`aspect > 0`、`0 < fovy < 180`、`0 < near < far`等、演算固有の成立条件を検査する。有限な入力同士でもoverflowからInf / NaNが生じ得るため、「内部浮動小数点値は常にfinite」という一般契約を作らない。

### 3.8 Runtime StatusとValidation Diagnosticsの分離

Runtime Status Reportはmemory usage、allocation count、resource count、capacity / usage、subsystem state等を観測するものであり、validity判定ではない。Status structureに`is_valid`等を組み込まない。

Validation Reportは明示的なshallow / canonical / diagnostic validationによってintegrityを診断する。Automatic validationはoperationに必要と判断した契約やinvariantだけを対象とし、明示的diagnosticsとは分離する。Status取得API内部でDEBUG / TESTのcanonical validationを行う場合も、その結果をRuntime Status Reportの一部とは扱わない。

---

## 4. Validator contract

### 4.1 Canonical validator

Validationが必要な型には原則として一つのcanonical validatorを設け、安定したobject modelまたはvalue typeが明示的に保証するsemantic modelに対して定義する。将来削除予定のfieldや、廃止予定の中間stable stateへ合わせるためだけにvalidatorを増やさない。

- 妥当性をboolで返す関数名は`*_is_valid`を基本とする。
- 対象は原則`const`で受け取る。
- NULL-safeとし、NULL root argumentに対してfalseを返す。
- Validatorの意味をBUILD_MODEで変えない。Runtime bool引数によるshallow / canonical切替も行わない。
- 対象をmutation / repairせず、原則dynamic memory allocation、文章log、assert / abortを行わない。
- 検査項目は、その型が実際に保証するdata structure、ownership、lifecycle、semantic invariantに限定する。
- 一般的な防御上の理由だけで、新たなcanonical invariantを追加しない。Finite性が明示的な型契約でない限り、genericなNaN / Inf checkも追加しない。
- 特定API用の`is_valid_for_*`を機械的に増やさない。Operation-specificな条件はAPI contractまたは意味の明確なstate predicateで扱う。

### 4.2 Shallow validator

Private shallow validatorは、実際にshallow depthを必要とするcall siteがある場合に設ける。Canonical validatorの存在だけを理由に必ず用意するものではない。

- 現時点のshallowとcanonicalの検査内容が同一でも、call siteでshallow depthを明示することに意味があれば分離してよい。
- Canonical validatorがshallow validatorを最初に呼ぶことはproject-wideに要求しない。
- Shallowとownership closureの検査順は、dereference前のallocation validityなど各検査のdependencyとmemory safety requirementから決定する。
- Shallow validatorが存在しなければcanonical validatorが必要な条件を直接検査する。
- `*_is_valid_shallow`をpublicへ公開する必要はない。Public validatorはcanonicalな`*_is_valid`を基本とする。
- その他のprivate validator / validation helperの粒度、分割、名称はmoduleのdata structureとoperation semanticsに従う。

### 4.3 Ownershipとborrow

Parentのcanonical validatorは、parentがlifetimeを所有するchildをownership closureとして扱う。

- Owned pointerのallocation-level validityをMemory moduleから確認できる場合は、childをdereference / child validatorへ渡す前に確認することを候補とする。
- Owned childに独立canonical validatorがあれば、その内部validityの検査を委譲してよい。Child rootのallocation validityの再検査は、child validatorには原則要求しない。
- Embedded valueのように独立allocationを持たないchildにはallocation-level validationを要求しない。
- Owned array / collectionのcanonical内部をどうhelperへ分割するかはmodule実装に委ね、raw array専用validatorをproject-wideで強制しない。
- Borrowed objectのfull validityはborrow先のowner / system側validatorが担当し、borrowerのownership closureへ含めない。
- Borrowerは自己のpointer存在条件、optional pointerとstateの関係、自己が所有するrelationなどを必要に応じて検証する。Checked / trustedの選択はSection 3.2に従う。
- Pointer値だけからborrow先のlivenessや正しいlifetime順序を保証しない。Borrow lifetimeはownerと上位lifecycle contractの責務である。
- 複数object間のrelationは、それをconsumeするoperationで必要な範囲を扱う。

### 4.4 Memory-safe and bounded validation

Canonical validatorは、corruption状態でも可能な範囲でmemory-safeかつ有限時間でfalseを返すよう設計する。

- NULL、count、size、capacity、overflowをdereferenceより前に検査する。
- Traversalには管理上限または明確な終端条件を設け、cycleによる無限loopを防止する。
- Raw owned pointerは、allocator metadata等から安全性を確認できるならdereference前に確認する。
- Root argument自身のstorage validityは、そのstorageを所有するownerまたはcaller contractの責務であり、root validatorが証明するものではない。
- 破損し得るlinkage pointerを無条件に次のdereference先へ使用しない。既知の安全なrange / physical layoutから検査位置を導出できれば、それを優先してよい。

完全なmemory-safe validationを要求できない場合はSection 2に従い、structureとoperationごとに安全性・複雑性・コストを判断する。

### 4.5 Memory validation trust boundary

`engine/memory`は、GLCE内部でのallocation-level validationのtrust rootとする。上位moduleのvalidatorは、Memory moduleのallocation query結果をauthorityとして扱い、その根拠となるallocator metadata validityを再帰的に検証しない。

Allocatorが利用できるmetadataは実装によって異なる。Free Listではper-allocation metadataとallocation payload / stateを検証し得るが、Linear Allocatorではper-allocation metadataを持たず、current in-use range等の粗い情報を扱う。上位validatorはMemory moduleが提供しないallocation identity、boundary、historical livenessを仮定しない。

Memory module自身のvalidatorも、allocator metadataが無傷であることをさらに別のAllocatorが証明するものではない。既知のbacking range、capacity、physical layout、局所metadataの整合性を用いて検出可能なcorruptionを確認する。Low-level Allocatorが自身のpoolについてallocation authorityを持ち、General Allocator等の上位Allocatorがそれに依存する。別Allocatorへ証明を要求し続ける無限後退は行わない。

このsoftware trust boundaryはOS、libc、virtual memory、MMU、hardwareを含む完全性を保証しない。Guard page、sanitizer、MPU等は、独立した補助的な診断・保護mechanismとする。

---

## 5. Stable state and lifecycle

### 5.1 Stable state

Documented preconditionが成立し、lifetimeを維持するpublic APIが正常終了した場合、objectや生成valueはその型が保証するvalidなstable state / canonical modelを満たさなければならない。Trusted / hard precondition違反時は対象外とする。

Stable stateを保証することと、return前にcanonical validatorで毎回検証することは別である。Private処理中のtransient / transitioning stateをcanonical validatorが受理する必要はない。Constructorだけが必要とするpartial stateはprivate temporaryに閉じ、public stable stateにはしない。

Empty、closed、unloaded、未初期化状態などをvalidなstable stateとするのは、その状態に独立した利用価値があり、許可操作、禁止操作、状態遷移を公開契約として定義する場合に限る。Validatorを実装しやすくするためだけに不要なstable stateを増やさない。Validとreadyの関係は型ごとの公開lifecycle contractで決める。

### 5.2 Recoverable failure

Checked precondition violationまたはrecoverable failureの後もlifetimeが継続するobjectは、validなstable stateを維持または回復する。

- Semantic Commit開始前の失敗では、原則としてoperationのsemantic effectを成立させず、公開state、ownership、lifecycle relationを変更しない。
- Commit開始後にrecoverable failureがあり得る場合は可能な範囲でrollbackし、仕様化されたstable stateへ戻す。
- Constructor中のprivate partial stateはrecoverable failure時にrollbackしてよい。
- Rollback不能なinternal corruptionはrecoverable failureと区別する。
- Trusted / hard precondition違反、`DATA_CORRUPTED`はこの保証の対象外とする。

### 5.3 Stable boundary / Postcondition validation

Automatic Postcondition validationを行う場合は、semantic effectがCommitされ、object state、ownership、lifecycle等が最終確定したstable boundaryで行う。「return直前」を機械的な境界とはしない。Replacement、move、registration、publication等では先にCommitの意味と位置を明確にする。

- 既存objectのmutationは最後のmutationまたはrollback完了後がstable boundaryとなる。
- Ownership / lifecycle transitionは契約を構成する遷移がすべてCommitされた時点をboundaryとする。
- Getter / read-only operationでは通常stable-exit canonical validationを行わない。
- External opaque resourceだけのstate変化を理由に、GLCE-owned objectのPostcondition canonical validationを機械的に実行しない。
- Destroy / deinitializeでlogical lifetime終了後のcanonical validationを要求しない。
- Prepare済みcandidateのOutput前の検査はPostconditionsではなくResult validationで扱う。

### 5.4 Result validation

Prepare等で作ったresult candidateをcallerへOutputする場合、APIのsuccess output contractを満たすか、必要に応じてOutput前に検査してよい。Parser / converter / value constructorの結果、derived field間relation、external representationのtrust promotion前candidate等が例となる。

Documented preconditionとimplementation contractからvalid resultが直接導かれ、追加診断の価値がない場合は機械的に実行しない。Capacity、target、resource availability、複数state間relationなどCommitを開始できるかという確認はPreflightであり、Result validationとは異なる。

Result validation failureは候補のtrust stateと検査対象で分類する。

- Trust promotion前のexternal / caller-controlled candidateがinvalidなら、`UNSUPPORTED_FILE`等のrecoverable rejectionを使用し、invalidという事実だけで`DATA_CORRUPTED`としない。
- Validated / trusted inputと成立済みinternal contractのもとでmodule自身が生成したcandidateがdiagnostic validationに失敗し、implementation bug / internal invariant破損を示すなら`DATA_CORRUPTED`候補となる。

Result validationを設けるか、どのdepth / validator / helperへ委譲するかはoperationごとに決定する。

---

## 6. Automatic validation policy

### 6.1 基本原則

Validatorやhelperが存在することと、それをpublic APIでautomaticに実行することは分離する。APIの種類だけで実行有無やdepthを機械的に決定しない。

1. Operationがconsumeするsemantic、state、relation、structureを特定する。
2. Semantic owner / contract ownerを特定する。
3. Checked preconditionとしてruntime検査するか、trusted / hard preconditionとするかを決定する。
4. DEBUG / TESTでinternal invariantの追加diagnostic validationを行う価値を評価する。
5. 実行する場合はdirect check、shallow、canonicalの必要なdepthを選ぶ。

判断には、memory safety、bounded execution、実際にdereference / traversal / mutationする範囲、lifecycle / ownership transition、実行頻度、要素数、validatorとAPIの計算量、漸近計算量の変化、bug局所化の価値、trust boundary等を考慮する。

Objectを受け取る、copy / forwardする、既存stateを全面上書きするといった理由だけでshallow / canonicalを追加しない。ただしExternal Trust Boundaryで受理する場合は、semantic consumptionとは独立にtrust promotionに必要な検査を行ってよい。Structがpublic / opaqueかというrepresentationだけでも判断しない。

Documented preconditionを満たすproducer / converterがvalid outputを構築する場合、保証の再確認だけを目的とするcanonical validationを通常要求しない。逆にoperationが意味や安全性のためにsemanticをconsumeするなら、必要なdepthで検査する。

### 6.2 BUILD_MODE

**RELEASE_BUILD**でも次は通常のruntime validation / error handlingとして残す。

- Checked precondition
- External factorから発生し得る通常のfailure
- External APIのEOF / error / status
- Recoverable runtime state violation

一方、trusted / hard precondition、正規APIで成立したinternal invariantの診断、diagnostic目的の可変長array全要素、producerが自分のoutput guaranteeを確認するだけのcanonical validationは、RELEASEで原則要求しない。ただしmodule boundary、memory safety、security、external trust boundary等から必要な検査は残してよい。

**DEBUG_BUILD / TEST_BUILD**ではchecked preconditionのsemanticsはRELEASEと同じにする。Trusted / hard preconditionやinternal invariantについて、bug局所化や安全上の価値があればassertion、direct check、shallow、canonical等をautomaticに実行してよい。ただしDEBUG / TESTという理由だけで追加のshallow / canonicalを機械的に実施しない。TESTであることだけを理由にDEBUGより深いvalidationを要求しない。

### 6.3 Common patterns

以下は傾向であり、固定されたAPI category tableではない。個別判断はSection 6.1に従う。

- Stateful create / initializeでcapacity、resource availability、target relation等の実行可能性を確認する場合はPreflightとする。
- Private result candidateの成功時契約をCommit / Output前に確認する診断価値があれば、DEBUG / TESTでResult validationを行ってよい。
- Caller-provided objectへ直接Commitした後のstable state検査はPostcondition validationとする。
- Value construction / conversionのvalid-by-construction保証を確認するだけならResult validationを省略してよい。
- Getter / read-only operationでもsemanticをconsumeするならcanonicalが適切な場合がある。
- Mutatorでも既存semanticを読まず既知stateへ全面上書きするならprecondition object validationが不要な場合がある。
- Destroy / deinitializeでowned resourceをtraverseする場合、suspect objectを辿る前のcanonical validationに価値がある場合がある。
- Forwarding / facade / orchestration layerではsemantic ownerの検証を重複させない。

#### Low-level Allocatorのpublic boundary

`engine/memory/low_level_allocators`のFree List AllocatorおよびLinear Allocatorは、自身のphysical memory rangeとallocation stateについてallocation-level validationのtrust rootとなる。

**DEBUG / TESTでは、初期化済みAllocator stateを受け取ってそのstateに依存するPublic APIで、API contract validation後かつstate-dependent operation前にcanonical validatorでAllocator state全体を確認する。** 一般的なsemantic consumptionだけなら要求されない場合でも、trust rootとしてbug局所化とmemory safetyを優先する特例である。

- Low-level Allocatorは上位moduleのallocation validityとmemory safetyの基盤になる。
- Canonical validationのcostが十分小さい場合が多い。
- Canonical検査項目が増えてもpublic boundaryの方針を維持できる。

General AllocatorやSubsystem Allocator等のhigh-level Allocatorには特例を適用せず、Section 6.1に従う。Owned low-level allocatorのsemantic検査は対応するAPIに委譲する。

初期化前objectを受け取るConstructor / InitializerのPreconditions、canonical validator自身、初期化済みstateを受け取らないUtility APIにはこの特例を適用しない。Constructor / Initializerのstable exitでcanonical Postcondition validationを行うかは、各moduleのoperation semanticsと既存の設計判断に従い個別に決定する。コメント形式の変更だけを理由に実行方針を変更しない。

### 6.4 Hot pathとvalidation cost

毎frame、毎object、elementごと、tight loopで呼ばれるAPIでは実行頻度とvalidation costを明示的に検討する。Checked preconditionや安全な実行に必要な検査をhot pathという理由だけで機械的に削除しない。

実測performance、対象規模、operation semantics、計算量に基づき判断する。現時点では強いvalidationを維持し、実測上の問題が生じたらdata structure / traversalやdepthを見直す選択も認める。安全性に不要なdiagnostic validationは、stable batch boundary、unit test、failure injection、sanitizer、明示的self-diagnostics等へ移すことを検討してよい。

---

## 7. Failure handling

### 7.1 Validator failure

現時点のcanonical validatorはboolを返す。Validator自身はlog、assert / abort、application state変更、repair、resource解放を行わない。Falseを受け取ったcallerがresult、diagnostic、test failure、将来のFATAL transition / report等へ変換する。

`is_valid() == false`だけでfailure classificationを決めない。**何を検査し、誰がstateを制御し、trust promotion前後のどちらなのか**を確認する。

- Caller-controlledなchecked precondition違反は、`INVALID_ARGUMENT`、`BAD_OPERATION`等のrecoverable result。
- External Trust Boundaryでpromotion前candidateがinvalidなら、`UNSUPPORTED_FILE`等のrecoverable rejection。
- Established internal stateのcanonical invariant破損は`DATA_CORRUPTED`候補。
- Trusted inputを基にmoduleが生成したcandidateのdiagnostic Result validationがimplementation bugを示す場合、またCommit後のestablished stateに対するPostcondition validation failureは、`DATA_CORRUPTED`の根拠となり得る。
- DEBUG / TESTでtrusted / hard precondition違反を診断した場合、そのAPIの契約に従う。Recoverableとして定義していない条件を機械的に通常errorへ変換しない。

Engine内部producer由来の可能性だけでconsumerが`INVALID_ARGUMENT`を`DATA_CORRUPTED`へ再分類しない。Producerのbug検出はproducer側のdiagnostics / stable boundaryへ委ねてよい。

### 7.2 `DATA_CORRUPTED`

`DATA_CORRUPTED`は、**成立済みと信頼していたGLCE内部state / invariantのintegrityが失われ、通常operationへ安全に復帰できないという明確な根拠がある**場合に使用するpanic-class signalである。Validator failure一般のfallbackではない。

代表例:

- Established internal object / stateのcanonical invariant破損やfield relation破損。
- Owner管理情報の矛盾。
- Trusted inputからmodule自身が作ったresult candidateのdiagnostic failureがinternal bugを示す場合。
- Established internal stateのPostcondition validation failure。
- Cleanup / rollback中に成立済みinternal stateの安全性を確定できなくなった場合。

次の事実だけで`DATA_CORRUPTED`としない。

- Application / Frontendのcaller-controlled valueがAPI validityを満たさない。
- Malformed file、unsupported format、外部OS / device / library data / statusの異常。
- Trust promotion前candidateが内部表現として受理できない。
- Internal producer由来かもしれないinputについてconsumerのvalidity checkが失敗した。

過剰分類はrecoverable failureを不要なFATALへ昇格させる。分類にはinternal integrity喪失を示す追加根拠を要求する。Producer bugがconsumerでは`INVALID_ARGUMENT`として観測されることを許容する。DEBUG / TESTのstable-boundary canonical failureは`DATA_CORRUPTED`へ分類してよい。

Trusted / hard precondition違反に起因するvalue construction / conversionのinvalid outputは正常系contract外であるが、これを一律`DATA_CORRUPTED`とする規則は設けない。

### 7.3 Fail-stop

Fail-stopは**`DATA_CORRUPTED`確定後**の規則である。Caller input failureやexternal rejectionを誤分類しないことが前提となる。

`DATA_CORRUPTED`を検出した後は、そのoperationで通常のrollback / destroy / deinitialize / release / free / close / unbindその他のcleanupや追加resource operationを行わない。

- Suspect objectのfieldを辿るcleanupだけでなく、別local variableで追跡していたconstruction中のtemporary resourceについてもcleanupしない。
- 下位APIから`DATA_CORRUPTED`を受けた上位layerも、通常cleanupを停止し、そのresultを上位へ伝播する。
- Resource leakを許容する。これはcleanup failureではなく、二次破壊を防ぐ意図的なfail-stop behaviorである。
- Recoverable failureのcleanup中に`DATA_CORRUPTED`へ昇格した場合は、その時点で残りのcleanupを停止する。
- 元のfailureが`DATA_CORRUPTED`ならcleanup failureで上書きしない。
- 上位layerは通常運転継続を前提にせずFATAL方向へ伝播する。FATAL state transitionとabort / graceful terminationの詳細境界は将来決定する。

```text
lower layer DATA_CORRUPTED
    ↓
current operationのcleanup停止
    ↓
upper layerへ伝播・upper layerもcleanup停止
    ↓
resource leakを許容
    ↓
FATAL方向へ伝播
```

### 7.4 Recoverable cleanup

Recoverable failure時のrollback / cleanupの構成、private unchecked helperの有無はmoduleごとに決定してよい。ただし、recoverable failure後に継続するobjectはvalidなstable stateへ戻す。Private partial stateのcleanup手段をpublic destroyのValidation迂回に流用しない。`DATA_CORRUPTED`後はunchecked helperを含め通常cleanupを実行しない。Trusted / hard precondition違反はrecoverable cleanup contractの対象外である。

---

## 8. 今後決定する事項

以下は本書では未確定とする。

- Validation Reportのstructured representation。
- System-wide self-diagnosticsの実行方式。
- Explicit diagnostic validationのdepth・実行対象の選択方式。
- 共通validation macro / helperの最終仕様。
- Trusted / hard preconditionをDEBUG / TESTで表現するassertion / diagnostic mechanismの統一方針。
- FATAL state transition API。
- FATAL後のgraceful shutdownと即時abortの境界。
- Subsystem stateとapplication stateの関係。
- 独立した`INVARIANTS`相当build optionの要否。

個別moduleのvalidator実装状況、適用順序、保留事項は`glce_near_term_todo.md`を正本とする。
