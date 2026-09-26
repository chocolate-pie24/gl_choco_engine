// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_LOW_LEVEL_ALLOCATORS_LINEAR_ALLOCATOR_LINEAR_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_LOW_LEVEL_ALLOCATORS_LINEAR_ALLOCATOR_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief linear_allocator実行結果コードリスト
 *
 */
typedef enum {
    LINEAR_ALLOCATOR_SUCCESS = 0,       /**< 処理成功 */
    LINEAR_ALLOCATOR_NO_MEMORY,         /**< メモリ不足 */
    LINEAR_ALLOCATOR_DATA_CORRUPTED,
    LINEAR_ALLOCATOR_INVALID_ARGUMENT,  /**< 無効な引数 */
} linear_allocator_result_t;

typedef struct linear_allocator_status {
    size_t memory_pool_size;
    size_t used_size;
    size_t free_size;
} linear_allocator_status_t;

typedef struct linear_allocator_allocation_metadata {
    size_t allocation_size;
} linear_allocator_allocation_metadata_t;

typedef struct linear_allocator {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス(実際にはアライメント要件分オフセットされたアドレスを渡す) */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
} linear_allocator_t;

linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_);

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, void** out_ptr_);

linear_allocator_result_t linear_allocator_reset(linear_allocator_t* allocator_);

linear_allocator_result_t linear_allocator_status_get(const linear_allocator_t* allocator_, linear_allocator_status_t* out_status_);

bool linear_allocator_is_valid(const linear_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
