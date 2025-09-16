/**　@addtogroup core_memory
 * @{
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

// TODO: 引数にretvalを追加し、base/macros.hに移動
#define CHECK_ARG_NULL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s requires a valid pointer.", function_name_, variable_name_); \
        ret = LINEAR_ALLOC_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

// TODO: 引数にretvalを追加し、base/macros.hに移動
#define CHECK_ARG_NOT_NULL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL != ptr_) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s requires a null pointer.", function_name_, variable_name_); \
        ret = LINEAR_ALLOC_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

// TODO: 引数にretvalを追加し、base/macros.hに移動
#define CHECK_ARG_NOT_VALID_GOTO_CLEANUP(is_valid_, function_name_, variable_name_) \
    if(!(is_valid_)) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s is not valid.", function_name_, variable_name_); \
        ret = LINEAR_ALLOC_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

// TODO: 引数にretvalを追加し、base/macros.hに移動
#define CHECK_ALLOC_FAIL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(NO_MEMORY) - Failed to allocate %s memory.", function_name_, variable_name_); \
        ret = LINEAR_ALLOC_NO_MEMORY; \
        goto cleanup;  \
    } \

#ifdef TEST_BUILD
#include <assert.h>
typedef struct malloc_test {
    bool fail_enable;
    int32_t malloc_counter;
    int32_t malloc_fail_n;
} malloc_test_t;

static malloc_test_t s_malloc_test;
static void test_test_malloc(void);
static void test_linear_allocator_create(void);
static void test_linear_allocator_destroy(void);
static void test_linear_allocator_allocate(void);
#endif

static void* test_malloc(size_t size_);

// 引数allocator_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
// 引数*allocator_ != NULLでLINEAR_ALLOC_INVALID_ARGUMENT
// 引数capacity_ == 0でLINEAR_ALLOC_INVALID_ARGUMENT
// 1回目のmallocに失敗でLINEAR_ALLOC_NO_MEMORY + リソース解放なし
// 2回目のmallocに失敗でLINEAR_ALLOC_NO_MEMORY + リソース解放
linear_alloc_err_t linear_allocator_create(linear_alloc_t** allocator_, size_t capacity_) {
    linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    linear_alloc_t* tmp = NULL;

    // Preconditions
    CHECK_ARG_NULL_GOTO_CLEANUP(allocator_, "linear_allocator_create", "allocator_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*allocator_, "linear_allocator_create", "allocator_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != capacity_, "linear_allocator_create", "capacity_")

    // Simulation
    tmp = (linear_alloc_t*)test_malloc(sizeof(*tmp));    // TODO: choco_malloc()
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp, "linear_allocator_create", "tmp")
    memset(tmp, 0, sizeof(*tmp));

    tmp->memory_pool = test_malloc(capacity_);   // TODO: choco_malloc()
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp->memory_pool, "linear_allocator_create", "tmp->memory_pool")
    memset(tmp->memory_pool, 0, capacity_);

    tmp->capacity = capacity_;
    tmp->head_ptr = tmp->memory_pool;

    // commit
    *allocator_ = tmp;
    ret = LINEAR_ALLOC_SUCCESS;

cleanup:
    if(LINEAR_ALLOC_SUCCESS != ret) {
        if(NULL != tmp && NULL != tmp->memory_pool) {
            free(tmp->memory_pool); // TODO: choco_free
            tmp->memory_pool = NULL;
        }
        if(NULL != tmp) {
            free(tmp);
            tmp = NULL;
        }
    }
    return ret;
}

// 引数allocator_ == NULLでno-op
// 引数*allocator_ == NULLでno-op
// 2重destroy OK
void linear_allocator_destroy(linear_alloc_t** allocator_) {
    // TODO: choco_free
    if(NULL == allocator_) {
        goto cleanup;
    }
    if(NULL == *allocator_) {
        goto cleanup;
    }
    if(NULL != (*allocator_)->memory_pool) {
        free((*allocator_)->memory_pool);
        (*allocator_)->memory_pool = NULL;
    }
    (*allocator_)->capacity = 0;
    (*allocator_)->head_ptr = NULL;
    free(*allocator_);
    *allocator_ = NULL;

cleanup:
    return;
}

// 引数allocator_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
// 引数out_ptr_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
// 引数*out_ptr != NULLでLINEAR_ALLOC_INVALID_ARGUMENT
// 引数req_size_が0またはreq_align_でワーニングメッセージ、結果はLINEAR_ALLOC_SUCCESSでメモリ確保はなし
// req_align_が2の冪乗ではない場合にLINEAR_ALLOC_INVALID_ARGUMENT
// 割り当て先頭アドレス+size_がUINTPTR_MAXの値を超過する場合にLINEAR_ALLOC_INVALID_ARGUMENT
// 割り当てるメモリ領域がallocator_が保有しているメモリ領域に収まらない場合、LINEAR_ALLOC_NO_MEMORY
linear_alloc_err_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_) {
    linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions
    CHECK_ARG_NULL_GOTO_CLEANUP(allocator_, "linear_allocator_allocate", "allocator_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_ptr_, "linear_allocator_allocate", "out_ptr_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, "linear_allocator_allocate", "out_ptr_")
    if(0 == req_align_ || 0 == req_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: req_align_ or req_size_ is 0.");
        ret = LINEAR_ALLOC_SUCCESS;
        goto cleanup;
    }
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(IS_POWER_OF_TWO(req_align_), "linear_allocator_allocate", "req_align_");

    // Simulation
    uintptr_t head = (uintptr_t)allocator_->head_ptr;
    uintptr_t align = (uintptr_t)req_align_;
    uintptr_t size = (uintptr_t)req_size_;
    uintptr_t offset = head % align;
    if(0 != offset) {
        offset = align - offset;    // 要求アライメントに先頭アドレスを調整
    }

    uintptr_t start_addr = head + offset;
    if(UINTPTR_MAX - size < start_addr) {
        ERROR_MESSAGE("linear_allocator_allocate(INVALID_ARGUMENT) - Requested size is too big.");
        ret = LINEAR_ALLOC_INVALID_ARGUMENT;
        goto cleanup;
    }
    uintptr_t pool = (uintptr_t)allocator_->memory_pool;
    uintptr_t cap = (uintptr_t)allocator_->capacity;
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

    INFO_MESSAGE("test_linear_allocator_create begin");
    test_linear_allocator_create();
    INFO_MESSAGE("test_linear_allocator_create done successfully.");

    INFO_MESSAGE("test_linear_allocator_destroy begin");
    test_linear_allocator_destroy();
    INFO_MESSAGE("test_linear_allocator_destroy done successfully.");

    INFO_MESSAGE("test_linear_allocator_allocate begin");
    test_linear_allocator_allocate();
    INFO_MESSAGE("test_linear_allocator_allocate done successfully.");
}

static void NO_COVERAGE test_linear_allocator_create(void) {
    linear_alloc_err_t ret = LINEAR_ALLOC_INVALID_ARGUMENT;
    {
        // 引数allocator_ == NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        ret = linear_allocator_create(NULL, 128);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        // 引数capacity_ == 0でLINEAR_ALLOC_INVALID_ARGUMENT
        linear_alloc_t* alloc = NULL;
        ret = linear_allocator_create(&alloc, 0);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        // success
        ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(128 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        // 引数*allocator_ != NULLでLINEAR_ALLOC_INVALID_ARGUMENT
        ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        linear_allocator_destroy(&alloc);
        assert(NULL == alloc);
    }
    {
        s_malloc_test.fail_enable = true;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 0;    // 1回目で失敗 + リソース解放なし

        linear_alloc_t* alloc = NULL;
        ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_NO_MEMORY == ret);

        linear_allocator_destroy(&alloc);
        assert(NULL == alloc);
    }
    {
        s_malloc_test.fail_enable = true;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 1;    // 2回目で失敗 + リソース解放

        linear_alloc_t* alloc = NULL;
        ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_NO_MEMORY == ret);

        linear_allocator_destroy(&alloc);
        assert(NULL == alloc);
    }
}

static void NO_COVERAGE test_linear_allocator_destroy(void) {
    {
        linear_allocator_destroy(NULL); // 引数allocator_ == NULLでno-op

        linear_alloc_t* alloc = NULL;
        linear_allocator_destroy(&alloc);   // 引数*allocator_ == NULLでno-op

        linear_alloc_err_t ret = linear_allocator_create(&alloc, 128);
        assert(LINEAR_ALLOC_SUCCESS == ret);
        assert(128 == alloc->capacity);
        assert(alloc->head_ptr == alloc->memory_pool);

        linear_allocator_destroy(&alloc);
        assert(NULL == alloc);
        linear_allocator_destroy(&alloc);
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

        linear_allocator_destroy(&alloc);
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

        linear_allocator_destroy(&alloc);
        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 8, 1, &out_ptr); // ギリギリサイズ
        assert(LINEAR_ALLOC_SUCCESS == ret);

        linear_allocator_destroy(&alloc);
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

        linear_allocator_destroy(&alloc);
        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, SIZE_MAX, 2, &out_ptr); // 要求サイズが過大
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        linear_allocator_destroy(&alloc);
        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 6, 7, &out_ptr); // アライメントが2の冪乗じゃないのでLINEAR_ALLOC_INVALID_ARGUMENT
        assert(LINEAR_ALLOC_INVALID_ARGUMENT == ret);

        linear_allocator_destroy(&alloc);
        alloc = NULL;
    }
    {
        void* out_ptr = NULL;
        linear_alloc_t* alloc = NULL;
        linear_alloc_err_t ret = linear_allocator_create(&alloc, 8);
        assert(LINEAR_ALLOC_SUCCESS == ret);

        ret = linear_allocator_allocate(alloc, 6, 2, &out_ptr); // 正常
        assert(LINEAR_ALLOC_SUCCESS == ret);

        linear_allocator_destroy(&alloc);
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
