<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Documentation Guide

## このドキュメントの使い方

本書は、GLCEのsource codeに付随するdocumentationについて、**何を、どこへ、どの程度記述するか**を定める正本文書である。

GLCEでは、責務分離、命名、API設計、制御フローを明確にし、詳細な解説文書に依存せず実装を理解できる構造を優先する。内部実装の動作や構造は、必要なときに最新のsource codeを人間またはAIが直接解析する。

documentationは、主として**公開APIの利用者が必要とする契約**を伝えるために記述する。実装から復元しにくい重要な意図や判断理由については、該当箇所に短いコメントを残す。

最終更新: 2026-11-22

---

## 1. 本書の責務

本書が扱う事項:

- `.h`における公開APIのDoxygen
- `.h` / `.c`先頭の説明の扱い
- `.c`におけるprivate helper・private validatorのコメント
- 実装上の重要な意図や判断理由を示す局所コメント
- ドキュメントの重複と陳腐化の防止

以下は本書とは別の正本で管理する。

- `coding_style.md`: 命名、source / header構成、関数内部の記述規則等
- `validation_policy.md`: Validation Architecture、validation depth、failure classification等
- `boundary_model.md`: boundary、trust、authority、ownership等の共通モデル
- `design_decisions.md`: 重要な設計判断と、その歴史・判断理由

本書はこれらの規範や設計モデルをAPI別に再説明することを求めない。

---

## 2. 基本原則

1. **実装の自己説明性を優先する。** 読みにくい実装を長いコメントで補う前に、構造・責務・命名を改善できないか検討する。
2. **公開契約と内部実装を区別する。** 利用者に保証する事項は`.h`へ記述し、内部の処理手順は原則としてコード自体を正本とする。
3. **コメントは必要な情報だけを記述する。** 単なる処理の言い換え、validatorの呼び出し一覧、コードから明らかな手順を重複記述しない。
4. **説明を機械的に増やさない。** 一律のsection、comment、Doxygen tagの追加を目的としない。
5. **変更されやすい実装詳細を契約へ昇格させない。** 内部データ構造やprivate helperの呼び出し順を、必要なく公開仕様に固定しない。
6. **契約と実装の不一致を放置しない。** 相違を発見した場合は、いずれかを自動的に正しいとみなさず、仕様または実装の修正を検討する。

---

## 3. Header Documentation（`.h`）

### 3.1 公開APIのDoxygen

利用者が呼び出す可能性のある公開APIは、原則として宣言の直前にDoxygenを記述する。

- `@brief`: APIの目的。簡潔で、他の宣言と区別できる内容とする。
- `@param`: 引数の意味。`[in]` / `[out]` / `[in,out]`は実際の契約に合わせる。
- `@return` / `@retval`: 戻り値やresult codeの意味。必要な場合に記述する。
- `@note` / `@details`: 利用者が理解すべき注意事項や補足契約がある場合に用いる。
- `@pre` / `@post`: 利用者に必要な事前条件・成功後の保証があり、明示すると有用な場合に用いる。

すべてのtagを機械的に追加する必要はない。単純なAPIは`@brief`と必要な引数説明だけでもよい。

特に、次の内容が利用者の正しい使用に影響する場合は明示する。

- 許容する入力の条件と、呼び出し側が保証する前提条件
- 成功時・回復可能な失敗時の出力および状態の保証
- Ownershipの取得・移転・返却、Move成功時のポインタのNULL化
- Borrowした参照の有効期間と無効化条件
- Resourceの解放方法と対応するdestroy / release API
- APIの使用順序やLifetime上の制約
- 呼び出し側から観測できる重要な副作用

例えば、生成されたStorageを利用者が所有するAPIでは、対応する解放APIが分かるようにする。

```c
/**
 * @brief BMPファイルを読み込み、Pixel Bufferを生成する。
 * @param[in] fullpath_ 読込対象ファイルのパス。
 * @param[out] out_resource_info_ 読み込んだ画像のMetadata出力先。
 * @param[out] out_pixels_ 生成したPixel Bufferの出力先。
 * @note 成功時に返されるPixel Bufferは呼び出し側が所有し、
 *       bmp_loader_pixels_destroy()で解放する。
 */
loader_result_t bmp_loader_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_);

/**
 * @brief BMP Loaderが生成したPixel Bufferを解放する。
 * @param[in,out] pixels_ 解放対象のポインタ。解放後はNULLとなる。
 */
void bmp_loader_pixels_destroy(uint8_t** pixels_);
```

上記は記述粒度の例であり、個々のAPIの正確な引数・失敗時契約は実装と照合して記述する。

### 3.2 公開Validator

公開Validatorは、**利用者が戻り値から何を保証・判断できるか**を説明する。

Validation対象、呼び出し側の必要な前提条件、検査対象外の事項が利用者の判断に影響する場合は明記する。ただし、実装内部の検査順序やprivate validator一覧を説明する必要はない。

### 3.3 Header先頭のDoxygen

`.h`先頭の`@file`、`@brief`、`@details`等は、公開APIだけでは伝わりにくい共通の説明がある場合や、Doxygenのファイル・グループ表示に必要な場合に使用する。

複数の公開APIに共通する利用者向けの制約は、必要に応じてファイル先頭で簡潔に説明してよい。ただし、個々のAPIの正しい使用に必要な契約が見つけにくくならないようにする。

### 3.4 外部データ形式

Loader等の利用者が入力データを作成・選択するために、ファイル形式、受理条件、制約を知る必要がある場合は、該当する公開APIのDoxygen、または関連する`.h`の説明に記述する。

必要な範囲を超えてparserの内部手順を説明しない。同じ形式仕様を複数箇所で独立した正本として維持しない。

---

## 4. Source Documentation（`.c`）

### 4.1 ファイル先頭

`.c`先頭のファイル説明用Doxygenは原則として設けない。

License表記、include、型定義、関数宣言等は通常どおり残す。Doxygen生成に必要な特別な理由がある場合に限り、ファイル先頭の記述を検討してよい。

### 4.2 Public APIの実装

`.c`に、公開APIごとの詳細なValidation Policyや処理説明を記述しない。

**公開APIのDoxygenは原則として`.h`側の宣言に置く。** `.c`の関数定義直前に別の説明ブロックを追加し、IDEのマウスオーバー等で`.h`のAPI仕様が参照しにくくなることを避ける。

実装上の重要な理由や注意事項がある場合は、関係する処理の近くに短いコメントを置く。

### 4.3 Private Helper・Private Validator

private helperおよびprivate validatorのコメントは、必要な場合に次の内容へ限定する。

- 関数の目的
- 実装または関数名から読み取りにくい注意事項

関数名と実装から明らかな場合は、コメントを省略してよい。Doxygen形式や固定の`Preconditions / Responsibility / Postconditions / Validation`構成は求めない。

```c
// 事前に成立したCommit条件に基づいてAllocation情報を確定する。
// この段階ではEligibilityを再判定しない。
static void allocation_commit(...);
```

上記のような注意事項も、実装から十分に読み取れる場合は記述不要とする。

### 4.4 意図・判断を残す局所コメント

実装から動作は分かっても、**なぜその処理を採用したか**が判断しにくい場合には、該当箇所へ短いコメントを残す。

例えば、意図的な非実行、特殊なRollback制限、上位で成立済みの条件への依存、例外的なOwnership操作等が対象となる。

コメントを残す場合も、処理手順の列挙や共通Policyの転載ではなく、誤解されやすい理由・注意点に絞る。Project全体に影響する重要な設計判断の経緯は、必要に応じて`design_decisions.md`へ記録する。

---

## 5. 廃止するドキュメント形式

ソースコード中に、次の名称の独立したドキュメントブロックは設けない。

- `Module Boundary Contract`
- `Module Internal Contract`
- `Module Validation Policy`
- `API-specific Validation Policy`
- `Private Helper Contract / Rationale`等の詳細な定型ブロック
- private validatorのPurpose / Validation Scopeを網羅する定型ブロック

これらの**記述形式を廃止するのであり、契約やValidationの実装規則を廃止するものではない**。

公開API利用者に必要な契約は`.h`のDoxygenへ記述する。内部InvariantやValidationの実行内容は最新の実装・Validatorから確認する。必要な意図や注意点だけを局所コメントへ残す。

既存の詳細コメントは、対象モジュールの修正時等に順次整理してよい。機械的な一括削除によって、公開契約や実装から復元できない意図まで失わないようにする。

---

## 6. Documentation Review

ドキュメントの新規作成・修正時には、次を確認する。

- API利用者が必要とする情報を、`.h`の該当する宣言から確認できるか。
- Ownership、Lifetime、Move、Borrow、解放方法、失敗時の保証などが実際の契約と一致しているか。
- 内部実装の詳細やValidation手順を、別コメントへ重複記述していないか。
- private helperのコメントは目的と必要な注意事項に収まっているか。
- コメントが必要以上に内部実装の変更を拘束していないか。
- コードから復元しにくい重要な判断理由を失っていないか。
- IDEのマウスオーバーやDoxygenの出力で、公開API仕様を確認しやすいか。

**内部実装の説明が長く必要になる場合は、まずコードの構造自体を改善できないか検討する。**

---

## 7. AIを用いた確認・生成

AIには、原則として最新の対象sourceと必要な共通Policyを与え、実装の振る舞い・内部構造を都度解析させる。古い内部説明文書を最新仕様の代用にしない。

- 現在の実装動作はsource codeから確認する。
- 公開APIの保証内容は`.h`のDoxygenと照合する。
- 実装と公開契約が不一致の場合は不整合として報告する。
- 実装から推定した意図と、コメント・設計判断記録で確認できた意図を区別する。
- ドキュメント生成時は、実装から分かる処理説明を増やすより、利用者に必要な契約の正確さを優先する。

---

## 8. 関連文書

- `project_policy.md`
- `coding_style.md`
- `validation_policy.md`
- `boundary_model.md`
- `design_decisions.md`
