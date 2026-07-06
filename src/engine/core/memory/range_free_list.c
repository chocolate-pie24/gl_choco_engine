#include "engine/core/memory/range_free_list.h"

#include <stdlib.h> // for malloc / free
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief Range Free List 内部状態管理構造体
 *
 */
struct range_free_list {
    size_t memory_pool_size;    /**< メモリプール容量(byte) */
    size_t max_node_count;      /**< 最大ノード数(個) */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コードRANGE_FREE_LIST_SUCCESS文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードRANGE_FREE_LIST_INVALID_ARGUMENT文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コードRANGE_FREE_LIST_LIMIT_EXCEEDED文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コードRANGE_FREE_LIST_NO_MEMORY文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コードRANGE_FREE_LIST_DATA_CORRUPTED文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コードRANGE_FREE_LIST_BAD_OPERATION文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コードRANGE_FREE_LIST_OVERFLOW文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードRANGE_FREE_LIST_UNDEFINED_ERROR文字列 */

static const char* rslt_to_str(range_free_list_result_t rslt_);

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, range_free_list_t** out_range_free_list_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    range_free_list_t* tmp_range_free_list = NULL;

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

    tmp_range_free_list->max_node_count = max_node_count_;
    tmp_range_free_list->memory_pool_size = memory_pool_size_;

    *out_range_free_list_ = tmp_range_free_list;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    if(RANGE_FREE_LIST_SUCCESS != ret) {
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
    (*range_free_list_)->max_node_count = 0;
    (*range_free_list_)->memory_pool_size = 0;

    free(*range_free_list_);
    *range_free_list_ = NULL;
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
