/** @ingroup ring_queue
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

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#endif

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
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< リングキューAPI実行結果コード(内部データ破損)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< リングキューAPI実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_empty = "EMPTY";                        /**< リングキューAPI実行結果コード(キューが空)に対応する文字列 */

static bool is_ring_queue_corrupted(const ring_queue_t* ring_queue_);
static const char* rslt_to_str(ring_queue_result_t rslt_);
static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_);

#ifdef TEST_BUILD
static void NO_COVERAGE test_rslt_to_str(void);
static void NO_COVERAGE test_rslt_convert_mem_sys(void);
static void NO_COVERAGE test_ring_queue_create(void);
static void NO_COVERAGE test_ring_queue_destroy(void);
static void NO_COVERAGE test_ring_queue_push(void);
static void NO_COVERAGE test_ring_queue_pop(void);
static void NO_COVERAGE test_is_ring_queue_corrupted(void);
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
// 0 == element_align_ -> RING_QUEUE_INVALID_ARGUMENT
// element_align_が2の冪乗ではない -> RING_QUEUE_INVALID_ARGUMENT
// element_align_がmax_align_tを超過 -> RING_QUEUE_INVALID_ARGUMENT
// tmp_queueのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
// tmp_queue->memory_poolのメモリ確保失敗 -> RING_QUEUE_NO_MEMORY
// element_size_ * max_element_count_がSIZE_MAXを超過 -> RING_QUEUE_OVERFLOW
// element_size_ + paddingがSIZE_MAXを超過 -> RING_QUEUE_OVERFLOW
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
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "ring_queue_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*ring_queue_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "*ring_queue_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_element_count_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "max_element_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != element_size_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(element_align_ > 0, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(element_align_), RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(element_align_ <= alignof(max_align_t), RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
    if(SIZE_MAX / element_size_ < max_element_count_) {
        ret = RING_QUEUE_OVERFLOW;
        ERROR_MESSAGE("ring_queue_create(%s) - Provided 'element_size_' and 'max_element_count_' are too large.", rslt_to_str(ret));
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
    mem_pool_ptr = (uintptr_t)tmp_queue->memory_pool;
    if(0 != (mem_pool_ptr % element_align_)) {  // TODO: memory_systemにmisalignするテスト用APIを追加し、ここを通過させるテストを追加する
        ret = RING_QUEUE_RUNTIME_ERROR;
        ERROR_MESSAGE("ring_queue_create(%s) - Allocated memory pool alignment is invalid.", rslt_to_str(ret));
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
            if(NULL != tmp_queue->memory_pool) {    // TODO: 現状ではallocate以降でエラーを踏ませる経路がないためカバレッジは未達となる(TODO:処理後に対応)
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
    if(!is_ring_queue_corrupted(*ring_queue_)) { // 内部データが破損していた場合にfreeするのは危険
        if(NULL != (*ring_queue_)->memory_pool) {
            memory_system_free((*ring_queue_)->memory_pool, (*ring_queue_)->capacity, MEMORY_TAG_RING_QUEUE);
            (*ring_queue_)->memory_pool = NULL;
        }
    } else {
        WARN_MESSAGE("ring_queue_destroy - Provided ring_queue_ is corrupted.");
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
// 内部データ破損(is_ring_queue_corrupted == true) -> RING_QUEUE_DATA_CORRUPTED
ring_queue_result_t ring_queue_push(ring_queue_t* ring_queue_, const void* data_, size_t element_size_, size_t element_align_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* target_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "data_")
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "ring_queue_->memory_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_size == element_size_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_align == element_align_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "element_align_")
    if(is_ring_queue_corrupted(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_push(%s) - Provided ring queue is corrupted.", rslt_to_str(ret));
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
// 内部データ破損(is_ring_queue_corrupted == true) -> RING_QUEUE_DATA_CORRUPTED
ring_queue_result_t ring_queue_pop(ring_queue_t* ring_queue_, void* data_, size_t element_size_, size_t element_align_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* head_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "data_")
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "ring_queue_->memory_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_size == element_size_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_align == element_align_, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "element_align_")
    if(is_ring_queue_corrupted(ring_queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_pop(%s) - Provided ring queue is corrupted.", rslt_to_str(ret));
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

/**
 * @brief エラー伝播のため、メモリシステム実行結果コードをリングキューAPI実行結果コードに変換する
 *
 * @param rslt_ メモリシステム実行結果コード
 * @return application_result_t 変換されたリングキュー実行結果コード
 */
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
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RING_QUEUE_LIMIT_EXCEEDED;
    default:
        return RING_QUEUE_UNDEFINED_ERROR;
    }
}

/**
 * @brief リングキュー内部データ破損判定
 *
 * @warning 本関数は内部データ破損判定が目的であるため下記のチェックは行わない
 * - 引数ring_queue_のNULLチェック
 *
 * @param ring_queue_ 判定対象リングキュー
 * @retval true 以下のいずれか
 * - 0 == ring_queue_->element_align
 * - 0 == ring_queue_->max_element_count
 * - 0 == ring_queue_->element_size
 * - 0 == ring_queue_->stride
 * - 0 == ring_queue_->capacity
 * - 0 != (ring_queue_->stride % ring_queue_->element_align)(1要素の格納に必要なメモリ領域は、格納要素のアライメント要件の倍数でなければアライメントできない)
 * - ring_queue_->padding >= ring_queue_->element_align(パディングサイズが要素のアライメント要件を超過することはない、最大でもアライメント要件-1)
 * - ring_queue_->stride != (ring_queue_->element_size + ring_queue_->padding)(1要素の格納に必要なメモリ領域が要素サイズ+パディングサイズではない)
 * - 0 == ring_queue_->len && 0 != ring_queue_->head(格納している要素数が0の時にheadが0になっていない)
 * - 0 == ring_queue_->len && 0 != ring_queue_->tail(格納している要素数が0の時にtailが0になっていない)
 * - (0 < ring_queue_->len && ring_queue_->max_element_count > ring_queue_->len) && ring_queue_->head == ring_queue_->tail(0 < len < max_element_countの時にhead==tailになるのはおかしい)
 * - ring_queue_->len == ring_queue_->max_element_count && ring_queue_->head != ring_queue_->tail(リングキューがfullの状態ではhead==tailになるはず)
 * - alignof(max_align_t) < ring_queue_->element_align || !IS_POWER_OF_TWO(ring_queue_->element_align)(アライメント要件が破損)
 * - ring_queue_->head >= ring_queue_->max_element_count
 * - ring_queue_->tail >= ring_queue_->max_element_count
 * - ring_queue_->len > ring_queue_->max_element_count
 * - ring_queue_->capacity != ring_queue_->stride * ring_queue_->max_element_count(メモリプール容量異常)
 * - ring_queue_->memory_pool == NULL(ring_queue_createでmemory_pool != NULLが保証されるため内部データ破損扱い)
 * @retval false 内部データ破損なし
 */
static bool is_ring_queue_corrupted(const ring_queue_t* ring_queue_) {
    if(0 == ring_queue_->element_align || 0 == ring_queue_->max_element_count || 0 == ring_queue_->element_size || 0 == ring_queue_->stride || 0 == ring_queue_->capacity) {
        return true;
    }
    if(0 != (ring_queue_->stride % ring_queue_->element_align)) {
        return true;
    }
    if(ring_queue_->padding >= ring_queue_->element_align) {
        return true;
    }
    if(ring_queue_->stride != (ring_queue_->element_size + ring_queue_->padding)) {
        return true;
    }
    if(0 == ring_queue_->len && 0 != ring_queue_->head) {
        return true;
    }
    if(0 == ring_queue_->len && 0 != ring_queue_->tail) {
        return true;
    }
    if((0 < ring_queue_->len && ring_queue_->max_element_count > ring_queue_->len) && ring_queue_->head == ring_queue_->tail) {
        return true;
    }
    if(ring_queue_->len == ring_queue_->max_element_count && ring_queue_->head != ring_queue_->tail) {
        return true;
    }
    if(alignof(max_align_t) < ring_queue_->element_align || !IS_POWER_OF_TWO(ring_queue_->element_align)) {
        return true;
    }
    if(ring_queue_->head >= ring_queue_->max_element_count) {
        return true;
    }
    if(ring_queue_->tail >= ring_queue_->max_element_count) {
        return true;
    }
    if(ring_queue_->len > ring_queue_->max_element_count) {
        return true;
    }
    if(ring_queue_->capacity != ring_queue_->stride * ring_queue_->max_element_count) {
        return true;
    }
    if(NULL == ring_queue_->memory_pool) {
        return true;
    }
    return false;
}

/**
 * @brief リングキュー実行結果コードを文字列に変換し出力する
 *
 * @param rslt_ リングキュー実行結果コード
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

#ifdef TEST_BUILD

void test_ring_queue(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_rslt_to_str();
    test_rslt_convert_mem_sys();
    test_ring_queue_create();
    test_ring_queue_destroy();
    test_ring_queue_push();
    test_ring_queue_pop();
    test_is_ring_queue_corrupted();
    test_ring_queue_empty();

    memory_system_destroy();
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
        // element_size_ * max_element_count_がSIZE_MAXを超過 -> RING_QUEUE_OVERFLOW
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(128, SIZE_MAX, 8, &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
        assert(NULL == ring_queue);
    }
    {
        // max_element_count_ * strideがSIZE_MAXを超過 -> RING_QUEUE_OVERFLOW
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(SIZE_MAX, 1, alignof(max_align_t), &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
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
    {
        // element_size_ + padding が SIZE_MAX を超過 -> RING_QUEUE_OVERFLOW
        // ※ max_element_count_=1 にすることで、先頭の「element_size_ * max_element_count_」オーバーフロー検出を回避し、
        //    padding を 1 以上にして「(SIZE_MAX - padding) < element_size_」分岐を踏む。
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(1, SIZE_MAX, 8, &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
        assert(NULL == ring_queue);
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
        assert(MEMORY_SYSTEM_SUCCESS == memory_system_allocate(sizeof(*ring_queue), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue));
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
        assert(MEMORY_SYSTEM_SUCCESS == memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue));
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
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(0 == ring_queue->tail);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_EMPTY == ret);
        assert(5 == a);
        assert(0 == ring_queue->head);
        assert(0 == ring_queue->len);
        assert(0 == ring_queue->tail);

        ring_queue_destroy(&ring_queue);
    }
    {
        // 内部データ破損(is_ring_queue_corrupted == true) -> RING_QUEUE_DATA_CORRUPTED
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        const ring_queue_t backup = *ring_queue;

        // 破損状態を作る（preconditionは満たす：memory_poolは生きたまま）
        ring_queue->len = ring_queue->max_element_count + 1;

        int a = 123;
        ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_DATA_CORRUPTED == ret);

        // destroyでリークしないよう復元してから破棄
        *ring_queue = backup;
        ring_queue_destroy(&ring_queue);
    }
}

static void NO_COVERAGE test_ring_queue_pop(void) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    {
        int a = 0;
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ret = ring_queue_pop(NULL, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        int a = 0;
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
        int a = 0;
        // ring_queue->memory_pool == NULL ->RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        assert(MEMORY_SYSTEM_SUCCESS == memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue));
        memset(ring_queue, 0, sizeof(ring_queue_t));

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        memory_system_free(ring_queue, sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE);
    }
    {
        int a = 0;
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
        int a = 0;
        // ring_queue_->element_align != element_align_ -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(char));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    {
        int a = 0;
        // ring_queueが空でRING_QUEUE_EMPTY
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_EMPTY == ret);
        assert(0 == a);

        ring_queue_destroy(&ring_queue);
    }
    {
        // 内部データ破損(is_ring_queue_corrupted == true) -> RING_QUEUE_DATA_CORRUPTED
        ring_queue_t* ring_queue = NULL;
        ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        const ring_queue_t backup = *ring_queue;

        // 破損状態を作る（preconditionは満たす：memory_poolは生きたまま）
        ring_queue->len = ring_queue->max_element_count + 1;

        int a = 0;
        ret = ring_queue_pop(ring_queue, &a, sizeof(int), alignof(int));
        assert(RING_QUEUE_DATA_CORRUPTED == ret);
        assert(0 == a); // pop前に書き換えられないことの確認（任意）

        // destroyでリークしないよう復元してから破棄
        *ring_queue = backup;
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
        assert(MEMORY_SYSTEM_SUCCESS == memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue));
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
        assert(RING_QUEUE_SUCCESS == ring_queue_push(ring_queue, &a, sizeof(int), alignof(int)));
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
        const char* tmp = rslt_to_str(RING_QUEUE_LIMIT_EXCEEDED);
        assert(0 == strcmp(tmp, s_rslt_str_limit_exceeded));
        DEBUG_MESSAGE("test_rslt_to_str - message limit exceeded: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_DATA_CORRUPTED);
        assert(0 == strcmp(tmp, s_rslt_str_data_corrupted));
        DEBUG_MESSAGE("test_rslt_to_str - message data coruupted: %s", tmp);
    }
    {
        const char* tmp = rslt_to_str(RING_QUEUE_OVERFLOW);
        assert(0 == strcmp(tmp, s_rslt_str_overflow));
        DEBUG_MESSAGE("test_rslt_to_str - message overflow: %s", tmp);
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

static void NO_COVERAGE test_is_ring_queue_corrupted(void) {
    ring_queue_t* rq = NULL;

    assert(RING_QUEUE_SUCCESS == ring_queue_create(8, sizeof(int), alignof(int), &rq));
    assert(false == is_ring_queue_corrupted(rq));

    const ring_queue_t backup = *rq;

    // element_align == 0
    rq->element_align = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // max_element_count == 0
    rq->max_element_count = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // element_size == 0
    rq->element_size = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // stride == 0
    rq->stride = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // capacity == 0
    rq->capacity = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // stride % element_align != 0
    rq->stride = backup.stride + 1;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // padding >= element_align
    rq->padding = backup.element_align;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // stride != element_size + padding  (modは通す)
    rq->stride = backup.stride + backup.element_align;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // len == 0 && head != 0
    rq->head = 1;
    rq->len = 0;
    rq->tail = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // len == 0 && tail != 0
    rq->tail = 1;
    rq->len = 0;
    rq->head = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // 0 < len < max && head == tail
    rq->len = 1;
    rq->head = 0;
    rq->tail = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // len == max && head != tail
    rq->len = backup.max_element_count;
    rq->head = 0;
    rq->tail = 1;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // element_align is not power-of-two (この分岐に到達させるために整合するよう調整)
    rq->element_align = 3;
    rq->padding = 2;
    rq->stride = rq->element_size + rq->padding; // 4 + 2 = 6
    assert(0 == (rq->stride % rq->element_align)); // 念のため
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // head >= max
    rq->len = 1;
    rq->head = backup.max_element_count;
    rq->tail = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // tail >= max
    rq->len = 1;
    rq->head = 0;
    rq->tail = backup.max_element_count;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // len > max
    rq->len = backup.max_element_count + 1;
    rq->head = 0;
    rq->tail = 0;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    // capacity mismatch (最後のチェックを踏ませる)
    rq->capacity = backup.capacity + 1;
    assert(true == is_ring_queue_corrupted(rq));
    *rq = backup;

    {
        void* saved_pool = rq->memory_pool;
        rq->memory_pool = NULL;
        assert(is_ring_queue_corrupted(rq));
        rq->memory_pool = saved_pool;
    }

    ring_queue_destroy(&rq);
    assert(NULL == rq);
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

    ret = rslt_convert_mem_sys(MEMORY_SYSTEM_LIMIT_EXCEEDED);
    assert(RING_QUEUE_LIMIT_EXCEEDED == ret);

    ret = rslt_convert_mem_sys(1024);
    assert(RING_QUEUE_UNDEFINED_ERROR == ret);
}

#endif
