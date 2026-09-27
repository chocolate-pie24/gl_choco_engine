// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file subsystem_allocator.h
 * @brief subsystem lifetime向けmemory allocationとmemory tag別accountingを提供するSubsystem Allocator
 *
 * @details
 * Subsystem Allocatorは、長寿命なsubsystem resource向けのmemory allocationを提供する。
 *
 * allocator objectおよびbacking memory poolはGeneral Allocatorから確保し、
 * backing memory poolのallocation mechanismにはLinear Allocatorを使用する。
 *
 * allocationごとのindividual freeは提供せず、確保済みmemoryの一括破棄にはresetを使用する。
 * 過去に取得したrollback pointまでallocation stateを戻す場合はrollbackを使用する。
 *
 * Subsystem Allocatorはcallerが要求した論理allocation sizeをmemory tag別にaccountingする。
 * Linear Allocatorが管理するalignment paddingを含む物理memory使用量とは区別して扱う。
 *
 * subsystem_allocator_tはopaque typeとし、
 * allocator objectおよびbacking memory poolのstorageは本moduleが所有する。
 *
 * @section subsystem_allocator_boundary_contract Module Boundary Contract
 *
 * - subsystem_allocator_create()を使用する前にGeneral Allocatorが利用可能でなければならない。
 * - Subsystem Allocatorのlifetime中、およびsubsystem_allocator_destroy()完了まで
 *   General Allocatorは利用可能でなければならない。
 *
 * - allocator objectおよびbacking memory poolはSubsystem Allocatorが所有する。
 * - callerはallocator objectまたはbacking memory poolを直接解放してはならない。
 * - allocator objectの破棄にはsubsystem_allocator_destroy()を使用する。
 *
 * - allocationされるmemoryはSubsystem Allocatorが所有するbacking memory pool内から取得する。
 * - allocationのalignmentはalignof(max_align_t)である。
 * - allocation_sizeは0より大きくなければならない。
 * - allocation成功時、callerが要求したallocation_size byteの領域は0で初期化される。
 * - individual allocationのfreeは提供しない。
 *
 * - total_allocatedおよびmemory_tag_allocatedは、callerが要求した論理allocation sizeをaccountingする。
 * - statusのused_size / free_sizeはLinear Allocatorが管理する物理memory rangeを表し、alignment paddingを含み得る。
 * - total_allocatedとused_sizeは異なるsemanticを持ち、同一値であることを保証しない。
 *
 * - rollback pointはsubsystem_allocator_rollback_point_get()によって取得する。
 * - callerはrollback pointの内容を変更してはならない。
 * - earlier rollback pointへのrollbackを行った場合、それより後に取得されたrollback pointは無効となる。
 * - resetを行った場合、それ以前に取得されたrollback pointはすべて無効となる。
 * - rollback pointのgenerationまたはallocation historyはmodule内部では追跡しないため、stale rollback pointの再利用を完全には検出しない。
 * - rollbackおよびresetはbacking memory pool内のbyte内容を消去しない。
 *
 * - subsystem_allocator_ptr_is_in_use_range()は、
 *   pointerが現在Linear Allocatorによって使用中とされるmemory range内に存在するかのみを判定する。
 * - allocation payloadの先頭、allocation boundary、allocation identity、
 *   requested allocation sizeまたはhistorical livenessは保証しない。
 * - alignment paddingもin-use rangeに含まれ得る。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_MEMORY_SUBSYSTEM_ALLOCATOR_SUBSYSTEM_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_SUBSYSTEM_ALLOCATOR_SUBSYSTEM_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct subsystem_allocator subsystem_allocator_t;

typedef enum {
    SUBSYSTEM_ALLOCATOR_SUCCESS = 0,
    SUBSYSTEM_ALLOCATOR_BAD_OPERATION,
    SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED,
    SUBSYSTEM_ALLOCATOR_NO_MEMORY,
    SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT,
    SUBSYSTEM_ALLOCATOR_OVERFLOW,
    SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED,
    SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR,
} subsystem_allocator_result_t;

typedef enum {
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_PLATFORM = 0,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_EVENT,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX,
} subsystem_allocator_memory_tag_t;

typedef struct subsystem_allocator_rollback_point {
    size_t offset;
    size_t total_allocated;
    size_t memory_tag_allocated[SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX];
} subsystem_allocator_rollback_point_t;

typedef struct subsystem_allocator_status {
    size_t memory_pool_size;
    size_t used_size;
    size_t free_size;

    size_t total_allocated;
    size_t memory_tag_allocated[SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX];
} subsystem_allocator_status_t;

subsystem_allocator_result_t subsystem_allocator_create(size_t memory_pool_size_, subsystem_allocator_t** out_allocator_);

void subsystem_allocator_destroy(subsystem_allocator_t** allocator_);

subsystem_allocator_result_t subsystem_allocator_allocate(subsystem_allocator_t* allocator_, size_t allocation_size_, subsystem_allocator_memory_tag_t memory_tag_, void** out_ptr_);

subsystem_allocator_result_t subsystem_allocator_reset(subsystem_allocator_t* allocator_);

subsystem_allocator_result_t subsystem_allocator_rollback_point_get(const subsystem_allocator_t* allocator_, subsystem_allocator_rollback_point_t* out_rollback_point_);

subsystem_allocator_result_t subsystem_allocator_rollback(subsystem_allocator_t* allocator_, const subsystem_allocator_rollback_point_t* rollback_point_);

subsystem_allocator_result_t subsystem_allocator_status_get(const subsystem_allocator_t* allocator_, subsystem_allocator_status_t* out_status_);

bool subsystem_allocator_ptr_is_in_use_range(const subsystem_allocator_t* allocator_, const void* ptr_);

const char* subsystem_allocator_memory_tag_to_str(subsystem_allocator_memory_tag_t memory_tag_);

bool subsystem_allocator_is_valid(const subsystem_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
