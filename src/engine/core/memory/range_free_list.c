#include "engine/core/memory/range_free_list.h"

#include <stdlib.h> // for malloc / free
#include <string.h> // for memset
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief 空き領域ノード状態定義
 *
 * @note 状態遷移仕様
 * - acquire() : NOT_USED -> NOT_CONNECTED
 * - insert()  : NOT_CONNECTED -> CONNECTED
 * - remove()  : CONNECTED -> NOT_CONNECTED
 * - release() : NOT_CONNECTED -> NOT_USED
 */
typedef enum {
    NODE_STATE_NOT_USED,        /**< node_poolにはいるが, どの範囲管理にも使われていない */
    NODE_STATE_CONNECTED,       /**< 空き範囲nodeとしてfree_block_listに接続されている */
    NODE_STATE_NOT_CONNECTED,   /**< node_poolから借用中だが, free_block_listには接続されていない */
} node_state_t;

/**
 * @brief 各空き領域を管理するノード構造体
 *
 */
typedef struct node {
    struct node* next;  /**< 次の空き領域ノードへのポインタ */
    struct node* prev;  /**< 前の空き領域ノードへのポインタ */

    size_t block_size;  /**< 空き領域サイズ(byte) */
    size_t offset;      /**< この空き領域の開始オフセット(byte) */

    node_state_t state; /**< ノード状態 */
} node_t;

/**
 * @brief Range Free List 内部状態管理構造体
 *
 */
struct range_free_list {
    size_t memory_pool_size;    /**< メモリプール容量(byte) */
    size_t max_node_count;      /**< 最大ノード数(個) */
    size_t unused_node_count;   /**< 未使用ノード数 */

    size_t base_align;          /**< メモリプールの先頭アドレスのアライメント */

    node_t** node_pool;             /**< range_free_listが所有する全ノードへのポインタ配列 */
    node_t* free_block_list_head;   /**< 空き領域を繋いだ双方向リストの先頭ノードで, node_pool配列の要素 */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コードRANGE_FREE_LIST_SUCCESS文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードRANGE_FREE_LIST_INVALID_ARGUMENT文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コードRANGE_FREE_LIST_LIMIT_EXCEEDED文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コードRANGE_FREE_LIST_NO_MEMORY文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コードRANGE_FREE_LIST_DATA_CORRUPTED文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コードRANGE_FREE_LIST_BAD_OPERATION文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コードRANGE_FREE_LIST_OVERFLOW文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードRANGE_FREE_LIST_UNDEFINED_ERROR文字列 */

// allocation関連
static range_free_list_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_);
static range_free_list_result_t find_first_fit_node(const range_free_list_t* range_free_list_, size_t allocation_size_, node_t** out_node_);
static range_free_list_result_t allocate_from_node(range_free_list_t* range_free_list_, node_t* node_, size_t allocation_size_, size_t* out_offset_);
static range_free_list_result_t node_remove(range_free_list_t* range_free_list_, node_t* node_);
static range_free_list_result_t node_release(range_free_list_t* range_free_list_, node_t* node_);

// free関連
static range_free_list_result_t find_free_block_insert_position(const range_free_list_t* range_free_list_, size_t offset_, size_t free_size_, node_t** out_prev_node_, node_t** out_next_node_);
static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, size_t offset_, size_t block_size_, node_t** out_node_);
static range_free_list_result_t node_insert_between(range_free_list_t* range_free_list_, node_t* insert_node_, node_t* prev_, node_t* next_);
static range_free_list_result_t node_adjacent_check_prev(const node_t* node_, bool* out_is_adjacent_);
static range_free_list_result_t node_adjacent_check_next(const node_t* node_, bool* out_is_adjacent_);
static range_free_list_result_t merge_free_block(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_);

// validation
static bool range_free_list_is_valid(const range_free_list_t* range_free_list_);
static bool node_is_valid(const node_t* node_);
static bool check_range_relation(size_t prev_offset_, size_t prev_block_size_, size_t next_offset_);

// その他ヘルパー
static void set_node_to_not_used(node_t* target_);
static void set_node_to_not_connected(node_t* target_, size_t offset_, size_t block_size_);
static void set_node_to_connected(node_t* target_, node_t* prev_, node_t* next_);
static const char* rslt_to_str(range_free_list_result_t rslt_);

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, size_t base_align_, range_free_list_t** out_range_free_list_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    range_free_list_t* tmp_range_free_list = NULL;
    node_t** tmp_node_pool = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "out_range_free_list_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "*out_range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != memory_pool_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "memory_pool_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_node_count_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "max_node_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_create", "base_align_")

    tmp_range_free_list = (range_free_list_t*)malloc(sizeof(range_free_list_t));
    if(NULL == tmp_range_free_list) {
        ret = RANGE_FREE_LIST_NO_MEMORY;
        ERROR_MESSAGE("range_free_list_create(%s) - Failed to allocate range_free_list_t instance.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_range_free_list, 0, sizeof(range_free_list_t));

    tmp_node_pool = (node_t**)malloc(sizeof(node_t*) * max_node_count_);
    if(NULL == tmp_node_pool) {
        ret = RANGE_FREE_LIST_NO_MEMORY;
        ERROR_MESSAGE("range_free_list_create(%s) - Failed to allocate node_t* array instance.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_node_pool, 0, sizeof(node_t*) * max_node_count_);

    for(size_t i = 0; i != max_node_count_; ++i) {
        tmp_node_pool[i] = (node_t*)malloc(sizeof(node_t));
        if(NULL == tmp_node_pool[i]) {
            ret = RANGE_FREE_LIST_NO_MEMORY;
            ERROR_MESSAGE("range_free_list_create(%s) - Failed to allocate node_t instance.", rslt_to_str(ret));
            goto cleanup;
        }
        memset(tmp_node_pool[i], 0, sizeof(node_t));
        tmp_node_pool[i]->state = NODE_STATE_NOT_USED;
    }

    tmp_range_free_list->max_node_count = max_node_count_;
    tmp_range_free_list->memory_pool_size = memory_pool_size_;
    tmp_range_free_list->unused_node_count = max_node_count_ - 1;
    tmp_range_free_list->base_align = base_align_;

    tmp_range_free_list->free_block_list_head = tmp_node_pool[0];
    tmp_range_free_list->free_block_list_head->block_size = memory_pool_size_;
    tmp_range_free_list->free_block_list_head->offset = 0;
    tmp_range_free_list->free_block_list_head->next = NULL;
    tmp_range_free_list->free_block_list_head->prev = NULL;

    tmp_node_pool[0]->state = NODE_STATE_CONNECTED;

    tmp_range_free_list->node_pool = tmp_node_pool;

    *out_range_free_list_ = tmp_range_free_list;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        if(NULL != tmp_node_pool) {
            for(size_t i = 0; i != max_node_count_; ++i) {
                if(NULL != tmp_node_pool[i]) {
                    free(tmp_node_pool[i]);
                    tmp_node_pool[i] = NULL;
                }
            }
            free(tmp_node_pool);
            tmp_node_pool = NULL;
        }
        if(NULL != tmp_range_free_list) {
            free(tmp_range_free_list);
            tmp_range_free_list = NULL;
        }
    }
    return ret;
}

void range_free_list_destroy(range_free_list_t** range_free_list_) {
    if(NULL == range_free_list_) {
        return;
    }
    if(NULL == *range_free_list_) {
        return;
    }


    for(size_t i = 0; i != (*range_free_list_)->max_node_count; ++i) {
        if(NULL != (*range_free_list_)->node_pool[i]) {
            free((*range_free_list_)->node_pool[i]);
            (*range_free_list_)->node_pool[i] = NULL;
        }
    }
    free((*range_free_list_)->node_pool);
    (*range_free_list_)->node_pool = NULL;

    (*range_free_list_)->max_node_count = 0;
    (*range_free_list_)->memory_pool_size = 0;

    free(*range_free_list_);
    *range_free_list_ = NULL;
}

range_free_list_result_t range_free_list_allocate(range_free_list_t* range_free_list_, size_t required_size_, size_t required_align_, size_t* out_offset_, size_t* out_allocated_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t allocation_size = 0;
    size_t tmp_offset = 0;
    node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "required_align_")
    IF_ARG_NULL_GOTO_CLEANUP(out_offset_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "out_offset_")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocated_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "out_allocated_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->base_align == required_align_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_allocate", "required_align_")

    if(!range_free_list_is_valid(range_free_list_)) {   // TODO: この処理は動作実績ができたらRELEASE_BUILDでのチェックを軽めにする
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("range_free_list_allocate(%s) - range_free_list_ corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = align_up(range_free_list_->base_align, required_size_, &allocation_size);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Range free list allocation failed. reason=align_up failed. base_align=%zu, required_size=%zu.", rslt_to_str(ret), range_free_list_->base_align, required_size_);
        goto cleanup;
    }

    ret = find_first_fit_node(range_free_list_, allocation_size, &node);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Range free list allocation failed. reason=find_first_fit_node failed. required_size=%zu.", rslt_to_str(ret), required_size_);
        goto cleanup;
    }

    ret = allocate_from_node(range_free_list_, node, allocation_size, &tmp_offset);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Range free list allocation failed. reason=allocate_from_node failed. allocation_size=%zu.", rslt_to_str(ret), allocation_size);
        goto cleanup;
    }

    *out_offset_ = tmp_offset;
    *out_allocated_size_ = allocation_size;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

range_free_list_result_t range_free_list_free(range_free_list_t* range_free_list_, size_t offset_, size_t allocation_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* new_node = NULL;
    node_t* prev = NULL;
    node_t* next = NULL;

    bool should_merge_prev = false;
    bool should_merge_next = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_free", "range_free_list_")

    if(!range_free_list_is_valid(range_free_list_)) {   // TODO: この処理は動作実績ができたらRELEASE_BUILDでのチェックを軽めにする
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_ corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = find_free_block_insert_position(range_free_list_, offset_, allocation_size_, &prev, &next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = node_acquire(range_free_list_, offset_, allocation_size_, &new_node);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = node_insert_between(range_free_list_, new_node, prev, next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = node_adjacent_check_prev(new_node, &should_merge_prev);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = node_adjacent_check_next(new_node, &should_merge_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = merge_free_block(range_free_list_, new_node, should_merge_prev, should_merge_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// 要求サイズを満たす最初の空き領域ノードをfree_block_list_headから探索する。
// 探索方式: first-fit
// allocation_sizeにはoffsetがbase_alignになるよう調整されたrequired_size + paddingの容量を渡すこと
static range_free_list_result_t find_first_fit_node(const range_free_list_t* range_free_list_, size_t allocation_size_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "allocation_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->free_block_list_head, ret, RANGE_FREE_LIST_NO_MEMORY, rslt_to_str(RANGE_FREE_LIST_NO_MEMORY), "find_first_fit_node", "range_free_list_->free_block_list_head")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != range_free_list_->base_align, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "find_first_fit_node", "range_free_list_->base_align")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(range_free_list_->base_align), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "find_first_fit_node", "range_free_list_->base_align")

    node = range_free_list_->free_block_list_head;
    while(NULL != node) {
        if(0 != (node->offset % range_free_list_->base_align)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first_fit_node. reason=offset is not aligned to base_align.", rslt_to_str(ret));
            goto cleanup;
        }
        if(node->block_size >= allocation_size_) {
            found = true;
            break;
        } else {
            node = node->next;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_NO_MEMORY;
        ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first_fit_node. reason=required free space size could not be found. allocation_size=%zu", rslt_to_str(ret), allocation_size_);
        goto cleanup;
    }
    *out_node_ = node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// 対象nodeからallocation_size分を確保する
// out_offsetに確保開始offsetを返す
// nodeに残り領域がある場合はoffset/block_sizeを更新する
// もしnodeを完全に使い切ったらnode_remove + node_releaseする
// node_removeもしくはnode_releaseが失敗した場合はnode_の状態は変化している場合がある
static range_free_list_result_t allocate_from_node(range_free_list_t* range_free_list_, node_t* node_, size_t allocation_size_, size_t* out_offset_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t escape_offset = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "allocate_from_node", "node_->state")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "allocation_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_offset_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "out_offset_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_->block_size >= allocation_size_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "allocate_from_node", "allocation_size_")

    escape_offset = node_->offset;

    if(node_->block_size == allocation_size_) {
        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate from node. reason=node_remove failed.", rslt_to_str(ret));
            goto cleanup;
        }

        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate from node. reason=node_release failed.", rslt_to_str(ret));
            goto cleanup;
        }
    } else {
        if((SIZE_MAX - allocation_size_) < node_->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate from node. reason=overflow. allocated_size=%zu, offset=%zu.", rslt_to_str(ret), allocation_size_, node_->offset);
            goto cleanup;
        }
        node_->block_size -= allocation_size_;
        node_->offset += allocation_size_;
    }

    *out_offset_ = escape_offset;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, size_t offset_, size_t block_size_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    node_t* tmp_node = NULL;

    // TODO: node_pool validation
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "range_free_list")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->node_pool, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_acquire", "node_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != block_size_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_acquire", "block_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(block_size_ <= range_free_list_->memory_pool_size, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_acquire", "block_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(offset_ <= (range_free_list_->memory_pool_size - block_size_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_acquire", "offset")

    if(0 == range_free_list_->unused_node_count) {
        ret = RANGE_FREE_LIST_LIMIT_EXCEEDED;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free node. reason=unused_node_count is zero.", rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(NULL == range_free_list_->node_pool[i]) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free node. reason=range_free_list_ is already initialized, but node_pool[%zu] is null.", rslt_to_str(ret), i);
            goto cleanup;
        }
        if(NODE_STATE_NOT_USED == range_free_list_->node_pool[i]->state) {
            tmp_node = range_free_list_->node_pool[i];
            found = true;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free node. reason=unused_node_count != 0, but free slot not found.", rslt_to_str(ret));
        goto cleanup;
    } else {
        if(!node_is_valid(tmp_node)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free node. reason=contents of the found node are corrupted.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    set_node_to_not_connected(tmp_node, offset_, block_size_);

    range_free_list_->unused_node_count--;
    *out_node_ = tmp_node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// リストから外されたnode_を未使用状態に戻す
// node_は呼び出し側でremove()によりリストから外し、next/prevをNULLにしておくこと
static range_free_list_result_t node_release(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_release", "range_free_list")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_release", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->node_pool, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_release", "node_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->unused_node_count < range_free_list_->max_node_count, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_release", "unused_node_count")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(node_->prev, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_release", "node_->prev")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(node_->next, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_release", "node_->next")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_NOT_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_release", "node_->state")

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(NULL == range_free_list_->node_pool[i]) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_release(%s) - Failed to release node. reason=range_free_list_ is already initialized, but node_pool[%zu] is null.", rslt_to_str(ret), i);
            goto cleanup;
        }
        if(range_free_list_->node_pool[i] == node_) {
            found = true;
            index = i;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node. reason=requested node was not found in the pool.", rslt_to_str(ret));
        goto cleanup;
    }

    set_node_to_not_used(range_free_list_->node_pool[index]);

    range_free_list_->unused_node_count++;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// node_のreleaseに先立ち、node_をリストから外す
static range_free_list_result_t node_remove(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_remove", "range_free_list")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_remove", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->node_pool, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_remove", "node_pool")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_remove", "node_->state")

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(NULL == range_free_list_->node_pool[i]) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node. reason=range_free_list_ is already initialized, but node_pool[%zu] is null.", rslt_to_str(ret), i);
            goto cleanup;
        }
        if(range_free_list_->node_pool[i] == node_) {
            found = true;
            index = i;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node. reason=requested node was not found in the pool.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == range_free_list_->node_pool[index]->prev && NULL == range_free_list_->node_pool[index]->next) {  // node_が唯一のノード
        if(range_free_list_->free_block_list_head != range_free_list_->node_pool[index]) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node. reason=only node, but not connected to the list.", rslt_to_str(ret));
            goto cleanup;
        }
        range_free_list_->free_block_list_head = NULL;
    } else if(NULL == range_free_list_->node_pool[index]->prev && NULL != range_free_list_->node_pool[index]->next) {   // node_が先頭で、node_の次に別のノードがある
        if(range_free_list_->free_block_list_head != range_free_list_->node_pool[index]) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node. reason=head node, but not connected to the list.", rslt_to_str(ret));
            goto cleanup;
        }
        range_free_list_->free_block_list_head = range_free_list_->node_pool[index]->next;

        range_free_list_->node_pool[index]->next->prev = NULL;
    } else if(NULL != range_free_list_->node_pool[index]->prev && NULL != range_free_list_->node_pool[index]->next) {   // node_の前後に別のノードがある
        range_free_list_->node_pool[index]->prev->next = range_free_list_->node_pool[index]->next;
        range_free_list_->node_pool[index]->next->prev = range_free_list_->node_pool[index]->prev;
    } else if(NULL != range_free_list_->node_pool[index]->prev && NULL == range_free_list_->node_pool[index]->next) {   // node_の前にノードが存在し、かつ、node_が末尾ノード
        range_free_list_->node_pool[index]->prev->next = NULL;
    }

    // block_size, offsetは保持する
    set_node_to_not_connected(range_free_list_->node_pool[index], range_free_list_->node_pool[index]->offset, range_free_list_->node_pool[index]->block_size);

    range_free_list_->node_pool[index]->state = NODE_STATE_NOT_CONNECTED;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t find_free_block_insert_position(const range_free_list_t* range_free_list_, size_t offset_, size_t free_size_, node_t** out_prev_node_, node_t** out_next_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;
    node_t* tmp_prev_node = NULL;
    node_t* tmp_next_node = NULL;
    bool found = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(out_prev_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "out_prev_node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_next_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "out_next_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_prev_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "*out_prev_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_next_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "*out_next_node_")

    // TODO: check_area(range_free_list, offset, free_size)
    IF_ARG_FALSE_GOTO_CLEANUP(0 != free_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "free_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->memory_pool_size >= free_size_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "free_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(offset_ <= (range_free_list_->memory_pool_size - free_size_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "free_size_")

    // アライメントチェック(free_size_はalign_upで必ずbase_alignにアラインされている, もしされていなければrequired_sizeをそのまま使用している可能性あり)
    // TODO: base_align 2の冪乗、非0チェック -> ragen_free_list_is_valid()を作る(順序整合チェックは安定したらリリースビルドでは行わないとコメントを入れておく, todoにも追加する)
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (offset_ % range_free_list_->base_align), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "offset_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (free_size_ % range_free_list_->base_align), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "free_size_")

    // TODO: find_first_fit_nodeも上記ヘルパーでpreconditionsを修正, allocateも必要であれば
    // TODO: プライベート関数のチェックは勘弁にし、公開API側できちんとvalidationチェックをする

    // TODO: check_range_relationは最後に一箇所でやるようにして見通しをよくする
    tmp_node = range_free_list_->free_block_list_head;
    if(NULL == tmp_node) {  // free listが空
        tmp_prev_node = NULL;
        tmp_next_node = NULL;
        found = true;
    } else {
        if(tmp_node->offset > offset_) {    // 先頭に挿入
            if(!check_range_relation(offset_, free_size_, tmp_node->offset)) {
                ret = RANGE_FREE_LIST_BAD_OPERATION;
                ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                goto cleanup;
            }
            tmp_prev_node = NULL;
            tmp_next_node = tmp_node;
            found = true;
        } else {
            while(NULL != tmp_node) {
                if(tmp_node->offset == offset_) {
                    ret = RANGE_FREE_LIST_BAD_OPERATION;
                    ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                    goto cleanup;
                }
                if(NULL != tmp_node->next && tmp_node->offset >= tmp_node->next->offset) {
                    ret = RANGE_FREE_LIST_DATA_CORRUPTED;
                    ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                    goto cleanup;
                }
                // TODO: ここから下はもっとスッキリできる
                if(tmp_node->offset < offset_ && NULL != tmp_node->next) {
                    if(tmp_node->next->offset > offset_) {  // 途中に挿入
                        if(!check_range_relation(tmp_node->offset, tmp_node->block_size, offset_)) {
                            ret = RANGE_FREE_LIST_BAD_OPERATION;
                            ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                            goto cleanup;
                        }
                        if(!check_range_relation(offset_, free_size_, tmp_node->next->offset)) {
                            ret = RANGE_FREE_LIST_BAD_OPERATION;
                            ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                            goto cleanup;
                        }
                        tmp_prev_node = tmp_node;
                        tmp_next_node = tmp_node->next;
                        found = true;
                        break;
                    }
                } else if(tmp_node->offset < offset_ && NULL == tmp_node->next) {   // 末尾に挿入
                    if(!check_range_relation(tmp_node->offset, tmp_node->block_size, offset_)) {
                        ret = RANGE_FREE_LIST_BAD_OPERATION;
                        ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
                        goto cleanup;
                    }
                    tmp_prev_node = tmp_node;
                    tmp_next_node = NULL;
                    found = true;
                    break;
                }
                tmp_node = tmp_node->next;
            }
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("find_free_block_insert_position(%s) - find_free_block_insert_position failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_prev_node_ = tmp_prev_node;
    *out_next_node_ = tmp_next_node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_insert_between(range_free_list_t* range_free_list_, node_t* insert_node_, node_t* prev_, node_t* next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert_between", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(insert_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert_between", "insert_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(insert_node_->next, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_->next")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(insert_node_->prev, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_->prev")
    IF_ARG_FALSE_GOTO_CLEANUP(insert_node_ != prev_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_ != prev_")
    IF_ARG_FALSE_GOTO_CLEANUP(insert_node_ != next_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_ != next_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_NOT_CONNECTED == insert_node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_->state")
    if(NULL != prev_ && NULL != next_ && prev_ == next_) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("node_insert_between(%s) - node_insert_between failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == prev_ && NULL == next_) {
        if(NULL != range_free_list_->free_block_list_head) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - node_insert_between failed.", rslt_to_str(ret));
            goto cleanup;
        }
        range_free_list_->free_block_list_head = insert_node_;
    } else if(NULL == prev_ && NULL != next_) {
        if(next_ != range_free_list_->free_block_list_head || NULL != range_free_list_->free_block_list_head->prev) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - node_insert_between failed.", rslt_to_str(ret));
            goto cleanup;
        }
        next_->prev = insert_node_;
        range_free_list_->free_block_list_head = insert_node_;
    } else if(NULL != prev_ && NULL != next_) {
        if(prev_->next != next_ || prev_ != next_->prev) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - node_insert_between failed.", rslt_to_str(ret));
            goto cleanup;
        }
        prev_->next = insert_node_;
        next_->prev = insert_node_;
    } else if(NULL != prev_ && NULL == next_) {
        if(NULL != prev_->next) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - node_insert_between failed.", rslt_to_str(ret));
            goto cleanup;
        }
        prev_->next = insert_node_;
    }

    set_node_to_connected(insert_node_, prev_, next_);

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_adjacent_check_prev(const node_t* node_, bool* out_is_adjacent_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool is_adjacent = false;
    size_t prev_end = 0;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_prev", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_is_adjacent_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_prev", "out_is_adjacent_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_adjacent_check_prev", "node_->state")

    if(NULL == node_->prev) {
        is_adjacent = false;
    } else {
        if(NODE_STATE_CONNECTED != node_->prev->state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - node_adjacent_check_prev failed.", rslt_to_str(ret));
            goto cleanup;
        }
        if(node_->prev->next != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - node_adjacent_check_prev failed.", rslt_to_str(ret));
            goto cleanup;
        }

        if(SIZE_MAX - node_->prev->block_size < node_->prev->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - node_adjacent_check_prev failed.", rslt_to_str(ret));
            goto cleanup;
        }
        prev_end = node_->prev->block_size + node_->prev->offset;
        if(prev_end > node_->offset) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - node_adjacent_check_prev failed.", rslt_to_str(ret));
            goto cleanup;
        }

        if(prev_end == node_->offset) {
            is_adjacent = true;
        } else {
            is_adjacent = false;
        }
    }

    *out_is_adjacent_ = is_adjacent;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_adjacent_check_next(const node_t* node_, bool* out_is_adjacent_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool is_adjacent = false;
    size_t node_end = 0;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_next", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_is_adjacent_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_next", "out_is_adjacent_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_adjacent_check_next", "node_->state")

    if(NULL == node_->next) {
        is_adjacent = false;
    } else {
        if(NODE_STATE_CONNECTED != node_->next->state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - node_adjacent_check_next failed.", rslt_to_str(ret));
            goto cleanup;
        }
        if(node_->next->prev != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - node_adjacent_check_next failed.", rslt_to_str(ret));
            goto cleanup;
        }

        if(SIZE_MAX - node_->block_size < node_->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - node_adjacent_check_next failed.", rslt_to_str(ret));
            goto cleanup;
        }
        node_end = node_->block_size + node_->offset;
        if(node_end > node_->next->offset) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - node_adjacent_check_next failed.", rslt_to_str(ret));
            goto cleanup;
        }

        if(node_end == node_->next->offset) {
            is_adjacent = true;
        } else {
            is_adjacent = false;
        }
    }

    *out_is_adjacent_ = is_adjacent;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t merge_free_block(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* next = NULL;
    node_t* prev = NULL;
    size_t new_block_size = 0;
    size_t new_offset = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "merge_free_block", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "merge_free_block", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "merge_free_block", "node_->state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "merge_free_block", "node_")

    if(should_merge_prev_ && should_merge_next_) {
        // 前後ノードマージ
        // node_->prevを残し、node_とnode_->nextを削除
        if(NODE_STATE_CONNECTED != node_->prev->state || NODE_STATE_CONNECTED != node_->next->state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        if(!node_is_valid(node_->prev) || !node_is_valid(node_->next)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        prev = node_->prev;
        next = node_->next;

        if((SIZE_MAX - node_->block_size) < prev->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        new_block_size = prev->block_size + node_->block_size;

        if((SIZE_MAX - next->block_size) < new_block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        new_block_size += next->block_size;

        ret = node_remove(range_free_list_, next);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        ret = node_release(range_free_list_, next);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }

        prev->block_size = new_block_size;
    } else if(should_merge_prev_ && !should_merge_next_) {
        // 前のノードのみマージ
        prev = node_->prev;
        next = node_->next;
        if((SIZE_MAX - prev->block_size) < node_->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        new_block_size = node_->block_size + prev->block_size;

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }

        prev->block_size = new_block_size;
    } else if(!should_merge_prev_ && should_merge_next_) {
        prev = node_->prev;
        next = node_->next;

        if((SIZE_MAX - next->block_size) < node_->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        new_block_size = node_->block_size + next->block_size;
        new_offset = node_->offset;

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - merge_free_block failed.", rslt_to_str(ret));
            goto cleanup;
        }

        next->offset = new_offset;
        next->block_size = new_block_size;
    }

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// NOTE: この関数は、状態遷移を行う場所と、状態遷移に必要なパラメータを明示することが目的, 以下は行わないため, 呼び出し側で保証すること
// - 遷移元の検証
static void set_node_to_not_used(node_t* target_) {
    if(NULL == target_) {
        return;
    }
    target_->block_size = 0;
    target_->offset = 0;
    target_->prev = NULL;
    target_->next = NULL;
    target_->state = NODE_STATE_NOT_USED;
}

// NOTE: この関数は、状態遷移を行う場所と、状態遷移に必要なパラメータを明示することが目的, 以下は行わないため, 呼び出し側で保証すること
// - 遷移元の検証
// - offset_, block_size_の検証
static void set_node_to_not_connected(node_t* target_, size_t offset_, size_t block_size_) {
    if(NULL == target_) {
        return;
    }
    target_->block_size = block_size_;
    target_->offset = offset_;
    target_->next = NULL;
    target_->prev = NULL;
    target_->state = NODE_STATE_NOT_CONNECTED;
}

// NOTE: この関数は、状態遷移を行う場所と、状態遷移に必要なパラメータを明示することが目的, 以下は行わないため, 呼び出し側で保証すること
// - 遷移元の検証
// - prev_, next_の検証
// - 遷移前target_の検証
static void set_node_to_connected(node_t* target_, node_t* prev_, node_t* next_) {
    if(NULL == target_) {
        return;
    }
    target_->prev = prev_;
    target_->next = next_;
    target_->state = NODE_STATE_CONNECTED;
}

static bool range_free_list_is_valid(const range_free_list_t* range_free_list_) {
    if(NULL == range_free_list_) {
        return false;
    }
    if(0 == range_free_list_->base_align || 0 == range_free_list_->max_node_count || 0 == range_free_list_->memory_pool_size) {
        return false;
    }
    if(!IS_POWER_OF_TWO(range_free_list_->base_align)) {
        return false;
    }

    // NOTE: 以下はfree_listの動作実績が増えたらDEBUG_BUILD, TEST_BUILDのみで動かす
    node_t* head = range_free_list_->free_block_list_head;
    node_t* prev = NULL;
    size_t loop_count = 0;
    size_t used_count = 0;
    while(NULL != head) {
        if(loop_count >= range_free_list_->max_node_count) {
            return false;
        }
        if(!node_is_valid(head)) {
            return false;
        }
        if(head->prev != prev) {
            return false;
        }
        if(range_free_list_->memory_pool_size < head->block_size) {
            return false;
        }
        if(head->offset > (range_free_list_->memory_pool_size - head->block_size)) {
            return false;
        }
        if(0 != (head->offset % range_free_list_->base_align)) {
            return false;
        }
        if(0 != (head->block_size % range_free_list_->base_align)) {
            return false;
        }
        if(NODE_STATE_CONNECTED != head->state) {
            return false;
        }
        if(NULL != head->next) {
            if(!check_range_relation(head->offset, head->block_size, head->next->offset)) {
                return false;
            }
        }
        head = head->next;
        used_count++;
        loop_count++;
    }

    if((range_free_list_->max_node_count - used_count) != range_free_list_->unused_node_count) {
        return false;
    }

    return true;
}

static bool node_is_valid(const node_t* node_) {
    if(NULL == node_) {
        return false;
    }
    if(NODE_STATE_CONNECTED == node_->state) {
        if(0 == node_->block_size) {
            return false;
        }
        if(NULL != node_->prev && node_->prev == node_) {
            return false;
        }
        if(NULL != node_->next && node_->next == node_) {
            return false;
        }
    } else if(NODE_STATE_NOT_CONNECTED == node_->state) {
        if(0 == node_->block_size) {
            return false;
        }
        if(NULL != node_->prev || NULL != node_->next) {
            return false;
        }
    } else if(NODE_STATE_NOT_USED == node_->state) {
        if(NULL != node_->prev || NULL != node_->next) {
            return false;
        }
        if(0 != node_->offset || 0 != node_->block_size) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

static range_free_list_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t padding = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "out_allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "align_up", "base_align_")

    // このrange_free_listはcreate時に指定されたbase_align固定で範囲を管理する。
    // そのため、各空き領域ノードのoffsetは常にbase_align境界に整列している必要がある。よって、確保範囲としてpadding分も消費する
    //
    // offsetがbase_align境界に整列している場合、確保後の次の空き領域offsetを
    // base_align境界に保つために必要なpadding量は、各nodeのoffsetには依存しない
    // よって、required_size_をbase_align単位に丸めた値が、実際に消費するallocation_sizeとなる。
    padding = required_size_ % base_align_;
    if(0 != padding) {
        padding = base_align_ - padding;
    }

    if((SIZE_MAX - padding) < required_size_) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("align_up(%s) - Failed to find first_fit_node. reason=size overflow.", rslt_to_str(ret));
        goto cleanup;
    }
    *out_allocation_size_ = required_size_ + padding;    // 割り当て領域の後ろにpaddingを追加し、offsetは常にbase_alignに整列されるようにする

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// prev_nodeの領域と、next_nodeの領域が干渉していないかをチェックする
// (prev_offset_ + prev_block_size_) < next_offset_であることをチェックする
static bool check_range_relation(size_t prev_offset_, size_t prev_block_size_, size_t next_offset_) {
    if(prev_offset_ > next_offset_) {
        return false;
    } else if((next_offset_ - prev_offset_) < prev_block_size_) {
        return false;
    } else {
        return true;
    }
}

static const char* rslt_to_str(range_free_list_result_t rslt_) {
    switch(rslt_) {
    case RANGE_FREE_LIST_SUCCESS:
        return s_rslt_str_success;
    case RANGE_FREE_LIST_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RANGE_FREE_LIST_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RANGE_FREE_LIST_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RANGE_FREE_LIST_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RANGE_FREE_LIST_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RANGE_FREE_LIST_OVERFLOW:
        return s_rslt_str_overflow;
    case RANGE_FREE_LIST_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}
