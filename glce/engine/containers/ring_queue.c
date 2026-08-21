/** @ingroup containers
 *
 * @file ring_queue.c
 * @author chocolate-pie24
 * @brief ジェネリック型のリングキューモジュールの実装
 *
 * @version 0.1
 * @date 2025-10-14
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
#include <string.h> // for memset

#include "engine/containers/ring_queue.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief ring_queue_t内部データ構造
 *
 */
struct ring_queue {
    size_t head;                /**< リングキュー配列の先頭インデックス */
    size_t tail;                /**< リングキューに次に追加する要素インデックス */
    size_t len;                 /**< リングキューに格納済みの要素数 */
    size_t element_align;       /**< リングキューに格納する要素のアライメント要件 */
    size_t max_element_count;   /**< リングキューに格納可能な最大要素数 */
    size_t padding;             /**< 1要素ごとに必要なパディング量 */
    size_t element_size;        /**< 格納要素のサイズ(パディングサイズを含まない実際の構造体のサイズ) */
    size_t stride;              /**< 1要素に必要なメモリ領域(element_size + padding) */
    size_t capacity;            /**< memory_poolのサイズ */
    void* memory_pool;          /**< 要素を格納するバッファ */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< リングキューAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< リングキューAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< リングキューAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< リングキューAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< リングキューAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< リングキューAPI実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< リングキューAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< リングキューAPI実行結果コード(内部データ破損)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< リングキューAPI実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_empty = "EMPTY";                        /**< リングキューAPI実行結果コード(キューが空)に対応する文字列 */

static const char* rslt_to_str(ring_queue_result_t rslt_);
static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_);

ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** ring_queue_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    ring_queue_t* tmp_queue = NULL;
    size_t capacity = 0;
    size_t stride = 0;
    size_t padding = 0;
    size_t diff = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "ring_queue_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "*ring_queue_")
    if(0 == max_element_count_ || 0 == element_size_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_create(%s) - Provided max_element_count_ or element_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == element_align_ || !IS_POWER_OF_TWO(element_align_) || element_align_ > alignof(max_align_t)) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_create(%s) - Provided element_align_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Simulation.
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
    if((SIZE_MAX - padding) < element_size_) {
        ret = RING_QUEUE_OVERFLOW;
        ERROR_MESSAGE("ring_queue_create(%s) - Computed stride is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    stride = element_size_ + padding;
    if(SIZE_MAX / max_element_count_ < stride) {
        ret = RING_QUEUE_OVERFLOW;
        ERROR_MESSAGE("ring_queue_create(%s) - Computed element stride is too large.", rslt_to_str(ret));
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

    tmp_queue->capacity = capacity;
    tmp_queue->element_align = element_align_;
    tmp_queue->element_size = element_size_;
    tmp_queue->head = 0;
    tmp_queue->len = 0;
    tmp_queue->max_element_count = max_element_count_;
    tmp_queue->padding = padding;
    tmp_queue->stride = stride;
    tmp_queue->tail = 0;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(tmp_queue)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_create(%s) - Postcondition validation failed for 'tmp_queue'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit
    *ring_queue_ = tmp_queue;
    tmp_queue = NULL;

    ret = RING_QUEUE_SUCCESS;

cleanup:
    if(NULL != tmp_queue) {
        if(NULL != tmp_queue->memory_pool) {    // TODO: 現状ではallocate以降でエラーを踏ませる経路がないためカバレッジは未達となる(TODO:処理後に対応)
            memory_system_free(tmp_queue->memory_pool, capacity, MEMORY_TAG_RING_QUEUE);
            tmp_queue->memory_pool = NULL;
        }
        memory_system_free(tmp_queue, sizeof(*tmp_queue), MEMORY_TAG_RING_QUEUE);
        tmp_queue = NULL;
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

ring_queue_result_t ring_queue_push(const void* data_, size_t element_size_, size_t element_align_, ring_queue_t* ring_queue_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* target_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "data_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_push(%s) - Precondition validation failed for 'ring_queue_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(ring_queue_->element_size != element_size_ || ring_queue_->element_align != element_align_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_push(%s) - Provided element_size_ or element_align_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

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

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_push(%s) - Postcondition validation failed for 'ring_queue_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

ring_queue_result_t ring_queue_pop(size_t element_size_, size_t element_align_, ring_queue_t* ring_queue_, void* data_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* head_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "data_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_pop(%s) - Precondition validation failed for 'ring_queue_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(ring_queue_->element_size != element_size_ || ring_queue_->element_align != element_align_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_pop(%s) - Provided element_size_ or element_align_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

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

    if(0 == ring_queue_->len) {
        ring_queue_->head = 0;
        ring_queue_->tail = 0;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_pop(%s) - Postcondition validation failed for 'ring_queue_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

bool ring_queue_empty(const ring_queue_t* ring_queue_) {
    if(NULL == ring_queue_) {
        return true;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(ring_queue_)) {
        ERROR_MESSAGE("ring_queue_empty(%s) - Precondition validation failed for 'ring_queue_'.", rslt_to_str(RING_QUEUE_DATA_CORRUPTED));
        return true;
    }
#endif
    if(0 != ring_queue_->len) {
        return false;
    } else {
        return true;
    }
}

bool ring_queue_is_valid(const ring_queue_t* ring_queue_) {
    size_t distance_to_end = 0;
    size_t expected_tail = 0;
    if(NULL == ring_queue_) {
        return false;
    }
    if(0 == ring_queue_->element_align || 0 == ring_queue_->max_element_count || 0 == ring_queue_->element_size || 0 == ring_queue_->stride || 0 == ring_queue_->capacity) {
        return false;
    }
    if(0 != (ring_queue_->stride % ring_queue_->element_align)) {
        return false;
    }
    if(ring_queue_->padding >= ring_queue_->element_align) {
        return false;
    }
    if((SIZE_MAX - ring_queue_->padding) < ring_queue_->element_size) {
        return false;
    }
    if(ring_queue_->stride != (ring_queue_->element_size + ring_queue_->padding)) {
        return false;
    }
    if(0 == ring_queue_->len && 0 != ring_queue_->head) {
        return false;
    }
    if(0 == ring_queue_->len && 0 != ring_queue_->tail) {
        return false;
    }
    if(alignof(max_align_t) < ring_queue_->element_align || !IS_POWER_OF_TWO(ring_queue_->element_align)) {
        return false;
    }
    if(ring_queue_->head >= ring_queue_->max_element_count) {
        return false;
    }
    if(ring_queue_->tail >= ring_queue_->max_element_count) {
        return false;
    }
    if(ring_queue_->len > ring_queue_->max_element_count) {
        return false;
    }
    if((SIZE_MAX / ring_queue_->stride) < ring_queue_->max_element_count) {
        return false;
    }
    if(ring_queue_->capacity != ring_queue_->stride * ring_queue_->max_element_count) {
        return false;
    }
    if(NULL == ring_queue_->memory_pool) {
        return false;
    }
    if(0 != ((uintptr_t)ring_queue_->memory_pool % ring_queue_->element_align)) {
        return false;
    }

    distance_to_end = ring_queue_->max_element_count - ring_queue_->head;
    if(ring_queue_->len < distance_to_end) {
        expected_tail = ring_queue_->head + ring_queue_->len;
    } else {
        expected_tail = ring_queue_->len - distance_to_end;
    }
    if(ring_queue_->tail != expected_tail) {
        return false;
    }

    return true;
}

/**
 * @brief メモリシステム実行結果コードをリングキュー実行結果コードに変換する
 *
 * @param[in] rslt_ メモリシステム実行結果コード
 * @return ring_queue_result_t 変換されたリングキュー実行結果コード
 */
static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RING_QUEUE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RING_QUEUE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RING_QUEUE_NO_MEMORY;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RING_QUEUE_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RING_QUEUE_BAD_OPERATION;
    default:
        return RING_QUEUE_UNDEFINED_ERROR;
    }
}

/**
 * @brief リングキュー実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ リングキュー実行結果コード
 * @return const char* 変換された文字列
 */
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
    case RING_QUEUE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RING_QUEUE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RING_QUEUE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RING_QUEUE_OVERFLOW:
        return s_rslt_str_overflow;
    case RING_QUEUE_EMPTY:
        return s_rslt_str_empty;
    default:
        return s_rslt_str_undefined_error;
    }
}
