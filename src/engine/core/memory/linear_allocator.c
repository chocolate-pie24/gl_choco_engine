/**
 *
 * @file linear_allocator.c
 * @author chocolate-pie24
 * @brief linear_alloc_tオブジェクトの定義と関連APIの内部実装
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
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

/**
 * @brief linear_alloc_t内部データ構造
 * @todo 必要であればメモリトラッキング追加(現状では入れる予定はなし)
 */
struct linear_alloc {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス(実際にはアライメント要件分オフセットされたアドレスを渡す) */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
};

static const char* const s_err_str_success = "SUCCESS";                     /**< エラー種別文字列(処理成功) */
static const char* const s_err_str_no_memory = "NO_MEMORY";                 /**< エラー種別文字列(メモリ確保失敗) */
static const char* const s_err_str_invalid_argument = "INVALID_ARGUMENT";   /**< エラー種別文字列(無効な引数) */

#ifdef TEST_BUILD
#include <assert.h>
static void test_linear_allocator_preinit(void);
static void test_linear_allocator_init(void);
static void test_linear_allocator_allocate(void);
#endif

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        return;
    }
    *memory_requirement_ = sizeof(linear_alloc_t);
    *align_requirement_ = alignof(linear_alloc_t);
}

// 引数allocator_   == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
// 引数memory_pool_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
// 引数capacity_    == 0    -> LINEAR_ALLOC_INVALID_ARGUMENT
linear_alloc_err_t linear_allocator_init(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_) {
    linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
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

// LINEAR_ALLOC_INVALID_ARGUMENT allocator_ == NULL
// LINEAR_ALLOC_INVALID_ARGUMENT out_ptr_ == NULL
// LINEAR_ALLOC_INVALID_ARGUMENT *out_ptr_ != NULL
// LINEAR_ALLOC_INVALID_ARGUMENT req_align_が2の冪乗ではない
// LINEAR_ALLOC_INVALID_ARGUMENT メモリを割り当てた場合、割り当て先頭アドレスの値がUINTPTR_MAXを超過
// LINEAR_ALLOC_INVALID_ARGUMENT メモリ割り当て先頭アドレス+割り当てサイズがUINTPTR_MAXを超過
// LINEAR_ALLOC_NO_MEMORY        メモリを割り当てた場合、メモリプール内に収まらない
// LINEAR_ALLOC_SUCCESS          req_align_ == 0 または req_size_ == 0でワーニング出力し何もしない
// LINEAR_ALLOC_SUCCESS          メモリ割り当てに成功し正常終了
linear_alloc_err_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_) {
    linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    uintptr_t head = 0;
    uintptr_t align = 0;
    uintptr_t size = 0;
    uintptr_t offset = 0;
    uintptr_t start_addr = 0;
    uintptr_t pool = 0;
    uintptr_t cap = 0;

    // Preconditions
    CHECK_ARG_NULL_GOTO_CLEANUP(allocator_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_allocate", "allocator_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_ptr_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_allocate", "out_ptr_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_allocate", "out_ptr_")
    if(0 == req_align_ || 0 == req_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: req_align_ or req_size_ is 0.");
        ret = LINEAR_ALLOC_SUCCESS;
        goto cleanup;
    }
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(IS_POWER_OF_TWO(req_align_), LINEAR_ALLOC_INVALID_ARGUMENT, "linear_allocator_allocate", "req_align_")

    // Simulation
    head = (uintptr_t)allocator_->head_ptr;
    align = (uintptr_t)req_align_;
    size = (uintptr_t)req_size_;
    offset = head % align;
    if(0 != offset) {
        offset = align - offset;    // 要求アライメントに先頭アドレスを調整
    }
    if(UINTPTR_MAX - offset < head) {
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested alignment offset is too large.", s_err_str_invalid_argument);
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        goto cleanup;
    }
    start_addr = head + offset;
    if(UINTPTR_MAX - size < start_addr) {
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested size is too large.", s_err_str_invalid_argument);
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        goto cleanup;
    }
    pool = (uintptr_t)allocator_->memory_pool;
    cap = (uintptr_t)allocator_->capacity;
    if((start_addr + size) > (pool + cap)) {
        uintptr_t free_space = pool + cap - start_addr;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Cannot allocate requested size. Requested size: %zu / Free space: %zu", s_err_str_no_memory, req_size_, (size_t)free_space);
        ret = LINEAR_ALLOC_NO_MEMORY;
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

#ifdef TEST_BUILD
void NO_COVERAGE test_linear_allocator(void) {
    INFO_MESSAGE("test_linear_allocator_preinit begin");
    test_linear_allocator_preinit();
    INFO_MESSAGE("test_linear_allocator_preinit done successfully.");

    INFO_MESSAGE("test_linear_allocator_init begin");
    test_linear_allocator_init();
    INFO_MESSAGE("test_linear_allocator_init done successfully.");

    INFO_MESSAGE("test_linear_allocator_allocate begin");
    test_linear_allocator_allocate();
    INFO_MESSAGE("test_linear_allocator_allocate done successfully.");
}

static void NO_COVERAGE test_linear_allocator_preinit(void) {
    {
        // 最初のif文のreturnを通ることをステップ実行で確認
        size_t mem = 0;
        linear_allocator_preinit(&mem, NULL);

        // 最初のif文のreturnを通ることをステップ実行で確認
        size_t align = 0;
        linear_allocator_preinit(NULL, &align);

        // 最初のif文のreturnを通ることをステップ実行で確認
        linear_allocator_preinit(NULL, NULL);
    }
    {
        size_t mem = 0;
        size_t align = 0;
        linear_allocator_preinit(&mem, &align);
        assert(sizeof(linear_alloc_t) == mem);
        assert(alignof(linear_alloc_t) == align);
    }
}

static void NO_COVERAGE test_linear_allocator_init(void) {
    {
        // 引数allocator_   == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        size_t cap = 128;
        void* pool = malloc(128);
        assert(NULL != pool);

        ret = linear_allocator_init(NULL, cap, pool);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        free(pool);
        pool = NULL;
    }
    {
        // 引数memory_pool_ == NULL -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        size_t cap = 128;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        ret = linear_allocator_init(alloc, cap, NULL);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(0 == alloc->capacity);
        assert(0 == alloc->head_ptr);
        assert(0 == alloc->memory_pool);

        free(alloc);
        alloc = NULL;
    }
    {
        // 引数capacity_    == 0    -> LINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128);
        assert(NULL != pool);
        memset(pool, 0, 128);

        ret = linear_allocator_init(alloc, 0, pool);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        assert(0 == alloc->capacity);
        assert(0 == alloc->head_ptr);
        assert(0 == alloc->memory_pool);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
    {
        // 正常系
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        size_t cap = 128;

        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));

        void* pool = malloc(128);
        assert(NULL != pool);
        memset(pool, 0, 128);

        ret = linear_allocator_init(alloc, cap, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(cap == alloc->capacity);
        assert(alloc->memory_pool == alloc->head_ptr);

        free(alloc);
        alloc = NULL;

        free(pool);
        pool = NULL;
    }
}

static void NO_COVERAGE test_linear_allocator_allocate(void) {
    {
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        // 引数allocator_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        void* out_ptr = NULL;
        ret = linear_allocator_allocate(NULL, 128, 128, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        // 引数out_ptr_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(128);

        ret = linear_allocator_init(alloc, 128, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(128 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        ret = linear_allocator_allocate(alloc, 128, 128, NULL);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        // 引数*out_ptr != NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        out_ptr = malloc(128);
        ret = linear_allocator_allocate(alloc, 128, 128, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);
        free(out_ptr);
        out_ptr = NULL;

        // 引数req_size_が0でワーニングメッセージ、結果はLINEAR_ALLOC_SUCCESSでメモリ確保はなし
        ret = linear_allocator_allocate(alloc, 0, 128, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(NULL == out_ptr);

        // 引数req_align_が0でワーニングメッセージ、結果はLINEAR_ALLOC_SUCCESSでメモリ確保はなし
        ret = linear_allocator_allocate(alloc, 128, 0, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(NULL == out_ptr);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        // 正常系
        ret = linear_allocator_allocate(alloc, 1, 1, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert((uintptr_t)(alloc->head_ptr) == ((uintptr_t)alloc->memory_pool + (uintptr_t)1));

        // キャパシティオーバー
        void* out_ptr2 = NULL;
        ret = linear_allocator_allocate(alloc, 8, 8, &out_ptr2);    // simulationでno_memory
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr2);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        void* out_ptr = NULL;
        ret = linear_allocator_allocate(alloc, 8, 1, &out_ptr); // ギリギリサイズ
        assert(LINEAR_ALLOC_SUCCESS == ret);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        ret = linear_allocator_allocate(alloc, 10, 1, &out_ptr); // サイズオーバー
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        ret = linear_allocator_allocate(alloc, SIZE_MAX, 2, &out_ptr); // 要求サイズが過大
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        ret = linear_allocator_allocate(alloc, 6, 7, &out_ptr); // アライメントが2の冪乗じゃないのでLINEAR_ALLOC_INVALID_ARGUMENT
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = (linear_alloc_t*)malloc(sizeof(linear_alloc_t));
        assert(NULL != alloc);
        memset(alloc, 0, sizeof(linear_alloc_t));
        void* pool = malloc(8);

        linear_alloc_err_t ret = linear_allocator_init(alloc, 8, pool);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(8 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        ret = linear_allocator_allocate(alloc, 6, 2, &out_ptr); // 正常
        assert(LINEAR_ALLOC_SUCCESS == ret);

        free(alloc);
        free(pool);
        alloc = NULL;
        pool = NULL;
    }
}
#endif
