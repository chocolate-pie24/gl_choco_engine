---
title: "step2_2: リニアアロケータの仕様変更"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook2に対応しています。

- [このステップでやること](#このステップでやること)
- [linear\_allocator\_preinit実装](#linear_allocator_preinit実装)
- [linear\_allocator\_init実装](#linear_allocator_init実装)
- [アプリケーション側実装変更](#アプリケーション側実装変更)
- [その他小変更](#その他小変更)

## このステップでやること

前回、メモリシステムの仕様変更を行いました。今後は、全てのメモリ確保をメモリシステムで一元管理するよう変更していきます。
その一環で、今回はリニアアロケータのメモリをメモリシステムを使用して確保できるように仕様変更を行っていきます。

リニアアロケータは、最初に大きなメモリプールを確保し、あとはそのメモリプールからメモリを割り当てます。
このため、メモリシステムを使用したメモリ確保はメモリプールの確保のみになります。

前回作成したリニアアロケータでは、linear_allocator_create APIによってメモリプールのメモリを確保していました。
このメモリ確保にメモリシステムを使用したいのですが、リニアアロケータとメモリシステムが同一レイヤー(core/memory)に配置されているため、
linear_allocator_createの中でメモリシステムのAPIを呼びたくありません。そこで、次のようなやり方を取ることにします。

```c
memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

size_t linear_alloc_mem_req;    /**< リニアアロケータオブジェクトに必要なメモリ量 */
size_t linear_alloc_align_req;  /**< リニアアロケータオブジェクトが要求するメモリアライメント */
size_t linear_alloc_pool_size;  /**< リニアアロケータオブジェクトが使用するメモリプールのサイズ */
void* linear_alloc_pool;        /**< リニアアロケータオブジェクトが使用するメモリプールのアドレス */

// linear_alloc_t用メモリ確保のためのメモリ要件取得
linear_alloc_t* linear_alloc = NULL;
linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);

// linear_alloc_tのメモリ確保
ret_mem_sys = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&linear_alloc);

// linear_alloc_tが管理するメモリプールのメモリ確保
linear_alloc_pool_size = 1 * KIB;
ret_mem_sys = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);

// linear_alloc_tの初期化(メモリプールのセット)
ret_linear_alloc = linear_allocator_init(linear_alloc, linear_alloc_pool_size, linear_alloc_pool);
```

このやり方では、リニアアロケータ自身でメモリプールを確保するのではなく、使用側となる上位層でメモリ確保を行い、リニアアロケータに渡します。
こうすることで、リニアアロケータのメモリシステムへの依存を排除でき、上から下のみへの依存を維持することができます。
使い方として若干手間ではあるのですが、リニアアロケータを使用する箇所というのは限定的であるため、この方式で行くことにします。

この方式を取ることにより、メモリの確保がAPI呼び出し側の責務になりました。それに伴い、不要となるAPIは、

- linear_allocator_create
- linear_allocator_destroy

です。また、仕様変更の影響を受けないAPIは、

- linear_allocator_allocate

となります。また、linear_allocator_t構造体のメンバについても今回の仕様変更の影響を受けません。

以降、追加となるlinear_allocator_preinitとlinear_allocator_initについて実装の解説を行います。

## linear_allocator_preinit実装

linear_allocator_preinitでは、APIの呼び出し側にlinear_allocator_tのメモリを確保するために必要な、

- メモリ容量
- メモリアライメント

を伝えるために使用します。実装はこのようになります。

```c
void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        return;
    }
    *memory_requirement_ = sizeof(linear_alloc_t);
    *align_requirement_ = alignof(linear_alloc_t);
}
```

## linear_allocator_init実装

次にlinear_allocator_initの実装です。linear_allocator_initでは、API呼び出し側で確保された

- linear_alloc_tオブジェクトのメモリ
- メモリプールの先頭アドレス

を渡してlinear_alloc_tオブジェクトを初期化します。実装はこのようになります。

```c
linear_allocator_result_t linear_allocator_init(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_) {
    linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(allocator_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_init", "allocator_")
    CHECK_ARG_NULL_GOTO_CLEANUP(memory_pool_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_init", "memory_pool_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != capacity_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_init", "capacity_")

    allocator_->capacity = capacity_;
    allocator_->head_ptr = memory_pool_;
    allocator_->memory_pool = memory_pool_;
    ret = LINEAR_ALLOC_SUCCESS;

cleanup:
    return ret;
}
```

## アプリケーション側実装変更

以上に変更により、application_createのリニアアロケータ初期化処理が変更になります。
変更後のapplication_create(エラー処理は省略)を貼り付けます。
linear_allocatorのメモリ確保をapplication_create内で行うようになったため、エラー発生時のクリーンナップが少し複雑になっています。

```c
application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    // メモリシステムの初期化
    ret_mem_sys = memory_system_create();

    // app_state_tのメモリ確保
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    memset(tmp, 0, sizeof(*tmp));

    // リニアアロケータのためのメモリ要件取得
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);

    // リニアアロケータメモリプールメモリの確保
    tmp->linear_alloc_pool_size = 1 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);

    // リニアアロケータ初期化
    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp) {
            if(NULL != tmp->linear_alloc_pool) {
                memory_system_free(tmp->linear_alloc_pool, tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
            }
            if(NULL != tmp->linear_alloc) {
                memory_system_free(tmp->linear_alloc, tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
            }
            memory_system_free(tmp, sizeof(*tmp), MEMORY_TAG_SYSTEM);
            tmp = NULL;
        }
        memory_system_destroy();
    }

    return ret;
}
```

次に、application_destroyについてもリニアアロケータのメモリ解放が必要となるため、修正が必要です。
修正後の実装を貼り付けます。

```c
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    if(NULL != s_app_state->linear_alloc_pool) {
        memory_system_free(s_app_state->linear_alloc_pool, s_app_state->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc_pool = NULL;
    }
    if(NULL != s_app_state->linear_alloc) {
        memory_system_free(s_app_state->linear_alloc, s_app_state->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc = NULL;
    }

    memory_system_free(s_app_state, sizeof(*s_app_state), MEMORY_TAG_SYSTEM);
    s_app_state = NULL;
    INFO_MESSAGE("Freed all memory.");
    memory_system_report();
    memory_system_destroy();
    // end cleanup all systems.

    INFO_MESSAGE("Application destroyed successfully.");
cleanup:
    return;
}
```

## その他小変更

***app_err_t列挙体をapplication_result_tに名称変更***

列挙子にAPPLICATION_SUCCESSが含まれ、名前の矛盾があるため名称を変更しました。
同様に、memory_sys_err_tをmemory_system_result_tに、linear_alloc_err_tをlinear_allocator_result_tに変更しました。

***application_result_t列挙体の列挙子SUCCESSは必ず0に固定***

一般的なエラーコードの慣習に習い、実行結果の値は成功を必ず0になるようにします。
memory_system_result_tとlinear_allocator_result_tについても同様の変更を適用しています。


以上で前回からの仕様変更についての解説は終了です。次回からは前回の仕様変更ではなく、新しい機能の追加についての解説を行っていきます。
次回の解説内容はLinux環境のサポートを追加することと、OpenGLの実行環境の整備について解説していきます。
