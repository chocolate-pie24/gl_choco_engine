---
title: "Step2_1: メモリシステムの仕様変更"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook2に対応しています。

## このステップでやること

前回作成したメモリシステムのAPI仕様は、

```c
// メモリ確保
memory_sys_err_t memory_system_allocate(memory_system_t* memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

// メモリ開放
void memory_system_free(memory_system_t* memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_);
```

となっていました。この仕様は動作上は全く問題ないのですが、使用側はmemory_system_tのインスタンスを保有しておく必要があります。

このため、現状の仕様では、アプリケーションレイヤーより下層でメモリシステムを使用したメモリ確保を行うために、
アプリケーションレイヤーからmemory_system_tのインスタンスを渡す必要があります。
使用するレイヤーごとにmemory_system_tのインスタンスを保有する手もありますが、それでは全体のメモリ使用量のトラッキングがやりづらくなってしまいます。

そこで、仕様変更として、memory_system_tのインスタンスをシングルトンのオブジェクトとしてcore/memory/choco_memory.cの中で静的に保持する構造に変更していきます。
また、前回は、リニアアロケータを作成し、リニアアロケータからメモリシステム用のメモリを確保する方式を取りましたが、
今回はメモリシステムのメモリは自身で確保、破棄するように仕様を変更していきます。こうすることで、以前は、

1. アプリケーション内部状態管理オブジェクトのメモリをmallocで確保
2. リニアアロケータのメモリをmallocで確保
3. リニアアロケータを使用してメモリシステムを初期化

という手順だったのが、

1. メモリシステムを初期化(メモリはmallocで確保)
2. メモリシステムを使用してアプリケーション内部状態管理オブジェクトのメモリを確保
3. メモリシステムを使用してリニアアロケータのメモリを確保

というように、全てのメモリをメモリシステムを経由して確保することができるようになり、スッキリします。

## 初期化処理の更新

前回の仕様では、メモリシステムの内部状態情報をアプリケーション側で保有するようにしていましたが、core/memory/choco_memory.cで保有するように変更します。

src/core/memory/choco_memory.c

```c
typedef struct memory_system {
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
    const char* mem_tag_str[MEMORY_TAG_MAX];    /**< 各メモリタグ文字列 */
} memory_system_t;

// choco_memory.cで内部状態を保有するように変更
static memory_system_t* s_mem_sys_ptr = NULL;   /**< メモリシステム内部状態管理オブジェクトインスタンス */
```

次に、s_mem_sys_ptrの初期化関数を作っていきます(エラー処理は省略してあります)。

src/core/memory/choco_memory.c

```c
memory_system_result_t memory_system_create(void) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* tmp = NULL;

    tmp = (memory_system_t*)malloc(sizeof(memory_system_t));
    memset(tmp, 0, sizeof(memory_system_t));

    tmp->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        tmp->mem_tag_allocated[i] = 0;
    }
    tmp->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    tmp->mem_tag_str[MEMORY_TAG_STRING] = "string";

    s_mem_sys_ptr = tmp;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```

ちなみに、前回の初期化関数はこのようになっています。

```c
memory_sys_err_t memory_system_init(memory_system_t* memory_system_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    memory_system_->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        memory_system_->mem_tag_allocated[i] = 0;
    }
    memory_system_->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    memory_system_->mem_tag_str[MEMORY_TAG_STRING] = "string";

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```

s_mem_sys_ptr用のメモリ確保処理が追加になっている以外は処理内容は同じです。
メモリ確保までを自身で行うため、関数名をmemory_system_initからmemory_system_createに変更しています。

この変更により、前回の2段階初期化は不要となりますので、

- memory_system_init
- memory_system_preinit

の2つがmemory_system_createに置き換わります。

## メモリシステム終了処理の更新

前回の仕様では、終了処理はメモリタグごとのメモリ割当量を0に戻すのみでしたが、
今回はmemory_system自身で自身のメモリを解放する必要が出てきます。このように変更します。

src/core/memory/choco_memory.c

```c
void memory_system_destroy(void) {
    if(NULL == s_mem_sys_ptr) {
        goto cleanup;
    }
    if(0 != s_mem_sys_ptr->total_allocated) {
        WARN_MESSAGE("memory_system_destroy - total_allocated != 0. Check memory leaks.");
    }
    s_mem_sys_ptr->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        s_mem_sys_ptr->mem_tag_allocated[i] = 0;
    }
    free(s_mem_sys_ptr);
    s_mem_sys_ptr = NULL;

cleanup:
    return;
}
```

前回のコードはこのようになっています。s_mem_sys_ptrのメモリ解放が追加されたことが主な違いです。
また、メモリシステム終了時に全てのメモリが解放されていない状態だった場合に備えて、ワーニングメッセージの出力を追加しました。

```c
void memory_system_destroy(memory_system_t* memory_system_) {
    if(NULL == memory_system_) {
        goto cleanup;
    }
    memory_system_->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        memory_system_->mem_tag_allocated[i] = 0;
    }

cleanup:
    return;
}
```

## メモリ確保処理の更新

次はメモリ確保処理です。こちらは以前のメモリ割当量更新処理を、引数のmemory_system_経由からs_mem_sys_ptr経由に変更するのみです。

***新コード***

```c
memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Simulation.
    tmp = malloc(size_);
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    s_mem_sys_ptr->total_allocated += size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] += size_;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```

***旧コード***

```c
memory_sys_err_t memory_system_allocate(memory_system_t* memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Simulation.
    tmp = malloc(size_);
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    memory_system_->total_allocated += size_;
    memory_system_->mem_tag_allocated[mem_tag_] += size_;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```

## メモリ解放処理の更新

次はメモリ解放処理です。こちらも、割当処理同様、以前のメモリ割当量更新処理を、引数のmemory_system_経由からs_mem_sys_ptr経由に変更するのみです。

***新コード***

```c
void memory_system_free(void* ptr_, size_t size_, memory_tag_t mem_tag_) {
    if(NULL == s_mem_sys_ptr) {
        WARN_MESSAGE("memory_system_free - No-op: memory system is uninitialized.");
        goto cleanup;
    }
    if(NULL == ptr_) {
        WARN_MESSAGE("memory_system_free - No-op: 'ptr_' must not be NULL.");
        goto cleanup;
    }
    if(mem_tag_ >= MEMORY_TAG_MAX) {
        WARN_MESSAGE("memory_system_free - No-op: 'mem_tag_' is invalid.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] < size_) {
        WARN_MESSAGE("memory_system_free - No-op: 'mem_tag_allocated' would underflow.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated < size_) {
        WARN_MESSAGE("memory_system_free: No-op: 'total_allocated' would underflow.");
        goto cleanup;
    }

    free(ptr_);
    s_mem_sys_ptr->total_allocated -= size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] -= size_;
cleanup:
    return;
}
```

***旧コード***

```c
void memory_system_free(memory_system_t* memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_) {
    if(NULL == memory_system_) {
        WARN_MESSAGE("memory_system_free - No-op: memory_system_ is NULL.");
        goto cleanup;
    }
    if(NULL == ptr_) {
        WARN_MESSAGE("memory_system_free - No-op: ptr_ is NULL.");
        goto cleanup;
    }
    if(mem_tag_ >= MEMORY_TAG_MAX) {
        WARN_MESSAGE("memory_system_free - No-op: invalid mem_tag_.");
        goto cleanup;
    }
    if(memory_system_->mem_tag_allocated[mem_tag_] < size_) {
        WARN_MESSAGE("memory_system_free - No-op: mem_tag_allocated broken.");
        goto cleanup;
    }
    if(memory_system_->total_allocated < size_) {
        WARN_MESSAGE("memory_system_free: No-op: total_allocated broken.");
        goto cleanup;
    }

    free(ptr_);
    memory_system_->total_allocated -= size_;
    memory_system_->mem_tag_allocated[mem_tag_] -= size_;
cleanup:
    return;
}
```

### メモリ割当量レポート処理の更新

次はメモリ解放処理です。こちらも、割当処理同様、以前のメモリ割当量更新処理を、引数のmemory_system_経由からs_mem_sys_ptr経由に変更するのみです。

***新コード***

```c
void memory_system_report(void) {
    if(NULL == s_mem_sys_ptr) {
        WARN_MESSAGE("memory_system_report - No-op: s_mem_sys_ptr is NULL.");
        goto cleanup;
    }
    INFO_MESSAGE("memory_system_report");
    // TODO: [INFORMATION]を出力しないINFO_MESSAGE_RAW(...)をbase/messageに追加し、fprintfを廃止する
    fprintf(stdout, "\033[1;35m\tTotal allocated: %zu\n", s_mem_sys_ptr->total_allocated);
    fprintf(stdout, "\tMemory tag allocated:\n");
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        const char* const tag_str = s_mem_sys_ptr->mem_tag_str[i];
        fprintf(stdout, "\t\ttag(%s): %zu\n", (NULL != tag_str) ? tag_str : "unknown", s_mem_sys_ptr->mem_tag_allocated[i]);
    }
    fprintf(stdout, "\033[0m\n");
cleanup:
    return;
}
```

***旧コード***

```c
void memory_system_report(const memory_system_t* memory_system_) {
    if(NULL == memory_system_) {
        WARN_MESSAGE("memory_system_report - No-op: memory_system_ is NULL.");
        goto cleanup;
    }
    INFO_MESSAGE("memory_system_report");
    // TODO: [INFORMATION]を出力しないINFO_MESSAGE_RAW(...)をbase/messageに追加し、fprintfを廃止する
    fprintf(stdout, "\033[1;35m\tTotal allocated: %zu\n", memory_system_->total_allocated);
    fprintf(stdout, "\tMemory tag allocated:\n");
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        const char* const tag_str = memory_system_->mem_tag_str[i];
        fprintf(stdout, "\t\ttag(%s): %zu\n", (NULL != tag_str) ? tag_str : "unknown", memory_system_->mem_tag_allocated[i]);
    }
    fprintf(stdout, "\033[0m\n");
cleanup:
    return;
}
```

## choco_memory.hの更新

以上で仕様変更に必要なchoco_memory.cの変更が完了しました。新しい仕様でのchoco_memory.hの内容を貼り付けます。

```c
#ifndef GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H
#define GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    MEMORY_TAG_SYSTEM,      /**< メモリタグ: システム系 */
    MEMORY_TAG_STRING,      /**< メモリタグ: 文字列系 */
    MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} memory_tag_t;

typedef enum {
    MEMORY_SYSTEM_SUCCESS = 0,      /**< メモリシステム成功 */
    MEMORY_SYSTEM_INVALID_ARGUMENT, /**< 無効な引数 */
    MEMORY_SYSTEM_RUNTIME_ERROR,    /**< 実行時エラー */
    MEMORY_SYSTEM_NO_MEMORY,        /**< メモリ不足 */
} memory_system_result_t;

memory_system_result_t memory_system_create(void);

void memory_system_destroy(void);

memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

void memory_system_free(void* ptr_, size_t size_, memory_tag_t mem_tag_);

void memory_system_report(void);

#ifdef __cplusplus
}
#endif
#endif
```

## application.cの更新

これでメモリシステムの更新が完了しましたので、application.c側も修正していきます。
まず、アプリケーション内部状態管理オブジェクトのメンバからメモリシステム関連が不要になりますので、このようになります。

src/application/application.c

```c
typedef struct app_state {
    // core/memory/linear_allocator
    linear_alloc_t* linear_allocator;   /**< リニアアロケータオブジェクト */
} app_state_t;
```

次に、application_create内でmemory_system_createを最初に呼ぶように変更します。

src/application/application.c

```c
application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(RUNTIME_ERROR) - Application state is already initialized.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
```

これでメモリシステムの構築ができました。ここで、memory_system_createのエラー処理についてなのですが、
返り値がmemory_system_result_t型となっています。これをentryポイントまで伝播させるため、エラーコードをapplication_result_t型に変換する必要があります。

エラーコード変換用のヘルパー関数を作成します。

src/application/application.c

```c
static application_result_t rslt_convert_mem_sys(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
```

また、エラーメッセージ内のエラー種別表示を統一感を持って行えるようにエラーコード文字列変換のヘルパー関数も用意します。

```c
static const char* const s_rslt_str_success = "SUCCESS";                    /**< アプリケーション実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< アプリケーション実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< アプリケーション実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< アプリケーション実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< アプリケーション実行結果コード(未定義エラー)に対応する文字列 */

static const char* rslt_to_str(application_result_t rslt_) {
    switch(rslt_) {
    case APPLICATION_SUCCESS:
        return s_rslt_str_success;
    case APPLICATION_NO_MEMORY:
        return s_rslt_str_no_memory;
    case APPLICATION_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case APPLICATION_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case APPLICATION_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}
```

これで、先程のapplication_createはこのようになります。

```c
application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", s_rslt_str_runtime_error);
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create memory system.", rslt_to_str(ret));
        goto cleanup;
    }
```

以上でメモリシステムを使用してメモリ確保が行えるようになったので、アプリケーション内部状態管理オブジェクトのメモリをメモリシステムを介して行うように修正します。

```c
application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", s_rslt_str_runtime_error);
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create memory system.", rslt_to_str(ret));
        goto cleanup;
    }

    // アプリケーション内部状態管理オブジェクトのメモリ確保
    // tmpのメモリ確保を行い、成功すればs_app_stateへアドレスを差し替え
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for application state.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    /////////////////////////////////////
    // 途中省略
    /////////////////////////////////////

    s_app_state = tmp;
    INFO_MESSAGE("Application created successfully.");
    memory_system_report();
    ret = APPLICATION_SUCCESS;
```

リニアロケータのメモリ確保については、次回、リニアアロケータの仕様変更を行った際に追加していくことにします。
最後にメモリシステムの破棄処理です。

```c
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.

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

以上がメモリシステムの仕様変更の説明になります。次回はリニアアロケータの仕様変更について説明していきます。
