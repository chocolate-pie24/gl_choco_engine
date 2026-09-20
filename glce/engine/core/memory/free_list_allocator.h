// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file free_list_allocator.h
 * @brief 外部から提供されたmemory poolを管理するFree List Allocator
 *
 * @details
 * Free List Allocatorは、callerがあらかじめ確保した連続memory poolを
 * backing storageとして使用し、その領域内でmemory allocation / freeを行う。
 *
 * 本moduleはEngine内部で使用するprivate moduleであり、
 * Applicationレイヤーからは直接使用せず、上位のMemory Systemを経由して操作する。
 *
 * free_list_allocator_tはMemory System起動前に実体を生成する必要があるため、
 * heap allocationを前提としたopaque typeにはせず、caller側で実体を保持可能な型として公開する。
 *
 * @section free_list_allocator_boundary_contract Module Boundary Contract
 *
 * - backing memory poolはcallerがあらかじめ確保し、initialize時に提供する。
 * - Free List Allocatorはbacking memory poolを所有せず、その確保および解放を行わない。
 * - backing memory poolはFree List Allocatorの使用期間中、有効な状態を維持しなければならない。
 * - allocationされるmemoryは、initialize時に提供されたmemory pool内からのみ取得する。
 * - allocationのalignmentはalignof(max_align_t)に固定する。
 * - free_list_allocator_tの実体はcallerが保持し、本moduleはそのstorageを確保・解放しない。
 *
 * @todo
 * - free_list_allocator_ptr_is_alive()
 * - free_list_allocator_status_report()
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
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

    size_t allocation_size; // callerが要求した論理allocation size, FREE blockでは0
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
