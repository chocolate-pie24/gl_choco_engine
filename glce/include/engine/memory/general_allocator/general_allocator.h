// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file general_allocator.h
 * @brief general lifetime向けmemory allocationとmemory tag別accountingを提供するGeneral Allocator
 *
 * @details
 * General Allocatorは、Engine全体で共有するgeneral lifetime向けmemory allocationを提供する。
 *
 * General Allocatorはmodule内部に単一instanceを保持するsingletonとして動作し、
 * backing memory poolのallocation mechanismにはFree List Allocatorを使用する。
 *
 * individual allocationのfreeを提供し、allocation時に指定されたmemory tagごとに
 * callerが要求した論理allocation sizeをaccountingする。
 *
 * @section general_allocator_boundary_contract Module Boundary Contract
 *
 * - General Allocatorはprocess-wide singletonとして使用する。
 * - 他のGeneral Allocator APIを使用する前にgeneral_allocator_create()を実行する。
 * - initializedな状態でgeneral_allocator_create()を再実行してはならない。
 * - 二重createはGENERAL_ALLOCATOR_BAD_OPERATIONを返し、既存stateを変更しない。
 * - 利用終了時はgeneral_allocator_destroy()を実行する。
 *
 * - backing memory poolはGeneral Allocatorが所有する。
 * - callerはbacking memory poolを直接取得、変更または解放してはならない。
 * - backing memory poolのcapacityおよびstorage policyはbuild configurationによって決定する。
 *
 * - allocationされるmemoryはGeneral Allocatorが所有するbacking memory pool内から取得する。
 * - allocationのalignmentはalignof(max_align_t)である。
 * - allocation_sizeは0より大きくなければならない。
 * - allocation時には有効なgeneral_allocator_memory_tag_tを指定する。
 * - allocation成功時、callerが要求したallocation_size byteの領域は0で初期化される。
 * - allocation成功時に返されるpointerはallocation payloadの先頭を指す。
 *
 * - allocationをfreeする場合は、general_allocator_allocate()によって返された
 *   現在allocation中のpayload先頭pointerを指定する。
 * - interior pointerまたはすでにfreeされたpointerをfreeしてはならない。
 * - free時には、そのallocationを取得した際と同じmemory tagを指定する。
 * - free成功時はcallerが保持するpointerをNULLへ変更する。
 *
 * - total_allocatedおよびmemory_tag_allocatedは、
 *   callerが要求した論理allocation sizeをaccountingする。
 * - allocated_block_size / free_block_size等のblock情報は、
 *   Free List Allocatorが管理する物理memory layoutを表し、
 *   block headerおよびalignment paddingの影響を含み得る。
 * - total_allocatedとallocated_block_sizeは異なるsemanticを持ち、
 *   同一値であることを保証しない。
 *
 * - general_allocator_ptr_is_allocated()は、
 *   pointerが現在allocation中のpayload先頭を指す場合にtrueを返す。
 * - allocationをfreeする場合は、general_allocator_allocate()によって返された現在allocation中のpayload先頭pointerを指定する。
 *   allocation payloadの途中を指すpointerをfreeしてはならない。
 * - raw pointerのaddressのみを用いて判定するため、
 *   free後に同じaddressが別allocationへ再利用された場合のhistorical allocation identityは保証しない。
 *
 * - general_allocator_is_valid()は、現在のGeneral Allocatorが
 *   moduleのcanonical invariantを満たすStable stateであるかを判定する。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_MEMORY_GENERAL_ALLOCATOR_GENERAL_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_GENERAL_ALLOCATOR_GENERAL_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct general_allocator general_allocator_t;

typedef enum {
    GENERAL_ALLOCATOR_SUCCESS = 0,
    GENERAL_ALLOCATOR_DATA_CORRUPTED,
    GENERAL_ALLOCATOR_BAD_OPERATION,
    GENERAL_ALLOCATOR_INVALID_ARGUMENT,
    GENERAL_ALLOCATOR_NO_MEMORY,
    GENERAL_ALLOCATOR_OVERFLOW,
    GENERAL_ALLOCATOR_LIMIT_EXCEEDED,
    GENERAL_ALLOCATOR_UNDEFINED_ERROR,
} general_allocator_result_t;

typedef enum {
    GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM = 0,  /**< メモリタグ: システム系 */
    GENERAL_ALLOCATOR_MEMORY_TAG_STRING,      /**< メモリタグ: 文字列系 */
    GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE,  /**< メモリタグ: リングキュー */
    GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER,    /**< メモリタグ: レンダラー */
    GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO,     /**< メモリタグ: ファイルI/O */
    GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA,      /**< メモリタグ: カメラシステム */
    GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE,     /**< メモリタグ: テクスチャ */
    GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY,    /**< メモリタグ: ジオメトリ */
    GENERAL_ALLOCATOR_MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} general_allocator_memory_tag_t;

typedef struct general_allocator_status {
    size_t memory_pool_size;

    size_t allocated_block_size;
    size_t free_block_size;

    size_t allocated_block_count;
    size_t free_block_count;

    size_t largest_free_block_size;
    size_t max_allocation_size;

    size_t total_allocated;
    size_t memory_tag_allocated[GENERAL_ALLOCATOR_MEMORY_TAG_MAX];
} general_allocator_status_t;

general_allocator_result_t general_allocator_create(void);

void general_allocator_destroy(void);

general_allocator_result_t general_allocator_allocate(size_t allocation_size_, general_allocator_memory_tag_t memory_tag_, void** out_ptr_);

void general_allocator_free(void** ptr_, general_allocator_memory_tag_t memory_tag_);

bool general_allocator_ptr_is_allocated(const void* ptr_);

general_allocator_result_t general_allocator_status_get(general_allocator_status_t* out_status_);

const char* general_allocator_memory_tag_to_str(general_allocator_memory_tag_t memory_tag_);

bool general_allocator_is_valid(void);

#ifdef __cplusplus
}
#endif
#endif
