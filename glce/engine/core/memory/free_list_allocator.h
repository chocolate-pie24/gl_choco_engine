// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_CORE_FREE_LIST_ALLOCATOR_H
#define GLCE_ENGINE_CORE_FREE_LIST_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/core/memory/memory_tag.h"

typedef enum {
    FREE_LIST_ALLOCATOR_SUCCESS = 0,
    FREE_LIST_ALLOCATOR_DATA_CORRUPTED,
    FREE_LIST_ALLOCATOR_BAD_OPERATION,
    FREE_LIST_ALLOCATOR_INVALID_ARGUMENT,
    FREE_LIST_ALLOCATOR_NO_MEMORY,
    FREE_LIST_ALLOCATOR_OVERFLOW,
    FREE_LIST_ALLOCATOR_UNDEFINED_ERROR,
} free_list_allocator_result_t;

typedef enum {
    FREE_LIST_BLOCK_STATE_FREE = 0,
    FREE_LIST_BLOCK_STATE_ALLOCATED,
} free_list_block_state_t;

typedef struct free_list_block_header {
    struct free_list_block_header* prev;
    struct free_list_block_header* next;

    size_t allocation_size; // callerが要求した論理allocation size, FREE blockでは意味を持たない
    size_t block_size;      // headerを含む、そのblock全体の物理サイズ, FREE / ALLOCATEDの両方で常に有効

    free_list_block_state_t block_state;

    memory_tag_t memory_tag;
} free_list_block_header_t;

// memory_systemでfree_list_allocator_t allocatorとして宣言したい(memory_poolからのみメモリを確保したいため)ため、内部構造は.hに書く(ただしapplicationには公開しない)
typedef struct {
    void* memory_pool;  // mutable borrowed pointer

    size_t memory_pool_size;
    size_t minimum_block_size;  // 最小ブロックサイズ(payload_offset + alignof(max_align_t))
    size_t payload_offset;      // header先頭からuser payloadまでのサイズ(sizeof(free_list_block_header_t) + padding)

    free_list_block_header_t* head;
} free_list_allocator_t;

// free_list_allocatorの生成にmallocを使用したくないためcreateではなくてinitialize
free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_);

free_list_allocator_result_t free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_);

free_list_allocator_result_t free_list_allocator_allocate(free_list_allocator_t* free_list_allocator_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_);

free_list_allocator_result_t free_list_allocator_free(free_list_allocator_t* free_list_allocator_, void* ptr_);

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_);

#ifdef __cplusplus
}
#endif
#endif
