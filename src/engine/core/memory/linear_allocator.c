/**
 *
 * @file linear_allocator.c
 * @author chocolate-pie24
 * @brief linear_alloc_tオブジェクトの定義と関連APIの内部実装
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stddef.h>
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset
#include <stdbool.h>
#include <stdalign.h>
#include <stdint.h>

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

/**
 * @brief linear_alloc_t内部データ構造
 * @todo 必要であればメモリトラッキング追加(現状では入れる予定はなし)
 */
struct linear_alloc {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス(実際にはアライメント要件分オフセットされたアドレスを渡す) */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
};

#ifdef TEST_BUILD
#include <assert.h>
typedef struct malloc_test {
    bool fail_enable;
    int32_t malloc_counter;
    int32_t malloc_fail_n;
} malloc_test_t;

static malloc_test_t s_malloc_test;
static void test_test_malloc(void);
static void test_linear_allocator_allocate(void);
#endif

static void* test_malloc(size_t size_);

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        return;
    }
    *memory_requirement_ = sizeof(linear_alloc_t);
    *align_requirement_ = alignof(linear_alloc_t);
}

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
        ERROR_MESSAGE("linear_allocator_allocate(INVALID_ARGUMENT) - Requested offset is too big.");
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        goto cleanup;
    }
    start_addr = head + offset;
    if(UINTPTR_MAX - size < start_addr) {
        ERROR_MESSAGE("linear_allocator_allocate(INVALID_ARGUMENT) - Requested size is too big.");
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        goto cleanup;
    }
    pool = (uintptr_t)allocator_->memory_pool;
    cap = (uintptr_t)allocator_->capacity;
    if((start_addr + size) > (pool + cap)) {
        uintptr_t free_space = pool + cap - start_addr;
        ERROR_MESSAGE("linear_allocator_allocate(NO_MEMORY) - Can not allocate requested size. Requested size: %zu / Free space: %zu", req_size_, (size_t)free_space);
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

static void* test_malloc(size_t size_) {
    void* ret = NULL;
#ifdef TEST_BUILD
    if(s_malloc_test.fail_enable) {
        if(s_malloc_test.malloc_counter == s_malloc_test.malloc_fail_n) {
            ret = NULL;
        } else {
            ret = malloc(size_);
        }
        s_malloc_test.malloc_counter++;
    } else {
        ret = malloc(size_);
    }
#else
    ret = malloc(size_);
#endif
    return ret;
}

#ifdef TEST_BUILD
void test_linear_allocator(void) {
    s_malloc_test.fail_enable = false;
    s_malloc_test.malloc_counter = 0;
    s_malloc_test.malloc_fail_n = 0;

    INFO_MESSAGE("test_test_malloc begin");
    test_test_malloc();
    INFO_MESSAGE("test_test_malloc done successfully.");

    INFO_MESSAGE("test_linear_allocator_allocate begin");
    test_linear_allocator_allocate();
    INFO_MESSAGE("test_linear_allocator_allocate done successfully.");
}

static void NO_COVERAGE test_linear_allocator_allocate(void) {
    {
        linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

        // 引数allocator_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        void* out_ptr = NULL;
        ret = linear_allocator_allocate(NULL, 128, 128, &out_ptr);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        // 引数out_ptr_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_t* alloc = NULL;
        ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_SUCCESS == ret);
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

        ret = linear_allocator_allocate(alloc, 128, 0, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(NULL == out_ptr);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 1, 1, &out_ptr);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert((uintptr_t)(alloc->head_ptr) == ((uintptr_t)alloc->memory_pool + (uintptr_t)1));

        void* out_ptr2 = NULL;
        ret = linear_allocator_allocate(alloc, 8, 8, &out_ptr2);    // simulationでno_memory
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr2);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 8, 1, &out_ptr); // ギリギリサイズ
        assert(LINEAR_ALLOC_SUCCESS == ret);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 10, 1, &out_ptr); // サイズオーバー
        assert(LINEAR_ALLOC_NO_MEMORY == ret);
        assert(NULL == out_ptr);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, SIZE_MAX, 2, &out_ptr); // 要求サイズが過大
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 6, 7, &out_ptr); // アライメントが2の冪乗じゃないのでLINEAR_ALLOC_INVALID_ARGUMENT
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 6, 2, &out_ptr); // 正常
        assert(LINEAR_ALLOC_SUCCESS == ret);

        alloc = NULL;
    }
}

static void NO_COVERAGE test_test_malloc(void) {
    {
        DEBUG_MESSAGE("test_test_malloc test_case1");
        s_malloc_test.fail_enable = false;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 0;
        void* tmp = NULL;
        tmp = test_malloc(128);
        assert(NULL != tmp);
        free(tmp);
        tmp = NULL;
    }
    {
        DEBUG_MESSAGE("test_test_malloc test_case2");
        s_malloc_test.fail_enable = true;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 1;
        void* tmp = NULL;
        tmp = test_malloc(128); // 1回目は成功
        assert(NULL != tmp);
        free(tmp);
        tmp = NULL;

        tmp = test_malloc(128); // 2回目で失敗
        assert(NULL == tmp);
        free(tmp);
        tmp = NULL;
    }
    s_malloc_test.fail_enable = false;
    s_malloc_test.malloc_counter = 0;
    s_malloc_test.malloc_fail_n = 0;
}
#endif
