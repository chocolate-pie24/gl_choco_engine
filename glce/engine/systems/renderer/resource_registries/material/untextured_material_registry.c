// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_registries/material/untextured_material_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/material/material_types.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Resource Identity:
 * - Material IDはEntry配列のindexに対応する。
 * - IDとEntryの対応を管理する独立したmappingは持たない。
 *
 * Registration State:
 * - Entryのresource_nameがNULLでない場合、そのEntryは登録済みとみなす。
 * - 未登録Entryではresource_nameがNULLであり、
 *   untextured_materialの格納値は意味を持たない。
 * - 登録解除済みEntryは、後続の登録処理で再利用できる。
 *
 * Initialization:
 * - Subsystem Allocatorが返すzero-initialized storageを使用する。
 * - これにより、生成直後の全Entryは未登録状態となる。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/* Materialの登録名と値を保持するEntry。 */
typedef struct untextured_material_registry_entry {
    choco_string_t* resource_name;
    untextured_material_t untextured_material;
} untextured_material_registry_entry_t;

/*
 * Untextured Materialの登録状態を保持するRegistry本体。
 *
 * max_untextured_material_countはEntry配列の容量を表し、
 * entriesはMaterial IDによって直接参照する配列である。
 */
struct untextured_material_registry {
    size_t max_untextured_material_count;
    untextured_material_registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static void registry_entry_deinitialize(untextured_material_registry_entry_t* entry_);

// State Queries
static bool untextured_material_id_is_in_range(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_);
static bool registry_entry_is_registered(const untextured_material_registry_entry_t* entry_);

// Validation
static bool registry_entry_is_valid(const untextured_material_registry_entry_t* entry_);

// Lookup
static bool find_by_name(const untextured_material_registry_t* registry_, const char* name_, size_t* out_index_);

// ============================================================
// Public API
// ============================================================
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - allocator_とout_registry_のNULL、および*out_registry_の
 *   NULL初期化状態を検査する。
 * - 最大登録数をuint16_tのID表現範囲に収め、
 *   Entry配列のsize計算でoverflowが発生しないことを確認する。
 * - allocator_の内部整合性とallocationの成立条件は、
 *   Subsystem Allocator側のAPI contractとvalidationに委ねる。
 *
 * Result validation:
 * - DEBUG / TESTでは、構築したRegistry全体をcanonical validationし、
 *   callerへ公開する前にstable object modelの成立を確認する。
 * - RELEASEでは、zero-initialized storageと構築処理のcontractを信頼し、
 *   canonical validationを省略する。
 *
 * Failure:
 * - 回復可能な構築失敗ではAllocatorをrollbackし、
 *   構築前のallocation stateを復元する。
 * - DATA_CORRUPTEDでは、内部状態の安全性を信頼できないため
 *   rollbackを行わない。
 *
 * Postconditions:
 * - Result validation済みのRegistryをOutputするだけであり、
 *   Output後にRegistryのsemantic stateを変更しないため、
 *   同じcanonical validationを繰り返さない。
 */
resource_registry_result_t untextured_material_registry_create(size_t max_untextured_material_count_, subsystem_allocator_t* allocator_, untextured_material_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    untextured_material_registry_t* tmp_registry = NULL;
    untextured_material_registry_entry_t* tmp_entries = NULL;

    size_t array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_create", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "untextured_material_registry_create", "*out_registry_")
    if(0 == max_untextured_material_count_ || UINT16_MAX < max_untextured_material_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_material_registry_create(%s) - Provided max_untextured_material_count_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }
    if((SIZE_MAX / max_untextured_material_count_) < sizeof(untextured_material_registry_entry_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("untextured_material_registry_create(%s) - array size overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    array_size = sizeof(untextured_material_registry_entry_t) * max_untextured_material_count_;

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_material_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(untextured_material_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_material_registry_create(%s) - Failed to allocate registry instance. target=untextured_material_registry_create, bytes=%zu, max_untextured_material_count=%zu", resource_registry_result_to_str(ret), sizeof(untextured_material_registry_t), max_untextured_material_count_);
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_entries);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_material_registry_create(%s) - allocation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_untextured_material_count = max_untextured_material_count_;
    tmp_registry->entries = tmp_entries;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(tmp_registry)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_material_registry_create(%s) - Result validation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_registry_ = tmp_registry;
    tmp_registry = NULL;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    if(RESOURCE_REGISTRY_DATA_CORRUPTED != ret && NULL != tmp_registry) {
        ret_subsystem_allocator = subsystem_allocator_rollback(allocator_, &rollback_point);
        if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
            ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_material_registry_create(%s) - subsystem_allocator rollback failed.", resource_registry_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、登録済みEntryのowned resourceを安全に
 *   破棄できることを確認するため、Registry全体を
 *   canonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なstable object modelから
 *   外れるため、通常のcanonical validationを実行しない。
 */
void untextured_material_registry_deinitialize(untextured_material_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("untextured_material_registry_deinitialize(%s) - provided registry_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ERROR_MESSAGE("untextured_material_registry_deinitialize(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif

    for(size_t i = 0; i != registry_->max_untextured_material_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i]);
    }
    registry_->max_untextured_material_count = 0;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_とname_のNULL、および空のResource Nameを検査する。
 * - DEBUG / TESTでは、Entry配列と登録名を走査する前に
 *   Registry全体のcanonical validationを行い、
 *   内部表現の整合性を確認する。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * - このAPIは登録状態を変更しないため、
 *   Postconditions validationは行わない。
 */
bool untextured_material_registry_exists(const untextured_material_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("untextured_material_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ERROR_MESSAGE("untextured_material_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return false;
    }
#endif

    return find_by_name(registry_, name_, &tmp_id);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Material IDの範囲、
 *   および対象Entryの登録状態を直接検査する。
 * - 対象Entryが登録済みであることを確認したうえで、
 *   Registryが保持するResource Nameを参照する。
 * - Registry全体のcanonical validationは実行せず、
 *   対象Entry以外の状態については成立済みの
 *   Registry internal contractを信頼する。
 *
 * - 戻り値は既存Resource Nameへの読み取り専用borrowであり、
 *   新しいstateを構築・変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
const char* untextured_material_registry_name_get(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("untextured_material_registry_name_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!untextured_material_id_is_in_range(registry_, untextured_material_id_)) {
        ERROR_MESSAGE("untextured_material_registry_name_get(%s) - provided untextured_material_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[untextured_material_id_])) {
        ERROR_MESSAGE("untextured_material_registry_name_get(%s) - provided untextured_material_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    return choco_string_c_str(registry_->entries[untextured_material_id_].resource_name);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Material IDの範囲、
 *   および対象Entryの登録状態を直接検査する。
 * - Registry全体およびMaterial値のcanonical validationは
 *   実行せず、登録時に確立されたinternal contractを信頼する。
 *
 * - 戻り値はRegistry内部のMaterial値への読み取り専用borrowであり、
 *   新しいstateを構築・変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
const untextured_material_t* untextured_material_registry_material_get(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("untextured_material_registry_material_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!untextured_material_id_is_in_range(registry_, untextured_material_id_)) {
        ERROR_MESSAGE("untextured_material_registry_material_get(%s) - provided untextured_material_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[untextured_material_id_])) {
        ERROR_MESSAGE("untextured_material_registry_material_get(%s) - provided untextured_material_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return &registry_->entries[untextured_material_id_].untextured_material;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、name_、出力先のNULL、および空のResource Nameを検査する。
 * - DEBUG / TESTでは、Entry配列と登録名を走査する前に
 *   Registry全体のcanonical validationを行い、
 *   内部表現の整合性を確認する。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * Output:
 * - 名前検索が成功した場合に限りMaterial IDを出力する。
 * - IDは検証済みのRegistry容量内のindexから得られるため、
 *   Output後に追加のResult validationは行わない。
 * - Registryの登録状態を変更しないため、
 *   Postconditions validationは行わない。
 */
resource_registry_result_t untextured_material_registry_id_get(const untextured_material_registry_t* registry_, const char* name_, uint16_t* out_untextured_material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_untextured_material_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_id_get", "out_untextured_material_id_")
    if('\0' == name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_material_registry_id_get(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_material_registry_id_get(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_material_registry_id_get(%s) - find_by_name failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_untextured_material_id_ = (uint16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - 入力ポインタのNULLとResource Nameの空文字列を検査する。
 * - callerから受け取るMaterialはEngine API Trust Boundaryを
 *   越える値であるため、全Build Modeでcanonical validationする。
 *   invalidなMaterialはINVALID_ARGUMENTとして扱う。
 * - DEBUG / TESTでは、既存Entryを探索・更新する前に
 *   Registry全体のcanonical validationを行う。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   Registryのcanonical validationを省略する。
 *
 * Postconditions:
 * - DEBUG / TESTでは、MaterialとResource Nameを登録した後の
 *   Registry全体をcanonical validationし、Commit後の
 *   stable stateが成立していることを確認する。
 * - RELEASEでは、PreconditionsおよびCommitのcontractを信頼し、
 *   canonical validationを省略する。
 *
 * Failure:
 * - Commit前の回復可能な失敗では、生成済みの一時Resource Nameを
 *   破棄し、Registryの登録状態を変更しない。
 * - DATA_CORRUPTED発生時は内部状態の整合性を信頼できないため、
 *   cleanupによる通常の状態復元を試みない。
 */
resource_registry_result_t untextured_material_registry_register(untextured_material_registry_t* registry_, const char* resource_name_, const untextured_material_t* material_, uint16_t* out_untextured_material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(material_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_register", "material_")
    IF_ARG_NULL_GOTO_CLEANUP(out_untextured_material_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_register", "out_untextured_material_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!untextured_material_is_valid(material_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - provided material_ is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - provided resource name is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_untextured_material_count; ++i) {
        if(!registry_entry_is_registered(&registry_->entries[i])) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - free slot not found.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_choco_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_registry_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_material_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    registry_->entries[tmp_index].untextured_material = *material_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_material_registry_register(%s) - Postcondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_untextured_material_id_ = (uint16_t)tmp_index;
    tmp_name = NULL;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    if(RESOURCE_REGISTRY_DATA_CORRUPTED != ret) {
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
 * - registry_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、owned Resource Nameの破棄に先立ち、
 *   Registry全体のcanonical validationを行う。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 * - Material IDの範囲と対象Entryの登録状態は、
 *   誤ったEntryの参照や登録解除を防ぐため、全Build Modeで検査する。
 *
 * Postconditions:
 * - 登録解除は、成立済みのEntry contractに従って
 *   owned Resource Nameを破棄し、Entryを未登録状態へ遷移させる。
 * - この状態遷移は使用するResourceのlifecycle contractから
 *   成立するため、Registry全体のcanonical validationを繰り返さない。
 */
resource_registry_result_t untextured_material_registry_unregister(untextured_material_registry_t* registry_, uint16_t untextured_material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_material_registry_unregister", "registry_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_material_registry_unregister(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!untextured_material_id_is_in_range(registry_, untextured_material_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_material_registry_unregister(%s) - Provided untextured_material_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!registry_entry_is_registered(&registry_->entries[untextured_material_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_material_registry_unregister(%s) - provided texture id entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    registry_entry_deinitialize(&registry_->entries[untextured_material_id_]);

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * - Registryのcanonical validityを診断する公開Validatorであり、
 *   BUILD_MODEによって検査内容を変更しない。
 * - NULL、最大登録数、Entry配列ポインタを先に検査し、
 *   明らかに不正な構造を走査しない。
 * - 最大登録数を走査上限として各Entryのvalidityを確認する。
 * - 登録済みEntryではResource NameとMaterialのvalidityを確認し、
 *   未登録EntryではMaterial値の検査を行わない。
 * - Resource Nameの一意性、Entry配列のallocation-level validity、
 *   Allocatorの内部整合性は検査対象としない。
 *
 * - Validator自体はRegistryを変更せず、Result validationや
 *   Postconditions validationは行わない。
 */
bool untextured_material_registry_is_valid(const untextured_material_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_untextured_material_count || UINT16_MAX < registry_->max_untextured_material_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_untextured_material_count; ++i) {
        if(!registry_entry_is_valid(&registry_->entries[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================
// Lifecycle
// ============================================================
static void registry_entry_deinitialize(untextured_material_registry_entry_t* entry_) {
    if(NULL == entry_) {
        return;
    }
    choco_string_destroy(&entry_->resource_name);
}

// ============================================================
// State Queries
// ============================================================
static bool untextured_material_id_is_in_range(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_untextured_material_count <= (size_t)untextured_material_id_) {
        return false;
    }
    return true;
}

static bool registry_entry_is_registered(const untextured_material_registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL == entry_->resource_name) {
        return false;
    }
    return true;
}

// ============================================================
// Validation
// ============================================================
static bool registry_entry_is_valid(const untextured_material_registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // 未登録Entryは正常
    if(!registry_entry_is_registered(entry_)) {
        return true;
    }

    if(!choco_string_is_valid(entry_->resource_name)) {
        return false;
    }

    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }

    if(!untextured_material_is_valid(&entry_->untextured_material)) {
        return false;
    }

    return true;
}

// ============================================================
// Lookup
// ============================================================
static bool find_by_name(const untextured_material_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_untextured_material_count; ++i) {
        if(registry_entry_is_registered(&registry_->entries[i]) && choco_string_is_equal(choco_string_c_str(registry_->entries[i].resource_name), name_)) {
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
