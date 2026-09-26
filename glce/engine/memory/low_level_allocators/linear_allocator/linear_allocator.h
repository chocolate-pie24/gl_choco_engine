// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file linear_allocator.h
 * @brief 外部から提供された連続memory poolをbump allocation方式で管理するLinear Allocator
 *
 * @details
 * Linear Allocatorは、callerがあらかじめ確保した連続memory poolを
 * backing storageとして使用し、headを単調に前進させることでmemory allocationを行う。
 *
 * allocationごとのmetadataは保持せず、個別allocationのfreeは提供しない。
 * 確保済み領域の一括破棄にはresetを使用し、過去に取得したrollback pointまでheadを戻す場合はrollbackを使用する。
 *
 * 本moduleはEngine内部で使用するlow-level allocatorであり、
 * Applicationレイヤーから直接使用しない。
 *
 * linear_allocator_tのstorageはcaller側で保持し、本module自身はallocator objectまたはbacking memory poolを動的確保しない。
 *
 * @section linear_allocator_boundary_contract Module Boundary Contract
 *
 * - backing memory poolはcallerがあらかじめ確保し、initialize時に提供する。
 * - Linear Allocatorはbacking memory poolを所有せず、その確保および解放を行わない。
 * - backing memory poolはLinear Allocatorの使用期間中、有効な状態を維持しなければならない。
 * - backing memory poolの先頭addressはalignof(max_align_t)にalignmentされていなければならない。
 * - allocationされるmemoryは、initialize時に提供されたmemory pool内からのみ取得する。
 * - allocationのalignmentはalignof(max_align_t)に固定する。
 * - allocationに指定するrequired_sizeは0より大きくなければならない。
 * - 各allocationはcallerが要求したsizeをalignof(max_align_t)へ切り上げた物理sizeを消費する。
 * - allocationごとのmetadataは保持せず、個別allocationのboundary、要求size、identityは追跡しない。
 * - 個別allocationのfreeは提供しない。
 *
 * - 現在使用中のmemory rangeは[memory_pool, head_ptr)である。
 * - linear_allocator_ptr_is_in_use_range()は、pointerが現在使用中のmemory range内に存在するかのみを判定する。
 * - linear_allocator_ptr_is_in_use_range()は、pointerがallocation payloadの先頭であること、
 *   特定allocationに属すること、またはhistorical allocation identityが一致することを保証しない。
 * - alignment paddingを含め、[memory_pool, head_ptr)内のaddressはin-use rangeとして扱う。
 *
 * - rollback pointはlinear_allocator_rollback_point_get()によって取得する。
 * - callerはrollback pointの内容を変更してはならない。
 * - earlier rollback pointへのrollbackを行った場合、それより後に取得されたrollback pointは無効となる。
 * - resetを行った場合、それ以前に取得されたrollback pointはすべて無効となる。
 * - rollback pointのgenerationまたはallocation historyはmodule内部では追跡しないため、
 *   stale rollback pointの再利用を完全には検出しない。
 * - rollbackおよびresetはmemory pool内のbyte内容を消去しない。
 *
 * - linear_allocator_tおよびlinear_allocator_rollback_point_tのstorageはcallerが保持する。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 */
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
    LINEAR_ALLOCATOR_BAD_OPERATION,
    LINEAR_ALLOCATOR_INVALID_ARGUMENT,  /**< 無効な引数 */
    LINEAR_ALLOCATOR_OVERFLOW,
    LINEAR_ALLOCATOR_UNDEFINED_ERROR,
} linear_allocator_result_t;

typedef struct linear_allocator_status {
    size_t memory_pool_size;
    size_t used_size;
    size_t free_size;
} linear_allocator_status_t;

typedef struct linear_allocator_rollback_point {
    size_t offset;
} linear_allocator_rollback_point_t;

typedef struct linear_allocator {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
} linear_allocator_t;

linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_);

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, void** out_ptr_);

linear_allocator_result_t linear_allocator_reset(linear_allocator_t* allocator_);

linear_allocator_result_t linear_allocator_rollback_point_get(const linear_allocator_t* allocator_, linear_allocator_rollback_point_t* out_rollback_point_);

linear_allocator_result_t linear_allocator_rollback(linear_allocator_t* allocator_, const linear_allocator_rollback_point_t* rollback_point_);

linear_allocator_result_t linear_allocator_status_get(const linear_allocator_t* allocator_, linear_allocator_status_t* out_status_);

bool linear_allocator_ptr_is_in_use_range(const linear_allocator_t* allocator_, const void* ptr_);

bool linear_allocator_is_valid(const linear_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
