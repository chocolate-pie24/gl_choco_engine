// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/camera/camera_registries/flight_camera_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/camera/flight_camera.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"
#include "engine/systems/camera/camera_registries/core/camera_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Entry State:
 * - 各EntryのStable Stateは、EmptyとRegisteredの2種類とする。
 * - Empty Stateでは、resource_nameとflight_cameraがともにNULLとなる。
 * - Registered Stateでは、両フィールドが非NULLとなる。
 * - 登録および解除では両フィールドを一つの論理的なEntryとして扱い、
 *   部分的な登録状態をStable Stateとして公開しない。
 *
 * Initialization:
 * - Subsystem Allocatorが確保領域をゼロ初期化するContractを利用し、
 *   create時にすべてのEntryをEmpty Stateとして初期化する。
 * - 利用可能期間中はEntry配列の容量を変更しない。
 * - deinitialize後はmax_flight_camera_countが0となり、
 *   Registryは利用可能なStable Stateではなくなる。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/*
 * Flight Camera IDに対応する一つの登録Slotを表す。
 *
 * resource_nameを登録Resourceの検索キーとし、
 * flight_cameraをRegistryが所有するResourceとして保持する。
 * 独立した登録状態フラグは持たず、両フィールドのNULL状態によって
 * Entryの状態を表現する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    flight_camera_t* flight_camera;
} registry_entry_t;

/*
 * Flight Camera Registryの内部Storage Representation。
 *
 * max_flight_camera_countは登録済みFlight Camera数ではなく、
 * Entry配列の容量を表す。
 * entriesの配列IndexがFlight Camera IDに対応する。
 */
struct flight_camera_registry {
    size_t max_flight_camera_count;
    registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static void registry_entry_deinitialize(registry_entry_t* entry_);

// State Queries
static bool flight_camera_id_is_in_range(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_);

// Validation
static bool registry_entry_is_valid(const registry_entry_t* entry_);

// Lookup
static bool find_by_name(const flight_camera_registry_t* registry_, const char* name_, size_t* out_index_);

// ============================================================
// Public API
// ============================================================
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - 引数のNULL状態、出力先の初期状態、最大登録数の許容範囲、
 *   Entry配列のサイズ計算におけるoverflowを全Buildで確認する。
 * - Subsystem Allocatorの内部状態は直接Canonical Validationせず、
 *   Allocation操作に必要な検証を下位Allocator APIへ委譲する。
 *
 * Result validation:
 * - DEBUG / TESTでは、構築したRegistryがCanonical Invariantを
 *   満たしていることを、callerへ公開する前に検証する。
 * - 検証失敗はDATA_CORRUPTEDとして扱い、通常のrollbackを行わない。
 */
camera_registry_result_t flight_camera_registry_create(size_t max_flight_camera_count_, subsystem_allocator_t* allocator_, flight_camera_registry_t** out_registry_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    flight_camera_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entry_array = NULL;

    size_t entry_array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_create", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, CAMERA_REGISTRY_BAD_OPERATION, camera_registry_result_to_str(CAMERA_REGISTRY_BAD_OPERATION), "flight_camera_registry_create", "*out_registry_")
    if(0 == max_flight_camera_count_ || UINT16_MAX < max_flight_camera_count_) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_create(%s) - Provided max_flight_camera_count_ is not valid.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / max_flight_camera_count_) < sizeof(registry_entry_t)) {
        ret = CAMERA_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("flight_camera_registry_create(%s) - overflow.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
    entry_array_size = sizeof(registry_entry_t) * max_flight_camera_count_;

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = camera_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("flight_camera_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // flight_camera_registry_tメモリ確保
    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(flight_camera_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = camera_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("flight_camera_registry_create(%s) - Failed to allocate registry instance. target=flight_camera_registry_t, bytes=%zu, max_flight_camera_count=%zu", camera_registry_result_to_str(ret), sizeof(flight_camera_registry_t), max_flight_camera_count_);
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, entry_array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA, (void**)&tmp_entry_array);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = camera_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("flight_camera_registry_create(%s) - allocation failed.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_flight_camera_count = max_flight_camera_count_;
    tmp_registry->entries = tmp_entry_array;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(tmp_registry)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_create(%s) - tmp_registry is corrupted.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_registry_ = tmp_registry;
    tmp_registry = NULL;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    if(CAMERA_REGISTRY_DATA_CORRUPTED != ret && NULL != tmp_registry) {
        ret_subsystem_allocator = subsystem_allocator_rollback(allocator_, &rollback_point);
        if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
            ret = CAMERA_REGISTRY_DATA_CORRUPTED;
            ERROR_MESSAGE("flight_camera_registry_create(%s) - subsystem_allocator rollback failed.", camera_registry_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULLを全Buildで確認する。
 * - 全EntryのOwned Resourceを破棄するため、DEBUG / TESTでは
 *   Registry全体のCanonical Validityを検証する。
 * - RELEASEでは成立済みInternal Contractを信頼する。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なStable Stateではなくなるため、
 *   Canonical Validationは行わない。
 */
void flight_camera_registry_deinitialize(flight_camera_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("flight_camera_registry_deinitialize(%s) - provided registry_ is NULL.", camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ERROR_MESSAGE("flight_camera_registry_deinitialize(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_result_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif
    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i]);
    }
    registry_->max_flight_camera_count = 0;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryとResource NameのNULL、およびResource Nameが
 *   空文字列でないことを全Buildで確認する。
 * - find_by_name()によって全EntryのResource Nameを走査するため、
 *   DEBUG / TESTでは事前にRegistryの整合性を検証する。
 * - Resource Nameに限定したValidationでも検索処理の安全性に必要な
 *   条件は確認できるが、既存Canonical Validatorとの検証処理の
 *   重複や保守負担を避けるため、Registry全体のCanonical Validatorを使用する。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、独立したResult Candidateも生成しないため、
 *   追加のValidationは行わない。
 */
bool flight_camera_registry_exists(const flight_camera_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("flight_camera_registry_exists(%s) - provided resource name is not valid.", camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ERROR_MESSAGE("flight_camera_registry_exists(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_result_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
        return false;
    }
#endif

    return find_by_name(registry_, name_, &tmp_id);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULL、Flight Camera IDの範囲、対象Entryの
 *   Flight Cameraが非NULLであることを全Buildで確認する。
 * - Flight Cameraの内部状態をConsumeせず、既存Pointerを
 *   borrowとして返すだけなので、Flight CameraのCanonical Validationは行わない。
 * - Flight Cameraの内部状態に対するValidationは、
 *   それをConsumeするOperationの責務とする。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、既存Resourceへのborrowを返すため、
 *   追加のValidationは行わない。
 */
flight_camera_t* flight_camera_registry_flight_camera_get(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided registry_ is not valid.", camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!flight_camera_id_is_in_range(registry_, flight_camera_id_)) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided flight_camera_id_ is not valid.", camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == registry_->entries[flight_camera_id_].flight_camera) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided flight_camera_id_ is not registered.", camera_registry_result_to_str(CAMERA_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[flight_camera_id_].flight_camera;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - Registry、Resource Name、出力PointerのNULL、および
 *   Resource Nameが空文字列でないことを全Buildで確認する。
 * - find_by_name()によって全EntryのResource Nameを走査するため、
 *   DEBUG / TESTでは事前にRegistryの整合性を検証する。
 * - Resource Nameに限定したValidationでも検索処理の安全性に必要な
 *   条件は確認できるが、既存Canonical Validatorとの検証処理の
 *   重複や保守負担を避けるため、Registry全体のCanonical Validatorを使用する。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、既存EntryのIndexを出力するだけなので、
 *   追加のValidationは行わない。
 */
camera_registry_result_t flight_camera_registry_id_get(const flight_camera_registry_t* registry_, const char* name_, uint16_t* out_flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_id_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "out_flight_camera_id_")
    if('\0' == name_[0]) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - provided resource name is not valid.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - find_by_name failed.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_flight_camera_id_ = (uint16_t)tmp_id;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - Registry、Resource Name、Flight Cameraの所有Pointer、
 *   Flight Camera IDの出力Pointerについて、Operationに必要なNULL条件を
 *   全Buildで確認する。
 * - Resource Nameが空文字列でないこと、および同名のResourceが
 *   登録済みでないことを全Buildで確認する。
 * - Flight Cameraの内部状態はConsumeせず、所有権のみを移転するため、
 *   入力Flight CameraのCanonical Validationは行わない。
 * - find_by_name()で全EntryのResource Nameを走査するため、
 *   DEBUG / TESTではRegistryのCanonical Validityを確認する。
 * - Resource Name専用Validatorでも必要な検証は可能だが、
 *   既存Canonical Validatorとの重複や保守負担を避け、
 *   Validation Architectureの単純さを優先する。
 *
 * Preflight:
 * - 両フィールドがNULLのEntryを登録先として選び、
 *   既存Resourceの上書きを防ぐ。
 * - Resource Nameの複製をCommit前に完了し、
 *   回復可能な失敗でRegistryやCallerの所有状態を変更しない。
 *
 * Postconditions:
 * - DEBUG / TESTではEntryへの登録後にRegistry全体をCanonical Validationし、
 *   新たに登録したFlight Cameraを含むStable Stateを確認する。
 * - 検証失敗はDATA_CORRUPTEDとして扱い、通常のCleanupを行わない。
 * - Validation Costが問題になる場合は、Diagnostic Coverageとの
 *   バランスを考慮し、対象EntryのみのValidationへの変更を検討する。
 */
camera_registry_result_t flight_camera_registry_register(flight_camera_registry_t* registry_, const char* flight_camera_name_, flight_camera_t** flight_camera_, uint16_t* out_flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_name_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "flight_camera_name_")
    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(*flight_camera_, ret, CAMERA_REGISTRY_BAD_OPERATION, camera_registry_result_to_str(CAMERA_REGISTRY_BAD_OPERATION), "flight_camera_registry_register", "*flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_id_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "out_flight_camera_id_")
    if('\0' == flight_camera_name_[0]) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided resource name is not valid.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(find_by_name(registry_, flight_camera_name_, &tmp_index)) {  // リソースの重複チェック
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided resource name is already registered.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(NULL == registry_->entries[i].flight_camera && NULL == registry_->entries[i].resource_name) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = CAMERA_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - free slot not found.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_choco_string = choco_string_create_from_c_string(flight_camera_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = camera_registry_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("flight_camera_registry_register(%s) - choco_string_create_from_c_string failed.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    registry_->entries[tmp_index].flight_camera = *flight_camera_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - registry_ is corrupted.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *flight_camera_ = NULL;
    tmp_name = NULL;

    *out_flight_camera_id_ = (uint16_t)tmp_index;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    if(CAMERA_REGISTRY_DATA_CORRUPTED != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULLとFlight Camera IDの範囲を全Buildで確認する。
 * - 指定EntryのOwned Resourceを破棄するため、
 *   DEBUG / TESTでは対象EntryのCanonical Validityを確認する。
 * - 他のEntryの内部状態はConsumeしないため、
 *   Registry全体のCanonical Validationは行わない。
 * - Registry本体とEntry配列の構造的Validityは、
 *   成立済みInternal Contractとして信頼する。
 * - 両フィールドが非NULLであることを全Buildで確認し、
 *   登録解除Operationが実行可能かを判定する。
 *   このNULLチェックはEntryのCanonical Validationを目的としない。
 *
 * Postconditions:
 * - 対象EntryはEmpty Stateへ遷移するが、破棄済みResourceの
 *   再検証は行わない。
 * - 登録解除は検証済みの対象Entryを破棄するOperationであり、
 *   追加のCanonical Validationは行わない。
 */
camera_registry_result_t flight_camera_registry_unregister(flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_result_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_unregister", "registry_")
    if(!flight_camera_id_is_in_range(registry_, flight_camera_id_)) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - provided flight_camera_id_ is not valid.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!registry_entry_is_valid(&registry_->entries[flight_camera_id_])) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - Entry validation failed.", camera_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL == registry_->entries[flight_camera_id_].resource_name || NULL == registry_->entries[flight_camera_id_].flight_camera) {
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - provided flight_camera id entry is empty.", camera_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    registry_entry_deinitialize(&registry_->entries[flight_camera_id_]);

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - このAPI自身がRegistryのCanonical Validatorであるため、
 *   事前に別のCanonical Validationは実行しない。
 * - RegistryのNULL、最大登録数の許容範囲、Entry配列のNULL、
 *   配列サイズ計算のoverflowを確認した後、
 *   各EntryのCanonical Validationを実行する。
 * - Registry本体とEntry配列のMemory Readabilityおよび
 *   LifetimeはCallerのContractとして扱う。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、Canonical Validityの判定結果を
 *   返すOperationであるため、追加のValidationは行わない。
 */
bool flight_camera_registry_is_valid(const flight_camera_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_flight_camera_count || UINT16_MAX < registry_->max_flight_camera_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    if((SIZE_MAX / registry_->max_flight_camera_count) < sizeof(registry_entry_t)) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(!registry_entry_is_valid(&registry_->entries[i])) {
            return false;
        }
    }

    return true;
}

// ============================================================
// Lifecycle
// ============================================================
/*
 * registry_entry_deinitialize() Contract
 *
 * Preconditions:
 * - EntryはEmptyまたはRegisteredの成立済みStable Stateにあること。
 * - Registered Stateでは、各Owned Resourceが対応するDestroy APIの
 *   Contractを満たしていること。
 *
 * Responsibility:
 * - Entryが所有するFlight CameraおよびResource Nameの
 *   lifetimeを終了し、EntryをEmpty Stateへ遷移させる。
 * - Empty Stateに対する呼び出しも許容する。
 * - Resourceの破棄処理は、それぞれのSemantic Ownerへ委譲する。
 *
 * Postconditions:
 * - 正常終了後、Entryの両フィールドはNULLとなる。
 */
static void registry_entry_deinitialize(registry_entry_t* entry_) {
    if(NULL == entry_) {
        return;
    }

    flight_camera_destroy(&entry_->flight_camera);
    choco_string_destroy(&entry_->resource_name);
}

// ============================================================
// State Queries
// ============================================================
static bool flight_camera_id_is_in_range(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_flight_camera_count <= (size_t)flight_camera_id_) {
        return false;
    }
    return true;
}

// ============================================================
// Validation
// ============================================================
/*
 * registry_entry_is_valid() Validation
 *
 * Purpose:
 * - Registry EntryがEmptyまたはRegisteredのCanonical Stable Stateに
 *   あることを確認する。
 * - Registered Stateでは、Owned ResourceのCanonical Validityまで検証する。
 *
 * Validation Scope:
 * - Entry Pointerが非NULLであること。
 * - Empty Stateでは、resource_nameとflight_cameraが
 *   ともにNULLであること。
 * - Registered Stateでは、両フィールドが非NULLであること。
 * - Registered Stateの各Owned Pointerが、General Allocatorの
 *   有効なAllocationを参照していること。
 * - Resource NameがCanonical Validityを満たし、
 *   空文字列ではないこと。
 * - Flight CameraがCanonical Validityを満たすこと。
 *
 * Validation Limitations:
 * - 他のEntryとのResource Nameの一意性は検証しない。
 * - Registry本体およびEntry配列の構造的Validityは検証しない。
 */
static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // Empty State.
    if(NULL == entry_->flight_camera && NULL == entry_->resource_name) {
        return true;
    }

    // Registered State.
    if(NULL == entry_->flight_camera || NULL == entry_->resource_name) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(entry_->resource_name)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(entry_->flight_camera)) {
        return false;
    }

    if(!choco_string_is_valid(entry_->resource_name)) {
        return false;
    }
    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }
    if(!flight_camera_is_valid(entry_->flight_camera)) {
        return false;
    }

    return true;
}

// ============================================================
// Lookup
// ============================================================
/*
 * find_by_name() Contract
 *
 * Preconditions:
 * - Registryは検索可能な成立済みInternal Stateにあること。
 * - name_は有効なNUL終端文字列であること。
 *
 * Responsibility:
 * - Registered EntryのResource Nameを検索し、
 *   最初に一致したEntryのIndexを返す。
 * - Empty Entryは検索対象から除外する。
 * - Resource Nameの比較処理はChoco Stringへ委譲する。
 *
 * Postconditions:
 * - 一致するEntryが存在する場合のみ、out_index_へIndexを書き込む。
 * - 一致するEntryが存在しない場合、out_index_は変更しない。
 */
static bool find_by_name(const flight_camera_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(NULL != registry_->entries[i].resource_name && choco_string_is_equal(choco_string_c_str(registry_->entries[i].resource_name), name_)) {
            tmp_slot = i;
            found = true;
            break;
        }
    }
    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}
