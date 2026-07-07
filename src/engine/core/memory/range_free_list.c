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

static range_free_list_result_t find_first_fit_node(range_free_list_t* range_free_list_, size_t required_size_, node_t** out_node_, size_t* out_allocated_size_);

static range_free_list_result_t node_insert(node_t* insert_node_, node_t* node_);
static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, node_t** out_node_);
static range_free_list_result_t node_release(range_free_list_t* range_free_list_, node_t* node_);
static range_free_list_result_t node_remove(range_free_list_t* range_free_list_, node_t* node_);
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

// 要求サイズを満たす最初の空き領域ノードをfree_block_list_headから探索する。
// 探索方式: first-fit
static range_free_list_result_t find_first_fit_node(range_free_list_t* range_free_list_, size_t required_size_, node_t** out_node_, size_t* out_allocated_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    node_t* node = NULL;

    size_t updated_size = 0;    // アライメントを考慮して更新された要求サイズ
    size_t padding = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "required_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->free_block_list_head, ret, RANGE_FREE_LIST_NO_MEMORY, rslt_to_str(RANGE_FREE_LIST_NO_MEMORY), "find_first_fit_node", "range_free_list_->free_block_list_head")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocated_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "out_allocated_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != range_free_list_->base_align, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "find_first_fit_node", "range_free_list_->base_align")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(range_free_list_->base_align), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "find_first_fit_node", "range_free_list_->base_align")

    // このrange_free_listはcreate時に指定されたbase_align固定で範囲を管理する。
    // そのため、各空き領域ノードのoffsetは常にbase_align境界に整列している必要がある。よって、確保したメモリの後ろにpaddingを追加する
    //
    // offsetがbase_align境界に整列している場合、確保後の次の空き領域offsetを
    // base_align境界に保つために必要なpadding量は、各nodeのoffsetには依存しない
    // よって、required_size_をbase_align単位に丸めた値が、実際に消費するallocated_sizeとなる。
    padding = required_size_ % range_free_list_->base_align;
    if(0 != padding) {
        padding = range_free_list_->base_align - padding;
    }

    if((SIZE_MAX - padding) < required_size_) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first_fit_node. reason=size overflow.", rslt_to_str(ret));
        goto cleanup;
    }
    updated_size = required_size_ + padding;    // 割り当て領域の後ろにpaddingを追加し、offsetは常にbase_alignに整列されるようにする

    node = range_free_list_->free_block_list_head;
    while(NULL != node) {
        if(0 != (node->offset % range_free_list_->base_align)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first_fit_node. reason=offset is not aligned to base_align.", rslt_to_str(ret));
            goto cleanup;
        }
        if(node->block_size >= updated_size) {
            found = true;
            break;
        } else {
            node = node->next;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_NO_MEMORY;
        ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first_fit_node. reason=required free space size could not be found. required size=%zu", rslt_to_str(ret), required_size_);
        goto cleanup;
    }
    *out_node_ = node;
    *out_allocated_size_ = updated_size;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// 双方向リストにノード挿入処理
// node_の直後にinsert_node_を挿入する
// insert_node_がすでに別リストに接続されている場合はBAD_OPERATION
static range_free_list_result_t node_insert(node_t* insert_node_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* next = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(insert_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert", "insert_node_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert", "node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(insert_node_->next, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert", "insert_node_->next")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(insert_node_->prev, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert", "insert_node_->prev")
    IF_ARG_FALSE_GOTO_CLEANUP(insert_node_ != node_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert", "insert_node_ != node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_NOT_CONNECTED == insert_node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert", "insert_node_->state")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_CONNECTED == node_->state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert", "node_->state")

    if(NULL == node_->prev && NULL == node_->next) {    // node_が唯一のノード
        node_->next = insert_node_;
        insert_node_->prev = node_;
    } else if(NULL == node_->prev && NULL != node_->next) { // node_が先頭で、node_の次に別のノードがある
        next = node_->next;
        node_->next = insert_node_;
        insert_node_->prev = node_;
        insert_node_->next = next;
        next->prev = insert_node_;
    } else if(NULL != node_->prev && NULL != node_->next) { // node_の前後に別のノードがある
        next = node_->next;
        node_->next = insert_node_;
        insert_node_->next = next;
        insert_node_->prev = node_;
        next->prev = insert_node_;
    } else if(NULL != node_->prev && NULL == node_->next) { // node_の前にノードが存在し、かつ、node_が末尾ノード
        node_->next = insert_node_;
        insert_node_->prev = node_;
        insert_node_->next = NULL;
    }

    insert_node_->state = NODE_STATE_CONNECTED;
    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "range_free_list")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->node_pool, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_acquire", "node_pool")

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
        if(NULL != tmp_node->next || NULL != tmp_node->prev) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free node. reason=contents of the found node are corrupted.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    tmp_node->block_size = 0;
    tmp_node->offset = 0;
    tmp_node->next = NULL;
    tmp_node->prev = NULL;
    tmp_node->state = NODE_STATE_NOT_CONNECTED; // insert後にCONNECTEDに遷移する

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

    range_free_list_->node_pool[index]->block_size = 0;
    range_free_list_->node_pool[index]->offset = 0;
    range_free_list_->node_pool[index]->state = NODE_STATE_NOT_USED;
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

        range_free_list_->node_pool[index]->next = NULL;
        range_free_list_->node_pool[index]->prev = NULL;
    } else if(NULL != range_free_list_->node_pool[index]->prev && NULL != range_free_list_->node_pool[index]->next) {   // node_の前後に別のノードがある
        range_free_list_->node_pool[index]->prev->next = range_free_list_->node_pool[index]->next;
        range_free_list_->node_pool[index]->next->prev = range_free_list_->node_pool[index]->prev;

        range_free_list_->node_pool[index]->next = NULL;
        range_free_list_->node_pool[index]->prev = NULL;
    } else if(NULL != range_free_list_->node_pool[index]->prev && NULL == range_free_list_->node_pool[index]->next) {   // node_の前にノードが存在し、かつ、node_が末尾ノード
        range_free_list_->node_pool[index]->prev->next = NULL;

        range_free_list_->node_pool[index]->next = NULL;
        range_free_list_->node_pool[index]->prev = NULL;
    }

    range_free_list_->node_pool[index]->state = NODE_STATE_NOT_CONNECTED;
    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
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
