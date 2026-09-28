// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/memory/general_allocator/general_allocator.h"

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/free_list_allocator/free_list_allocator.h"

/*
 * Module Internal Contract
 *
 * Stable state:
 * - initializedなGeneral Allocatorは、有効なowned backing memory poolとinitializedなFree List Allocatorを保持する。
 * - memory_pool != NULLである。
 * - free_list_allocator.memory_pool == memory_poolである。
 *
 * Backing memory:
 * - General Allocatorは自身が使用するbacking memory poolを所有する。
 * - GLCE_BUILD_MEMORY_POLICY_DESKTOPではbacking memory poolをmalloc()によって取得し、destroy時にfree()によって解放する。
 * - GLCE_BUILD_MEMORY_POLICY_EMBEDDEDではstatic storageをbacking memory poolとして使用し、
 *   destroy時にstorage自体の解放は行わない。
 * - どちらのBuild Policyでもdeinitialize後はmemory_pool == NULLへ戻す。
 * - backing memory pool上のphysical allocation管理はFree List Allocatorへ委譲する。
 *
 * Accounting:
 * - total_allocatedはcallerが要求した論理allocation sizeの累積値を表す。
 * - memory_tag_allocated[i]は各memory tagについて、callerが要求した論理allocation sizeの累積値を表す。
 * - Stable stateでは、sum(memory_tag_allocated[]) == total_allocatedが成立する。
 * - accounting値にはFree List Allocatorが管理するblock header、
 *   alignment paddingその他のphysical allocation overheadを含めない。
 *
 * State Transition:
 * - Public APIのentry / exitではStable stateを維持する。
 * - createではbacking memory pool取得からFree List Allocator initialization、
 *   accounting initialization完了までpartial initialization stateを許容する。
 * - allocateではFree List Allocatorのallocation成功からaccounting更新完了まで、
 *   Free List stateとGeneral Allocator accountingが一時的に異なる時点を許容する。
 * - freeではFree List Allocatorのfree成功からaccounting更新完了まで、
 *   同様のtransient stateを許容する。
 * - destroyではFree List Allocatorを先にdeinitializeし、
 *   その後backing memory poolとlogical accountingをdeinitializeする。
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
 *   General Allocator自身のlocal invariant、logical accounting invariant、
 *   backing memory poolとFree List Allocatorのownership relation、
 *   およびowned Free List Allocatorのcanonical validityを検証する。
 *
 * - shallow validatorはGeneral Allocator rootについて、
 *   operationを開始するために必要なlocal structural invariantのみを検証する。
 * - 現在のshallow validatorはmemory_poolが存在することを確認する。
 *
 * - accounting_is_valid()はGeneral Allocatorが所有するlogical accounting invariantを検証する。
 * - memory_tag_allocated[]の合計がoverflowせず計算可能であり、その合計がtotal_allocatedと一致することを要求する。
 * - General Allocatorのlogical accountingとFree List Allocatorが保持する
 *   live allocation payload総量のcross-layer整合性検証は将来検討する。
 *
 * - public APIのPreconditionsでは、General Allocator自身が直接使用または変更するstateについて必要なvalidationを行う。
 * - Free List Allocator固有のstateおよびsemantic invariantは、
 *   対応するFree List Allocator public APIへvalidationを委譲し、General Allocator側では重複して検証しない。
 *
 * - canonical validator内でfree_list_allocator_is_valid()を実行する。
 *   これはowned childを含むGeneral Allocator全体のownership closureを
 *   canonical validationするためであり、通常Public APIのPreconditionでFree List Allocator固有validationを
 *   重複実行することとは区別する。
 *
 * - Postcondition validationでは、
 *   current operationによってGeneral Allocator自身が変更したstateを検証する。
 * - Free List Allocator自身のPostcondition validationは
 *   対応するFree List Allocator APIへ委譲する。
 *
 * - DEBUG_BUILD / TEST_BUILD / RELEASE_BUILDごとのvalidation実行条件は、
 *   各Public APIのValidation Policyで個別に定義する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

typedef struct general_allocator {
    // Allocator
    free_list_allocator_t free_list_allocator;
    void* memory_pool;

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t memory_tag_allocated[GENERAL_ALLOCATOR_MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
} general_allocator_t;

// ============================================================
// Private Constants
// ============================================================
static general_allocator_t s_general_allocator;

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

static const char* const s_memory_tag_system = "SYSTEM";
static const char* const s_memory_tag_string = "STRING";
static const char* const s_memory_tag_ring_queue = "RING_QUEUE";
static const char* const s_memory_tag_renderer = "RENDERER";
static const char* const s_memory_tag_file_io = "FILE_IO";
static const char* const s_memory_tag_camera = "CAMERA";
static const char* const s_memory_tag_texture = "TEXTURE";
static const char* const s_memory_tag_geometry = "GEOMETRY";
static const char* const s_memory_tag_undefined = "UNDEFINED";

// 組み込み向けに静的領域でmemory poolを用意(アライメントはmax_align_tでfree list allocatorのメモリ要件を満たす)
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
alignas(max_align_t)
static unsigned char s_memory_pool[GLCE_BUILD_MEMORY_POOL_SIZE];
#endif

// ============================================================
// Private Function Declarations
// ============================================================
// Initialize, Deinitialize helpers
static general_allocator_result_t memory_pool_initialize(void);
static void memory_pool_deinitialize(void);
static void accounting_reset(void);
static bool general_allocator_is_initialized(void);

// Utilities
static const char* result_to_str(general_allocator_result_t result_);
static general_allocator_result_t result_convert_free_list_allocator(free_list_allocator_result_t result_);

// Validators
static bool is_valid_shallow(void);
static bool memory_tag_is_valid(general_allocator_memory_tag_t memory_tag_);
static bool accounting_is_valid(void);

// ============================================================
// Public API
// ============================================================

// general_allocator_create Validation Policy
//
// - create前にはcanonicalなGeneral Allocatorが存在しないため、PreconditionsではGeneral Allocatorのvalidatorを使用しない。
// - DEBUG_BUILD / TEST_BUILDでは、すべてのinitializationが完了したStable boundaryでcanonical validatorを実行する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
//
// NOTE: 以下は後日doxygenコメントとして正式に記載予定
// - initializedな状態での二重createはGENERAL_ALLOCATOR_BAD_OPERATIONとし、既存stateを変更せずearly returnする。
// - backing memory poolの取得とFree List Allocatorのinitializationが完了した後、accountingをzero stateへ初期化する。
// - recoverable failureではcreate中に取得したbacking memory resourceをrollbackする。
// - DATA_CORRUPTEDが確定した場合は通常rollbackを行わない。
// - create途中のrollbackに限り、canonical validationを行わずmemory poolをdeinitializeしてよい。
general_allocator_result_t general_allocator_create(void) {
    general_allocator_result_t ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    if(general_allocator_is_initialized()) {
        ret = GENERAL_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("general_allocator_create(%s) - general allocator is already initialized.", result_to_str(ret));
        return ret;
    }

    ret = memory_pool_initialize();
    if(GENERAL_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("general_allocator_create(%s) - memory_pool_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_free_list_allocator = free_list_allocator_initialize(GLCE_BUILD_MEMORY_POOL_SIZE, s_general_allocator.memory_pool, &s_general_allocator.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("general_allocator_create(%s) - free_list_allocator_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    accounting_reset();

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_create(%s) - Postcondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = GENERAL_ALLOCATOR_SUCCESS;

cleanup:
    if(GENERAL_ALLOCATOR_SUCCESS != ret && GENERAL_ALLOCATOR_DATA_CORRUPTED != ret) {
        memory_pool_deinitialize();
    }
    return ret;
}

// general_allocator_destroy Validation Policy
//
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、
//   General AllocatorがModule Internal Contractを満たすStable stateであることを検証する。
// - Free List Allocator固有のdeinitialize validationはfree_list_allocator_deinitialize()へ委譲する。
// - destroy完了後はinitializedなGeneral Allocatorが存在しないため、Postconditionsでcanonical validatorは使用しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
//
// NOTE: 以下は後日doxygenコメントとして正式に記載予定
// - Free List Allocatorのdeinitializeに成功した場合のみbacking memory poolをdeinitializeする。
// - backing memory poolのdeinitialize後にlogical accountingをzero stateへ戻す。
void general_allocator_destroy(void) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ERROR_MESSAGE("general_allocator_destroy(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    ret_free_list_allocator = free_list_allocator_deinitialize(&s_general_allocator.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_destroy(%s) - free_list_allocator_deinitialize failed.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }

    memory_pool_deinitialize();
    accounting_reset();
}

// general_allocator_allocate Validation Policy
//
// - out_ptr_はNULLでなく、*out_ptr_ == NULLであることを要求する。
// - allocation_size_は0より大きいことを要求する。
// - memory_tag_は有効なgeneral_allocator_memory_tag_tであることを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorとaccounting validatorを実行する。
// - Free List Allocator固有のallocation validationはfree_list_allocator_allocate()へ委譲する。
// - logical accountingの加算がsize_tの表現可能範囲を超えないことを
//   Commit前に全BUILDで検証し、超える場合はGENERAL_ALLOCATOR_LIMIT_EXCEEDEDとする。
// - DEBUG_BUILD / TEST_BUILDではaccounting更新後のStable boundaryで
//   accounting validatorを実行する。
// - Postcondition validationでDATA_CORRUPTEDが確定した場合はallocationをrollbackせず、output pointerをcallerへ公開しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
general_allocator_result_t general_allocator_allocate(size_t allocation_size_, general_allocator_memory_tag_t memory_tag_, void** out_ptr_) {
    general_allocator_result_t ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, GENERAL_ALLOCATOR_INVALID_ARGUMENT, result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT), "general_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, GENERAL_ALLOCATOR_BAD_OPERATION, result_to_str(GENERAL_ALLOCATOR_BAD_OPERATION), "general_allocator_allocate", "*out_ptr_")
    if(0 == allocation_size_) {
        ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Provided memory_tag_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if((SIZE_MAX - allocation_size_) < s_general_allocator.total_allocated) {
        ret = GENERAL_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - general allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < s_general_allocator.memory_tag_allocated[memory_tag_]) {
        ret = GENERAL_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - general allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    ret_free_list_allocator = free_list_allocator_allocate(&s_general_allocator.free_list_allocator, allocation_size_, &tmp_ptr);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("general_allocator_allocate(%s) - free_list_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_ptr, 0, allocation_size_);

    s_general_allocator.memory_tag_allocated[memory_tag_] += allocation_size_;
    s_general_allocator.total_allocated += allocation_size_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Postcondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_ptr_ = tmp_ptr;

    ret = GENERAL_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// general_allocator_free Validation Policy
//
// - ptr_および*ptr_はNULLでないことを要求する。
// - memory_tag_は有効なgeneral_allocator_memory_tag_tであることを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorとaccounting validatorを実行する。
// - allocation identityおよびallocation sizeの検証はfree_list_allocator_allocation_info_get()へ委譲する。
// - accounting減算前にtotal_allocatedおよび対象memory tagの値がallocation size以上であることを全BUILDで検証する。
// - DEBUG_BUILD / TEST_BUILDではaccounting更新後のStable boundaryでaccounting validatorを実行する。
// - Postcondition validationに成功した場合のみcallerが保持するpointerをNULLへ変更する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
//
// NOTE: 以下は後日doxygenコメントとして正式に記載予定
// - 指定pointerが現在のlive allocationではない場合はAPI misuseとしてGENERAL_ALLOCATOR_BAD_OPERATION相当のerrorを報告する。
// - Free List Allocatorからそれ以外のfailureを受けた場合は
//   General Allocator内部の整合性を信頼できないためDATA_CORRUPTEDとして扱う。
// - Free List Allocatorによるfree成功後にlogical accountingを更新する。
void general_allocator_free(void** ptr_, general_allocator_memory_tag_t memory_tag_) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t allocation_size = 0;

    // Preconditions.
    if(NULL == ptr_ || NULL == *ptr_) {
        ERROR_MESSAGE("general_allocator_free(%s) - Provided ptr_ is not valid.", result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT));
        return;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ERROR_MESSAGE("general_allocator_free(%s) - Provided memory_tag_ is not valid.", result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow()) {
        ERROR_MESSAGE("general_allocator_free(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
    if(!accounting_is_valid()) {
        ERROR_MESSAGE("general_allocator_free(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    // Prepare.
    ret_free_list_allocator = free_list_allocator_allocation_info_get(&s_general_allocator.free_list_allocator, *ptr_, &allocation_size);
    // FREE_LIST_ALLOCATOR_INVALID_ARGUMENTを返した場合はptr_がaliveではないため、BAD_OPERATIONエラーを表示する
    if(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT == ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_free(%s) - Provided ptr_ is already freed.", result_to_str(GENERAL_ALLOCATOR_BAD_OPERATION));
        return;
    }
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
    if(s_general_allocator.total_allocated < allocation_size) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
    if(s_general_allocator.memory_tag_allocated[memory_tag_] < allocation_size) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }

    // Commit.
    ret_free_list_allocator = free_list_allocator_free(&s_general_allocator.free_list_allocator, *ptr_);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }

    s_general_allocator.total_allocated -= allocation_size;
    s_general_allocator.memory_tag_allocated[memory_tag_] -= allocation_size;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid()) {
        ERROR_MESSAGE("general_allocator_free(%s) - Postcondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    *ptr_ = NULL;
}

// general_allocator_ptr_is_allocated Validation Policy
//
// - ptr_ == NULLの場合はfalseを返す。
// - General Allocator側ではallocation identityを独自に検証せず、
//   free_list_allocator_ptr_is_allocated()へqueryを委譲する。
// - Free List Allocator固有のvalidationおよびallocation-state semanticsは
//   Free List Allocatorのcontractに従う。
// - 本APIはstateを変更しないためPostcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool general_allocator_ptr_is_allocated(const void* ptr_) {
    if(NULL == ptr_) {
        return false;
    }

    return free_list_allocator_ptr_is_allocated(&s_general_allocator.free_list_allocator, ptr_);
}

// general_allocator_status_get Validation Policy
//
// - out_status_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorとaccounting validatorを実行する。
// - Free List Allocatorが所有するphysical memory statusのvalidationと取得はfree_list_allocator_status_get()へ委譲する。
// - General Allocator自身が所有するlogical accountingは
//   validation済みstateからstatusへcopyする。
// - 本APIはallocator stateを変更しないためPostcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
general_allocator_result_t general_allocator_status_get(general_allocator_status_t* out_status_) {
    general_allocator_result_t ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_status_t free_list_allocator_status = { 0 };

    size_t memory_pool_size = 0;
    size_t allocated_block_size = 0;
    size_t free_block_size = 0;
    size_t allocated_block_count = 0;
    size_t free_block_count = 0;
    size_t largest_free_block_size = 0;
    size_t max_allocation_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_status_, ret, GENERAL_ALLOCATOR_INVALID_ARGUMENT, result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT), "general_allocator_status_get", "out_status_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_status_get(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_status_get(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    ret_free_list_allocator = free_list_allocator_status_get(&s_general_allocator.free_list_allocator, &free_list_allocator_status);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("general_allocator_status_get(%s) - free_list_allocator_status_get failed.", result_to_str(ret));
        goto cleanup;
    }

    memory_pool_size = free_list_allocator_status.memory_pool_size;
    allocated_block_size = free_list_allocator_status.allocated_block_size;
    free_block_size = free_list_allocator_status.free_block_size;
    allocated_block_count = free_list_allocator_status.allocated_block_count;
    free_block_count = free_list_allocator_status.free_block_count;
    largest_free_block_size = free_list_allocator_status.largest_free_block_size;
    max_allocation_size = free_list_allocator_status.max_allocation_size;

    // Output.
    out_status_->total_allocated = s_general_allocator.total_allocated;
    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        out_status_->memory_tag_allocated[i] = s_general_allocator.memory_tag_allocated[i];
    }
    out_status_->allocated_block_count = allocated_block_count;
    out_status_->allocated_block_size = allocated_block_size;
    out_status_->free_block_count = free_block_count;
    out_status_->free_block_size = free_block_size;
    out_status_->largest_free_block_size = largest_free_block_size;
    out_status_->max_allocation_size = max_allocation_size;
    out_status_->memory_pool_size = memory_pool_size;

    ret = GENERAL_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// general_allocator_memory_tag_to_str Validation Policy
//
// - allocator stateへ依存しないpure queryとして扱い、allocator validatorは使用しない。
// - 定義済みmemory tagには対応する文字列を返す。
// - stateを変更しないためPostcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
const char* general_allocator_memory_tag_to_str(general_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM:
        return s_memory_tag_system;
    case GENERAL_ALLOCATOR_MEMORY_TAG_STRING:
        return s_memory_tag_string;
    case GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE:
        return s_memory_tag_ring_queue;
    case GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER:
        return s_memory_tag_renderer;
    case GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO:
        return s_memory_tag_file_io;
    case GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA:
        return s_memory_tag_camera;
    case GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE:
        return s_memory_tag_texture;
    case GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY:
        return s_memory_tag_geometry;
    case GENERAL_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}

// general_allocator_is_valid Validation Policy
//
// - 本APIはGeneral Allocatorのcanonical validatorである。
// - shallow invariantを検証する。
// - logical accounting invariantを検証する。
// - General Allocatorが保持するmemory_poolとowned Free List Allocatorが保持する
//   memory_poolが同一であることを検証する。
// - owned Free List Allocatorについてfree_list_allocator_is_valid()を実行し、
//   ownership closureを含めてcanonical validationする。
// - explicit validation APIであるためBUILD modeに関係なくcanonical validationを実行する。
// - validation中にstateを変更しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool general_allocator_is_valid(void) {
    if(!is_valid_shallow()) {
        return false;
    }
    if(!accounting_is_valid()) {
        return false;
    }
    if(s_general_allocator.memory_pool != s_general_allocator.free_list_allocator.memory_pool) {
        return false;
    }
    if(!free_list_allocator_is_valid(&s_general_allocator.free_list_allocator)) {
        return false;
    }
    return true;
}

// ============================================================
// Initialize, Deinitialize helpers
// ============================================================
static general_allocator_result_t memory_pool_initialize(void) {
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)  // 組み込み用静的メモリ領域(mallocを使わない)
    s_general_allocator.memory_pool = (void*)s_memory_pool;
#elif defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    s_general_allocator.memory_pool = malloc(GLCE_BUILD_MEMORY_POOL_SIZE);  // デスクトップ用ヒープメモリ領域(mallocで確保)
#else
    return GENERAL_ALLOCATOR_UNDEFINED_ERROR;
#endif

    if(NULL == s_general_allocator.memory_pool) {
        return GENERAL_ALLOCATOR_NO_MEMORY;
    }

    return GENERAL_ALLOCATOR_SUCCESS;
}

// memory_pool_deinitialize()の実行前に必ずcanonical validatorによる検証を済ませておくこと(create APIの失敗時ロールバックは除く)
static void memory_pool_deinitialize(void) {
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    if(NULL != s_general_allocator.memory_pool) {
        free(s_general_allocator.memory_pool);
    }
#endif

    s_general_allocator.memory_pool = NULL;
}

static void accounting_reset(void) {
    s_general_allocator.total_allocated = 0;
    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        s_general_allocator.memory_tag_allocated[i] = 0;
    }
}

static bool general_allocator_is_initialized(void) {
    return NULL != s_general_allocator.memory_pool;
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static general_allocator_result_t result_convert_free_list_allocator(free_list_allocator_result_t result_) {
    switch(result_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return GENERAL_ALLOCATOR_SUCCESS;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return GENERAL_ALLOCATOR_DATA_CORRUPTED;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return GENERAL_ALLOCATOR_BAD_OPERATION;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return GENERAL_ALLOCATOR_NO_MEMORY;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return GENERAL_ALLOCATOR_OVERFLOW;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return GENERAL_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return GENERAL_ALLOCATOR_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(void) {
    if(NULL == s_general_allocator.memory_pool) {
        return false;
    }
    return true;
}

static bool memory_tag_is_valid(general_allocator_memory_tag_t memory_tag_) {
    if(memory_tag_ >= GENERAL_ALLOCATOR_MEMORY_TAG_MAX || 0 > (int)memory_tag_) {
        return false;
    }
    return true;
}

static bool accounting_is_valid(void) {
    size_t expected_total_size = 0;
    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        if((SIZE_MAX - s_general_allocator.memory_tag_allocated[i]) < expected_total_size) {
            return false;
        }
        expected_total_size += s_general_allocator.memory_tag_allocated[i];
    }
    if(s_general_allocator.total_allocated != expected_total_size) {
        return false;
    }
    return true;
}
