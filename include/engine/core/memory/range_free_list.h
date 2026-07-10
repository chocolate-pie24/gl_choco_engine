#ifndef GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H
#define GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct range_free_list range_free_list_t;   /**< Range Free List内部状態管理構造体のopaque型 */

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

typedef struct range_allocation {
    size_t offset;
    size_t allocated_size;
} range_allocation_t;

typedef struct range_free_list_status {
    size_t memory_pool_size;
    size_t base_align;

    size_t max_node_count;
    size_t unused_node_count;
    size_t free_block_count;

    size_t total_free_size;
    size_t max_free_block_size;
} range_free_list_status_t;

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, size_t base_align_, range_free_list_t** out_range_free_list_);

void range_free_list_destroy(range_free_list_t** range_free_list_);

range_free_list_result_t range_free_list_allocate(range_free_list_t* range_free_list_, size_t required_size_, size_t required_align_, range_allocation_t* out_allocation_);

range_free_list_result_t range_free_list_free(range_free_list_t* range_free_list_, range_allocation_t allocation_);

void range_free_list_status_get(const range_free_list_t* range_free_list_, range_free_list_status_t* out_status_);

void range_free_list_debug_print(const range_free_list_t* range_free_list_);

#ifdef __cplusplus
}
#endif
#endif
