<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->

# GLCE Memory Model

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

## 1. Purpose

本書は、GLCEにおけるCPU-side memory managementの全体構造、各Allocatorの責務、ownership、およびvalidation上のtrust boundaryを説明する。

個別APIの詳細なcontractは各header / sourceを、validationの一般規則は`validation_policy.md`を参照する。

---

## 2. Architecture Overview

GLCEのMemory基盤は、単一のMemory Systemではなく、用途とlifetimeの異なるAllocatorを独立したmoduleとして構成する。

```text
engine/memory
│
├─ General Allocator
│      │
│      └─ Free List Allocator
│
└─ Subsystem Allocator
       │
       └─ Linear Allocator
```

General AllocatorとSubsystem Allocatorは、memory lifetime、ownership、accounting等の上位semanticを持つ。

Free List AllocatorとLinear Allocatorは、callerから渡されたmemory poolを管理するlow-level allocation mechanismであり、ApplicationやSubsystem等の上位semanticを持たない。

---

## 3. General Allocator

General Allocatorは、GLCE process全体で共有するgeneral lifetime向けAllocatorである。

process-wide singletonとして存在し、backing memory poolを所有する。

```text
Raw backing storage
        │
        ▼
General Allocator
        │
        ▼
Free List Allocator
        │
        ▼
general lifetime allocations
```

backing storageの取得方式とcapacityはBuild Configurationによって決定する。

- Desktopではprocess startup時に`malloc()`でpool全体を取得し、process shutdown時に解放する。
- Embeddedではcompile-time固定容量のstatic storageを使用し、General Allocator自身は`malloc()` / `free()`を使用しない。

General Allocator内部ではFree List Allocatorを使用し、individual allocationのallocate / freeを提供する。

また、callerが要求したlogical allocation sizeをmemory tag別にaccountingする。

---

## 4. Subsystem Allocator

Subsystem Allocatorは、ApplicationやEngine subsystem等の長寿命resourceをまとめて管理するためのAllocatorである。

現在の実装では、Subsystem Allocator object自身とそのbacking memory poolをGeneral Allocatorから取得する。

```text
General Allocator
        │
        ├─ Subsystem Allocator object
        │
        └─ Subsystem backing memory pool
                    │
                    ▼
             Subsystem Allocator
                    │
                    ▼
              Linear Allocator
                    │
                    ▼
             subsystem resources
```

Subsystem Allocatorはbacking memory poolのownershipを持ち、そのpool上のphysical allocation mechanismをLinear Allocatorへ委譲する。

individual allocationのfreeは提供しない。

allocation stateをまとめて破棄する場合は`reset`を使用し、過去のallocation stateまで戻す場合はrollback point / rollbackを使用する。

Subsystem Allocatorも、callerが要求したlogical allocation sizeをmemory tag別にaccountingする。

---

## 5. Low-level Allocators

### Free List Allocator

Free List Allocatorは、General Allocatorのphysical allocation mechanismとして使用する。

各blockにmetadataを保持し、allocation / free / split / coalesceを管理する。

このため、現在allocation中のpayload先頭pointerであるか、allocation sizeはいくつか、といったallocation-level informationを判定できる。

### Linear Allocator

Linear Allocatorは、Subsystem Allocatorのphysical allocation mechanismとして使用するbump allocatorである。

```text
memory_pool
    │
    ├───────────────┬──────────────────
    │   in use      │      unused
    │               │
    └───────────────┴──────────────────
                    ^
                 head_ptr
```

Linear Allocatorはallocationごとのmetadata、boundary、requested size、identityを保持しない。

そのため、pointerについて判定できるのは、

```text
memory_pool <= ptr < head_ptr
```

というcurrent in-use rangeへのmembershipまでである。

allocation payloadの先頭、特定allocationへの所属、historical allocation identityは保証しない。

---

## 6. Logical Accounting and Physical Usage

General AllocatorとSubsystem Allocatorは、callerが要求したallocation sizeをlogical accountingとして保持する。

一方、low-level allocatorが管理するphysical memory usageには、alignment paddingやblock metadata等が含まれる場合がある。

したがって、

```text
logical allocated size
    !=
physical used size
```

となることを許容する。

両者は異なるsemanticを持つため、同一値であることを要求しない。

---

## 7. Alignment

CPU-side Allocatorが返すallocationは、原則として`alignof(max_align_t)`へalignmentする。

個別のcallerが任意alignmentを指定するmodelは採用せず、Memory基盤側で共通のalignment contractを持つ。

alignment calculation等の共通処理は`engine/base/memory_utility`が担当する。

---

## 8. Allocation-level Validation Authority

`engine/memory`は、GLCE内部におけるallocation-level validationのauthorityである。

上位moduleのvalidatorは、対象pointerを管理するMemory moduleが提供するquery結果を信頼してよい。

ただし、Allocatorごとに保持するmetadataとallocation semanticsが異なるため、提供できるvalidation能力も異なる。

```text
General / Free List based allocation
    allocation-level metadataあり
    → live allocationのpayload先頭を判定可能

Subsystem / Linear based allocation
    per-allocation metadataなし
    → current in-use rangeへのmembershipのみ判定可能
```

上位validatorは、管理Allocatorが提供していないallocation identityを独自に仮定しない。

---

## 9. Memory Validation Trust Boundary

Memory基盤自身のvalidatorは、allocator metadataが完全に無傷であることを別のallocation authorityによって証明するものではない。

各Allocatorは、既知のbacking memory range、capacity、physical layout、および局所metadata間の整合性から、検出可能なcorruptionを検査する。

```text
Upper-layer validator
        │
        │ allocation queryを信頼
        ▼
    engine/memory
========================
 allocation-level
 validation trust boundary
========================
        │
        ├─ Free List Allocator metadata
        └─ Linear Allocator state
```

これはallocator metadataのvalidityを別のAllocatorへ再帰的に問い合わせ続ける構造を避け、GLCE内部でvalidationのtrust boundaryを明確にするためである。

なお、このboundaryはGLCE software architecture内部のものであり、OS、libc、linker、virtual memory、hardware等まで含めて完全性を保証するものではない。

---

## 10. Current Ownership Model

現在の主要なownershipは次のようになる。

```text
Process
│
└─ General Allocator
   ├─ General backing memory pool
   └─ Free List Allocator
        │
        ├─ Application state
        ├─ general lifetime resources
        │
        └─ Subsystem Allocator
             ├─ Subsystem Allocator object
             ├─ Subsystem backing memory pool
             └─ Linear Allocator
                  │
                  └─ subsystem lifetime resources
```

low-level allocatorはbacking memory poolを所有しない。

backing memoryのownershipとlifetimeは、それを利用する上位Allocatorが持つ。

---
