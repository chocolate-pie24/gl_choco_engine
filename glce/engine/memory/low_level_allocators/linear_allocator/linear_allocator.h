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
 * @brief linear_allocator構造体前方宣言
 * @note 内部データ構造はlinear_allocator.cで定義し、外部からは隠蔽する
 */
typedef struct linear_allocator linear_allocator_t;

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

void linear_allocator_preinit(size_t* out_memory_requirement_, size_t* out_align_requirement_);

linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_);

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, size_t required_align_, void** out_ptr_);

linear_allocator_result_t linear_allocator_reset(linear_allocator_t* allocator_);

linear_allocator_result_t linear_allocator_status_get(const linear_allocator_t* allocator_, linear_allocator_status_t* out_status_);

bool linear_allocator_is_valid(const linear_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
