/** @ingroup choco_memory
 *
 * @file choco_memory.c
 * @author chocolate-pie24
 * @brief 不定期に発生するメモリ確保、解放に対応するメモリアロケータモジュールの実装
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>  // for fprintf
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "engine/core/memory/test_choco_memory.h"

// 外部公開APIテスト設定
static test_call_control_t s_test_config_memory_system_create;       /**< memory_system_create()テスト設定 */
static test_call_control_t s_test_config_memory_system_allocate;     /**< memory_system_allocate()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_test_malloc;                /**< test_malloc()テスト設定値 */

// 全テスト関数プロトタイプ宣言
static void test_memory_system_create(void);
static void test_memory_system_destroy(void);
static void test_memory_system_allocate(void);
static void test_memory_system_free(void);
static void test_memory_system_report(void);
static void test_rslt_to_str(void);
static void test_test_malloc(void);
#endif

/**
 * @brief メモリシステム内部状態管理構造体
 *
 */
typedef struct memory_system {
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
    const char* mem_tag_str[MEMORY_TAG_MAX];    /**< 各メモリタグ文字列 */
} memory_system_t;

static memory_system_t* s_mem_sys_ptr = NULL;   /**< メモリシステム内部状態管理構造体インスタンス */

static const char* const s_rslt_str_success = "SUCCESS";                    /**< メモリシステムAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< メモリシステムAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< メモリシステムAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< メモリシステムAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< メモリシステムAPI実行結果コード(システム使用上限超過)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< メモリシステムAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< メモリシステムAPI実行結果コード(不明なエラー)に対応する文字列 */

static const char* rslt_to_str(memory_system_result_t rslt_);
static void* test_malloc(size_t size_); // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く

memory_system_result_t memory_system_create(void) {
#ifdef TEST_BUILD
    s_test_config_memory_system_create.call_count++;
    if(s_test_config_memory_system_create.fail_on_call != 0) {
        if(s_test_config_memory_system_create.call_count == s_test_config_memory_system_create.fail_on_call) {
            return (memory_system_result_t)s_test_config_memory_system_create.forced_result;
        }
    }
#endif
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* tmp = NULL;

    // Preconditions.
    if(NULL != s_mem_sys_ptr) {
        ret = MEMORY_SYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("memory_system_create(%s) - Memory system is already initialized.", rslt_to_str(ret));
        goto cleanup;
    }

    // Simulation.
    tmp = (memory_system_t*)test_malloc(sizeof(memory_system_t));
    IF_ALLOC_FAIL_GOTO_CLEANUP(tmp, ret, MEMORY_SYSTEM_NO_MEMORY, "memory_system_create", "tmp")
    memset(tmp, 0, sizeof(memory_system_t));

    tmp->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        tmp->mem_tag_allocated[i] = 0;
    }
    tmp->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    tmp->mem_tag_str[MEMORY_TAG_STRING] = "string";
    tmp->mem_tag_str[MEMORY_TAG_RING_QUEUE] = "ring_queue";
    tmp->mem_tag_str[MEMORY_TAG_RENDERER] = "renderer";
    tmp->mem_tag_str[MEMORY_TAG_FILE_IO] = "file_io";
    tmp->mem_tag_str[MEMORY_TAG_CAMERA] = "camera";

    // commit
    s_mem_sys_ptr = tmp;

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    // NOTE: test_malloc以降でエラーの発生は現状ではないためリソース解放コードはなし
    // ただし、将来的に構造体への変数追加等で仕様変更が発生し、リソース解放が必要になった際に即気付けるよう、assertを仕込んでおく
#ifdef TEST_BUILD
    if(MEMORY_SYSTEM_SUCCESS != ret) {
        assert(NULL == tmp);
    }
#endif
    return ret;
}

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

memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
#ifdef TEST_BUILD
    s_test_config_memory_system_allocate.call_count++;
    if(s_test_config_memory_system_allocate.fail_on_call != 0) {
        if(s_test_config_memory_system_allocate.call_count == s_test_config_memory_system_allocate.fail_on_call) {
            return (memory_system_result_t)s_test_config_memory_system_allocate.forced_result;
        }
    }
#endif
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, ret, MEMORY_SYSTEM_BAD_OPERATION, rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION), "memory_system_allocate", "s_mem_sys_ptr")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "*out_ptr_")
    IF_ARG_FALSE_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "mem_tag_")

    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - No-op: size_ is 0.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] > (SIZE_MAX - size_)) {
        ret = MEMORY_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("memory_system_allocate(%s) - size_t overflow: tag=%s used=%zu, requested=%zu, sum would exceed SIZE_MAX.", rslt_to_str(ret), s_mem_sys_ptr->mem_tag_str[mem_tag_], s_mem_sys_ptr->mem_tag_allocated[mem_tag_], size_);
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated > (SIZE_MAX - size_)) {
        ret = MEMORY_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("memory_system_allocate(%s) - size_t overflow: total_allocated=%zu, requested=%zu, sum would exceed SIZE_MAX.", rslt_to_str(ret), s_mem_sys_ptr->total_allocated, size_);
        goto cleanup;
    }

    // Simulation.
    tmp = test_malloc(size_);    // TODO: FreeList
    IF_ALLOC_FAIL_GOTO_CLEANUP(tmp, ret, MEMORY_SYSTEM_NO_MEMORY, "memory_system_allocate", "tmp")
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    s_mem_sys_ptr->total_allocated += size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] += size_;

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

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

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return s_rslt_str_success;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case MEMORY_SYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief mallocのラッパ関数で、size_のメモリを確保する
 *
 * @note choco_memory保有APIの単体テストのため、test_config_test_mallocの設定により、強制的にNULLを返させる、以下の条件でNULLになる
 * - s_test_config_test_malloc.fail_on_call > 0 && s_test_config_test_malloc.call_count == s_test_config_test_malloc.fail_on_call
 *
 * @param[in] size_ 確保するメモリ容量
 *
 * @return void* 確保されたメモリの先頭アドレス
 */
static void* test_malloc(size_t size_) {
    void* ret = NULL;
#ifdef TEST_BUILD
    s_test_config_test_malloc.call_count++;
    if(0 == s_test_config_test_malloc.fail_on_call) {
        ret = malloc(size_);
    } else {
        if(s_test_config_test_malloc.call_count == s_test_config_test_malloc.fail_on_call) {
            ret = NULL;
        } else {
            ret = malloc(size_);
        }
    }
#else
    ret = malloc(size_);
#endif
    return ret;
}

#ifdef TEST_BUILD
void test_memory_system_create_config_set(const test_call_control_t* config_) {
    s_test_config_memory_system_create.fail_on_call = config_->fail_on_call;
    s_test_config_memory_system_create.forced_result = config_->forced_result;
}

void test_memory_system_allocate_config_set(const test_call_control_t* config_) {
    s_test_config_memory_system_allocate.fail_on_call = config_->fail_on_call;
    s_test_config_memory_system_allocate.forced_result = config_->forced_result;
}

void test_choco_memory_config_reset(void) {
    test_call_control_reset(&s_test_config_memory_system_create);
    test_call_control_reset(&s_test_config_memory_system_allocate);
    test_call_control_reset(&s_test_config_test_malloc);
}

void test_choco_memory(void) {
    test_choco_memory_config_reset();

    test_rslt_to_str();
    test_test_malloc();
    test_memory_system_create();
    test_memory_system_destroy();
    test_memory_system_allocate();
    test_memory_system_free();
    test_memory_system_report();

    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_memory_system_create(void) {
    {
        // memory_system_create() 冒頭で強制的に MEMORY_SYSTEM_NO_MEMORY を返させる
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_create_config_set(&config);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_NO_MEMORY == ret);
        assert(NULL == s_mem_sys_ptr);

        test_choco_memory_config_reset();
    }
    {
        // NULL != s_mem_sys_ptr -> MEMORY_SYSTEM_RUNTIME_ERROR
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        s_mem_sys_ptr = (memory_system_t*)malloc(sizeof(memory_system_t));
        assert(NULL != s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_RUNTIME_ERROR == ret);
        assert(NULL != s_mem_sys_ptr);

        free(s_mem_sys_ptr);
        s_mem_sys_ptr = NULL;
    }
    {
        // 内部 test_malloc() の1回目を失敗させる -> MEMORY_SYSTEM_NO_MEMORY
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        s_test_config_test_malloc.fail_on_call = 1U;

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_NO_MEMORY == ret);
        assert(NULL == s_mem_sys_ptr);
        assert(1U == s_test_config_test_malloc.call_count);

        test_choco_memory_config_reset();
    }
    {
        // 正常系
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);

        assert(0U == s_mem_sys_ptr->total_allocated);
        for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
        }

        assert(0 == strcmp("system", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_SYSTEM]));
        assert(0 == strcmp("string", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_STRING]));
        assert(0 == strcmp("ring_queue", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_RING_QUEUE]));
        assert(0 == strcmp("renderer", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_RENDERER]));
        assert(0 == strcmp("file_io", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_FILE_IO]));
        assert(0 == strcmp("camera", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_CAMERA]));

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_memory_system_destroy(void) {
    {
        // s_mem_sys_ptr == NULL -> no-op
        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        memory_system_destroy();

        assert(NULL == s_mem_sys_ptr);
    }
    {
        // 正常系
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);

        // 連続 destroy でも no-op
        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
    {
        // total_allocated != 0 -> warning を出しつつ正常破棄
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);

        s_mem_sys_ptr->total_allocated = 128U;
        s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = 32U;
        s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM] = 96U;

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_memory_system_allocate(void) {
    {
        // s_mem_sys_ptr == NULL -> MEMORY_SYSTEM_BAD_OPERATION
        memory_system_result_t ret = MEMORY_SYSTEM_BAD_OPERATION;
        void* ptr = NULL;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr);
        assert(MEMORY_SYSTEM_BAD_OPERATION == ret);
        assert(NULL == ptr);
        assert(NULL == s_mem_sys_ptr);
    }
    {
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);
        assert(0U == s_mem_sys_ptr->total_allocated);
        for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
        }

        {
            // out_ptr_ == NULL -> MEMORY_SYSTEM_INVALID_ARGUMENT
            ret = memory_system_allocate(128U, MEMORY_TAG_STRING, NULL);
            assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
            assert(0U == s_mem_sys_ptr->total_allocated);
            for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
                assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
            }
        }
        {
            // *out_ptr_ != NULL -> MEMORY_SYSTEM_INVALID_ARGUMENT
            void* ptr = malloc(8U);
            assert(NULL != ptr);

            ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
            assert(NULL != ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
                assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
            }

            free(ptr);
            ptr = NULL;
        }
        {
            // mem_tag_ >= MEMORY_TAG_MAX -> MEMORY_SYSTEM_INVALID_ARGUMENT
            void* ptr = NULL;

            ret = memory_system_allocate(128U, MEMORY_TAG_MAX, &ptr);
            assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
            assert(NULL == ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
                assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
            }
        }
        {
            // size_ == 0 -> warning + MEMORY_SYSTEM_SUCCESS
            void* ptr = NULL;

            ret = memory_system_allocate(0U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_SUCCESS == ret);
            assert(NULL == ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
                assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
            }
        }
        {
            // mem_tag_allocated[mem_tag_] の加算で SIZE_MAX を超える -> MEMORY_SYSTEM_LIMIT_EXCEEDED
            void* ptr = NULL;

            s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = SIZE_MAX - 100U;

            ret = memory_system_allocate(101U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_LIMIT_EXCEEDED == ret);
            assert(NULL == ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
            assert((SIZE_MAX - 100U) == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);

            s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = 0U;
        }
        {
            // total_allocated の加算で SIZE_MAX を超える -> MEMORY_SYSTEM_LIMIT_EXCEEDED
            void* ptr = NULL;

            s_mem_sys_ptr->total_allocated = SIZE_MAX - 100U;

            ret = memory_system_allocate(101U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_LIMIT_EXCEEDED == ret);
            assert(NULL == ptr);
            assert((SIZE_MAX - 100U) == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);

            s_mem_sys_ptr->total_allocated = 0U;
        }
        {
            // memory_system_allocate() 冒頭で強制的に MEMORY_SYSTEM_NO_MEMORY を返させる
            memory_system_result_t ret_local = MEMORY_SYSTEM_INVALID_ARGUMENT;
            test_call_control_t config = {0};
            void* ptr = NULL;

            test_choco_memory_config_reset();

            config.fail_on_call = 1U;
            config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
            test_memory_system_allocate_config_set(&config);

            ret_local = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_NO_MEMORY == ret_local);
            assert(NULL == ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

            // 冒頭で return しているので、内部 test_malloc() には到達しない
            assert(0U == s_test_config_test_malloc.call_count);

            test_choco_memory_config_reset();
        }
        {
            // 内部 test_malloc() の1回目を失敗させる -> MEMORY_SYSTEM_NO_MEMORY
            void* ptr = NULL;

            test_choco_memory_config_reset();
            s_test_config_test_malloc.fail_on_call = 1U;

            ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_NO_MEMORY == ret);
            assert(NULL == ptr);
            assert(0U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
            assert(1U == s_test_config_test_malloc.call_count);

            test_choco_memory_config_reset();
        }
        {
            // 正常系
            void* ptr = NULL;

            test_choco_memory_config_reset();

            ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr);
            assert(MEMORY_SYSTEM_SUCCESS == ret);
            assert(NULL != ptr);
            assert(128U == s_mem_sys_ptr->total_allocated);
            assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

            memory_system_free(ptr, 128U, MEMORY_TAG_STRING);
            ptr = NULL;

            assert(0U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_memory_system_free(void) {
    {
        // s_mem_sys_ptr == NULL -> no-op
        void* ptr = malloc(8U);
        assert(NULL != ptr);

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        memory_system_free(ptr, 8U, MEMORY_TAG_STRING);

        free(ptr);
        ptr = NULL;
    }
    {
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        void* ptr_string = NULL;
        void* ptr_system = NULL;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);

        ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr_string);
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != ptr_string);
        assert(128U == s_mem_sys_ptr->total_allocated);
        assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
        assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

        ret = memory_system_allocate(256U, MEMORY_TAG_SYSTEM, &ptr_system);
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != ptr_system);
        assert((128U + 256U) == s_mem_sys_ptr->total_allocated);
        assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
        assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

        {
            // ptr_ == NULL -> no-op
            memory_system_free(NULL, 128U, MEMORY_TAG_STRING);
            assert((128U + 256U) == s_mem_sys_ptr->total_allocated);
            assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }
        {
            // mem_tag_ >= MEMORY_TAG_MAX -> no-op
            memory_system_free(ptr_string, 128U, MEMORY_TAG_MAX);
            assert((128U + 256U) == s_mem_sys_ptr->total_allocated);
            assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }
        {
            // mem_tag_allocated[mem_tag_] < size_ -> no-op
            memory_system_free(ptr_string, 1024U, MEMORY_TAG_STRING);
            assert((128U + 256U) == s_mem_sys_ptr->total_allocated);
            assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }
        {
            // total_allocated < size_ -> no-op
            s_mem_sys_ptr->total_allocated = 64U;

            memory_system_free(ptr_string, 128U, MEMORY_TAG_STRING);
            assert(64U == s_mem_sys_ptr->total_allocated);
            assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

            s_mem_sys_ptr->total_allocated = 128U + 256U;
        }
        {
            // 正常系: string を free
            memory_system_free(ptr_string, 128U, MEMORY_TAG_STRING);
            ptr_string = NULL;

            assert(256U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }
        {
            // 正常系: system を free
            memory_system_free(ptr_system, 256U, MEMORY_TAG_SYSTEM);
            ptr_system = NULL;

            assert(0U == s_mem_sys_ptr->total_allocated);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
        }

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_memory_system_report(void) {
    {
        // s_mem_sys_ptr == NULL -> no-op
        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        memory_system_report();

        assert(NULL == s_mem_sys_ptr);
    }
    {
        memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        void* ptr_string = NULL;
        void* ptr_system = NULL;

        test_choco_memory_config_reset();
        assert(NULL == s_mem_sys_ptr);

        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != s_mem_sys_ptr);

        // all 0 の状態で呼べる
        memory_system_report();
        assert(0U == s_mem_sys_ptr->total_allocated);
        for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
        }

        // string だけ確保した状態で呼べる
        ret = memory_system_allocate(128U, MEMORY_TAG_STRING, &ptr_string);
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != ptr_string);
        assert(128U == s_mem_sys_ptr->total_allocated);
        assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
        assert(0U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

        memory_system_report();

        // string + system の両方を確保した状態で呼べる
        ret = memory_system_allocate(256U, MEMORY_TAG_SYSTEM, &ptr_system);
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(NULL != ptr_system);
        assert((128U + 256U) == s_mem_sys_ptr->total_allocated);
        assert(128U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
        assert(256U == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

        memory_system_report();

        // タグ文字列が NULL でも report が通る
        s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_STRING] = NULL;
        memory_system_report();

        memory_system_free(ptr_system, 256U, MEMORY_TAG_SYSTEM);
        ptr_system = NULL;
        memory_system_free(ptr_string, 128U, MEMORY_TAG_STRING);
        ptr_string = NULL;

        assert(0U == s_mem_sys_ptr->total_allocated);
        for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
            assert(0U == s_mem_sys_ptr->mem_tag_allocated[i]);
        }

        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_SUCCESS);
        assert(NULL != str);
        assert(0 == strcmp("SUCCESS", str));
    }
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT);
        assert(NULL != str);
        assert(0 == strcmp("INVALID_ARGUMENT", str));
    }
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_RUNTIME_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("RUNTIME_ERROR", str));
    }
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_LIMIT_EXCEEDED);
        assert(NULL != str);
        assert(0 == strcmp("LIMIT_EXCEEDED", str));
    }
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION);
        assert(NULL != str);
        assert(0 == strcmp("BAD_OPERATION", str));
    }
    {
        const char* str = rslt_to_str(MEMORY_SYSTEM_NO_MEMORY);
        assert(NULL != str);
        assert(0 == strcmp("NO_MEMORY", str));
    }
    {
        const char* str = rslt_to_str((memory_system_result_t)100);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_test_malloc(void) {
    {
        // fail_on_call == 0 -> 常に通常の malloc 動作
        void* tmp = NULL;

        test_choco_memory_config_reset();
        assert(0U == s_test_config_test_malloc.call_count);
        assert(0U == s_test_config_test_malloc.fail_on_call);
        assert(0 == s_test_config_test_malloc.forced_result);

        tmp = test_malloc(128U);
        assert(NULL != tmp);
        assert(1U == s_test_config_test_malloc.call_count);

        free(tmp);
        tmp = NULL;
    }
    {
        // 2回目で失敗
        void* tmp = NULL;

        test_choco_memory_config_reset();
        s_test_config_test_malloc.fail_on_call = 2U;

        tmp = test_malloc(128U);
        assert(NULL != tmp);
        assert(1U == s_test_config_test_malloc.call_count);

        free(tmp);
        tmp = NULL;

        tmp = test_malloc(128U);
        assert(NULL == tmp);
        assert(2U == s_test_config_test_malloc.call_count);
    }
    {
        // 1回目で失敗
        void* tmp = NULL;

        test_choco_memory_config_reset();
        s_test_config_test_malloc.fail_on_call = 1U;

        tmp = test_malloc(128U);
        assert(NULL == tmp);
        assert(1U == s_test_config_test_malloc.call_count);
    }

    test_choco_memory_config_reset();
}
#endif
