// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_CORE_FREE_LIST_ALLOCATOR_H
#define GLCE_ENGINE_CORE_FREE_LIST_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    FREE_LIST_ALLOCATOR_SUCCESS = 0,
    FREE_LIST_ALLOCATOR_DATA_CORRUPTED,
    FREE_LIST_ALLOCATOR_BAD_OPERATION,
    FREE_LIST_ALLOCATOR_INVALID_ARGUMENT,
    FREE_LIST_ALLOCATOR_LIMIT_EXCEEDED,
    FREE_LIST_ALLOCATOR_NO_MEMORY,
    FREE_LIST_ALLOCATOR_OVERFLOW,
    FREE_LIST_ALLOCATOR_UNDEFINED_ERROR,
} free_list_allocator_result_t;

// memory_systemでfree_list_allocator_t allocatorとして宣言したい(memory_poolからのみメモリを確保したいため)ため、内部構造は.hに書く(ただしapplicationには公開しない)
typedef struct {
    size_t memory_pool_size;
    void* memory_pool;  // mutable borrowed pointer
} free_list_allocator_t;

// free_list_allocatorの生成にmallocを使用したくないためcreateではなくてinitialize
free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_);

void free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_);

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_);

#ifdef __cplusplus
}
#endif
#endif
