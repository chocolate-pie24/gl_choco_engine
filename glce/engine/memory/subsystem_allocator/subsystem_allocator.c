// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

/*
 * Module Internal Contract
 *
 * Stable state:
 * - initializedなsubsystem_allocator_tは、
 *   有効なowned backing memory poolとinitializedなLinear Allocatorを保持する。
 * - linear_allocator_pool != NULLである。
 * - linear_allocator.memory_pool == linear_allocator_poolである。
 *
 * Ownership:
 * - subsystem_allocator_t自身のstorageはGeneral Allocatorから取得し、Subsystem Allocatorが所有する。
 * - linear_allocator_poolはGeneral Allocatorから取得し、Subsystem Allocatorが所有する。
 * - destroy時はlinear_allocator_poolを先に解放し、その後subsystem_allocator_t自身を解放する。
 *
 * Accounting:
 * - total_allocatedはcallerが要求した論理allocation sizeの累積値を表す。
 * - memory_tag_allocated[i]は各memory tagについて、callerが要求した論理allocation sizeの累積値を表す。
 * - Stable stateでは、sum(memory_tag_allocated[]) == total_allocatedが成立する。
 * - accounting値にはLinear Allocatorが消費するalignment paddingを含めない。
 *
 * Allocation:
 * - Linear Allocatorによるallocation成功後、callerへ返すallocation_size byteを0で初期化する。
 * - allocation成功時はtotal_allocatedおよび対応するmemory tag accountingへallocation_sizeを加算する。
 *
 * Reset:
 * - physical allocation stateのresetはembedded Linear Allocatorへ委譲する。
 * - Linear Allocatorのreset成功後、total_allocatedおよびすべてのmemory tag accountingを0へ戻す。
 *
 * Rollback:
 * - rollback pointはLinear Allocatorのoffset snapshotと、Subsystem Allocatorのlogical accounting snapshotを保持する。
 * - Linear Allocatorのrollbackが成功した後、total_allocatedおよびmemory tag accountingをrollback pointのsnapshotへ戻す。
 * - Linear Allocator固有のrollback offset semanticsはSubsystem Allocatorでは再定義しない。
 *
 * Pointer range query:
 * - pointer range判定はembedded Linear Allocatorへ委譲する。
 * - Subsystem Allocator独自のallocation identityまたはallocation boundaryは保持しない。
 *
 * State Transition:
 * - Public APIのentry / exitではStable stateを維持する。
 * - allocateではLinear Allocatorのallocation成功からaccounting更新完了まで、
 *   Linear stateとSubsystem accountingが一時的に異なる時点を許容する。
 * - resetではLinear Allocatorのreset成功からaccounting reset完了まで、
 *   同様のtransient stateを許容する。
 * - rollbackではLinear Allocatorのrollback成功からaccounting restore完了まで、
 *   同様のtransient stateを許容する。
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
 *   Subsystem Allocator自身のlocal invariant、logical accounting invariant、
 *   owned backing memory poolとLinear Allocatorのownership relation、
 *   およびowned Linear Allocatorのcanonical validityを検証する。
 *
 * - shallow validatorはSubsystem Allocator rootについて、
 *   operationを開始するために必要なlocal structural invariantのみを検証する。
 * - 現在のshallow validatorはlinear_allocator_poolが存在することを確認する。
 *
 * - accounting_is_valid()はSubsystem Allocatorが所有するlogical accounting invariantを検証する。
 * - memory_tag_allocated[]の合計がoverflowせず計算可能であり、
 *   その合計がtotal_allocatedと一致することを要求する。
 *
 * - public APIのPreconditionsでは、Subsystem Allocator自身が直接使用または変更するstateについて必要なvalidationを行う。
 * - Linear Allocator固有のstateおよびsemantic invariantは、対応するLinear Allocator public APIへvalidationを委譲し、
 *   Subsystem Allocator側では重複して検証しない。
 *
 * - canonical validator内でlinear_allocator_is_valid()を実行する。
 *   これはowned childを含むSubsystem Allocator全体のownership closureをcanonical validationするためであり、
 *   通常Public APIのPreconditionでLinear Allocator固有validationを重複実行することとは区別する。
 *
 * - Postcondition validationでは、
 *   current operationによってSubsystem Allocator自身が変更したstateを検証する。
 * - Linear Allocator自身のPostcondition validationは対応するLinear Allocator APIへ委譲する。
 *
 * - DATA_CORRUPTEDが確定した後は通常cleanupを継続しない。
 * - create途中のrecoverable failureでは、
 *   それまでにGeneral Allocatorから取得したtemporary resourceをcleanupする。
 *
 * - DEBUG_BUILD / TEST_BUILD / RELEASE_BUILDごとのvalidation実行条件は、
 *   各Public APIのValidation Policyで個別に定義する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

struct subsystem_allocator {
    void* linear_allocator_pool;             /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_allocator_t linear_allocator;    /**< リニアアロケータ構造体インスタンス */

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t memory_tag_allocated[SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
};

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

static const char* const s_memory_tag_platform = "PLATFORM_SYSTEM";
static const char* const s_memory_tag_renderer = "RENDERER_SYSTEM";
static const char* const s_memory_tag_event = "EVENT_SYSTEM";
static const char* const s_memory_tag_camera = "CAMERA_SYSTEM";
static const char* const s_memory_tag_undefined = "UNDEFINED";

// ============================================================
// Private Function Declarations
// ============================================================
// Initialize, Deinitialize helpers
static void accounting_reset(subsystem_allocator_t* allocator_);

// Utilities
static const char* result_to_str(subsystem_allocator_result_t result_);
static subsystem_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_);
static subsystem_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_);

// Validators
static bool is_valid_shallow(const subsystem_allocator_t* allocator_);
static bool memory_tag_is_valid(subsystem_allocator_memory_tag_t memory_tag_);
static bool accounting_is_valid(const subsystem_allocator_t* allocator_);

// ============================================================
// Public API
// ============================================================
// subsystem_allocator_create Validation Policy
//
// - out_allocator_はNULLでなく、*out_allocator_ == NULLであることを要求する。
// - memory_pool_size_は0より大きいことを要求する。
// - create前には有効なsubsystem_allocator_tが存在しないため、PreconditionsではSubsystem Allocatorのvalidatorを使用しない。
//
// - allocator objectおよびbacking memory poolのallocationに関するvalidationはGeneral Allocatorへ委譲する。
// - Linear Allocatorのinitializationに関するvalidationはLinear Allocatorへ委譲する。
//
// - DEBUG_BUILD / TEST_BUILDでは、allocator object、backing memory pool、
//   Linear Allocatorおよびlogical accountingの初期化が完了したStable boundaryでcanonical validatorを実行する。
// - canonical validation成功後に*out_allocator_へ完成済みobjectを公開する。
// - RELEASE_BUILDではautomatic canonical validationを行わない。
//
// - recoverable failureではcreate中に取得したtemporary resourceをcleanupする。
// - DATA_CORRUPTEDが確定した場合は通常cleanupを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_create(size_t memory_pool_size_, subsystem_allocator_t** out_allocator_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    subsystem_allocator_t* tmp_allocator = NULL;
    void* tmp_memory_pool = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_create", "out_allocator_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_allocator_, ret, SUBSYSTEM_ALLOCATOR_BAD_OPERATION, result_to_str(SUBSYSTEM_ALLOCATOR_BAD_OPERATION), "subsystem_allocator_create", "*out_allocator_")
    if(0 == memory_pool_size_) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_create(%s) - Provided memory_pool_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(subsystem_allocator_t), GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, (void**)&tmp_allocator);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(memory_pool_size_, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, &tmp_memory_pool);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_linear_allocator = linear_allocator_initialize(&tmp_allocator->linear_allocator, memory_pool_size_, tmp_memory_pool);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - linear_allocator_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_allocator->linear_allocator_pool = tmp_memory_pool;
    accounting_reset(tmp_allocator);

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!subsystem_allocator_is_valid(tmp_allocator)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_create(%s) - Postcondition validation failed for 'tmp_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_allocator_ = tmp_allocator;
    tmp_allocator = NULL;
    tmp_memory_pool = NULL;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    if(SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED != ret) {
        if(NULL != tmp_memory_pool) {
            general_allocator_free((void**)&tmp_memory_pool, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
        }
        if(NULL != tmp_allocator) {
            general_allocator_free((void**)&tmp_allocator, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
        }
    }

    return ret;
}

// subsystem_allocator_destroy Validation Policy
//
// - allocator_ == NULLまたは*allocator_ == NULLの場合は何も行わずreturnする。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、
//   Subsystem Allocator自身、owned backing memory pool、logical accounting、
//   およびowned Linear Allocatorを含むownership closureがvalidなStable stateであることを確認する。
// - canonical validationに失敗した場合はownership releaseを行わずreturnする。
//
// - backing memory poolおよびallocator objectのfreeに関するvalidationはGeneral Allocatorへ委譲する。
// - RELEASE_BUILDではautomatic canonical validationを行わず、Module Internal Contractが成立していることを前提としてdestroyを実行する。
// - destroyによってobject lifetimeが終了するため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void subsystem_allocator_destroy(subsystem_allocator_t** allocator_) {
    if(NULL == allocator_) {
        return;
    }
    if(NULL == *allocator_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!subsystem_allocator_is_valid(*allocator_)) {
        ERROR_MESSAGE("subsystem_allocator_destroy(%s) - Precondition validation failed for 'allocator_'.", result_to_str(SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    accounting_reset(*allocator_);
    general_allocator_free((void**)&(*allocator_)->linear_allocator_pool, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    general_allocator_free((void**)allocator_, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
}

// subsystem_allocator_allocate Validation Policy
//
// - allocator_およびout_ptr_はNULLでなく、*out_ptr_ == NULLであることを要求する。
// - memory_tag_はSubsystem Allocatorが定義する有効なmemory tagであることを要求する。
// - allocation_size_は0より大きいことを要求する。
//
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorを使用し、
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - DEBUG_BUILD / TEST_BUILDではaccounting_is_valid()を使用し、現在のlogical accountingが整合していることを確認する。
// - Linear Allocator固有のstate validationはlinear_allocator_allocate()へ委譲する。
//
// - total_allocatedおよび対象memory tagのaccountingへallocation_size_を加算した結果が
//   size_tで管理可能な上限を超えないことを全BUILDでLinear allocation開始前に検証する。
// - 上限を超える場合はSUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDEDを返し、allocator stateを変更しない。
//
// - Linear Allocatorによるphysical allocation成功後にlogical accountingを更新する。
// - DEBUG_BUILD / TEST_BUILDではaccounting更新完了後のStable boundaryで
//   accounting_is_valid()を実行し、Subsystem Allocator自身が変更したlogical accountingのPostconditionを確認する。
// - Linear Allocator自身のPostcondition validationはLinear Allocatorへ委譲する。
// - RELEASE_BUILDではautomatic shallow / accounting validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_allocate(subsystem_allocator_t* allocator_, size_t allocation_size_, subsystem_allocator_memory_tag_t memory_tag_, void** out_ptr_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, SUBSYSTEM_ALLOCATOR_BAD_OPERATION, result_to_str(SUBSYSTEM_ALLOCATOR_BAD_OPERATION), "subsystem_allocator_allocate", "*out_ptr_")
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Provided memory_tag_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if((SIZE_MAX - allocation_size_) < allocator_->total_allocated) {
        ret = SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - subsystem allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < allocator_->memory_tag_allocated[memory_tag_]) {
        ret = SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - subsystem allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    ret_linear_allocator = linear_allocator_allocate(&allocator_->linear_allocator, allocation_size_, (void**)&tmp_ptr);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - linear_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_ptr, 0, allocation_size_);

    allocator_->total_allocated += allocation_size_;
    allocator_->memory_tag_allocated[memory_tag_] += allocation_size_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_ptr_ = tmp_ptr;
    tmp_ptr = NULL;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_reset Validation Policy
//
// - allocator_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorを使用し、
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - DEBUG_BUILD / TEST_BUILDではaccounting_is_valid()を使用し、reset前のlogical accountingが整合していることを確認する。
// - resetはcorruptedなlogical accountingを修復するためのAPIとして扱わない。
// - Linear Allocator固有のstate validationはlinear_allocator_reset()へ委譲する。
//
// - Linear Allocatorのreset成功後にtotal_allocatedおよびmemory_tag_allocated[]を0へ戻す。
// - DEBUG_BUILD / TEST_BUILDではaccounting reset完了後のStable boundaryで
//   accounting_is_valid()を実行し、Subsystem Allocator自身が変更した
//   logical accountingのPostconditionを確認する。
// - Linear Allocator自身のPostcondition validationはLinear Allocatorへ委譲する。
// - RELEASE_BUILDではautomatic shallow / accounting validationを行わず、
//   Module Internal Contractが成立していることを前提としてresetを実行する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_reset(subsystem_allocator_t* allocator_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_reset", "allocator_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    ret_linear_allocator = linear_allocator_reset(&allocator_->linear_allocator);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - linear_allocator_reset failed.", result_to_str(ret));
        goto cleanup;
    }
    accounting_reset(allocator_);

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_rollback_point_get Validation Policy
//
// - allocator_およびout_rollback_point_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorを使用し、
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - DEBUG_BUILD / TEST_BUILDではaccounting_is_valid()を使用し、
//   snapshot対象となるlogical accountingが整合していることを確認する。
// - Linear Allocatorのrollback offset snapshotに関するvalidationは
//   linear_allocator_rollback_point_get()へ委譲する。
//
// - 本APIはallocator stateを変更しないread-only operationであるため、
//   Postcondition validationは行わない。
// - RELEASE_BUILDではautomatic shallow / accounting validationを行わない。
// - rollback pointのlifetimeおよびprovenanceはModule Boundary Contractに従う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_rollback_point_get(const subsystem_allocator_t* allocator_, subsystem_allocator_rollback_point_t* out_rollback_point_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_rollback_point_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_rollback_point_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_rollback_point_get", "out_rollback_point_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback_point_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback_point_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    ret_linear_allocator = linear_allocator_rollback_point_get(&allocator_->linear_allocator, &rollback_point);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_rollback_point_get(%s) - linear_allocator_rollback_point_get failed.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    out_rollback_point_->total_allocated = allocator_->total_allocated;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        out_rollback_point_->memory_tag_allocated[i] = allocator_->memory_tag_allocated[i];
    }
    out_rollback_point_->offset = rollback_point.offset;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_rollback Validation Policy
//
// - allocator_およびrollback_point_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorを使用し、
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - DEBUG_BUILD / TEST_BUILDではaccounting_is_valid()を使用し、
//   rollback前のcurrent logical accountingが整合していることを確認する。
//
// - rollback_point_に保存されたmemory_tag_allocated[]について、
//   合計値をsize_tの範囲内で計算可能であることを全BUILDで検証する。
// - memory_tag_allocated[]の合計値がrollback_point_->total_allocatedと
//   一致することを全BUILDで検証する。
// - これらはSubsystem Allocatorが所有するlogical accounting snapshotの
//   API Contract validationとして行う。
//
// - rollback_point_->offsetのalignment、current used rangeとの関係、
//   その他のLinear Allocator固有semanticはSubsystem Allocatorでは検証せず、
//   linear_allocator_rollback()へ委譲する。
// - rollback pointの取得元allocator、generationおよびallocation historyは検証せず、
//   Module Boundary Contractとしてcallerに要求する。
//
// - Linear Allocatorのrollback成功後にlogical accountingをrollback pointのsnapshotへ戻す。
// - DEBUG_BUILD / TEST_BUILDではaccounting restore完了後のStable boundaryで
//   shallow validatorおよびaccounting_is_valid()を実行し、
//   Subsystem Allocator自身のPostconditionを確認する。
// - Linear Allocator自身のPostcondition validationはLinear Allocatorへ委譲する。
// - RELEASE_BUILDではautomatic shallow / current accounting validationを行わない。
//   rollback_point_自身に対するAPI Contract validationは全BUILDで行う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_rollback(subsystem_allocator_t* allocator_, const subsystem_allocator_rollback_point_t* rollback_point_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_rollback_point_t rollback_point = { 0 };
    size_t expected_total_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_rollback", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(rollback_point_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_rollback", "rollback_point_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        if((SIZE_MAX - rollback_point_->memory_tag_allocated[i]) < expected_total_size) {
            ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
            ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Provided rollback_point_ is not valid.", result_to_str(ret));
            goto cleanup;
        }
        expected_total_size += rollback_point_->memory_tag_allocated[i];
    }
    if(expected_total_size != rollback_point_->total_allocated) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Provided rollback_point_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    rollback_point.offset = rollback_point_->offset;
    ret_linear_allocator = linear_allocator_rollback(&allocator_->linear_allocator, &rollback_point);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - linear_allocator_rollback failed.", result_to_str(ret));
        goto cleanup;
    }

    allocator_->total_allocated = rollback_point_->total_allocated;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        allocator_->memory_tag_allocated[i] = rollback_point_->memory_tag_allocated[i];
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_rollback(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_status_get Validation Policy
//
// - allocator_およびout_status_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでshallow validatorを使用し、
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - DEBUG_BUILD / TEST_BUILDではaccounting_is_valid()を使用し、
//   statusへ出力するlogical accountingが整合していることを確認する。
// - Linear Allocatorが所有するphysical memory statusのvalidationは
//   linear_allocator_status_get()へ委譲する。
//
// - 本APIはallocator stateを変更しないread-only operationであるため、
//   Postcondition validationは行わない。
// - RELEASE_BUILDではautomatic shallow / accounting validationを行わず、
//   Module Internal Contractが成立していることを前提としてstatusを取得する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_status_get(const subsystem_allocator_t* allocator_, subsystem_allocator_status_t* out_status_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_status_t linear_allocator_status = { 0 };
    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_status_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_status_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_status_get", "out_status_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_status_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!accounting_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_status_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    ret_linear_allocator = linear_allocator_status_get(&allocator_->linear_allocator, &linear_allocator_status);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_status_get(%s) - linear_allocator_status_get failed.", result_to_str(ret));
        goto cleanup;
    }

    memory_pool_size = linear_allocator_status.memory_pool_size;
    used_size = linear_allocator_status.used_size;
    free_size = linear_allocator_status.free_size;

    // Output.
    out_status_->free_size = free_size;
    out_status_->memory_pool_size = memory_pool_size;
    out_status_->used_size = used_size;
    out_status_->total_allocated = allocator_->total_allocated;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        out_status_->memory_tag_allocated[i] = allocator_->memory_tag_allocated[i];
    }

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_ptr_is_in_use_range Validation Policy
//
// - allocator_またはptr_がNULLの場合はfalseを返す。
// - Subsystem Allocator自身のshallow / canonical validationは行わない。
// - pointer rangeのsemanticおよびLinear Allocator自身のstate validationはlinear_allocator_ptr_is_in_use_range()へ委譲する。
// - Subsystem AllocatorではLinear Allocator固有のrange conditionを重複して検証しない。
//
// - 本APIはallocator stateを変更しないread-only queryであるため、
//   Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool subsystem_allocator_ptr_is_in_use_range(const subsystem_allocator_t* allocator_, const void* ptr_) {
    if(NULL == allocator_ || NULL == ptr_) {
        ERROR_MESSAGE("subsystem_allocator_ptr_is_in_use_range(%s) - Provided allocator_ or ptr_ is not valid.", result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT));
        return false;
    }

    return linear_allocator_ptr_is_in_use_range(&allocator_->linear_allocator, ptr_);
}

// subsystem_allocator_memory_tag_to_str Validation Policy
//
// - memory_tag_に対してvalidatorは使用しない。
// - 定義済みmemory tagは対応する文字列へ変換する。
// - SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAXおよび定義範囲外の値は
//   UNDEFINEDを表す文字列へ変換する。
// - 本APIはstateを参照または変更しないため、
//   shallow / canonical / Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
const char* subsystem_allocator_memory_tag_to_str(subsystem_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_PLATFORM:
        return s_memory_tag_platform;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER:
        return s_memory_tag_renderer;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_EVENT:
        return s_memory_tag_event;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA:
        return s_memory_tag_camera;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}

// subsystem_allocator_is_valid Validation Policy
//
// - 本APIはSubsystem Allocatorのpublic canonical validatorである。
// - allocator_ == NULLの場合はfalseを返す。
// - shallow validatorを最初に実行し、canonical validationを安全に継続できる
//   Subsystem Allocator rootのlocal structural invariantを確認する。
// - accounting_is_valid()によってSubsystem Allocatorが所有するlogical accounting invariantを検証する。
// - linear_allocator_poolとLinear Allocatorが参照するmemory_poolの一致を、
//   Subsystem Allocatorが所有するownership relationとして検証する。
// - owned Linear Allocatorに対してlinear_allocator_is_valid()を実行し、
//   ownership closureをcanonical validationする。
// - owned Linear Allocatorのcanonical validationは、通常Public APIにおける
//   Linear固有validationの重複ではなく、Subsystem Allocator全体のcanonical modelを検証する責務として行う。
//
// - explicit validatorであるためBUILD_MODEによってvalidation semanticsを変更しない。
// - allocator stateを変更せず、validation failure時もfalseを返すのみとする。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool subsystem_allocator_is_valid(const subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(!is_valid_shallow(allocator_)) {
        return false;
    }
    if(!accounting_is_valid(allocator_)) {
        return false;
    }
    if(allocator_->linear_allocator_pool != allocator_->linear_allocator.memory_pool) {
        return false;
    }
    if(!linear_allocator_is_valid(&allocator_->linear_allocator)) {
        return false;
    }
    return true;
}

// ============================================================
// Initialize, Deinitialize helpers
// ============================================================
static void accounting_reset(subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return;
    }
    allocator_->total_allocated = 0;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        allocator_->memory_tag_allocated[i] = 0;
    }
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static subsystem_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return SUBSYSTEM_ALLOCATOR_SUCCESS;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return SUBSYSTEM_ALLOCATOR_NO_MEMORY;
    case LINEAR_ALLOCATOR_DATA_CORRUPTED:
        return SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
    case LINEAR_ALLOCATOR_BAD_OPERATION:
        return SUBSYSTEM_ALLOCATOR_BAD_OPERATION;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    case LINEAR_ALLOCATOR_OVERFLOW:
        return SUBSYSTEM_ALLOCATOR_OVERFLOW;
    case LINEAR_ALLOCATOR_UNDEFINED_ERROR:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    }
}

static subsystem_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return SUBSYSTEM_ALLOCATOR_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return SUBSYSTEM_ALLOCATOR_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return SUBSYSTEM_ALLOCATOR_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return SUBSYSTEM_ALLOCATOR_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }

    if(NULL == allocator_->linear_allocator_pool) {
        return false;
    }

    return true;
}

static bool memory_tag_is_valid(subsystem_allocator_memory_tag_t memory_tag_) {
    if(SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX <= memory_tag_ || 0 > (int)memory_tag_) {
        return false;
    }
    return true;
}

static bool accounting_is_valid(const subsystem_allocator_t* allocator_) {
    size_t expected_total_size = 0;
    if(NULL == allocator_) {
        return false;
    }
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        if((SIZE_MAX - allocator_->memory_tag_allocated[i]) < expected_total_size) {
            return false;
        }
        expected_total_size += allocator_->memory_tag_allocated[i];
    }
    if(allocator_->total_allocated != expected_total_size) {
        return false;
    }
    return true;
}
