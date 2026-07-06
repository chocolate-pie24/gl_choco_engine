#include "engine/core/memory/range_free_list.h"

#include <stdlib.h> // for malloc / free
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief 各空き領域を管理するノード構造体
 *
 */
typedef struct node {
    struct node* next;  /**< 次の空き領域ノードへのポインタ */
    struct node* prev;  /**< 前の空き領域ノードへのポインタ */

    size_t block_size;  /**< 空き領域サイズ(byte) */
    size_t offset;      /**< この空き領域の開始オフセット(byte) */
} node_t;

/**
 * @brief Range Free List 内部状態管理構造体
 *
 */
struct range_free_list {
    size_t memory_pool_size;    /**< メモリプール容量(byte) */
    size_t max_node_count;      /**< 最大ノード数(個) */
    size_t unused_node_count;   /**< 未使用ノード数 */

    node_t* head;               /**< 先頭の空き領域ノードへのポインタ */

    node_t** node_pool;         /**< range_free_listが所有する全ノードへのポインタ配列 */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コードRANGE_FREE_LIST_SUCCESS文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードRANGE_FREE_LIST_INVALID_ARGUMENT文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コードRANGE_FREE_LIST_LIMIT_EXCEEDED文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コードRANGE_FREE_LIST_NO_MEMORY文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コードRANGE_FREE_LIST_DATA_CORRUPTED文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コードRANGE_FREE_LIST_BAD_OPERATION文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コードRANGE_FREE_LIST_OVERFLOW文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードRANGE_FREE_LIST_UNDEFINED_ERROR文字列 */

static range_free_list_result_t node_insert(node_t* insert_node_, node_t* node_);
static const char* rslt_to_str(range_free_list_result_t rslt_);

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, range_free_list_t** out_range_free_list_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    range_free_list_t* tmp_range_free_list = NULL;
    node_t** tmp_node_pool = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "out_range_free_list_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "*out_range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != memory_pool_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "memory_pool_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_node_count_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "max_node_count_")

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
    }

    tmp_range_free_list->max_node_count = max_node_count_;
    tmp_range_free_list->memory_pool_size = memory_pool_size_;
    tmp_range_free_list->unused_node_count = max_node_count_ - 1;
    tmp_range_free_list->head = tmp_node_pool[0];
    tmp_range_free_list->head->block_size = memory_pool_size_;
    tmp_range_free_list->head->offset = 0;
    tmp_range_free_list->head->next = NULL;
    tmp_range_free_list->head->prev = NULL;
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
