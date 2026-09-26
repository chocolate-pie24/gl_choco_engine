// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/memory_utility.h"

/*
 * Module Internal Contract
 *
 * Stable state:
 * - initializedなlinear_allocator_tは有効なmemory poolを参照する。
 * - memory_pool != NULLである。
 * - head_ptr != NULLである。
 * - capacity > 0である。
 * - memory_poolはalignof(max_align_t)にalignmentされている。
 * - head_ptrはalignof(max_align_t)にalignmentされている。
 * - memory_pool + capacityはuintptr_tで表現可能である。
 * - head_ptrは[memory_pool, memory_pool + capacity]の範囲内に存在する。
 *
 * Memory range:
 * - [memory_pool, head_ptr)を現在使用中のmemory rangeとする。
 * - [head_ptr, memory_pool + capacity)を未使用memory rangeとする。
 * - 使用中rangeにはallocation payloadに加えてalignment paddingが含まれ得る。
 * - allocationごとのmetadata、boundary、要求size、stateは保持しない。
 *
 * Allocation:
 * - allocation payloadはalignof(max_align_t)にalignmentされる。
 * - required_sizeは0より大きい値でなければならない
 * - 消費する物理sizeはrequired_sizeをalignof(max_align_t)へ切り上げた値である。
 * - allocation成功時はCommit前のhead_ptrをcallerへ返し、head_ptrを消費する物理size分だけ前進させる。
 * - allocationはmemory poolの範囲を超えてhead_ptrを前進させない。
 *
 * Reset:
 * - resetはhead_ptrをmemory_poolへ戻す。
 * - reset後の使用中memory rangeは空である。
 * - resetはmemory pool内のbyte内容を消去しない。
 *
 * Rollback:
 * - rollback pointのoffsetは、その取得時点における
 *   memory_poolからhead_ptrまでのbyte offsetを表す。
 * - 正規に取得されたrollback pointのoffsetはalignof(max_align_t)にalignmentされている。
 * - rollbackはhead_ptrを現在位置より後方へ移動させない。
 * - rollback成功時はhead_ptr == memory_pool + rollback_point.offsetとなる。
 * - rollbackはmemory pool内のbyte内容を消去しない。
 *
 * Pointer range query:
 * - linear_allocator_ptr_is_in_use_range()は、
 *   ptrが[memory_pool, head_ptr)に存在する場合にtrueを返す。
 * - allocation boundary、payload boundary、allocation identityは判定しない。
 *
 * State Transition:
 * - Public APIのentry / exitではStable stateを維持する。
 * - allocation、reset、rollbackによるallocator stateのmutationはhead_ptrの更新のみで行う。
 * - Commit完了時にはStable stateへ復帰する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

 /*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractを基準として行う。
 *
 * - canonical validatorはinitializedなStable stateについて、
 *   Module Internal Contract全体のうちallocator自身から検証可能なstructural invariantを検証する。
 * - Linear Allocatorはallocationごとのmetadataやdeep structureを保持しないため、
 *   canonical validatorはallocation traversalを行わない。
 * - 現在のobject modelではcanonical validatorとshallow validatorの検査内容は実質的に近いが、
 *   validation depthを明示するため両者を分離して保持する。
 *
 * - shallow validatorはallocator rootについて、
 *   required pointer、capacity、alignment、address representability、
 *   memory_pool / head_ptr間のrange relationを検証する。
 *
 * - public APIは、そのoperationをmemory-safeかつboundedに実行するために必要なvalidation depthを個別に選択する。
 *
 * - rollback pointの取得元allocator、generation、allocation historyはmodule内部で保持しない。
 * - rollback pointのlifetimeおよびprovenanceはModule Boundary Contractとしてcallerに要求する。
 * - rollback時には、module自身が検証可能なoffset alignmentおよびcurrent used rangeとの関係を検証する。
 *
 * - linear_allocator_ptr_is_in_use_range()はrange membershipのみを判定し、
 *   allocation identityやallocation boundaryのvalidationには使用しない。
 *
 * - private helperはcanonical / shallow validatorを呼び出さない。
 * - private helperはpublic API boundaryまたは直前の処理によって自身のContractが成立していることを前提とする。
 *
 * - Postcondition validationはPublic APIのCommit完了後、
 *   Stable stateへ復帰した時点でのみ行う。
 *
 * - DEBUG_BUILD / TEST_BUILD / RELEASE_BUILDごとのvalidation実行条件は、
 *   各Public APIのValidation Policyで個別に定義する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";                     /**< 実行結果種別文字列(処理成功) */
static const char* const s_result_str_no_memory = "NO_MEMORY";                 /**< 実行結果種別文字列(メモリ確保失敗) */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果種別文字列(無効な引数) */
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果種別文字列(不明なエラー) */

// ============================================================
// Private Function Declarations
// ============================================================
// Allocation helpers
static linear_allocator_result_t allocation_is_ready(const linear_allocator_t* allocator_, size_t required_block_size_);

// Utilities
static const char* result_to_str(linear_allocator_result_t result_);

// Validators
static bool is_valid_shallow(const linear_allocator_t* allocator_);

// ============================================================
// Public API
// ============================================================

// linear_allocator_initialize Validation Policy
//
// - initialize前のallocator_はinitialized stateではないため、
//   Preconditionsではlinear_allocator_tのvalidatorを使用しない。
// - PreconditionsではModule Boundary Contractおよび
//   linear_allocator_initialize()固有のAPI Contractを直接検証する。
// - memory_pool_のalignmentおよびmemory_pool_ + capacity_のaddress representabilityをCommit前に検証する。
linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    const uintptr_t pool_address = (uintptr_t)memory_pool_;
    bool is_aligned = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "memory_pool_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != capacity_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "capacity_")
    if(!memory_utility_is_aligned((uintptr_t)(memory_pool_), alignof(max_align_t), &is_aligned)) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_initialize(%s) - memory_utility_is_aligned failed.", result_to_str(ret));
        goto cleanup;
    }
    if(!is_aligned) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_initialize(%s) - Provided memory_pool_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if((UINTPTR_MAX - pool_address) < capacity_) {
        ret = LINEAR_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("linear_allocator_initialize(%s) - Provided capacity_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    allocator_->capacity = capacity_;
    allocator_->head_ptr = memory_pool_;
    allocator_->memory_pool = memory_pool_;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_allocate Validation Policy
//
// - allocator_、out_ptr_およびrequired_size_に関するdirect argument validationを
//   structural validationより前に行う。
//
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用、allocator_がvalidなStable stateであることを確認する。
//
// - required_size_をalignof(max_align_t)へ切り上げる際のoverflowをCommit前に検証する。
// - allocation_is_ready()によって、allocation後のhead_ptrがmemory poolの範囲を
//   超えないことをCommit前に検証する。
//
// - Commitではhead_ptrのみを更新する。
// - DEBUG_BUILD / TEST_BUILDではCommit完了後にcanonical validatorを使用し、
//   Stable stateへ復帰していることをPostconditionとして確認する。
linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, void** out_ptr_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t current_head_addr = 0;
    uintptr_t next_head_addr = 0;
    size_t block_size = 0;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    if(0 == required_size_) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Provided required_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if(!memory_utility_align_up(required_size_, alignof(max_align_t), &block_size)) {
        ret = LINEAR_ALLOCATOR_OVERFLOW;
        goto cleanup;
    }

    // Preflight.
    ret = allocation_is_ready(allocator_, block_size);
    if(LINEAR_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("linear_allocator_allocate(%s) - allocation_is_ready failed.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    current_head_addr = (uintptr_t)allocator_->head_ptr;
    next_head_addr = (uintptr_t)allocator_->head_ptr + (uintptr_t)block_size;
    allocator_->head_ptr = (void*)next_head_addr;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_ptr_ = (void*)current_head_addr;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_reset Validation Policy
//
// - allocator_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
// - Commitでは、canonical validation済みのmemory_poolをhead_ptrへ代入するだけであり、
//   複数field間のmutation、下位APIの呼び出し、複雑なstate transitionを伴わない。そのため、成功時のPostcondition canonical validationは行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、Module Internal Contractが成立していることを前提としてresetを実行する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
linear_allocator_result_t linear_allocator_reset(linear_allocator_t* allocator_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_reset", "allocator_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    allocator_->head_ptr = allocator_->memory_pool;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_rollback_point_get Validation Policy
//
// - allocator_およびout_rollback_point_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
// - 本APIはallocator stateを変更しないqueryであるため、Postcondition validationは行わない。
// - rollback pointのlifetimeおよびprovenanceはModule Boundary Contractに従う。
linear_allocator_result_t linear_allocator_rollback_point_get(const linear_allocator_t* allocator_, linear_allocator_rollback_point_t* out_rollback_point_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t start_addr = 0;
    uintptr_t end_addr = 0;
    size_t offset = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_rollback_point_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_rollback_point_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_rollback_point_get", "out_rollback_point_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_rollback_point_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    start_addr = (uintptr_t)allocator_->memory_pool;
    end_addr = (uintptr_t)allocator_->head_ptr;
    offset = end_addr - start_addr;

    // Output.
    out_rollback_point_->offset = offset;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_rollback Validation Policy
//
// - allocator_およびrollback_point_はNULLでないことを要求する。
// - rollback_point_->offsetがalignof(max_align_t)にalignmentされていることを検証する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
//
// - rollback_point_->offsetがcurrent used rangeを超えないことを全BUILDで検証し、
//   rollbackによってhead_ptrが現在位置より前方へ移動しないことを保証する。
// - rollback pointの取得元allocator、generation、allocation historyは検証しない。
//   これらはModule Boundary Contractとしてcallerに要求する。
//
// - DEBUG_BUILD / TEST_BUILDではCommit完了後にcanonical validatorを使用し、
//   Stable stateへ復帰していることをPostconditionとして確認する。
linear_allocator_result_t linear_allocator_rollback(linear_allocator_t* allocator_, const linear_allocator_rollback_point_t* rollback_point_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    bool is_aligned = false;
    size_t used_range = 0;
    uintptr_t start_addr = 0;
    uintptr_t end_addr = 0;
    uintptr_t next_end_addr = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_rollback", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(rollback_point_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_rollback", "rollback_point_")
    if(!memory_utility_is_aligned((uintptr_t)(rollback_point_->offset), alignof(max_align_t), &is_aligned)) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_rollback(%s) - memory_utility_is_aligned failed.", result_to_str(ret));
        goto cleanup;
    }
    if(!is_aligned) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_rollback(%s) - Provided offset is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_rollback(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    start_addr = (uintptr_t)allocator_->memory_pool;
    end_addr = (uintptr_t)allocator_->head_ptr;
    used_range = end_addr - start_addr;
    if(used_range < rollback_point_->offset) {
        ret = LINEAR_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("linear_allocator_rollback(%s) - Provided offset is not valid.", result_to_str(ret));
        goto cleanup;
    }
    next_end_addr = start_addr + rollback_point_->offset;

    // Commit.
    allocator_->head_ptr = (void*)next_end_addr;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_rollback(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_status_get Validation Policy
//
// - allocator_およびout_status_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
// - status算出はmemory_pool、head_ptr、capacityのみを使用し、deep structureには依存しない。
// - RELEASE_BUILDではModule Internal Contractが成立していることを前提としてstatusを算出する。
// - 本APIはallocator stateを変更しないqueryであるため、Postcondition validationは行わない。
linear_allocator_result_t linear_allocator_status_get(const linear_allocator_t* allocator_, linear_allocator_status_t* out_status_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;

    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_status_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_status_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_status_get", "out_status_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_status_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;

    memory_pool_size = allocator_->capacity;
    used_size = (size_t)(head_address - pool_address);
    free_size = memory_pool_size - used_size;

    // Output.
    out_status_->free_size = free_size;
    out_status_->memory_pool_size = memory_pool_size;
    out_status_->used_size = used_size;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_ptr_is_in_use_range Validation Policy
//
// - allocator_またはptr_がNULLの場合はfalseを返す。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
// - canonical validationの結果がfalseの場合はfalseを返す
// - allocation payloadの先頭address、allocation boundary、allocation identity、
//   requested allocation size、およびhistorical livenessは検証しない。
// - 本APIはallocator stateを変更しないqueryであるため、Postcondition validationは行わない。
bool linear_allocator_ptr_is_in_use_range(const linear_allocator_t* allocator_, const void* ptr_) {
    uintptr_t start_addr = 0;
    uintptr_t end_addr = 0;
    uintptr_t ptr_addr = 0;

    if(NULL == allocator_ || NULL == ptr_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ERROR_MESSAGE("linear_allocator_ptr_is_in_use_range(%s) - Precondition validation failed for 'allocator_'.", result_to_str(LINEAR_ALLOCATOR_DATA_CORRUPTED));
        return false;
    }
#endif

    start_addr = (uintptr_t)allocator_->memory_pool;
    end_addr = (uintptr_t)allocator_->head_ptr;
    ptr_addr = (uintptr_t)ptr_;

    if(ptr_addr < start_addr || ptr_addr >= end_addr) {
        return false;
    }
    return true;
}

bool linear_allocator_is_valid(const linear_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(!is_valid_shallow(allocator_)) {
        return false;
    }
    return true;
}

// ============================================================
// Allocation Helpers
// ============================================================
static linear_allocator_result_t allocation_is_ready(const linear_allocator_t* allocator_, size_t required_block_size_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;

    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "allocation_is_ready", "allocator_")
    if(0 == required_block_size_) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided required_block_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;

    memory_pool_size = allocator_->capacity;
    used_size = (size_t)(head_address - pool_address);
    free_size = memory_pool_size - used_size;

    if(required_block_size_ > free_size) {
        ret = LINEAR_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("allocation_is_ready(%s) - no memory.", result_to_str(ret));
        goto cleanup;
    }

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Utilities
// ============================================================
/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] result_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* result_to_str(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case LINEAR_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case LINEAR_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case LINEAR_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    case LINEAR_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const linear_allocator_t* allocator_) {
    bool is_aligned = false;
    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;
    uintptr_t end_address = 0;

    if(NULL == allocator_) {
        return false;
    }

    if(NULL == allocator_->head_ptr) {
        return false;
    }
    if(NULL == allocator_->memory_pool) {
        return false;
    }
    if(0 == allocator_->capacity) {
        return false;
    }

    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;
    if(!memory_utility_is_aligned(head_address, alignof(max_align_t), &is_aligned)) {
        return false;
    }
    if(!is_aligned) {
        return false;
    }
    if(!memory_utility_is_aligned(pool_address, alignof(max_align_t), &is_aligned)) {
        return false;
    }
    if(!is_aligned) {
        return false;
    }

    if((UINTPTR_MAX - pool_address) < allocator_->capacity) {
        return false;
    }
    end_address = pool_address + allocator_->capacity;
    if(head_address < pool_address || head_address > end_address) {
        return false;
    }

    return true;
}
