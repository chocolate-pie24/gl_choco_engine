// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_LOW_LEVEL_ALLOCATORS_LINEAR_ALLOCATOR_LINEAR_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_LOW_LEVEL_ALLOCATORS_LINEAR_ALLOCATOR_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief linear_allocator構造体前方宣言
 * @note 内部データ構造はlinear_allocator.cで定義し、外部からは隠蔽する
 */
typedef struct linear_alloc linear_alloc_t;

/**
 * @brief linear_allocator実行結果コードリスト
 *
 */
typedef enum {
    LINEAR_ALLOC_SUCCESS = 0,       /**< 処理成功 */
    LINEAR_ALLOC_NO_MEMORY,         /**< メモリ不足 */
    LINEAR_ALLOC_INVALID_ARGUMENT,  /**< 無効な引数 */
} linear_allocator_result_t;

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_);

linear_allocator_result_t linear_allocator_initialize(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_);

linear_allocator_result_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_);

#ifdef __cplusplus
}
#endif
#endif
