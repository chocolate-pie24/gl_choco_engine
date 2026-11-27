// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_GPU_ALLOCATORS_LINEAR_RANGE_ALLOCATOR_LINEAR_RANGE_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_GPU_ALLOCATORS_LINEAR_RANGE_ALLOCATOR_LINEAR_RANGE_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

typedef struct linear_range_allocator linear_range_allocator_t;

typedef enum {
    LINEAR_RANGE_ALLOCATOR_SUCCESS = 0,
    LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT,
    LINEAR_RANGE_ALLOCATOR_LIMIT_EXCEEDED,
    LINEAR_RANGE_ALLOCATOR_NO_MEMORY,
    LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED,
    LINEAR_RANGE_ALLOCATOR_BAD_OPERATION,
    LINEAR_RANGE_ALLOCATOR_OVERFLOW,
    LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR,
} linear_range_allocator_result_t;

typedef struct linear_range_allocator_rollback_point {
    size_t offset;
} linear_range_allocator_rollback_point_t;

linear_range_allocator_result_t linear_range_allocator_create(size_t memory_pool_size_, linear_range_allocator_t** out_allocator_);

void linear_range_allocator_destroy(linear_range_allocator_t** allocator_);

linear_range_allocator_result_t linear_range_allocator_allocate(linear_range_allocator_t* allocator_, size_t required_size_, size_t required_align_, size_t* out_offset_);

linear_range_allocator_result_t linear_range_allocator_reset(linear_range_allocator_t* allocator_);

linear_range_allocator_result_t linear_range_allocator_rollback_point_get(const linear_range_allocator_t* allocator_, linear_range_allocator_rollback_point_t* out_rollback_point_);

linear_range_allocator_result_t linear_range_allocator_rollback(linear_range_allocator_t* allocator_, const linear_range_allocator_rollback_point_t* rollback_point_);

bool linear_range_allocator_is_valid(const linear_range_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
