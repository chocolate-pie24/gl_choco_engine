/** @ingroup linear_allocator
 *
 * @file linear_allocator.c
 * @author chocolate-pie24
 * @brief サブシステム等、ライフサイクルが固定で、個別のメモリ開放が不要なメモリ確保に対応するリニアアロケータモジュールの実装
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include "test_controller.h"
#include "core/test_linear_allocator.h"

static test_config_linear_allocator_t s_test_config;    /**< Linear Allocatorモジュールの外部公開APIテスト設定値 */

static void test_linear_allocator_preinit(void);
static void test_linear_allocator_init(void);
static void test_linear_allocator_allocate(void);
static void test_rslt_to_str(void);
#endif

/**
 * @brief linear_alloc_t内部データ構造
 * @todo 必要であればメモリトラッキング追加(現状では入れる予定はなし)
 */
struct linear_alloc {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス(実際にはアライメント要件分オフセットされたアドレスを渡す) */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
};

static const char* const s_rslt_str_success = "SUCCESS";                     /**< 実行結果種別文字列(処理成功) */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                 /**< 実行結果種別文字列(メモリ確保失敗) */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果種別文字列(無効な引数) */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果種別文字列(不明なエラー) */

static const char* rslt_to_str(linear_allocator_result_t rslt_);

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        return;
    }
    *memory_requirement_ = sizeof(linear_alloc_t);
    *align_requirement_ = alignof(linear_alloc_t);
}

linear_allocator_result_t linear_allocator_init(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_) {
#ifdef TEST_BUILD
    s_test_config.test_linear_allocator_init.call_count++;
    if(s_test_config.test_linear_allocator_init.fail_on_call != 0) {
        if(s_test_config.test_linear_allocator_init.call_count == s_test_config.test_linear_allocator_init.fail_on_call) {
            return (linear_allocator_result_t)s_test_config.test_linear_allocator_init.forced_result;
        }
    }
#endif

    linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_init", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_init", "memory_pool_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != capacity_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_init", "capacity_")

    allocator_->capacity = capacity_;
    allocator_->head_ptr = memory_pool_;
    allocator_->memory_pool = memory_pool_;
    ret = LINEAR_ALLOC_SUCCESS;

cleanup:
    return ret;
}

linear_allocator_result_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_) {
#ifdef TEST_BUILD
    s_test_config.test_linear_allocator_allocate.call_count++;
    if(s_test_config.test_linear_allocator_allocate.fail_on_call != 0) {
        if(s_test_config.test_linear_allocator_allocate.call_count == s_test_config.test_linear_allocator_allocate.fail_on_call) {
            return (linear_allocator_result_t)s_test_config.test_linear_allocator_allocate.forced_result;
        }
    }
#endif

    linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    uintptr_t head = 0;
    uintptr_t align = 0;
    uintptr_t size = 0;
    uintptr_t offset = 0;
    uintptr_t start_addr = 0;
    uintptr_t pool = 0;
    uintptr_t cap = 0;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    if(0 == req_align_ || 0 == req_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: req_align_ or req_size_ is 0.");
        ret = LINEAR_ALLOC_SUCCESS;
        goto cleanup;
    }
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(req_align_), ret, LINEAR_ALLOC_INVALID_ARGUMENT, rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT), "linear_allocator_allocate", "req_align_")

    // Simulation
    head = (uintptr_t)allocator_->head_ptr;
    align = (uintptr_t)req_align_;
    size = (uintptr_t)req_size_;
    offset = head % align;
    if(0 != offset) {
        offset = align - offset;    // 要求アライメントに先頭アドレスを調整
    }
    if(UINTPTR_MAX - offset < head) {
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested alignment offset is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    start_addr = head + offset;
    if(UINTPTR_MAX - size < start_addr) {
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested size is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    pool = (uintptr_t)allocator_->memory_pool;
    cap = (uintptr_t)allocator_->capacity;
    if((start_addr + size) > (pool + cap)) {
        uintptr_t free_space = pool + cap - start_addr;
        ret = LINEAR_ALLOC_NO_MEMORY;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Cannot allocate requested size. Requested size: %zu / Free space: %zu", rslt_to_str(ret), req_size_, (size_t)free_space);
        goto cleanup;
    }

    // commit
    *out_ptr_ = (void*)start_addr;
    head += offset + size;
    allocator_->head_ptr = (void*)head;
    ret = LINEAR_ALLOC_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return s_rslt_str_success;
    case LINEAR_ALLOC_NO_MEMORY:
        return s_rslt_str_no_memory;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    default:
        return s_rslt_str_undefined_error;
    }
}

#ifdef TEST_BUILD
void test_linear_allocator_config_set(const test_config_linear_allocator_t* config_) {
    test_call_control_set(config_->test_linear_allocator_init.fail_on_call, config_->test_linear_allocator_init.forced_result, &s_test_config.test_linear_allocator_init);
    test_call_control_set(config_->test_linear_allocator_allocate.fail_on_call, config_->test_linear_allocator_allocate.forced_result, &s_test_config.test_linear_allocator_allocate);
}

void test_linear_allocator_config_reset(void) {
    test_call_control_reset(&s_test_config.test_linear_allocator_init);
    test_call_control_reset(&s_test_config.test_linear_allocator_allocate);
}

void NO_COVERAGE test_linear_allocator(void) {
    test_linear_allocator_config_reset();

    test_linear_allocator_preinit();
    test_linear_allocator_init();
    test_linear_allocator_allocate();
    test_rslt_to_str();

    test_linear_allocator_config_reset();
}

static void NO_COVERAGE test_linear_allocator_preinit(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // align_requirement_ == NULL の場合は何もしない
        size_t mem = 1234U;
        linear_allocator_preinit(&mem, NULL);
        assert(1234U == mem);
    }
    {
        // memory_requirement_ == NULL の場合は何もしない
        size_t align = 5678U;
        linear_allocator_preinit(NULL, &align);
        assert(5678U == align);
    }
    {
        // 両方 NULL の場合は何もしない
        linear_allocator_preinit(NULL, NULL);
    }
    {
        // 正常系
        size_t mem = 0U;
        size_t align = 0U;

        linear_allocator_preinit(&mem, &align);

        assert(sizeof(linear_alloc_t) == mem);
        assert(alignof(linear_alloc_t) == align);
    }
    {
        // 複数回呼んでも同じ値が返る
        size_t mem1 = 0U;
        size_t align1 = 0U;
        size_t mem2 = 9999U;
        size_t align2 = 9999U;

        linear_allocator_preinit(&mem1, &align1);
        linear_allocator_preinit(&mem2, &align2);

        assert(mem1 == mem2);
        assert(align1 == align2);
        assert(sizeof(linear_alloc_t) == mem2);
        assert(alignof(linear_alloc_t) == align2);
    }
}

static void NO_COVERAGE test_linear_allocator_init(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        // テスト基盤による強制失敗
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        test_config_linear_allocator_t config = {0};

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128);
        assert(NULL != pool);
        memset(pool, 0, 128);

        config.test_linear_allocator_init.fail_on_call = 1;
        config.test_linear_allocator_init.forced_result = LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_config_set(&config);

        ret = linear_allocator_init(alloc, 128, pool);
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(0U == alloc->capacity);
        assert(NULL == alloc->head_ptr);
        assert(NULL == alloc->memory_pool);

        test_linear_allocator_config_reset();

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 引数allocator_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        void* pool = malloc(128);
        assert(NULL != pool);
        memset(pool, 0, 128);

        ret = linear_allocator_init(NULL, 128, pool);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        free(pool);
        pool = NULL;
    }
    {
        // 引数memory_pool_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        ret = linear_allocator_init(alloc, 128, NULL);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(0U == alloc->capacity);
        assert(NULL == alloc->head_ptr);
        assert(NULL == alloc->memory_pool);

        free(alloc);
        alloc = NULL;
    }
    {
        // 引数capacity_ == 0 -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128);
        assert(NULL != pool);
        memset(pool, 0, 128);

        ret = linear_allocator_init(alloc, 0, pool);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(0U == alloc->capacity);
        assert(NULL == alloc->head_ptr);
        assert(NULL == alloc->memory_pool);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 正常系
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        const size_t cap = 128U;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(cap);
        assert(NULL != pool);
        memset(pool, 0, cap);

        ret = linear_allocator_init(alloc, cap, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(cap == alloc->capacity);
        assert(pool == alloc->head_ptr);
        assert(pool == alloc->memory_pool);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
}

static void NO_COVERAGE test_linear_allocator_allocate(void) {
    // Generated by ChatGPT 5.4 Thinking
    test_linear_allocator_config_reset();

    {
        // テスト基盤による強制失敗（前提条件チェックより前に失敗させる）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        test_config_linear_allocator_t config = {0};

        void* out_ptr = NULL;

        config.test_linear_allocator_allocate.fail_on_call = 1;
        config.test_linear_allocator_allocate.forced_result = LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_config_set(&config);

        ret = linear_allocator_allocate(NULL, 128U, 8U, &out_ptr);
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr);

        test_linear_allocator_config_reset();
    }
    {
        // 引数allocator_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        void* out_ptr = NULL;

        ret = linear_allocator_allocate(NULL, 128U, 8U, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(NULL == out_ptr);
    }
    {
        // 引数out_ptr_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128U);
        assert(NULL != pool);
        memset(pool, 0, 128U);

        ret = linear_allocator_init(alloc, 128U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 16U, 8U, NULL);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(pool == alloc->head_ptr);
        assert(pool == alloc->memory_pool);
        assert(128U == alloc->capacity);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 引数*out_ptr_ != NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128U);
        assert(NULL != pool);
        memset(pool, 0, 128U);

        void* out_ptr = malloc(16U);
        assert(NULL != out_ptr);

        ret = linear_allocator_init(alloc, 128U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 16U, 8U, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(NULL != out_ptr);
        assert(pool == alloc->head_ptr);

        free(out_ptr);
        out_ptr = NULL;

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // req_size_ == 0 -> SUCCESS（no-op）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128U);
        assert(NULL != pool);
        memset(pool, 0, 128U);

        void* out_ptr = NULL;
        void* head_before = NULL;

        ret = linear_allocator_init(alloc, 128U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        head_before = alloc->head_ptr;
        ret = linear_allocator_allocate(alloc, 0U, 8U, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(NULL == out_ptr);
        assert(head_before == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // req_align_ == 0 -> SUCCESS（no-op）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128U);
        assert(NULL != pool);
        memset(pool, 0, 128U);

        void* out_ptr = NULL;
        void* head_before = NULL;

        ret = linear_allocator_init(alloc, 128U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        head_before = alloc->head_ptr;
        ret = linear_allocator_allocate(alloc, 16U, 0U, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(NULL == out_ptr);
        assert(head_before == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // req_align_ が 2 の冪乗ではない -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128U);
        assert(NULL != pool);
        memset(pool, 0, 128U);

        void* out_ptr = NULL;
        void* head_before = NULL;

        ret = linear_allocator_init(alloc, 128U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        head_before = alloc->head_ptr;
        ret = linear_allocator_allocate(alloc, 16U, 7U, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(NULL == out_ptr);
        assert(head_before == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 正常系（offset == 0）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(16U);
        assert(NULL != pool);
        memset(pool, 0, 16U);

        void* out_ptr = NULL;

        ret = linear_allocator_init(alloc, 16U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 8U, 1U, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(pool == out_ptr);
        assert((void*)((uintptr_t)pool + 8U) == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 正常系（offset != 0）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(16U);
        assert(NULL != pool);
        memset(pool, 0, 16U);

        void* out_ptr = NULL;
        uintptr_t expected_start = 0U;

        ret = linear_allocator_init(alloc, 16U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        alloc->head_ptr = (void*)((uintptr_t)pool + 1U);
        expected_start = (uintptr_t)pool + 4U;

        ret = linear_allocator_allocate(alloc, 4U, 4U, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert((void*)expected_start == out_ptr);
        assert((void*)((uintptr_t)expected_start + 4U) == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 正常系（ちょうど使い切る）
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(8U);
        assert(NULL != pool);
        memset(pool, 0, 8U);

        void* out_ptr = NULL;

        ret = linear_allocator_init(alloc, 8U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 8U, 1U, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(pool == out_ptr);
        assert((void*)((uintptr_t)pool + 8U) == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // メモリ不足 -> LINEAR_ALLOC_NO_MEMORY
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(8U);
        assert(NULL != pool);
        memset(pool, 0, 8U);

        void* out_ptr = NULL;
        void* head_before = NULL;

        ret = linear_allocator_init(alloc, 8U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        head_before = alloc->head_ptr;
        ret = linear_allocator_allocate(alloc, 9U, 1U, &out_ptr);
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr);
        assert(head_before == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // UINTPTR_MAX - size < start_addr を成立させる -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(8U);
        assert(NULL != pool);
        memset(pool, 0, 8U);

        void* out_ptr = NULL;

        ret = linear_allocator_init(alloc, 8U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        alloc->head_ptr = (void*)(UINTPTR_MAX - 3U);

        ret = linear_allocator_allocate(alloc, 8U, 1U, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(NULL == out_ptr);

        alloc->head_ptr = alloc->memory_pool;

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // UINTPTR_MAX - offset < head を成立させる -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_allocator_result_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(8U);
        assert(NULL != pool);
        memset(pool, 0, 8U);

        void* out_ptr = NULL;

        ret = linear_allocator_init(alloc, 8U, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        alloc->head_ptr = (void*)(UINTPTR_MAX - 1U);

        ret = linear_allocator_allocate(alloc, 1U, 8U, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(NULL == out_ptr);

        alloc->head_ptr = alloc->memory_pool;

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
}

static void NO_COVERAGE test_rslt_to_str(void) {
    // Generated by ChatGPT 5.4 Thinking
    {
        const char* str = rslt_to_str(LINEAR_ALLOC_SUCCESS);
        assert(NULL != str);
        assert(0 == strcmp("SUCCESS", str));
    }
    {
        const char* str = rslt_to_str(LINEAR_ALLOC_NO_MEMORY);
        assert(NULL != str);
        assert(0 == strcmp("NO_MEMORY", str));
    }
    {
        const char* str = rslt_to_str(LINEAR_ALLOC_INVALID_ARGUMENT);
        assert(NULL != str);
        assert(0 == strcmp("INVALID_ARGUMENT", str));
    }
    {
        const char* str = rslt_to_str((linear_allocator_result_t)999);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
}
#endif
