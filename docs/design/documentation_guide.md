<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Documentation Guide

## このドキュメントの使い方

このドキュメントは、GLCEのsource codeに付随するdocumentationについて、**何を、どこへ、どの粒度で記述するか**を定めるためのguideです。

GLCEでは、header / sourceのcommentを単なる補足説明として扱わず、module boundary、internal contract、validation responsibility、API contract等を後から人間とAIの双方が復元できるknowledgeとして扱います。

新しいmoduleを作成する場合、既存moduleのdocumentationを整備する場合、またはAIへdocumentation生成を依頼する場合は、本書を読み込ませた上で、対象moduleのheader / sourceおよび必要なproject-wide policyを併せて参照してください。

本書は、すべてのfileへ同じsectionを機械的に追加するためのtemplateではありません。対象moduleに実際に存在するcontract / policy / responsibilityだけを記述し、存在しない概念のsectionを穴埋めとして作成しないでください。

本書自体の作成・更新にもAIを利用しています。ただし、AIが生成した内容がそのままGLCEの方針になるわけではありません。最終的なdocumentation structureと記述内容は、GLCEのarchitecture、implementation、Validation Policy、Boundary Model、およびproject固有の設計判断との整合性を確認した上で採用します。

最終更新: 2026-10-08

---

## 1. 文書の位置づけ

本書は、GLCEのsource documentation architectureの正本とする。

主に次を扱う。

- headerへ何を記述するか
- sourceへ何を記述するか
- Doxygenと通常commentをどう使い分けるか
- Module Boundary Contract / API-specific Validation Policy / 必要時のみ設けるModule Internal Contract / Module Validation Policyの責務分離
- Public API Contractの記述方法
- API-specific Validation Policyの記述方法
- public / private validator documentationの記述方法
- private helper固有contract / rationaleの記述方法
- 同じ情報を複数箇所へ重複記述しないためのplacement rule

本書は、次の正本文書を置き換えない。

- `glce_coding_style.md`
  - source / header構成、命名、comment形式、function phase名等
- `validation_policy.md`
  - validation responsibility、depth、automatic execution、failure classification等
- `boundary_model.md`
  - External Trust Boundary / Engine API Trust Boundary / Internal Integrity Boundary等
- `design_decisions.md`
  - いつ、なぜ、どの設計判断を採用または変更したか
- 各moduleのheader / source
  - module固有contractの正本

本書の責務は、これらの設計情報を**source documentationへどう配置するか**を定めることである。

---

## 2. 基本原則

### 2.1 Documentationは責務単位で分離する

同じmoduleについても、利用者が知るべき情報とimplementation内部だけで成立する情報は分離する。

基本構造は次のとおりとする。

```text
header
    ↓
module利用者が知るcontract

source
    ↓
implementationが維持するcontract / policy / rationale
```

さらに、validationについては、

```text
contract
    「何が成立していなければならないか」

validation policy
    「そのcontractの何を、なぜ、どの位置でautomaticに検査するか」
```

を分離する。

### 2.2 正本を一つにする

同じ情報を複数箇所へ同じ詳細度で複製しない。

例えば、

- ownership / lifetimeがModule Boundary Contractに書かれている場合、Module Internal Contractへ同じ文章を再掲しない。
- public APIのcontractをAPI-specific Validation Policyへコピーしない。
- project-wide Validation PolicyをModule Validation Policyへ全文複製しない。
- private validatorの検査項目をModule Validation Policyへ一覧として複製しない。

別のdocumentationから同じ概念へ言及する必要がある場合は、そのdocumentation固有の観点だけを記述する。

### 2.3 Sectionを機械的に作らない

documentation sectionはtemplateの穴埋めではない。

例えば、module全体を横断するinternal invariantが存在しない場合、`Module Internal Contract`を作成しない。

同様に、

- module全体に固有のvalidation strategyが存在しない
- ownership / lifetime説明が不要
- private helperに特別なcontractがない

場合は、対応sectionを機械的に追加しない。

ただし、Public APIの`API-specific Validation Policy`は原則として各APIに記述する。

validationを実行するAPIだけでなく、operation semantics上追加validationが不要なAPIについても、
「なぜvalidationしないのか」を明示し、意図的な非実行とcheck漏れを区別できるようにする。

### 2.4 実装から自明な処理の逐語説明を避ける

documentationはcodeを日本語へ置き換えるためのものではない。

次の情報を優先する。

- responsibility
- contract
- invariant
- ownership
- lifetime
- representation rule
- semantic authority
- validation rationale
- failure semantics
- design上重要なrelation

単純なassignmentやloopの内容をそのまま説明するだけのcommentは追加しない。

---

## 3. Documentation Placement Map

| Documentation | 配置 | 形式 | 主な目的 |
|---|---|---|---|
| File / Module Description | `.h` file-level | Doxygen | module全体の責務を説明する |
| Module Boundary Contract | `.h` file-level | Doxygen | module利用者が守る／期待できるmodule-level contract |
| External Format Specification | `.h` format ownerのfile-level | Doxygen | moduleが所有するexternal formatのcanonical specification |
| Public API Contract | `.h` 各public API | Doxygen | caller / callee間のoperation contract |
| Public Validator Documentation | `.h` 各public validator | Doxygen | validの意味と検査scopeを公開する |
| Module Internal Contract | `.c` file-level / 必要時のみ | 通常block comment | 複数private type / helper / APIを横断するinternal invariant |
| Module Validation Policy | `.c` file-level / 必要時のみ | 通常block comment | API単位へ局所化できないmodule-wide validation strategy |
| API-specific Validation Policy | `.c` 各public API definition直前 | 通常block comment | 何を、なぜvalidateするか、またはなぜ追加validationしないか |
| Private Validator Documentation | `.c` 各private validator definition直前 | 通常block comment | validatorのPurpose / Validation Scope |
| Private Helper Contract / Rationale | `.c` 必要箇所 | 通常block comment | helper固有のPreconditions / Responsibility / Postconditions / Validation |

---

## 4. Header Documentation

## 4.1 File-level Doxygen

public / Engine-internal public headerの先頭では、必要に応じて次を記述する。

```c
/**
 * @file module_name.h
 * @author chocolate-pie24
 * @brief moduleの責務を短く説明する
 * @details module全体の役割と責務境界を説明する
 *
 * @section module_name_boundary_contract Module Boundary Contract
 *
 * ...
 */
```

### `@file`

- 実ファイル名と一致させる。

### `@author`

- `chocolate-pie24`を使用する。

### `@brief`

- moduleが何を担当するかを短く説明する。
- implementation detailを列挙しない。

### `@details`

- module全体の責務を説明する。
- Module Boundary Contractの内容を全文繰り返さない。
- format-specific semanticを持たないutility等では、「何を扱い、何を扱わないか」を簡潔に説明してよい。

---

## 4.2 Module Boundary Contract

Module Boundary Contractは、**module利用者が知る必要のあるmodule-level contract**を記述する。

代表的な対象:

- module scope
- ownership
- lifetime
- storage responsibility
- borrow relation
- module利用者が成立させる必要のあるhard / trusted contract
- module利用者が依存できる保証
- fixed alignment
- public representationの有効期間
- trust / authority boundary上のmodule-level responsibility

必要に応じて、次のようなsubsectionを使用してよい。

```text
Module Scope
Trust Boundary
Ownership
Lifetime
Storage
Representation
Borrowed View
State / Resource Ownership
```

subsection名は固定しない。moduleに実際に存在する責務だけを使用する。

### Module Boundary Contractへ書かないもの

次は原則として書かない。

- private helper名
- private validator名
- validation callの順序
- DEBUG / TEST / RELEASEごとのvalidation depth
- canonical / shallow validatorのautomatic execution policy
- API固有のPreconditions / Result validation / Postconditions配置
- internal field relation
- private representation invariant
- implementation-specific helper contract

これらはsource側documentationまたはAPI固有documentationの責務とする。

### External Trust Boundaryを所有するmodule

Loader等、そのmoduleがexternal / untrusted representationをGLCE内部representationへ昇格させる
External Trust Boundary ownerである場合、その事実はModule Boundary Contractへ明示する。

少なくとも次が分かるようにする。

- moduleがExternal Trust Boundaryを所有すること
- boundary outsideではinputをuntrusted representationとして扱うこと
- required validation成功後にのみtrusted internal representationとして受理すること
- trust promotionがどのpublic operationの成功時に完了するか

個々のvalidation callやvalidation phaseはsource側のAPI-specific Validation Policyへ置く。

### External Format Specification

moduleが独自external formatのsemantic ownerである場合、そのformat specificationは
format ownerのheaderへDoxygenとして記述する。

代表的な対象:

- line / token grammar
- whitespace / comment rule
- delimiter
- supported key / field
- case sensitivity
- required field
- duplicate rule
- value representation
- numeric range
- ordering
- unsupported syntax

asset file自体をcanonical specificationにしない。

```text
header
    canonical format specification

asset
    concrete example / 必要に応じてheaderへの案内
```

同じformat ruleをheaderとasset commentの両方で独立した正本として管理しない。

---

## 4.3 Public API Contract

public APIが単純なsignature説明だけではcontractを十分に表現できない場合、`@par Public API Contract`を使用する。

例:

```c
/**
 * @brief ...
 *
 * @details
 * ...
 *
 * @par Public API Contract
 *
 * Input Representation:
 * - ...
 *
 * Output Contract:
 * - ...
 *
 * Lifetime / Ownership:
 * - ...
 *
 * @param[in] ...
 * @param[out] ...
 * @retval ...
 * @pre ...
 * @post ...
 */
```

### Public API Contractの目的

次をcallerへ明示する。

- accepted input representation
- success条件
- failure時のoutput state
- output representation
- ownership transfer
- borrow semantics
- lifetime
- mutation / side effect
- classification semantics
- API固有のstable guarantee

### subsectionは固定しない

例えば、APIによって次を使い分けてよい。

```text
Input Representation
Classification
Output Contract
Lifetime / Ownership
State Transition
Failure Contract
Side Effects
```

単純getter等へ不要なsubsectionを機械的に追加しない。

---

## 4.4 `@pre` / `@post`の使い方

`@pre` / `@post`はPublic API Contractを置き換えるものではない。

### `@pre`

callerがoperation開始前に成立させる必要がある条件を記述する。

特に、

- calleeが安全に検証できないmemory readability / addressability
- valid object lifetime
- required external lifetime
- trusted / hard contract

等を明示するために使用する。

API自身が通常のchecked validationによってreject可能なinput representationについては、単純にすべてを`@pre`へ押し込めず、Public API Contractや`@retval`で受理 / rejection semanticsを説明する。

### `@post`

API成功後にcallerが依存できる保証を記述する。

`@post`はimplementation上の`Postconditions` phaseと同義ではない。

例えば、implementationがtemporary resultを`Result validation`した後に単純copyでOutputする場合でも、public API contractとしては成功後の保証を`@post`に記述してよい。

---

## 4.5 Public Validator Documentation

public validatorは、単に「validか判定する」とだけ記述しない。

少なくとも次が分かるようにする。

- 何のvalidityを所有するvalidatorか
- structural validityとして何を見るか
- semantic validityとして何を見るか
- 何を検査しないか
- callerが成立させる必要のあるreadability / lifetime contract
- 禁止文字、range、relation等がpublic contractの場合は、その具体的条件

例えば、

```text
key/value tokenとしてvalid
```

だけではなく、

```text
space、tab、CR、LF、'='、NULをtoken内部に含まない
```

のように、利用者がvalidator semanticsを再現できる粒度で記述する。

validator implementationで使用するprivate helper名はheaderへ記述しない。

---

## 5. Source Documentation

## 5.1 Module Internal Contract

Module Internal Contractは、source fileへ標準的に必ず置くsectionではない。

まず、internal semanticをそのownerに近い場所へ配置する。

```text
canonical / local validity
    → validator documentation

private operation固有の成立条件
    → Private Helper Contract

public API固有のvalidation rationale
    → API-specific Validation Policy
```

その上でなお、**複数のprivate type / helper / public APIを横断するmodule-wide invariant**が存在し、
個別documentationへ分散すると理解が悪化する場合だけModule Internal Contractを作成する。

代表的な対象:

- 複数private type全体で成立するstate machine
- module全体で共有するtransient state rule
- 複数operationが共通して信頼するownership / representation relation
- implementation全体を理解するために必要なcross-cutting invariant

### 作成しない例

次の情報は、原則としてModule Internal Contractへ昇格させない。

- 特定validatorだけが所有するvalidity definition
- 特定helperだけのPreconditions / Postconditions
- 一つのtemporary representationだけの局所relation
- public API固有contract
- validation execution policy

これらはsemantic ownerの近くへ記述する。

### Module Internal Contractへ書かないもの

- Module Boundary Contractの再掲
- public API固有contract
- validation execution policy
- private validatorの検査項目一覧
- build mode別validation depth
- API-specific validation call sequence

---

## 5.2 Module Validation Policy

Module Validation Policyも標準sectionとして必須ではない。

validation executionの理由はoperationごとに異なるため、基本は各Public APIの
`API-specific Validation Policy`へ局所化する。

Module Validation Policyは、**API単位へ分解してもなおmodule全体に共通するvalidation strategyが存在する場合だけ**作成する。

例:

- module固有のtransient stateに対する共通validation ruleがある
- 複数API / helperを横断する一つのvalidation architectureがある
- module全体でcanonical / shallow validatorを特別な意味で使い分け、その意味を個別APIだけでは説明しにくい

### 作成しない例

次の情報だけしかない場合、Module Validation Policyを作らない。

- private validatorが複数存在する
- API Aはvalidator Xを呼ぶ
- API Bはvalidator Yを呼ぶ
- 各API固有のvalidation理由はそれぞれ異なる
- 「private helperでは成立済みcontractを信頼する」等、project-wide Validation Policyで既に定義されているruleだけで説明できる

この場合は、

```text
project-wide validation_policy.md
+
各validator documentation
+
API-specific Validation Policy
```

で十分である。

### Module Validation Policyへ書かないもの

- project-wide Validation Policyの全文
- 各public APIのvalidation手順一覧
- 各private validatorのPurpose / Scope
- Boundary Contract
- helper固有Contract

---

## 5.3 API-specific Validation Policy

Public APIには原則として、`.c`側のfunction definition直前へ
`API-specific Validation Policy`を通常commentとして記述する。

これはvalidationを多く実行するAPIだけを対象としない。

operation semantics上、追加validationが不要なAPIについても、
「なぜvalidationしないのか」を記述し、意図的な設計とcheck漏れを区別できるようにする。

例:

```c
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - ...
 *
 * Result validation:
 * - ...
 *
 * Postconditions:
 * - ...
 */
result_t module_operation(...);
```

存在しないphaseは記述しない。

### 記述する内容

必要に応じて、次を説明する。

- 何を検査するか
- なぜその検査が必要か
- checked / trusted contractのどちらか
- どのsemanticを安全にconsumeするための検査か
- 何を検査しないか
- なぜ追加validationが不要なのか
- failure時にoperation / output / stateをどう扱うか
- canonical / shallow / direct checkを選ぶ理由
- DEBUG / TEST / RELEASEで実行条件が異なる場合、その理由
- Result validation / Postconditionsを行う、または行わない理由

### validationを行わないAPI

例えば、existing representationをconsumeせず、直接reset stateへ上書きするAPIでは、
operation開始前のcanonical validationが不要な場合がある。

その場合も、

```text
- existing stateをsemantic inputとしてconsumeしないためPreconditionsでcanonical validationしない。
- success stateがdirect writeそのものから成立するため、同一conditionのPostconditions validationを行わない。
```

のように理由を残す。

### 単なるcall一覧にしない

次のような記述だけでは不十分。

```text
- xxx_is_valid()を呼ぶ。
- yyy_is_valid()を呼ぶ。
```

代わりに、

```text
- xxx_is_valid()によってcandidate resultがpublic output contractを満たすことを確認する。
- このvalidationは、invalid representationをcaller-visible outputへ公開しないために行う。
```

のように、validationのsemantic purposeを記述する。

### Function phaseとの関係

function phase名そのものの定義は`glce_coding_style.md`を正本とする。

API-specific Validation Policyでは、そのAPIについて必要なphaseだけを説明する。

代表例:

```text
Preconditions
    input / caller contractのautomatic validation

Result validation
    Prepare等で構築したresult candidateがsuccess output contractを満たすか確認

Postconditions
    Commit後のstable stateに対するautomatic validation
```

`Preflight`はoperation実行可能性の確認であり、result candidate validityとは区別する。

---

## 5.4 Private Validator Documentation

private validatorは、definition直前へ通常commentでPurposeとValidation Scopeを記述する。

基本形:

```c
/*
 * xxx_is_valid() Validation
 *
 * Purpose:
 * - このvalidatorが何のvalidityを確認するために存在するか。
 * - 後続処理が何を成立済みcontractとして扱えるようにするか。
 *
 * Validation Scope:
 * - 実際に検査するcondition。
 * - 必要に応じて、検査対象外であるcondition。
 */
static bool xxx_is_valid(...);
```

### Purpose

「validか確認する」の言い換えではなく、validatorのsemantic responsibilityを書く。

例:

```text
logical line representationとして成立していることを確認し、
後続処理がbounded scanとNUL terminationを前提として扱えるようにする。
```

### Validation Scope

実装が実際に検査するconditionを具体的に列挙する。

特にcharacter setやrange rule等は、

```text
token characterとしてvalid
```

だけではなく、

```text
space / tab / CR / LF / '=' / NULではない
```

のように具体化する。

### Private validator一覧の重複管理を避ける

private validatorの存在一覧は、`Private Function Declarations`内の`Validators` subgroupそのものをinventoryとして扱う。

Module Validation Policy等へ同じ一覧を複製しない。

---

## 5.5 Private Helper Contract / Rationale

意味のあるprivate helperについて、implementationだけではresponsibilityや成立条件を復元しにくい場合、
definition直前へ通常commentでhelper固有Contractを記述する。

基本形は次とする。

```c
/*
 * helper() Contract
 *
 * Preconditions:
 * - ...
 *
 * Responsibility:
 * - ...
 *
 * Postconditions:
 * - ...
 *
 * Validation:
 * - ...
 */
static result_t helper(...);
```

すべてのsubsectionを機械的に追加しない。

helper自身がruntime validationを行わない場合は`Validation`を省略してよい。

### Preconditions

helperが安全かつ正しくoperationを実行するために、call時点で成立している必要があるconditionを書く。

例:

- argument / stateについて何が成立済みか
- 上位処理でどのsemantic validationが完了しているか
- transient stateとして何が一時的に許容されるか
- helper自身が再検査せず信頼するcontract

### Responsibility

逐次algorithmではなく、そのhelperが所有するsemantic responsibilityを書く。

例えば、

```text
delimiterの左右からkey / value candidate rangeを導出する。
```

はResponsibilityである。

一方、

```text
for loopを回してindexを増やす。
```

のようなcodeから自明な手順説明は書かない。

### Postconditions

success時にcallerが信頼できるconditionを書く。

failure時にpartial mutationがあり得る場合、その事実がcaller contractとして重要なら明示する。

### Validation

helper自身がvalidationする場合は、

- 何を検査するか
- なぜこのhelperがその検査を所有するか
- failureをどのsemanticとして扱うか

を必要な範囲で説明する。

単に「validatorを呼ぶ」とだけ書かず、operation上の必要性を残す。

実装から明らかなeffectを説明するためだけに`Effect`等のsectionを機械的に追加しない。

---

## 6. Public / Privateの情報境界

### Headerへ出す

module利用者がAPIを正しく、安全に利用するために必要な情報。

例:

- ownership
- lifetime
- borrow invalidation
- storage readability
- accepted representation
- output contract
- success / failure semantics
- public validator semantics

### Sourceへ閉じる

module利用者が知る必要のないimplementation strategy。

例:

- private validator名
- validation helperの呼び出し順
- internal index relation
- private temporary representation
- build mode別automatic validation strategy
- API内部でcanonical / shallowをどこで使うか
- helper間で信頼するinternal contract

---

## 7. 重複を避けるための判断順序

documentationを追加する前に、まず**最もsemantic ownerに近い場所**へ配置できるかを判断する。

```text
1. module利用者が知る必要があるか？
    yes → header
        module全体のboundary fact
            → Module Boundary Contract
        external formatそのものの仕様
            → External Format Specification
        特定public APIのcontract
            → Public API Contract
        public validatorのvalidity definition
            → Public Validator Documentation

2. 特定private validatorが何をvalidとみなすか？
    yes → Private Validator Documentation

3. 特定private helperの成立条件 / responsibility / postcondition / validation rationaleか？
    yes → helper直前のPrivate Helper Contract / Rationale

4. 特定public APIがなぜvalidationする／しないか？
    yes → API-specific Validation Policy

5. 上記へ局所化してもなお、
   複数private type / helper / APIを横断するinternal invariantが残るか？
    yes → Module Internal Contractを検討

6. 上記へ局所化してもなお、
   API単位では表しにくいmodule-wide validation strategyが残るか？
    yes → Module Validation Policyを検討
```

Module Internal Contract / Module Validation Policyを先に作ってから個別documentationへ展開するのではなく、
まずsemantic ownerへ情報を局所化する。

同じ情報が複数候補へ該当する場合は、最もsemantic ownerに近い一箇所を正本とし、
他ではそのdocumentation固有の観点だけを記述する。

---

## 8. Example: Parser Utility

`glce_config_utility`のようなlow-level parser utilityでは、次のように分離できる。

```text
header
    File / Module Description

    Module Boundary Contract
        module scope
        input / output representation
        lifetime / storage contract
        External Trust Boundaryを所有しないこと

    Public API Contract
        input representation
        classification / output semantics

    Public Validator Documentation
        public representation validity

source
    API-specific Validation Policy
        APIごとのchecked / trusted precondition
        Result validation
        Postconditions validationの採否理由
        validationしない場合の理由

    Private Validator Documentation
        logical line validity
        token / range validity

    Private Helper Contract / Rationale
        Preconditions
        Responsibility
        Postconditions
        helper自身がvalidationする場合のValidation
```

format-specific supported keyやrequired field等はparser utility自身へ持たせず、
そのformatを所有する上位Loader等へ配置する。

Module Internal Contract / Module Validation Policyは、
上記へ局所化してもなおmodule-wideなcross-cutting ruleが残る場合だけ追加する。

この構造では、

```text
何がvalidか
```

と、

```text
各operationがいつ、なぜvalidateするか
```

を別document responsibilityとして維持できる。

---

## 9. Documentation Review Checklist

documentation reviewでは、少なくとも次を確認する。

### Header

- `@file`は実ファイル名と一致しているか。
- `@brief`はmodule responsibilityを短く表しているか。
- `@details`がBoundary Contractの全文複製になっていないか。
- Module Boundary Contractにprivate implementation detailが漏れていないか。
- External Trust Boundary ownerである場合、その事実とtrust promotion pointが明示されているか。
- 独自external formatのownerである場合、canonical Format Specificationがheader側にあるか。
- ownership / lifetime / borrowがAPI利用に必要なら明記されているか。
- Public API Contractがchecked validation policyと混同されていないか。
- `@pre`へrecoverable input conditionを機械的に押し込めていないか。
- public validatorのvalidity definitionが具体的か。

### Source

- Module Internal Contractを、validator / helper / API固有情報の寄せ集めとして作っていないか。
- Module Validation Policyは本当にAPI単位へ局所化できないmodule-wide ruleを持つ場合だけ存在しているか。
- 各Public APIにAPI-specific Validation Policyがあり、validationする／しない理由が説明されているか。
- API-specific Validation Policyが「何を呼ぶか」ではなく「なぜvalidateするか」を説明しているか。
- private validatorにPurpose / Validation Scopeがあるか。
- meaningfulなprivate helperに必要なPreconditions / Responsibility / Postconditionsがあるか。
- helper自身がvalidationする場合、そのValidation rationaleが説明されているか。
- helper固有contractがmodule-wide contractへ不必要に昇格していないか。
- project-wide policyをsourceへ全文複製していないか。

### 全体

- 同じ事実の正本が複数箇所に存在していないか。
- 最もsemantic ownerに近い場所が正本になっているか。
- documentationが現在のimplementationと一致しているか。
- 存在しないsectionをtemplateとして追加していないか。
- external format specをasset commentと二重管理していないか。
- codeから自明な処理説明だけのcommentが増えていないか。
- 人間だけでなくAIが「なぜこの構造なのか」を復元できる情報になっているか。

---

## 10. AI支援でDocumentationを作成する場合

AIへdocumentation生成を依頼する場合は、対象sourceだけでなく、必要に応じて次を併せて与える。

```text
documentation_guide.md
glce_coding_style.md
validation_policy.md
boundary_model.md
対象moduleのheader / source
関連するdesign_decisions.md
```

依頼時には、生成対象のdocumentation categoryを明示する。

例:

```text
Module Boundary Contractだけを生成してください。
API固有情報、private helper contract、validation execution policyは含めないでください。
External Trust Boundary ownerである場合は、そのmodule-level responsibilityだけ含めてください。
```

```text
Private Helper Contractを生成してください。
Preconditions / Responsibility / Postconditionsを中心にし、
helper自身がvalidationする場合だけValidationを追加してください。
codeから自明なalgorithm説明は追加しないでください。
```

```text
API-specific Validation Policyを生成してください。
Public API Contractの再掲ではなく、
何を、なぜvalidateするか、またはなぜ追加validationしないかを書いてください。
```

Module Internal Contract / Module Validation Policyを依頼する場合は、
まずvalidator / helper / API-specific documentationへ情報を局所化できない
cross-cutting ruleが本当に存在するかを確認する。

AIが別categoryの情報を混在させた場合は、その情報を削除または正しいdocumentation locationへ移す。

---

## 11. 関連文書

- `glce_coding_style.md`
- `validation_policy.md`
- `boundary_model.md`
- `design_decisions.md`
- `design_notes.md`
