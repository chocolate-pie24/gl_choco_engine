#ifndef GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H
#define GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct range_free_list range_free_list_t;   /**< Range Free List内部状態管理構造体のopaque型 */

// TODO: core/memory内の実行結果コードを統一する
typedef enum {
    RANGE_FREE_LIST_SUCCESS = 0,
    RANGE_FREE_LIST_INVALID_ARGUMENT,
    RANGE_FREE_LIST_LIMIT_EXCEEDED,
    RANGE_FREE_LIST_NO_MEMORY,
    RANGE_FREE_LIST_DATA_CORRUPTED,
    RANGE_FREE_LIST_BAD_OPERATION,
    RANGE_FREE_LIST_OVERFLOW,
    RANGE_FREE_LIST_UNDEFINED_ERROR,
} range_free_list_result_t;

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, size_t base_align_, range_free_list_t** out_range_free_list_);

void range_free_list_destroy(range_free_list_t** range_free_list_);

// required_align_: 2の冪乗のみを許可, 最小アライメントサイズは64bit環境であれば8, 32bit環境であれば4, 最小アライメントサイズ未満のrequired_align_が渡された場合, 最小アライメントサイズに丸められる
range_free_list_result_t range_free_list_allocate(range_free_list_t* range_free_list_, size_t required_size_, size_t required_align_, size_t* out_offset_);

range_free_list_result_t range_free_list_free(range_free_list_t* range_free_list_, size_t offset_, size_t size_);

#ifdef __cplusplus
}
#endif
#endif
