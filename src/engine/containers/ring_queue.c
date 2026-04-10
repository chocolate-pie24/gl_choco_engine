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
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "engine/containers/test_ring_queue.h"
#include "engine/core/memory/test_choco_memory.h"

// ring_queueモジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_ring_queue_create;     /**< ring_queue_create()テスト設定 */
static test_call_control_t s_test_config_ring_queue_push;       /**< ring_queue_push()テスト設定 */
static test_call_control_t s_test_config_ring_queue_pop;        /**< ring_queue_pop()テスト設定 */
static test_call_control_bool_t s_test_config_ring_queue_empty; /**< ring_queue_empty()テスト設定 */

// プライベート関数テスト設定
static test_call_control_bool_t s_test_config_is_ring_queue_corrupted;  /**< is_ring_queue_corrupted()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_ring_queue_create(void);
static void test_ring_queue_destroy(void);
static void test_ring_queue_push(void);
static void test_ring_queue_pop(void);
static void test_ring_queue_empty(void);
static void test_rslt_convert_mem_sys(void);
static void test_is_ring_queue_corrupted(void);
static void test_rslt_to_str(void);
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
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< リングキューAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< リングキューAPI実行結果コード(内部データ破損)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< リングキューAPI実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_empty = "EMPTY";                        /**< リングキューAPI実行結果コード(キューが空)に対応する文字列 */

static bool is_ring_queue_corrupted(const ring_queue_t* ring_queue_);
static const char* rslt_to_str(ring_queue_result_t rslt_);
static ring_queue_result_t rslt_convert_mem_sys(memory_system_result_t rslt_);

ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** ring_queue_) {
#ifdef TEST_BUILD
    s_test_config_ring_queue_create.call_count++;
    if(s_test_config_ring_queue_create.fail_on_call != 0) {
        if(s_test_config_ring_queue_create.call_count == s_test_config_ring_queue_create.fail_on_call) {
            return (ring_queue_result_t)s_test_config_ring_queue_create.forced_result;
        }
    }
#endif
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
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "*ring_queue_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_element_count_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "max_element_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != element_size_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(element_align_ > 0, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(element_align_), ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(element_align_ <= alignof(max_align_t), ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "element_align_")
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

ring_queue_result_t ring_queue_push(ring_queue_t* ring_queue_, const void* data_, size_t element_size_, size_t element_align_) {
#ifdef TEST_BUILD
    s_test_config_ring_queue_push.call_count++;
    if(s_test_config_ring_queue_push.fail_on_call != 0) {
        if(s_test_config_ring_queue_push.call_count == s_test_config_ring_queue_push.fail_on_call) {
            return (ring_queue_result_t)s_test_config_ring_queue_push.forced_result;
        }
    }
#endif
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* target_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "data_")
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "ring_queue_->memory_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_size == element_size_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_align == element_align_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "element_align_")
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

ring_queue_result_t ring_queue_pop(ring_queue_t* ring_queue_, void* data_, size_t element_size_, size_t element_align_) {
#ifdef TEST_BUILD
    s_test_config_ring_queue_pop.call_count++;
    if(s_test_config_ring_queue_pop.fail_on_call != 0) {
        if(s_test_config_ring_queue_pop.call_count == s_test_config_ring_queue_pop.fail_on_call) {
            return (ring_queue_result_t)s_test_config_ring_queue_pop.forced_result;
        }
    }
#endif
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
    char* mem_ptr = NULL;
    char* head_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "ring_queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "data_")
    IF_ARG_NULL_GOTO_CLEANUP(ring_queue_->memory_pool, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "ring_queue_->memory_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_size == element_size_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "element_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(ring_queue_->element_align == element_align_, ret, RING_QUEUE_INVALID_ARGUMENT, rslt_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "element_align_")
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

bool ring_queue_empty(const ring_queue_t* ring_queue_) {
#ifdef TEST_BUILD
    s_test_config_ring_queue_empty.call_count++;
    if(s_test_config_ring_queue_empty.fail_on_call != 0) {
        if(s_test_config_ring_queue_empty.call_count == s_test_config_ring_queue_empty.fail_on_call) {
            return s_test_config_ring_queue_empty.forced_result;
        }
    }
#endif
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
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return RING_QUEUE_RUNTIME_ERROR;
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
 * @brief リングキュー内部データが破損しているかを判定する
 *
 * @warning 本関数は内部データ破損判定が目的であるため下記のチェックは行わない
 * - 引数ring_queue_のNULLチェック
 *
 * @param[in] ring_queue_ 判定対象リングキュー
 *
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
#ifdef TEST_BUILD
    s_test_config_is_ring_queue_corrupted.call_count++;
    if(s_test_config_is_ring_queue_corrupted.fail_on_call != 0) {
        if(s_test_config_is_ring_queue_corrupted.call_count == s_test_config_is_ring_queue_corrupted.fail_on_call) {
            return s_test_config_is_ring_queue_corrupted.forced_result;
        }
    }
#endif
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

#ifdef TEST_BUILD
void test_ring_queue_create_config_set(const test_call_control_t* config_) {
    s_test_config_ring_queue_create.fail_on_call = config_->fail_on_call;
    s_test_config_ring_queue_create.forced_result = config_->forced_result;
}

void test_ring_queue_push_config_set(const test_call_control_t* config_) {
    s_test_config_ring_queue_push.fail_on_call = config_->fail_on_call;
    s_test_config_ring_queue_push.forced_result = config_->forced_result;
}

void test_ring_queue_pop_config_set(const test_call_control_t* config_) {
    s_test_config_ring_queue_pop.fail_on_call = config_->fail_on_call;
    s_test_config_ring_queue_pop.forced_result = config_->forced_result;
}

void test_ring_queue_empty_config_set(const test_call_control_bool_t* config_) {
    s_test_config_ring_queue_empty.fail_on_call = config_->fail_on_call;
    s_test_config_ring_queue_empty.forced_result = config_->forced_result;
}

void test_ring_queue_config_reset(void) {
    test_call_control_reset(&s_test_config_ring_queue_create);
    test_call_control_reset(&s_test_config_ring_queue_push);
    test_call_control_reset(&s_test_config_ring_queue_pop);
    test_call_control_bool_reset(&s_test_config_ring_queue_empty);

    test_call_control_bool_reset(&s_test_config_is_ring_queue_corrupted);
}

void test_ring_queue(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_ring_queue_create();
    test_ring_queue_destroy();
    test_ring_queue_push();
    test_ring_queue_pop();
    test_ring_queue_empty();
    test_rslt_convert_mem_sys();
    test_is_ring_queue_corrupted();
    test_rslt_to_str();

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_ring_queue_create(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // ring_queue_create() 冒頭で強制的に RING_QUEUE_NO_MEMORY を返させる
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RING_QUEUE_NO_MEMORY;
        test_ring_queue_create_config_set(&config);

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), NULL);
        assert(RING_QUEUE_NO_MEMORY == ret);

        test_ring_queue_config_reset();
    }
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), NULL);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        // *ring_queue_ != NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t dummy = {0};
        ring_queue_t* ring_queue = &dummy;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(&dummy == ring_queue);
    }
    {
        // max_element_count_ == 0 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(0U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_size_ == 0 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, 0U, alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_align_ == 0 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), 0U, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_align_ が 2 の冪乗ではない -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), 3U, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // element_align_ > alignof(max_align_t) -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        const size_t invalid_align = alignof(max_align_t) * 2U;

        ret = ring_queue_create(4U, sizeof(uint32_t), invalid_align, &ring_queue);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(NULL == ring_queue);
    }
    {
        // SIZE_MAX / element_size_ < max_element_count_ -> RING_QUEUE_OVERFLOW
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(2U, SIZE_MAX, 1U, &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
        assert(NULL == ring_queue);
    }
    {
        // (SIZE_MAX - padding) < element_size_ -> RING_QUEUE_OVERFLOW
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(1U, (SIZE_MAX - 3U), 8U, &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
        assert(NULL == ring_queue);
    }
    {
        // SIZE_MAX / max_element_count_ < stride -> RING_QUEUE_OVERFLOW
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        const size_t element_size = (SIZE_MAX / 2U);

        ret = ring_queue_create(2U, element_size, 8U, &ring_queue);
        assert(RING_QUEUE_OVERFLOW == ret);
        assert(NULL == ring_queue);
    }
    {
        // 依存先 memory_system_allocate() の1回目を失敗させる
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        test_call_control_t config = {0};

        test_ring_queue_config_reset();
        test_choco_memory_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_NO_MEMORY == ret);
        assert(NULL == ring_queue);

        test_choco_memory_config_reset();
    }
    {
        // 依存先 memory_system_allocate() の2回目を失敗させる
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        test_call_control_t config = {0};

        test_ring_queue_config_reset();
        test_choco_memory_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 2U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_NO_MEMORY == ret);
        assert(NULL == ring_queue);

        test_choco_memory_config_reset();
    }
    {
        // 正常系（padding == 0）
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        test_ring_queue_config_reset();
        test_choco_memory_config_reset();

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);
        assert(alignof(uint32_t) == ring_queue->element_align);
        assert(4U == ring_queue->max_element_count);
        assert(0U == ring_queue->padding);
        assert(sizeof(uint32_t) == ring_queue->element_size);
        assert(sizeof(uint32_t) == ring_queue->stride);
        assert(sizeof(uint32_t) * 4U == ring_queue->capacity);
        assert(NULL != ring_queue->memory_pool);
        assert(0U == ((uintptr_t)ring_queue->memory_pool % alignof(uint32_t)));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系（padding != 0）
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        test_ring_queue_config_reset();
        test_choco_memory_config_reset();

        ret = ring_queue_create(3U, 1U, 4U, &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);
        assert(4U == ring_queue->element_align);
        assert(3U == ring_queue->max_element_count);
        assert(3U == ring_queue->padding);
        assert(1U == ring_queue->element_size);
        assert(4U == ring_queue->stride);
        assert(12U == ring_queue->capacity);
        assert(NULL != ring_queue->memory_pool);
        assert(0U == ((uintptr_t)ring_queue->memory_pool % 4U));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_ring_queue_destroy(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // ring_queue_ == NULL -> no-op
        ring_queue_destroy(NULL);
    }
    {
        // *ring_queue_ == NULL -> no-op
        ring_queue_t* ring_queue = NULL;

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(NULL != ring_queue->memory_pool);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 連続 destroy -> no-op
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 内部データ破損状態でも ring_queue 本体は破棄され、呼び出し元のポインタは NULL になる
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret_mem = memory_system_allocate(sizeof(ring_queue_t), MEMORY_TAG_RING_QUEUE, (void**)&ring_queue);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != ring_queue);
        memset(ring_queue, 0, sizeof(ring_queue_t));

        ring_queue->head = 0U;
        ring_queue->tail = 0U;
        ring_queue->len = 0U;
        ring_queue->element_align = 1U;
        ring_queue->max_element_count = 1U;
        ring_queue->padding = 0U;
        ring_queue->element_size = 1U;
        ring_queue->stride = 1U;
        ring_queue->capacity = 1U;
        ring_queue->memory_pool = NULL;   // is_ring_queue_corrupted() が true になる条件

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_ring_queue_push(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // ring_queue_push() 冒頭で強制的に RING_QUEUE_RUNTIME_ERROR を返させる
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        uint32_t value = 123U;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RING_QUEUE_RUNTIME_ERROR;
        test_ring_queue_push_config_set(&config);

        ret = ring_queue_push(NULL, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_RUNTIME_ERROR == ret);

        test_ring_queue_config_reset();
    }
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        uint32_t value = 123U;

        ret = ring_queue_push(NULL, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        // data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, NULL, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // ring_queue_->memory_pool == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        void* saved_memory_pool = NULL;
        uint32_t value = 123U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(NULL != ring_queue->memory_pool);

        saved_memory_pool = ring_queue->memory_pool;
        ring_queue->memory_pool = NULL;

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue->memory_pool = saved_memory_pool;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_size_ 不一致 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 123U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint16_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_align_ 不一致 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 123U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint16_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // is_ring_queue_corrupted() が true を返す -> RING_QUEUE_DATA_CORRUPTED
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 123U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        test_ring_queue_config_reset();

        s_test_config_is_ring_queue_corrupted.fail_on_call = 1U;
        s_test_config_is_ring_queue_corrupted.forced_result = true;

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_DATA_CORRUPTED == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        test_ring_queue_config_reset();

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系: 1件 push
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 0x12345678U;
        uint32_t stored = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(0U == ring_queue->head);
        assert(1U == ring_queue->tail);
        assert(1U == ring_queue->len);

        memcpy(&stored, ring_queue->memory_pool, sizeof(uint32_t));
        assert(value == stored);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系: 複数件 push
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value1 = 10U;
        uint32_t value2 = 20U;
        uint32_t stored1 = 0U;
        uint32_t stored2 = 0U;
        char* mem_ptr = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value1, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value2, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(0U == ring_queue->head);
        assert(2U == ring_queue->tail);
        assert(2U == ring_queue->len);

        mem_ptr = (char*)ring_queue->memory_pool;
        memcpy(&stored1, mem_ptr + (ring_queue->stride * 0U), sizeof(uint32_t));
        memcpy(&stored2, mem_ptr + (ring_queue->stride * 1U), sizeof(uint32_t));

        assert(value1 == stored1);
        assert(value2 == stored2);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // full 状態で push -> 最古要素を上書きし、head を進める
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value1 = 10U;
        uint32_t value2 = 20U;
        uint32_t value3 = 30U;
        uint32_t stored0 = 0U;
        uint32_t stored1 = 0U;
        char* mem_ptr = NULL;

        ret = ring_queue_create(2U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value1, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value2, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(2U == ring_queue->len);

        ret = ring_queue_push(ring_queue, &value3, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(1U == ring_queue->head);
        assert(1U == ring_queue->tail);
        assert(2U == ring_queue->len);

        mem_ptr = (char*)ring_queue->memory_pool;
        memcpy(&stored0, mem_ptr + (ring_queue->stride * 0U), sizeof(uint32_t));
        memcpy(&stored1, mem_ptr + (ring_queue->stride * 1U), sizeof(uint32_t));

        assert(value3 == stored0);
        assert(value2 == stored1);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_ring_queue_pop(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // ring_queue_pop() 冒頭で強制的に RING_QUEUE_RUNTIME_ERROR を返させる
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        uint32_t out = 0U;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RING_QUEUE_RUNTIME_ERROR;
        test_ring_queue_pop_config_set(&config);

        ret = ring_queue_pop(NULL, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_RUNTIME_ERROR == ret);
        assert(0U == out);

        test_ring_queue_config_reset();
    }
    {
        // ring_queue_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        uint32_t out = 0U;

        ret = ring_queue_pop(NULL, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == out);
    }
    {
        // data_ == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_pop(ring_queue, NULL, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // ring_queue_->memory_pool == NULL -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        void* saved_memory_pool = NULL;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(NULL != ring_queue->memory_pool);

        saved_memory_pool = ring_queue->memory_pool;
        ring_queue->memory_pool = NULL;

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue->memory_pool = saved_memory_pool;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_size_ 不一致 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint16_t), alignof(uint32_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_align_ 不一致 -> RING_QUEUE_INVALID_ARGUMENT
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint16_t));
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
        assert(0U == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // is_ring_queue_corrupted() が true を返す -> RING_QUEUE_DATA_CORRUPTED
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        test_ring_queue_config_reset();

        s_test_config_is_ring_queue_corrupted.fail_on_call = 1U;
        s_test_config_is_ring_queue_corrupted.forced_result = true;

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_DATA_CORRUPTED == ret);
        assert(0U == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        test_ring_queue_config_reset();

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 空キュー -> RING_QUEUE_EMPTY
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t out = 0xFFFFFFFFU;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_EMPTY == ret);
        assert(0xFFFFFFFFU == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系: 1件 pop して空になったら head/tail が 0 に戻る
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 0x12345678U;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(0U == ring_queue->head);
        assert(1U == ring_queue->tail);
        assert(1U == ring_queue->len);

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value == out);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系: 複数件 pop して FIFO を確認
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value1 = 10U;
        uint32_t value2 = 20U;
        uint32_t out1 = 0U;
        uint32_t out2 = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value1, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value2, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(0U == ring_queue->head);
        assert(2U == ring_queue->tail);
        assert(2U == ring_queue->len);

        ret = ring_queue_pop(ring_queue, &out1, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value1 == out1);
        assert(1U == ring_queue->head);
        assert(2U == ring_queue->tail);
        assert(1U == ring_queue->len);

        ret = ring_queue_pop(ring_queue, &out2, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value2 == out2);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系: padding を含む要素でも FIFO で取り出せる
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint8_t value1 = 0x11U;
        uint8_t value2 = 0x22U;
        uint8_t out1 = 0U;
        uint8_t out2 = 0U;

        ret = ring_queue_create(3U, sizeof(uint8_t), 4U, &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(3U == ring_queue->padding);
        assert(4U == ring_queue->stride);

        ret = ring_queue_push(ring_queue, &value1, sizeof(uint8_t), 4U);
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value2, sizeof(uint8_t), 4U);
        assert(RING_QUEUE_SUCCESS == ret);

        ret = ring_queue_pop(ring_queue, &out1, sizeof(uint8_t), 4U);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value1 == out1);

        ret = ring_queue_pop(ring_queue, &out2, sizeof(uint8_t), 4U);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value2 == out2);
        assert(0U == ring_queue->head);
        assert(0U == ring_queue->tail);
        assert(0U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // full 状態で上書き後に pop -> 最古要素ではなく次の要素が返る
        ring_queue_result_t ret = RING_QUEUE_SUCCESS;
        ring_queue_t* ring_queue = NULL;
        uint32_t value1 = 10U;
        uint32_t value2 = 20U;
        uint32_t value3 = 30U;
        uint32_t out = 0U;

        ret = ring_queue_create(2U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value1, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value2, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        ret = ring_queue_push(ring_queue, &value3, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);

        assert(1U == ring_queue->head);
        assert(1U == ring_queue->tail);
        assert(2U == ring_queue->len);

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value2 == out);
        assert(0U == ring_queue->head);
        assert(1U == ring_queue->tail);
        assert(1U == ring_queue->len);

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_ring_queue_empty(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // ring_queue_empty() 冒頭で強制的に false を返させる
        test_call_control_bool_t config = {0};

        test_call_control_bool_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = false;
        test_ring_queue_empty_config_set(&config);

        assert(false == ring_queue_empty(NULL));

        test_ring_queue_config_reset();
    }
    {
        // ring_queue_ == NULL -> true
        assert(true == ring_queue_empty(NULL));
    }
    {
        // ring_queue_->memory_pool == NULL -> true
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        void* saved_memory_pool = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(NULL != ring_queue->memory_pool);

        saved_memory_pool = ring_queue->memory_pool;
        ring_queue->memory_pool = NULL;

        assert(true == ring_queue_empty(ring_queue));

        ring_queue->memory_pool = saved_memory_pool;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len == 0 -> true
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);
        assert(0U == ring_queue->len);

        assert(true == ring_queue_empty(ring_queue));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len > 0 -> false
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 123U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(1U == ring_queue->len);

        assert(false == ring_queue_empty(ring_queue));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // push -> pop 後に空へ戻る -> true
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        uint32_t value = 456U;
        uint32_t out = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        ret = ring_queue_push(ring_queue, &value, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(false == ring_queue_empty(ring_queue));

        ret = ring_queue_pop(ring_queue, &out, sizeof(uint32_t), alignof(uint32_t));
        assert(RING_QUEUE_SUCCESS == ret);
        assert(value == out);
        assert(0U == ring_queue->len);

        assert(true == ring_queue_empty(ring_queue));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_rslt_convert_mem_sys(void) {
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_SUCCESS);
        assert(RING_QUEUE_SUCCESS == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_INVALID_ARGUMENT);
        assert(RING_QUEUE_INVALID_ARGUMENT == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_RUNTIME_ERROR);
        assert(RING_QUEUE_RUNTIME_ERROR == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_NO_MEMORY);
        assert(RING_QUEUE_NO_MEMORY == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_LIMIT_EXCEEDED);
        assert(RING_QUEUE_LIMIT_EXCEEDED == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys(MEMORY_SYSTEM_BAD_OPERATION);
        assert(RING_QUEUE_BAD_OPERATION == ret);
    }
    {
        ring_queue_result_t ret = rslt_convert_mem_sys((memory_system_result_t)999);
        assert(RING_QUEUE_UNDEFINED_ERROR == ret);
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_is_ring_queue_corrupted(void) {
    test_ring_queue_config_reset();
    test_choco_memory_config_reset();

    {
        // テスト基盤による強制返却
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        s_test_config_is_ring_queue_corrupted.fail_on_call = 1U;
        s_test_config_is_ring_queue_corrupted.forced_result = true;

        assert(true == is_ring_queue_corrupted(ring_queue));

        test_ring_queue_config_reset();

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 正常系
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);
        assert(NULL != ring_queue);

        assert(false == is_ring_queue_corrupted(ring_queue));

        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_align == 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved = ring_queue->element_align;
        ring_queue->element_align = 0U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->element_align = saved;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // max_element_count == 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved = ring_queue->max_element_count;
        ring_queue->max_element_count = 0U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->max_element_count = saved;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_size == 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved = ring_queue->element_size;
        ring_queue->element_size = 0U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->element_size = saved;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // stride == 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved = ring_queue->stride;
        ring_queue->stride = 0U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->stride = saved;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // capacity == 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved = ring_queue->capacity;
        ring_queue->capacity = 0U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->capacity = saved;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // stride % element_align != 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_stride = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_stride = ring_queue->stride;
        ring_queue->stride = 6U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->stride = saved_stride;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // padding >= element_align
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_padding = 0U;
        size_t saved_stride = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_padding = ring_queue->padding;
        saved_stride = ring_queue->stride;
        ring_queue->padding = ring_queue->element_align;
        ring_queue->stride = ring_queue->element_size + ring_queue->padding;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->padding = saved_padding;
        ring_queue->stride = saved_stride;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // stride != (element_size + padding)
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_padding = 0U;
        size_t saved_stride = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_padding = ring_queue->padding;
        saved_stride = ring_queue->stride;
        ring_queue->padding = 1U;
        ring_queue->stride = 8U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->padding = saved_padding;
        ring_queue->stride = saved_stride;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len == 0 && head != 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_head = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_head = ring_queue->head;
        ring_queue->head = 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->head = saved_head;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len == 0 && tail != 0
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_tail = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_tail = ring_queue->tail;
        ring_queue->tail = 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->tail = saved_tail;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // 0 < len < max_element_count && head == tail
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_head = 0U;
        size_t saved_tail = 0U;
        size_t saved_len = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_head = ring_queue->head;
        saved_tail = ring_queue->tail;
        saved_len = ring_queue->len;

        ring_queue->head = 1U;
        ring_queue->tail = 1U;
        ring_queue->len = 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->head = saved_head;
        ring_queue->tail = saved_tail;
        ring_queue->len = saved_len;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len == max_element_count && head != tail
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_head = 0U;
        size_t saved_tail = 0U;
        size_t saved_len = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_head = ring_queue->head;
        saved_tail = ring_queue->tail;
        saved_len = ring_queue->len;

        ring_queue->head = 0U;
        ring_queue->tail = 1U;
        ring_queue->len = ring_queue->max_element_count;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->head = saved_head;
        ring_queue->tail = saved_tail;
        ring_queue->len = saved_len;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // !IS_POWER_OF_TWO(element_align)
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_align = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_align = ring_queue->element_align;
        ring_queue->element_align = 3U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->element_align = saved_align;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // element_align > alignof(max_align_t)
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_align = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_align = ring_queue->element_align;
        ring_queue->element_align = alignof(max_align_t) * 2U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->element_align = saved_align;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // head >= max_element_count
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_head = 0U;
        size_t saved_tail = 0U;
        size_t saved_len = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_head = ring_queue->head;
        saved_tail = ring_queue->tail;
        saved_len = ring_queue->len;

        ring_queue->head = ring_queue->max_element_count;
        ring_queue->tail = 1U;
        ring_queue->len = 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->head = saved_head;
        ring_queue->tail = saved_tail;
        ring_queue->len = saved_len;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // tail >= max_element_count
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_head = 0U;
        size_t saved_tail = 0U;
        size_t saved_len = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_head = ring_queue->head;
        saved_tail = ring_queue->tail;
        saved_len = ring_queue->len;

        ring_queue->head = 0U;
        ring_queue->tail = ring_queue->max_element_count;
        ring_queue->len = 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->head = saved_head;
        ring_queue->tail = saved_tail;
        ring_queue->len = saved_len;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // len > max_element_count
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_len = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_len = ring_queue->len;
        ring_queue->len = ring_queue->max_element_count + 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->len = saved_len;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // capacity != stride * max_element_count
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        size_t saved_capacity = 0U;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_capacity = ring_queue->capacity;
        ring_queue->capacity = saved_capacity - 1U;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->capacity = saved_capacity;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }
    {
        // memory_pool == NULL
        ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
        ring_queue_t* ring_queue = NULL;
        void* saved_memory_pool = NULL;

        ret = ring_queue_create(4U, sizeof(uint32_t), alignof(uint32_t), &ring_queue);
        assert(RING_QUEUE_SUCCESS == ret);

        saved_memory_pool = ring_queue->memory_pool;
        ring_queue->memory_pool = NULL;

        assert(true == is_ring_queue_corrupted(ring_queue));

        ring_queue->memory_pool = saved_memory_pool;
        ring_queue_destroy(&ring_queue);
        assert(NULL == ring_queue);
    }

    test_ring_queue_config_reset();
    test_choco_memory_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* str = rslt_to_str(RING_QUEUE_SUCCESS);
        assert(NULL != str);
        assert(0 == strcmp("SUCCESS", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_INVALID_ARGUMENT);
        assert(NULL != str);
        assert(0 == strcmp("INVALID_ARGUMENT", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_NO_MEMORY);
        assert(NULL != str);
        assert(0 == strcmp("NO_MEMORY", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_RUNTIME_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("RUNTIME_ERROR", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_UNDEFINED_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_LIMIT_EXCEEDED);
        assert(NULL != str);
        assert(0 == strcmp("LIMIT_EXCEEDED", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_BAD_OPERATION);
        assert(NULL != str);
        assert(0 == strcmp("BAD_OPERATION", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_DATA_CORRUPTED);
        assert(NULL != str);
        assert(0 == strcmp("DATA_CORRUPTED", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_OVERFLOW);
        assert(NULL != str);
        assert(0 == strcmp("OVERFLOW", str));
    }
    {
        const char* str = rslt_to_str(RING_QUEUE_EMPTY);
        assert(NULL != str);
        assert(0 == strcmp("EMPTY", str));
    }
    {
        const char* str = rslt_to_str((ring_queue_result_t)999);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
}
#endif
