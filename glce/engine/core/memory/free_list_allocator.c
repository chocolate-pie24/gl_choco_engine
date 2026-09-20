// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/core/memory/free_list_allocator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/memory_utility.h"

#include "engine/core/memory/memory_tag.h"

/*
 * Module Internal Contract
 *
 * Stable state:
 * - initializedなfree_list_allocator_tは有効なmemory poolを参照する。
 * - headはmemory_poolの先頭に配置されたblock headerを指す。
 * - block chainはmemory_poolの先頭から末尾までをgap / overlapなく連続して覆う。
 * - 各blockはmemory poolの範囲内に存在する。
 * - 各block headerはalignof(max_align_t)にalignmentされている。
 * - 各blockのprev / nextは物理的に隣接するblockとの関係と一致する。
 * - head->prev == NULLである。
 * - tail->next == NULLである。
 * - tail blockの末尾はmemory poolの末尾と一致する。
 *
 * Block layout:
 * - payload_offsetはfree_list_block_header_tの末尾をalignof(max_align_t)へ切り上げた値である。
 * - minimum_block_sizeはpayload_offset + alignof(max_align_t)である。
 * - block_sizeはminimum_block_size以上である。
 * - block_sizeはblock headerを含むblock全体の物理サイズを表す。
 * - allocation_sizeはcallerが要求した論理allocation sizeを表す。
 *
 * Block state:
 * - block_stateはFREEまたはALLOCATEDのいずれかである。
 * - ALLOCATED blockではallocation_size > 0である。
 * - ALLOCATED blockではallocation_sizeはpayload領域内に収まる。
 * - ALLOCATED blockではmemory_tagは有効な値である。
 * - FREE blockではallocation_size == 0である。
 * - FREE blockではmemory_tagの値に意味を持たせない。
 * - Stable stateでは隣接するFREE blockは存在しない。
 *
 * Allocation:
 * - allocationはmemory pool内からのみ行う。
 * - allocation payloadはalignof(max_align_t)にalignmentされる。
 * - allocation対象blockはfirst-fitで選択する。
 * - split後の残余blockがminimum_block_size未満になる場合はsplitしない。
 *
 * State Transition:
 * - Public APIのentry / exitではStable stateを維持する。
 * - Commit中はprivate helperのContractで明示された範囲に限り、Stable stateの一部を一時的に満たさないtransient stateを許容する。
 * - Commit完了時にはStable stateへ復帰する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractを基準として行う。
 * - canonical validatorはinitializedなStable stateについて、Module Internal Contract全体の整合性を検証する。
 * - shallow validatorはallocator rootと、deep traversalを行う前提として必要なlocal invariantのみを検証する。
 *
 * - public APIは、対象処理をmemory poolの範囲内で安全に完了するために必要なvalidation depthを選択する。
 * - shallowで十分なAPIではcanonical validationを要求しない。
 * - block chain traversalやdeep structureの整合性に依存するAPIでは、必要に応じてcanonical validationを使用する。
 *
 * - private helperはcanonical / shallow validatorを呼び出さない。
 * - private helperはpublic API boundaryまたは直前のhelperによって自身のContractが成立していることを前提とする。
 * - private helper自身が直接受け取る引数やstateについて、個別Contractで再検証しないと定めた項目は再検証しない。
 *
 * - Commit専用のvoid helperは、呼び出し前に必要な条件がすべて成立していることを前提とし、Commit開始後に通常の失敗経路を持たない。
 * - Commit途中で許容されるtransient stateは、該当private helperのContractで明示する。
 *
 * - Postcondition validationはPublic APIのCommit完了後、Stable stateへ復帰した時点で行う。
 * - Postcondition validatorはCommit途中のtransient stateには適用しない。
 *
 * - canonical validatorはfree_list_allocator_tおよびbacking memory poolのstorageが有効であることを前提とする。
 * - canonical validator自身はmemory pool内のmetadataがcorrupted stateであっても、
 *   out-of-bounds access、invalid dereference、unbounded traversalを引き起こさないよう設計する。
 *
 * - DEBUG_BUILD / TEST_BUILD / RELEASE_BUILDごとのvalidation実行条件は、各Public APIのValidation Policyで個別に定義する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Constants
// ============================================================
static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

// ============================================================
// Private Function Declarations
// ============================================================
// Allocation helpers
static free_list_allocator_result_t allocation_block_size_calc(size_t payload_offset_, size_t allocation_size_, size_t* out_required_block_size_);
static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_free_block_);
static free_list_allocator_result_t allocation_is_ready(const free_list_block_header_t* allocation_block_, size_t required_block_size_, size_t allocation_size_, memory_tag_t memory_tag_);
static void free_block_split(size_t minimum_block_size_, free_list_block_header_t* free_block_, size_t required_block_size_);
static void free_block_allocate(size_t payload_offset_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_);

// Free helpers
static bool allocation_ptr_is_valid(const free_list_allocator_t* free_list_allocator_, const void* ptr_);
static void allocated_block_free(free_list_block_header_t* allocation_block_);
static void free_block_merge_next(free_list_block_header_t* free_block_);
static void free_block_coalesce(free_list_block_header_t* free_block_);

// Utilities
static const char* rslt_to_str(free_list_allocator_result_t rslt_);

// Validators
static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_);
static bool free_list_block_state_is_valid(free_list_block_state_t state_);
static bool free_list_block_is_valid(const free_list_block_header_t* block_, size_t payload_offset_, size_t minimum_block_size_);

// ============================================================
// Public API
// ============================================================

// free_list_allocator_initialize Validation Policy
//
// - initialize前のfree_list_allocator_はinitialized stateではないため、
//   Preconditionsではfree_list_allocator_tのvalidatorを使用しない。
// - PreconditionsではModule Boundary Contractおよびfree_list_allocator_initialize()固有のAPI Contractを直接検証する。
// - Commit完了後、DEBUG_BUILD / TEST_BUILDではcanonical validatorを使用し、
//   initializedなStable stateがModule Internal Contractを満たすことを検証する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* initial_block = NULL;

    size_t payload_offset = 0;
    size_t minimum_block_size = 0;

    bool is_aligned = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "memory_pool_")
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "free_list_allocator_")
    if(0 == memory_pool_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!memory_utility_is_aligned((uintptr_t)(memory_pool_), alignof(max_align_t), &is_aligned)) {
        ret = FREE_LIST_ALLOCATOR_UNDEFINED_ERROR;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - memory_utility_is_aligned failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_aligned) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if((UINTPTR_MAX - (uintptr_t)memory_pool_) < memory_pool_size_) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Address range overflow.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!memory_utility_align_up(sizeof(free_list_block_header_t), alignof(max_align_t), &payload_offset)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - payload_offset) < alignof(max_align_t)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - minimum_block_size overflow.", rslt_to_str(ret));
        goto cleanup;
    }
    minimum_block_size = payload_offset + alignof(max_align_t);
    if(memory_pool_size_ < minimum_block_size) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    initial_block = (free_list_block_header_t*)memory_pool_;

    // Commit.
    initial_block->block_size = memory_pool_size_;
    initial_block->allocation_size = 0;
    initial_block->block_state = FREE_LIST_BLOCK_STATE_FREE;
    initial_block->next = NULL;
    initial_block->prev = NULL;

    free_list_allocator_->memory_pool = memory_pool_;
    free_list_allocator_->memory_pool_size = memory_pool_size_;
    free_list_allocator_->payload_offset = payload_offset;
    free_list_allocator_->minimum_block_size = minimum_block_size;
    free_list_allocator_->head = initial_block;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// NOTE: free_list_allocator_deinitialize
// validなfree_list_allocator_tであれば失敗することは基本ないためvoidにしても良いが、
// engine private moduleであるためresult codeを返すことにする
//
// free_list_allocator_deinitialize Validation Policy
//
// - deinitialize対象はinitializedなStable stateであることを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、
//   deinitialize前のfree_list_allocator_がModule Internal Contractを満たすことを検証する。
// - deinitialize完了後はinitialized stateではなくなるため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
free_list_allocator_result_t free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    if(NULL == free_list_allocator_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_deinitialize(%s) - Provided free_list_allocator_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_deinitialize(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    free_list_allocator_->head = NULL;
    free_list_allocator_->payload_offset = 0;
    free_list_allocator_->memory_pool = NULL;
    free_list_allocator_->memory_pool_size = 0;
    free_list_allocator_->minimum_block_size = 0;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// free_list_allocator_allocate Validation Policy
//
// - free_list_allocator_のblock chainを安全に走査するため、Preconditionsではcanonical validatorを使用する。
// - direct argument validationはstructural validationより前に行う。
// - Commit完了後はStable stateに復帰していることをcanonical validatorで検証する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
free_list_allocator_result_t free_list_allocator_allocate(free_list_allocator_t* free_list_allocator_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t required_block_size = 0;
    free_list_block_header_t* allocation_block = NULL;
    void* tmp_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_allocate", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_list_allocator_allocate", "*out_ptr_")
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Provided memory_tag_ is not valid.", rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Provided allocation_size_ is not valid.", rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    ret = allocation_block_size_calc(free_list_allocator_->payload_offset, allocation_size_, &required_block_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - allocation_block_size_calc failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    ret = free_block_find_first_fit(free_list_allocator_, required_block_size, &allocation_block);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_find_first_fit failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Commit eligibility.
    ret = allocation_is_ready(allocation_block, required_block_size, allocation_size_, memory_tag_);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - allocation_is_ready failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Commit.
    free_block_split(free_list_allocator_->minimum_block_size, allocation_block, required_block_size);
    free_block_allocate(free_list_allocator_->payload_offset, allocation_block, allocation_size_, memory_tag_, &tmp_ptr);

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_ptr_ = tmp_ptr;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// NOTE: 正常にallocateされたptr_で、かつvalidなfree_list_allocator_tであれば失敗することは基本ないためvoidにしても良いが、
// engine private moduleであるためresult codeを返すことにする
//
// free_list_allocator_free Validation Policy
//
// - free_list_allocator_のblock chainを安全に走査するため、Preconditionsではcanonical validatorを使用する。
// - ptr_は対象allocatorが現在保持するlive allocationのpayload先頭であることを確認する。
// - allocation pointer validationは、allocatorのstructural validation後に行う。
// - Commit完了後はStable stateに復帰していることをcanonical validatorで検証する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
free_list_allocator_result_t free_list_allocator_free(free_list_allocator_t* free_list_allocator_, void* ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* allocation_block = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_free", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_free", "ptr_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    // NOTE: allocation_ptr_is_validについてはパフォーマンス上の問題が出た場合はRELEASE_BUILDでの実行はやめる
    if(!allocation_ptr_is_valid(free_list_allocator_, ptr_)) {  // 内部でblockを走査するため、canonical validatorの後で実行する
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Provided ptr_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    allocation_block = (free_list_block_header_t*)((uintptr_t)ptr_ - free_list_allocator_->payload_offset);

    // Commit.
    allocated_block_free(allocation_block);
    free_block_coalesce(allocation_block);

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_) {
    const free_list_block_header_t* node = NULL;
    const free_list_block_header_t* prev_node = NULL;

    uintptr_t pool_address = 0;
    uintptr_t pool_end_address = 0;
    uintptr_t node_address = 0;
    uintptr_t next_node_address = 0;

    size_t remaining_size = 0;

    bool is_aligned = false;
    bool prev_node_is_free = false;

    if(!is_valid_shallow(free_list_allocator_)) {
        return false;
    }

    pool_address = (uintptr_t)free_list_allocator_->memory_pool;
    pool_end_address = pool_address + free_list_allocator_->memory_pool_size;
    node_address = pool_address;

    while(node_address < pool_end_address) {
        remaining_size = (size_t)(pool_end_address - node_address);

        // block headerを安全に読み取れる領域が残っていること
        if(remaining_size < sizeof(free_list_block_header_t)) {
            return false;
        }

        // block headerのaddress alignment
        if(!memory_utility_is_aligned(node_address, alignof(max_align_t), &is_aligned)) {
            return false;
        }
        if(!is_aligned) {
            return false;
        }

        node = (const free_list_block_header_t*)node_address;

        // block単体のlocal invariant
        if(!free_list_block_is_valid(node, free_list_allocator_->payload_offset, free_list_allocator_->minimum_block_size)) {
            return false;
        }

        // block全体がmemory pool内に収まること
        if(node->block_size > remaining_size) {
            return false;
        }

        // prev隣接状態
        if(node->prev != prev_node) {
            return false;
        }

        // Stable stateではFREE blockが隣接しないこと
        if(prev_node_is_free && FREE_LIST_BLOCK_STATE_FREE == node->block_state) {
            return false;
        }

        next_node_address = node_address + node->block_size;

        if(next_node_address == pool_end_address) {
            if(NULL != node->next) {
                return false;
            }
        }
        else {
            if(NULL == node->next) {
                return false;
            }
            if((uintptr_t)node->next != next_node_address) {
                return false;
            }
        }

        prev_node_is_free = (FREE_LIST_BLOCK_STATE_FREE == node->block_state);
        prev_node = node;
        node_address = next_node_address;
    }

    return node_address == pool_end_address;
}

// ============================================================
// Allocation Helpers
// ============================================================
static free_list_allocator_result_t allocation_block_size_calc(size_t payload_offset_, size_t allocation_size_, size_t* out_required_block_size_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t required_block_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_required_block_size_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocation_block_size_calc", "out_required_block_size_")
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == payload_offset_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided payload_offset_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < payload_offset_) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ overflow.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!memory_utility_align_up(allocation_size_ + payload_offset_, alignof(max_align_t), &required_block_size)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗するのはOVERFLOWのみ
        ERROR_MESSAGE("allocation_block_size_calc(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_required_block_size_ = required_block_size;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_free_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* block = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "out_free_block_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_free_block_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_block_find_first_fit", "*out_free_block_")
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    block = free_list_allocator_->head;
    while(NULL != block) {
        if(FREE_LIST_BLOCK_STATE_FREE == block->block_state && block->block_size >= required_block_size_) {
            break;
        }
        block = block->next;
    }
    if(NULL == block) {
        ret = FREE_LIST_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - free block not found.", rslt_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_free_block_ = block;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t allocation_is_ready(const free_list_block_header_t* allocation_block_, size_t required_block_size_, size_t allocation_size_, memory_tag_t memory_tag_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocation_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocation_is_ready", "allocation_block_")
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided allocation_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided memory_tag_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(required_block_size_ > allocation_block_->block_size) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(FREE_LIST_BLOCK_STATE_FREE != allocation_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_is_ready(%s) - Provided free_block_ is not freed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/*
 * Contract:
 * - free_list_allocator_allocate() のCommit eligibility成功後にのみ呼び出す。
 * - free_block_はcanonical-validなallocatorに属するFREE blockを指す。
 * - minimum_block_size_ はallocatorの有効なminimum block sizeである。
 * - required_block_size_はallocation_block_size_calc()によって算出されたalignment済みのblock sizeである
 *
 * このhelperは上記contractを再検証しない。contract成立下では失敗しない。
 */
static void free_block_split(size_t minimum_block_size_, free_list_block_header_t* free_block_, size_t required_block_size_) {
    void* split_block_address = NULL;
    free_list_block_header_t* split_block = NULL;
    free_list_block_header_t* next_block = NULL;

    size_t remaining_block_size = 0;

    // Prepare.
    remaining_block_size = free_block_->block_size - required_block_size_;
    if(minimum_block_size_ > remaining_block_size) {
        return;
    }

    split_block_address = (unsigned char*)free_block_ + required_block_size_;
    split_block = (free_list_block_header_t*)(split_block_address);
    next_block = free_block_->next;

    // Commit.
    if(NULL != next_block) { // free_block_が末尾ノードでなければ更新
        next_block->prev = split_block;
    }

    split_block->block_size = remaining_block_size;
    split_block->allocation_size = 0;
    split_block->block_state = FREE_LIST_BLOCK_STATE_FREE;
    split_block->prev = free_block_;
    split_block->next = next_block;
    free_block_->next = split_block;
    free_block_->block_size = required_block_size_;
}

/*
 * Contract:
 * - free_list_allocator_allocate() のCommit eligibility成功後にのみ呼び出す。
 * - free_block_はallocation対象として選択されたFREE blockを指す。
 * - free_block_のblock layoutとlinkageはCommit開始前に確立されたcontractを維持している。
 * - payload_offset_はallocatorの有効なpayload offsetである。
 * - allocation_size_ > 0 である。
 * - allocation_size_ はfree_block_のpayload領域に収まる。
 * - memory_tag_は有効なmemory tagである。
 * - out_ptr_ != NULL である。
 * - *out_ptr_ == NULL である。
 *
 * このhelperは上記contractを再検証しない。
 * contract成立下では失敗しない。
 */
static void free_block_allocate(size_t payload_offset_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    void* payload_address = NULL;

    // Prepare.
    payload_address = (unsigned char*)free_block_ + payload_offset_;

    // Commit.
    free_block_->allocation_size = allocation_size_;
    free_block_->block_state = FREE_LIST_BLOCK_STATE_ALLOCATED;
    free_block_->memory_tag = memory_tag_;

    // Output.
    *out_ptr_ = payload_address;
}

// ============================================================
// Free Helpers
// ============================================================
static bool allocation_ptr_is_valid(const free_list_allocator_t* free_list_allocator_, const void* ptr_) {
    bool ret = false;

    const free_list_block_header_t* block = NULL;
    const void* payload_address = NULL;

    // Preconditions.
    if(NULL == free_list_allocator_ || NULL == ptr_) {
        return false;
    }

    // Preflight.
    block = free_list_allocator_->head;
    while(NULL != block) {
        if(FREE_LIST_BLOCK_STATE_ALLOCATED == block->block_state) {
            payload_address = (const unsigned char*)block + free_list_allocator_->payload_offset;
            if(ptr_ == payload_address) {
                ret = true;
                break;
            }
        }
        block = block->next;
    }

    return ret;
}

/*
 * Contract:
 * - free_list_allocator_free()のPreconditions成功後、Commit中にのみ呼び出す。
 * - allocation_block_ != NULLである。
 * - allocation_block_は対象allocatorに属するALLOCATED blockを指す。
 * - block layout、block size、prev / next linkageはCommit開始前に確立されたcontractを維持している。
 *
 * このhelperは上記contractを再検証しない。
 * contract成立下では失敗しない。
 */
static void allocated_block_free(free_list_block_header_t* allocation_block_) {
    // Commit.
    allocation_block_->allocation_size = 0;
    allocation_block_->block_state = FREE_LIST_BLOCK_STATE_FREE;
}

/*
 * Contract:
 * - free_block_coalesce()がmerge条件を確認した直後に呼び出す。
 * - free_block_ != NULLである。
 * - free_block_はFREE blockを指す。
 * - free_block_->next != NULLである。
 * - free_block_->nextはFREE blockを指す。
 * - free_block_とfree_block_->nextは物理的に隣接している。
 * - 現在のfree_block_とnextが物理的に隣接し、linkageが整合している
 * - 2 blockのblock_sizeの加算はoverflowしない。
 *
 * このhelperは上記contractを再検証しない。
 * contract成立下では失敗しない。
 */
static void free_block_merge_next(free_list_block_header_t* free_block_) {
    free_list_block_header_t* next_block = NULL;
    free_list_block_header_t* next_next_block = NULL;

    size_t merged_block_size = 0;

    // Prepare.
    merged_block_size = free_block_->block_size + free_block_->next->block_size;    // validなfree_list_allocatorであればオーバーフローは起こらないためチェック不要
    next_block = free_block_->next;
    next_next_block = next_block->next;

    // Commit.
    if(NULL != next_next_block) {
        next_next_block->prev = free_block_;
    }
    free_block_->next = next_next_block;
    free_block_->block_size = merged_block_size;
}

/*
 * Contract:
 * - free_list_allocator_free()のCommit中、allocated_block_free()の直後にのみ呼び出す。
 * - free_block_ != NULLである。
 * - free_block_は直前のallocated_block_free()によってFREEへ遷移したblockを指す。
 * - free_block_のblock_size、prev、nextはfree_list_allocator_free()のCommit開始時点から変更されていない。
 *
 * このhelperは上記contractを再検証しない。
 * contract成立下では失敗しない。
 */
static void free_block_coalesce(free_list_block_header_t* free_block_) {
    // Commit.
    // 後方merge
    if(NULL != free_block_->next && FREE_LIST_BLOCK_STATE_FREE == free_block_->next->block_state) {
        free_block_merge_next(free_block_);
    }
    // 前方merge
    if(NULL != free_block_->prev && FREE_LIST_BLOCK_STATE_FREE == free_block_->prev->block_state) {
        free_block_merge_next(free_block_->prev);
    }
}

// ============================================================
// Utilities
// ============================================================
static const char* rslt_to_str(free_list_allocator_result_t rslt_) {
    switch(rslt_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return s_rslt_str_success;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return s_rslt_str_overflow;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_) {
    size_t expected_payload_offset = 0;
    size_t expected_minimum_block_size = 0;
    uintptr_t pool_address = 0;
    bool is_aligned = false;

    if(NULL == free_list_allocator_) {
        return false;
    }
    if(NULL == free_list_allocator_->memory_pool) {
        return false;
    }
    if(NULL == free_list_allocator_->head) {
        return false;
    }
    if(free_list_allocator_->head != free_list_allocator_->memory_pool) {
        return false;
    }

    if(!memory_utility_align_up(sizeof(free_list_block_header_t), alignof(max_align_t), &expected_payload_offset)) {
        return false;
    }
    if(free_list_allocator_->payload_offset != expected_payload_offset) {
        return false;
    }

    if((SIZE_MAX - expected_payload_offset) < alignof(max_align_t)) {
        return false;
    }
    expected_minimum_block_size = expected_payload_offset + alignof(max_align_t);

    if(free_list_allocator_->minimum_block_size != expected_minimum_block_size) {
        return false;
    }
    if(free_list_allocator_->memory_pool_size < free_list_allocator_->minimum_block_size) {
        return false;
    }

    pool_address = (uintptr_t)free_list_allocator_->memory_pool;

    if(!memory_utility_is_aligned(pool_address, alignof(max_align_t), &is_aligned)) {
        return false;
    }
    if(!is_aligned) {
        return false;
    }

    if((UINTPTR_MAX - pool_address) < free_list_allocator_->memory_pool_size) {
        return false;
    }

    return true;
}

static bool free_list_block_state_is_valid(free_list_block_state_t state_) {
    if(FREE_LIST_BLOCK_STATE_FREE != state_ && FREE_LIST_BLOCK_STATE_ALLOCATED != state_) {
        return false;
    }
    return true;
}

static bool free_list_block_is_valid(const free_list_block_header_t* block_, size_t payload_offset_, size_t minimum_block_size_) {
    if(NULL == block_) {
        return false;
    }
    if(block_->block_size < minimum_block_size_) {
        return false;
    }
    if(!free_list_block_state_is_valid(block_->block_state)) {
        return false;
    }
    if(FREE_LIST_BLOCK_STATE_FREE == block_->block_state) {
        if(0 != block_->allocation_size) {
            return false;
        }
    }
    else {
        if(0 == block_->allocation_size) {
            return false;
        }
        if(block_->allocation_size > (block_->block_size - payload_offset_)) {
            return false;
        }
        if(!memory_tag_is_valid(block_->memory_tag)) {
            return false;
        }
    }

    return true;
}
