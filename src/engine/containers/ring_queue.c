#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset

#include "engine/containers/ring_queue.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#ifdef TEST_BUILD
#include <assert.h>
#endif

struct ring_queue {
    size_t head;                /**< リングキュー配列の先頭インデックス */
    size_t tail;                /**< リングキューに次に追加する要素インデックス */
    size_t len;                 /**< リングキューに格納済みの要素数 */
    size_t element_align;       /**< リングキューに格納する要素のアライメント要件 */
    size_t max_element_count;   /**< リングキューに格納可能な最大要素数 */
    size_t padding;             /**< 1要素ごとに必要なパディング量 */
    size_t element_size;        /**< 格納要素のサイズ(パディングは入れない実際のオブジェクト型のサイズ) */
    size_t stride;              /**< 1要素に必要なメモリ領域(element_size + padding) */
    size_t capacity;            /**< memory_poolのサイズ */
    void* memory_pool;
};

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";
static const char* const s_rslt_str_empty = "EMPTY";

static const char* rslt_to_str(ring_queue_result_t rslt_);
static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_);

#ifdef TEST_BUILD
static void NO_COVERAGE test_rslt_to_str(void);
static void NO_COVERAGE test_rslt_convert_mem_sys(void);
static void NO_COVERAGE test_ring_queue_create(void);
static void NO_COVERAGE test_ring_queue_destroy(void);
static void NO_COVERAGE test_ring_queue_push(void);
static void NO_COVERAGE test_ring_queue_pop(void);
static void NO_COVERAGE test_ring_queue_empty(void);
#endif

// 格納可能な最大数の要素を格納した結果、memory_poolに空きがあると、
// リングキューフルの状態でのpushの処理がややこしくなる。なので、下記の制約を設ける
//
// - element_alignは2の冪乗でなければいけない
// - max_align_tを越えるelement_alignは不可
// - memory_system_allocateで確保されるメモリはmax_align_tにアライメントされていることが前提
//
// ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
// *ring_queue_ != NULL -> RING_QUEUE_INVALID_ARGUMENT
// 0 == max_element_count_ -> RING_QUEUE_INVALID_ARGUMENT
// 0 == element_size_ -> RING_QUEUE_INVALID_ARGUMENT
// element_align_が2の冪乗ではない -> RING_QUEUE_INVALID_ARGUMENT
// element_align_がmax_align_tを超過 -> RING_QUEUE_INVALID_ARGUMENT
// tmp_queueのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
// tmp_queue->memory_poolのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
// element_size_ * max_element_count_がSIZE_MAXを超過 -> RING_QUEUE_INVALID_ARGUMENT
ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** ring_queue_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    ring_queue_t* tmp_queue = NULL;
    size_t capacity = 0;
    size_t stride = 0;
    size_t padding = 0;
    size_t diff = 0;
    uintptr_t mem_pool_ptr = 0;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(ring_queue_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "ring_queue_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*ring_queue_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "*ring_queue_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != max_element_count_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "max_element_count_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != element_size_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "element_size_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(IS_POWER_OF_TWO(element_align_), RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "element_align_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(element_align_ <= alignof(max_align_t), RING_QUEUE_INVALID_ARGUMENT, "ring_queue_create", "element_align_")
    if(SIZE_MAX / element_size_ < max_element_count_) {
        ERROR_MESSAGE("ring_queue_create(%s) - Provided 'element_size_' and 'max_element_count_' are too large.", s_rslt_str_invalid_argument);
        ret = RING_QUEUE_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Simulation.)
    diff = element_size_ % element_align_;   // アライメントのズレ量
    if(0 == diff) {
        padding = 0;
    } else {
        if(element_size_ > element_align_) {
            padding = element_align_ - diff;
        } else {
            padding = element_align_ - element_size_;
        }
    }
    stride = element_size_ + padding;
    if(SIZE_MAX / max_element_count_ < stride) {
        ERROR_MESSAGE("ring_queue_create(%s) - Computed element stride is too large.", s_rslt_str_invalid_argument);
        ret = RING_QUEUE_INVALID_ARGUMENT;
        goto cleanup;
    }
    capacity = stride * max_element_count_;

    ret_mem = memory_system_allocate(sizeof(*tmp_queue), MEMORY_TAG_RING_QUEUE, (void**)&tmp_queue);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = rslt_convert_mem_sys(ret_mem);
        ERROR_MESSAGE("ring_queue_create(%s) - Failed to allocate ring queue memory.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_queue, 0, sizeof(*tmp_queue));

    ret_mem = memory_system_allocate(capacity, MEMORY_TAG_RING_QUEUE, &tmp_queue->memory_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = rslt_convert_mem_sys(ret_mem);
        ERROR_MESSAGE("ring_queue_create(%s) - Failed to allocate memory pool memory.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_queue->memory_pool, 0, capacity);
    mem_pool_ptr = (uintptr_t)tmp_queue->memory_pool;
    if(0 != (mem_pool_ptr % element_align_)) {
        ERROR_MESSAGE("ring_queue_create(%s) - Allocated memory pool alignment is invalid.", s_rslt_str_runtime_error);
        ret = RING_QUEUE_RUNTIME_ERROR;
        goto cleanup;
    }

    tmp_queue->capacity = capacity;
    tmp_queue->element_align = element_align_;
    tmp_queue->element_size = element_size_;
    tmp_queue->head = 0;
    tmp_queue->len = 0;
    tmp_queue->max_element_count = max_element_count_;
    tmp_queue->padding = padding;
    tmp_queue->stride = stride;
    tmp_queue->tail = 0;

    *ring_queue_ = tmp_queue;
    ret = RING_QUEUE_SUCCESS;

cleanup:
    if(RING_QUEUE_SUCCESS != ret) {
        if(NULL != tmp_queue) {
            if(NULL != tmp_queue->memory_pool) {
                memory_system_free(tmp_queue->memory_pool, capacity, MEMORY_TAG_RING_QUEUE);
                tmp_queue->memory_pool = NULL;
            }
            memory_system_free(tmp_queue, sizeof(*tmp_queue), MEMORY_TAG_RING_QUEUE);
            tmp_queue = NULL;
        }
    }
    return ret;
}

void ring_queue_destroy(ring_queue_t** ring_queue_) {
    if(NULL == ring_queue_) {
        goto cleanup;
    }
    if(NULL == *ring_queue_) {
        goto cleanup;
    }
    if(NULL != (*ring_queue_)->memory_pool) {
        memory_system_free((*ring_queue_)->memory_pool, (*ring_queue_)->capacity, MEMORY_TAG_RING_QUEUE);
        (*ring_queue_)->memory_pool = NULL;
    }
    memory_system_free(*ring_queue_, sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE);
    *ring_queue_ = NULL;
cleanup:
    return;
}

// ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
// data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue_->memory_pool == NULL -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue_->element_size != element_size_   -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue_->element_align != element_align_ -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue_がfullでワーニングメッセージ
// 引数element_size_, element_align_は格納データの不整合のチェック用
ring_queue_result_t ring_queue_push(ring_queue_t* ring_queue_, const void* data_, size_t element_size_, size_t element_align_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* target_ptr = NULL;

    CHECK_ARG_NULL_GOTO_CLEANUP(ring_queue_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_push", "ring_queue_")
    CHECK_ARG_NULL_GOTO_CLEANUP(data_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_push", "data_")
    CHECK_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_push", "ring_queue_->memory_pool")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(ring_queue_->element_size == element_size_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_push", "element_size_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(ring_queue_->element_align == element_align_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_push", "element_align_")
    if(ring_queue_->max_element_count == ring_queue_->len) {
        DEBUG_MESSAGE("Ring queue is full; overwriting the oldest element.");
    }

    mem_ptr = (char*)ring_queue_->memory_pool;
    target_ptr = mem_ptr + (ring_queue_->stride * ring_queue_->tail);
    memcpy(target_ptr, data_, ring_queue_->element_size);

    ring_queue_->tail = (ring_queue_->tail + 1) % ring_queue_->max_element_count;
    if(ring_queue_->len != ring_queue_->max_element_count) {
        ring_queue_->len++;
    } else {
        ring_queue_->head = (ring_queue_->head + 1) % ring_queue_->max_element_count;
    }
    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

// ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
// data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue->memory_pool == NULL ->RING_QUEUE_INVALID_ARGUMENT
// ring_queue_->element_size != element_size_ -> RING_QUEUE_INVALID_ARGUMENT
// ring_queue_->element_align != element_align_ -> RING_QUEUE_INVALID_ARGUMENT
// ring_queueが空でRING_QUEUE_EMPTY

// 引数element_size_, element_align_は格納データの不整合のチェック用
ring_queue_result_t ring_queue_pop(ring_queue_t* ring_queue_, void* data_, size_t element_size_, size_t element_align_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* head_ptr = NULL;

    CHECK_ARG_NULL_GOTO_CLEANUP(ring_queue_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_pop", "ring_queue_")
    CHECK_ARG_NULL_GOTO_CLEANUP(data_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_pop", "data_")
    CHECK_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_pop", "ring_queue_->memory_pool")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(ring_queue_->element_size == element_size_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_pop", "element_size_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(ring_queue_->element_align == element_align_, RING_QUEUE_INVALID_ARGUMENT, "ring_queue_pop", "element_align_")
    if(ring_queue_empty(ring_queue_)) {
        DEBUG_MESSAGE("Ring queue is empty.");
        ret = RING_QUEUE_EMPTY;
        goto cleanup;
    }

    mem_ptr = (char*)ring_queue_->memory_pool;
    head_ptr = mem_ptr + (ring_queue_->head * ring_queue_->stride);
    memcpy(data_, head_ptr, ring_queue_->element_size);
    ring_queue_->len--;
    ring_queue_->head = (ring_queue_->head + 1) % ring_queue_->max_element_count;
    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

// ring_queue == NULL or ring_queue->memory_pool == NULLはtrue(空)
bool ring_queue_empty(const ring_queue_t* ring_queue_) {
    if(NULL == ring_queue_) {
        return true;
    } else if(NULL == ring_queue_->memory_pool) {
        return true;
    } else {
        if(0 != ring_queue_->len) {
            return false;
        } else {
            return true;
        }
    }
}

static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RING_QUEUE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RING_QUEUE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return RING_QUEUE_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RING_QUEUE_NO_MEMORY;
    default:
        return RING_QUEUE_UNDEFINED_ERROR;
    }
}

static const char* rslt_to_str(ring_queue_result_t rslt_) {
    switch(rslt_) {
    case RING_QUEUE_SUCCESS:
        return s_rslt_str_success;
    case RING_QUEUE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RING_QUEUE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RING_QUEUE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RING_QUEUE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case RING_QUEUE_EMPTY:
        return s_rslt_str_empty;
    default:
        return s_rslt_str_undefined_error;
    }
}

#ifdef TEST_BUILD

void test_ring_queue(void) {
    memory_system_report();
    test_rslt_to_str();
    memory_system_report();

    test_rslt_convert_mem_sys();
    memory_system_report();

    test_ring_queue_create();
    memory_system_report();

    test_ring_queue_destroy();
    memory_system_report();

    test_ring_queue_push();
    memory_system_report();

    test_ring_queue_pop();
    memory_system_report();

    test_ring_queue_empty();
    memory_system_report();
}

static void NO_COVERAGE test_ring_queue_create(void) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ret = ring_queue_create(128, 8, 8, NULL);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        // 正常系
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 10, 8, &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert((ring_queue->stride * ring_queue->max_element_count) == ring_queue->capacity);
        assert(8 == ring_queue->element_align);
        assert(10 == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(128 == ring_queue->max_element_count);
        assert(NULL != ring_queue->memory_pool);
        assert(6 == ring_queue->padding);
        assert(16 == ring_queue->stride);
        assert(0 == ring_queue->tail);

        // *ring_queue_ != NULL -> RING_QUEUE_INVALID_ARGUMENT
        ret = ring_queue_create(128, 10, 8, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert((ring_queue->stride * ring_queue->max_element_count) == ring_queue->capacity);
        assert(8 == ring_queue->element_align);
        assert(10 == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(128 == ring_queue->max_element_count);
        assert(NULL != ring_queue->memory_pool);
        assert(6 == ring_queue->padding);
        assert(16 == ring_queue->stride);
        assert(0 == ring_queue->tail);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
        ring_queue_destroy(&ring_queue);
    }
    {
        // 0 == max_element_count_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(0, 10, 8, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // 0 == element_size_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 0, 8, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_align_が2の冪乗ではない -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 10, 5, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_align_がmax_align_tを超過 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 10, alignof(max_align_t) + 1, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // tmp_queueのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
        memory_system_test_param_set(0);   // 1回目のmallocで失敗
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 10, 8, &ring_queue);
        assert(RING_QUEUE_NO_MEMORY == ret);
        assert(NULL == ring_queue);
        memory_system_test_param_reset();
    }
    {
        // tmp_queue->memory_poolのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
        memory_system_test_param_set(1);   // 2回目のmallocで失敗
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 10, 8, &ring_queue);
        assert(RING_QUEUE_NO_MEMORY == ret);
        assert(NULL == ring_queue);
        memory_system_test_param_reset();
    }
    {
        // element_size_ * max_element_count_がSIZE_MAXを超過 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, SIZE_MAX, 8, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // 正常系(padding == 0)
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 8, 8, &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert((ring_queue->stride * ring_queue->max_element_count) == ring_queue->capacity);
        assert(8 == ring_queue->element_align);
        assert(8 == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(128 == ring_queue->max_element_count);
        assert(NULL != ring_queue->memory_pool);
        assert(0 == ring_queue->padding);
        assert(8 == ring_queue->stride);
        assert(0 == ring_queue->tail);
        ring_queue_destroy(&ring_queue);
    }
    {
        // 正常系(align > element_size)
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, 3, 8, &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert((ring_queue->stride * ring_queue->max_element_count) == ring_queue->capacity);
        assert(8 == ring_queue->element_align);
        assert(3 == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(128 == ring_queue->max_element_count);
        assert(NULL != ring_queue->memory_pool);
        assert(5 == ring_queue->padding);
        assert(8 == ring_queue->stride);
        assert(0 == ring_queue->tail);
        ring_queue_destroy(&ring_queue);
    }
}

static void NO_COVERAGE test_ring_queue_destroy(void) {
    {
        // ring_queue_ == NULL -> no-op
        ring_queue_destroy(NULL);
    }
    {
        // memory_pool == NULL
        // メモリ使用量が戻ること
        ring_queue_t* ring_queue = NULL;
        memory_system_allocate(sizeof(*ring_queue), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue);
        memory_system_report();
        assert(NULL != ring_queue);
        memset(ring_queue, 0, sizeof(*ring_queue));
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
        memory_system_report();
    }
    {
        // *ring_queue == NULL;
        ring_queue_t* ring_queue = NULL;
        ring_queue_destroy(&ring_queue);
    }
    {
        // 正常系
        ring_queue_t* ring_queue = NULL;
        ring_queue_result_t ret = ring_queue_create(8, 8, 8, &ring_queue);
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
        ring_queue_destroy(&ring_queue);    // 2重destroy
    }
}

static void NO_COVERAGE test_ring_queue_push(void) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        int a = 0;
        ret = ring_queue_push(NULL, &a, 8, 8);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        // data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_push(ring_queue, NULL, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queue_->memory_pool == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue);
        assert(NULL != ring_queue);

        int a = 0;
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);

        memory_system_free(ring_queue, sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE);
    }
    {
        // ring_queue_->element_size != element_size_   -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(8 == ring_queue->max_element_count);
        assert(0 == ring_queue->tail);

        int a = 0;
        ret = ring_queue_push(ring_queue, &a, sizeof(char), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(8 == ring_queue->max_element_count);
        assert(0 == ring_queue->tail);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queue_->element_align != element_align_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(8 == ring_queue->max_element_count);
        assert(0 == ring_queue->tail);

        int a = 0;
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(char));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(8 == ring_queue->max_element_count);
        assert(0 == ring_queue->tail);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queue_がfullでワーニングメッセージ
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(3, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        int a = 1;
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(1 == ring_queue->tail);
        assert(1 == ring_queue->len);
        assert(3 == ring_queue->max_element_count);

        a = 2;
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(2 == ring_queue->tail);
        assert(2 == ring_queue->len);
        assert(3 == ring_queue->max_element_count);

        a = 3;
        // ring_queueがfullでワーニング
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->tail);
        assert(3 == ring_queue->len);
        assert(3 == ring_queue->max_element_count);

        a = 4;
        // ring_queueがfullでワーニング
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(1 == ring_queue->head);
        assert(1 == ring_queue->tail);
        assert(3 == ring_queue->len);
        assert(3 == ring_queue->max_element_count);

        a = 5;
        // ring_queueがfullでワーニング
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(alignof(int) == ring_queue->element_align);
        assert(sizeof(int) == ring_queue->element_size);
        assert(2 == ring_queue->head);
        assert(2 == ring_queue->tail);
        assert(3 == ring_queue->len);
        assert(3 == ring_queue->max_element_count);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(3 == a);
        assert(0 == ring_queue->head);
        assert(2 == ring_queue->len);
        assert(2 == ring_queue->tail);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(4 == a);
        assert(1 == ring_queue->head);
        assert(1 == ring_queue->len);
        assert(2 == ring_queue->tail);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(5 == a);
        assert(2 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(2 == ring_queue->tail);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_EMPTY == ret);
        assert(5 == a);
        assert(2 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(2 == ring_queue->tail);

        ring_queue_destroy(&ring_queue);
    }
}

static void NO_COVERAGE test_ring_queue_pop(void) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    int a = 0;
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ret = ring_queue_pop(NULL, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        // data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, NULL, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queue->memory_pool == NULL ->RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue);
        memset(ring_queue, 0, sizeof(ring_queue_t));

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        memory_system_free(ring_queue, sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE);
    }
    {
        // ring_queue_->element_size != element_size_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &a, sizeof(char), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queue_->element_align == element_align_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(char));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    {
        // ring_queueが空でRING_QUEUE_EMPTY
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_EMPTY == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    // 正常系はpushのテストで実施済み
}

static void NO_COVERAGE test_ring_queue_empty(void) {
    bool ret = false;
    {
        ret = ring_queue_empty(NULL);
        assert(true == ret);
    }
    {
        ring_queue_t* ring_queue = NULL;
        memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue);
        assert(NULL != ring_queue);
        memset(ring_queue, 0, sizeof(ring_queue_t));
        ret = ring_queue_empty(ring_queue);
        assert(true == ret);

        memory_system_free(ring_queue, sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE);
    }
    {
        ring_queue_t* ring_queue = NULL;
        ring_queue_result_t ret_create = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret_create);

        ret = ring_queue_empty(ring_queue);
        assert(true == ret);

        int a = 0;
        ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        ret = ring_queue_empty(ring_queue);
        assert(false == ret);

        ring_queue_destroy(&ring_queue);
    }
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* tmp = rslt_to_str(RING_QUEUE_SUCCESS);
        assert(0 == strcmp(tmp, s_rslt_str_success));
        DEBUG_MESSAGE("test_rslt_to_str - message success: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_INVALID_ARGUMENT);
        assert(0 == strcmp(tmp, s_rslt_str_invalid_argument));
        DEBUG_MESSAGE("test_rslt_to_str - message invalid argument: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_NO_MEMORY);
        assert(0 == strcmp(tmp, s_rslt_str_no_memory));
        DEBUG_MESSAGE("test_rslt_to_str - message no memory: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_RUNTIME_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_runtime_error));
        DEBUG_MESSAGE("test_rslt_to_str - message runtime error: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_UNDEFINED_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_undefined_error));
        DEBUG_MESSAGE("test_rslt_to_str - message undefined error: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_EMPTY);
        assert(0 == strcmp(tmp, s_rslt_str_empty));
        DEBUG_MESSAGE("test_rslt_to_str - message empty: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(1024);
        assert(0 == strcmp(tmp, s_rslt_str_undefined_error));
        DEBUG_MESSAGE("test_rslt_to_str - undefined error: %s", tmp);
    }
}

static void NO_COVERAGE test_rslt_convert_mem_sys(void) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;

    ret = rslt_convert_mem_sys(MEMORY_SYSTEM_SUCCESS);
    assert(RING_QUEUE_SUCCESS == ret);

    ret = rslt_convert_mem_sys(MEMORY_SYSTEM_INVALID_ARGUMENT);
    assert(RING_QUEUE_INVALID_ARGUMENT == ret);

    ret = rslt_convert_mem_sys(MEMORY_SYSTEM_RUNTIME_ERROR);
    assert(RING_QUEUE_RUNTIME_ERROR == ret);

    ret = rslt_convert_mem_sys(MEMORY_SYSTEM_NO_MEMORY);
    assert(RING_QUEUE_NO_MEMORY == ret);

    ret = rslt_convert_mem_sys(1024);
    assert(RING_QUEUE_UNDEFINED_ERROR == ret);
}

#endif
