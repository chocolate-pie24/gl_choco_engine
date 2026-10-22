// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/registries/materials/untextured_mesh_material_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/materials/material_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Resource Identity:
 * - Material IDはEntry配列のindexに対応する。
 * - IDとEntryの対応を管理する独立したmappingは持たない。
 * - 解放済みEntryのindexは、後の新規登録で再利用できる。
 *
 * Entry State:
 * - Entryの正常なstable stateはEmpty StateとRegistered Stateである。
 * - Empty StateではResource NameがNULL、
 *   Reference Countが0となる。
 * - Empty StateのMaterial格納値は意味を持たず、
 *   canonical validationの対象としない。
 * - Registered StateではResource NameとMaterial値を保持し、
 *   Reference Countは1以上となる。
 * - Entryは正常なstate transitionの途中を除き、
 *   Empty StateまたはRegistered Stateのいずれかを満たす。
 * - 両Stateの構造的条件は各State Predicateが判定し、
 *   canonical validityはregistry_entry_is_valid()が判定する。
 *
 * Reference Count:
 * - 新規登録によってEmpty StateからRegistered Stateへ遷移し、
 *   Reference Countを1に初期化する。
 * - 既存ResourceのAcquireではReference Countのみを増加させる。
 * - Release後もReference Countが残る場合、Resourceを維持する。
 * - 最後の利用権の返却ではResource Nameを解放し、
 *   EntryをEmpty Stateへ戻す。
 * - Registry全体のdeinitializeではReference Countを消費せず、
 *   各登録Resourceを一度だけ解放する。
 *
 * Initialization:
 * - Subsystem Allocatorが返すzero-initialized storageを使用する。
 * - これにより、生成直後の全EntryはEmpty Stateとなる。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/*
 * Untextured Mesh Materialの登録情報を保持するEntry。
 *
 * - resource_nameはRegistryが所有するResource Nameを指す。
 * - materialはRegistry内部に値として保持するMaterialである。
 * - reference_countは、このEntryのResourceに対して
 *   保持されている利用権の数を表す。
 * - Empty Stateではresource_nameはNULL、
 *   reference_countは0となり、materialの格納値は不問とする。
 * - Registered Stateではresource_nameはNULL以外、
 *   reference_countは1以上となる。
 * - 各Stateの構造的成立条件およびcanonical validityは、
 *   対応するState PredicateとValidatorが定義する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    untextured_mesh_material_t material;
    uint16_t reference_count;
} registry_entry_t;

/*
 * Untextured Mesh Materialの登録状態を管理するRegistry本体。
 *
 * - max_material_countはEntry配列の容量を表す。
 * - entriesはMaterial IDによって直接参照するEntry配列である。
 * - Registryが利用可能な状態では、max_material_countは
 *   1以上UINT16_MAX以下、entriesはNULL以外となる。
 * - Entry配列はRegistryの利用期間中、同じstorageを使用する。
 * - deinitializeが正常に完了するとmax_material_countが0となり、
 *   Registryのlogical lifetimeが終了する。
 * - 終了後のRegistryは通常のAPI操作やcanonical validationの
 *   対象としない。
 */
struct untextured_mesh_material_registry {
    size_t max_material_count;
    registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static void registry_entry_deinitialize(registry_entry_t* entry_);

// State Queries
static bool untextured_mesh_material_id_is_in_range(const untextured_mesh_material_registry_t* registry_, uint16_t material_id_);
static bool registry_entry_is_empty(const registry_entry_t* entry_);
static bool registry_entry_is_registered(const registry_entry_t* entry_);

// Validation
static bool registry_entry_is_valid(const registry_entry_t* entry_);

// Lookup
static bool find_by_name(const untextured_mesh_material_registry_t* registry_, const char* name_, size_t* out_index_);

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
 * Result Validation:
 * - DEBUG / TESTでは、構築したRegistry全体をcanonical validationし、
 *   全EntryがReference Count = 0のEmpty Stateであることを確認する。
 * - RELEASEでは、zero-initialized storageと構築処理のcontractを
 *   信頼し、canonical validationを省略する。
 *
 * Failure:
 * - 回復可能な構築失敗ではAllocatorをrollbackし、
 *   構築前のallocation stateを復元する。
 * - DATA_CORRUPTEDでは、内部状態の安全性を信頼できないため、
 *   rollbackを行わない。
 *
 * Postconditions:
 * - Result validation済みのRegistryをOutputするだけであり、
 *   Output後にRegistryのsemantic stateを変更しないため、
 *   同じcanonical validationを繰り返さない。
 */
resource_registry_result_t untextured_mesh_material_registry_create(size_t max_material_count_, subsystem_allocator_t* allocator_, untextured_mesh_material_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    untextured_mesh_material_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entries = NULL;

    size_t array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_create", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "untextured_mesh_material_registry_create", "*out_registry_")
    if(0 == max_material_count_ || UINT16_MAX < max_material_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - Provided max_material_count_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }
    if(!size_mul_checked(max_material_count_, sizeof(registry_entry_t), &array_size)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - array size overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(untextured_mesh_material_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - Failed to allocate registry instance. target=untextured_mesh_material_registry_create, bytes=%zu, max_material_count=%zu", resource_registry_result_to_str(ret), sizeof(untextured_mesh_material_registry_t), max_material_count_);
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_entries);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - allocation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_material_count = max_material_count_;
    tmp_registry->entries = tmp_entries;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(tmp_registry)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - Result validation failed.", resource_registry_result_to_str(ret));
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
            ERROR_MESSAGE("untextured_mesh_material_registry_create(%s) - subsystem_allocator rollback failed.", resource_registry_result_to_str(ret));
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
 * - DEBUG / TESTでは、登録済みResource Nameを安全に
 *   破棄できることを確認するため、Registry全体を
 *   canonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * Deinitialization:
 * - Entry配列の全Entryに対して
 *   registry_entry_deinitialize()を実行する。
 * - Empty StateのEntryではResource Nameの破棄は発生しない。
 * - Reference CountによるRelease処理は行わず、
 *   登録済みResource NameをEntry単位で一度ずつ解放する。
 * - 全Entryの終了処理後、max_material_countを0に変更し、
 *   Registryのlogical lifetimeを終了する。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なobjectではないため、
 *   通常のRegistry canonical validationは行わない。
 * - 終了処理はvoid APIであり、失敗をResultとして返さない。
 */
void untextured_mesh_material_registry_deinitialize(untextured_mesh_material_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("untextured_mesh_material_registry_deinitialize(%s) - provided registry_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ERROR_MESSAGE("untextured_mesh_material_registry_deinitialize(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif

    for(size_t i = 0; i != registry_->max_material_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i]);
    }
    registry_->max_material_count = 0;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、resource_name_、out_exists_のNULLと、
 *   Resource Nameの空文字列を検査する。
 * - DEBUG / TESTでは、Entry配列と登録名を走査する前に
 *   Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * Lookup:
 * - Resource Nameの検索はfind_by_name()に委譲する。
 * - 未登録は正常な検索結果として扱う。
 * - 検索によってReference Countを変更しない。
 *
 * Result:
 * - 検索完了後にout_exists_へ結果を出力する。
 * - 失敗時はout_exists_を変更しない。
 * - Registryのstate mutationを伴わないため、
 *   Postconditions validationは行わない。
 */
resource_registry_result_t untextured_mesh_material_registry_exists(const untextured_mesh_material_registry_t* registry_, const char* resource_name_, bool* out_exists_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;
    bool exists = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_exists", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_exists", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_exists_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_exists", "out_exists_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    exists = find_by_name(registry_, resource_name_, &tmp_id);
    *out_exists_ = exists;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Material IDの範囲、
 *   および対象EntryのRegistered Stateを直接検査する。
 * - 指定IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - Material値自体のcanonical validationも実行せず、
 *   登録時に確立されたinternal contractを信頼する。
 *
 * Result:
 * - 戻り値はRegistry内部のMaterial値への読み取り専用borrowである。
 * - Reference Countを変更せず、利用権を新たに取得しない。
 * - 新しいstateを構築・変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
const untextured_mesh_material_t* untextured_mesh_material_registry_material_get(const untextured_mesh_material_registry_t* registry_, uint16_t material_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("untextured_mesh_material_registry_material_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!untextured_mesh_material_id_is_in_range(registry_, material_id_)) {
        ERROR_MESSAGE("untextured_mesh_material_registry_material_get(%s) - provided material_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[material_id_])) {
        ERROR_MESSAGE("untextured_mesh_material_registry_material_get(%s) - provided material_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return &registry_->entries[material_id_].material;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、resource_name_、out_material_id_のNULLと、
 *   Resource Nameの空文字列を検査する。
 * - DEBUG / TESTでは、Resource NameによってEntry配列を
 *   検索するため、Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 *
 * Preflight:
 * - find_by_name()によって対象Resourceが登録済みであることを確認する。
 * - 未登録の場合はBAD_OPERATIONとして扱う。
 * - 対象Entryのindexがuint16_tの表現範囲内であることを確認する。
 * - 対象EntryのReference CountがUINT16_MAXに達していないことを
 *   確認する。
 * - これらの条件が成立するまでReference Countを変更しない。
 *
 * Commit:
 * - 対象EntryのReference Countを1増加させる。
 * - Resource Name、Material値、およびMaterial IDとの対応は
 *   変更しない。
 *
 * Postconditions:
 * - Commitは検証済みのReference Countに対する単一の加算であり、
 *   Registered Stateの構造を維持する。
 * - Commit後のcanonical validationは行わない。
 * - Commit後にMaterial IDを出力する。
 *
 * Failure:
 * - Commit前の失敗ではRegistryと出力先を変更しない。
 * - DATA_CORRUPTEDではRegistryの内部整合性を保証しない。
 */
resource_registry_result_t untextured_mesh_material_registry_acquire(untextured_mesh_material_registry_t* registry_, const char* resource_name_, uint16_t* out_material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_index = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_acquire", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_acquire", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_acquire", "out_material_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_acquire(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_acquire(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの登録状態確認
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_mesh_material_registry_acquire(%s) - Provided resource name is not registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < tmp_index) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_acquire(%s) - Resource registry is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    if(registry_->entries[tmp_index].reference_count == UINT16_MAX) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("untextured_mesh_material_registry_acquire(%s) - Reference count overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    registry_->entries[tmp_index].reference_count++;
    *out_material_id_ = (uint16_t)tmp_index;

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
 * - invalidなMaterialはINVALID_ARGUMENTとして扱う。
 * - DEBUG / TESTでは、既存Entryを探索・更新する前に
 *   Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   Registryのcanonical validationを省略する。
 *
 * Preflight:
 * - Resource Nameが未登録であることを確認する。
 * - registry_entry_is_empty()によって登録可能なEntryを探索する。
 * - 登録可能なslotが確定するまでRegistryの状態を変更しない。
 * - Resource Nameの生成に成功してからCommitへ進む。
 *
 * Commit:
 * - Material値を対象Entryにコピーする。
 * - Resource Nameを対象Entryに格納する。
 * - Reference Countを1に初期化する。
 * - Empty StateからRegistered Stateへの遷移を成立させる。
 *
 * Postconditions:
 * - DEBUG / TESTでは、登録後にRegistry全体をcanonical validationし、
 *   Commit後のstable stateが成立していることを確認する。
 * - RELEASEでは、PreconditionsおよびCommitのcontractを信頼し、
 *   canonical validationを省略する。
 * - Material IDの出力と一時Resource Nameの所有権処理は、
 *   Postconditions validationの成功後に行う。
 *
 * Failure:
 * - Commit前の回復可能な失敗では、生成済みの一時Resource Nameを
 *   破棄し、Registryと出力先を変更しない。
 * - DATA_CORRUPTED発生時は内部状態の整合性を信頼できないため、
 *   cleanupによる通常の状態復元を試みない。
 * - Commit後のPostconditions validationで失敗した場合も
 *   DATA_CORRUPTEDとなり、Registryの状態復元を保証しない。
 */
resource_registry_result_t untextured_mesh_material_registry_register(untextured_mesh_material_registry_t* registry_, const char* resource_name_, const untextured_mesh_material_t* material_, uint16_t* out_material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(material_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_register", "material_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_register", "out_material_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!untextured_mesh_material_is_valid(material_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - provided material_ is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - provided resource name is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_material_count; ++i) {
        if(registry_entry_is_empty(&registry_->entries[i])) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - free slot not found.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_choco_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_registry_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    registry_->entries[tmp_index].material = *material_;
    registry_->entries[tmp_index].resource_name = tmp_name;
    registry_->entries[tmp_index].reference_count = 1;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_register(%s) - Postcondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_material_id_ = (uint16_t)tmp_index;
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
 * - DEBUG / TESTでは、Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 * - Material IDの範囲と対象EntryのRegistered Stateを
 *   全Build Modeで直接検査する。
 * - 未登録EntryへのReleaseはBAD_OPERATIONとして扱う。
 *
 * Commit:
 * - Reference Countが2以上の場合は、Reference Countを
 *   1減少させ、Resourceの登録状態を維持する。
 * - Reference Countが1の場合はregistry_entry_deinitialize()へ委譲し、
 *   Resource Nameを破棄してEntryをEmpty Stateへ戻す。
 * - Material値は解放・初期化せず、そのまま保持する。
 * - Reference Countの減算とResource解放を、
 *   同一の利用権返却Operationとして扱う。
 *
 * Postconditions:
 * - Reference Countの減算のみの場合、Registered Stateを維持する。
 * - 最後の利用権の返却では、下位helperのcontractによって
 *   Empty Stateが成立することを信頼する。
 * - Commit後のcanonical validationは行わない。
 *
 * Failure:
 * - Commit前の失敗ではRegistryを変更しない。
 * - DATA_CORRUPTEDではRegistryの内部整合性を保証しない。
 */
resource_registry_result_t untextured_mesh_material_registry_release(untextured_mesh_material_registry_t* registry_, uint16_t material_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "untextured_mesh_material_registry_release", "registry_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_material_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_material_registry_release(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!untextured_mesh_material_id_is_in_range(registry_, material_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_registry_release(%s) - Provided material_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!registry_entry_is_registered(&registry_->entries[material_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_mesh_material_registry_release(%s) - provided material id entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(1 == registry_->entries[material_id_].reference_count) {
        registry_entry_deinitialize(&registry_->entries[material_id_]);
    } else {
        registry_->entries[material_id_].reference_count--;
    }

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * Canonical Validation Policy
 *
 * - Registryのcanonical validityを診断する公開Validatorであり、
 *   Build Modeによって検査内容を変更しない。
 * - RegistryのNULL、最大登録数の範囲、Entry配列ポインタを
 *   先に検査し、明らかに不正な構造を走査しない。
 * - 最大登録数を走査上限として、
 *   全Entryのcanonical validityを確認する。
 * - Entryごとの検証はregistry_entry_is_valid()に委譲する。
 * - 各EntryはReference Countを含めて、
 *   Empty StateまたはRegistered Stateのいずれかを満たす必要がある。
 * - Registered Stateでは、Resource Nameのallocation-level validity、
 *   Choco Stringのcanonical validity、空文字列でないこと、
 *   およびMaterialのcanonical validityを確認する。
 * - Empty StateではMaterialの格納値を検査しない。
 * - Resource Nameの一意性、Registry本体とEntry配列の
 *   allocation-level validity、およびAllocatorの内部状態は
 *   検証対象に含まない。
 *
 * - Validator自体はRegistryを変更しない。
 * - Result validationやPostconditions validationは行わない。
 */
bool untextured_mesh_material_registry_is_valid(const untextured_mesh_material_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_material_count || UINT16_MAX < registry_->max_material_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_material_count; ++i) {
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
 * Private Helper Contract
 *
 * - Entryが保持するResource Nameを破棄する。
 * - NULLのResource Nameに対する破棄は何も行わない。
 * - Reference Countを0に設定する。
 * - Materialは値として保持されているため、
 *   解放・初期化せず、そのまま維持する。
 * - 正常なEmpty StateまたはRegistered StateのEntryに対して、
 *   終了処理後にEmpty Stateを成立させる。
 * - Reference Countの値によって解放動作を切り替えない。
 * - 利用権の管理はcaller側のOperationが担当する。
 *
 * Preconditions:
 * - entry_のNULLを検査し、NULLの場合は何も行わない。
 * - NULL以外のentry_については、Entryの正常なstable stateが
 *   callerによって成立済みであることを前提とする。
 * - Resource Nameの内部整合性は、
 *   callerが成立済みのcontractによって保証する。
 *
 * - 本HelperはResultを返さず、rollbackも行わない。
 */
static void registry_entry_deinitialize(registry_entry_t* entry_) {
    if(NULL == entry_) {
        return;
    }
    choco_string_destroy(&entry_->resource_name);
    entry_->reference_count = 0;
}

// ============================================================
// State Queries
// ============================================================
/*
 * Material IDがRegistryのEntry配列の範囲内にあることを判定する。
 *
 * - registry_がNULLの場合はfalseを返す。
 * - material_id_がmax_material_count未満であることを確認する。
 * - 対象Entryの登録状態やcanonical validityは検査しない。
 * - Material ID = 0も有効なindexとして扱う。
 */
static bool untextured_mesh_material_id_is_in_range(const untextured_mesh_material_registry_t* registry_, uint16_t material_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_material_count <= (size_t)material_id_) {
        return false;
    }
    return true;
}

/*
 * EntryがEmpty Stateの構造的条件を満たすことを判定する。
 *
 * Empty State:
 * - Resource NameがNULLである。
 * - Reference Countが0である。
 * - Materialの格納値は不問とする。
 *
 * - NULL EntryはEmpty Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Materialのcanonical validityは検査しない。
 * - 本Predicateは新規登録可能なEntryの判定にも使用する。
 *
 * - !registry_entry_is_registered()と同義ではない。
 *   Corrupted Entryは両Predicateでfalseとなる場合がある。
 */
static bool registry_entry_is_empty(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL != entry_->resource_name) {
        return false;
    }
    if(0 != entry_->reference_count) {
        return false;
    }
    return true;
}

/*
 * EntryがRegistered Stateの構造的条件を満たすことを判定する。
 *
 * Registered State:
 * - Resource NameがNULLではない。
 * - Reference Countが1以上である。
 *
 * - NULL EntryはRegistered Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Resource NameおよびMaterialのcanonical validityは
 *   本Predicateの検証対象に含まない。
 * - Registered Stateの完全な整合性は
 *   registry_entry_is_valid()が検証する。
 *
 * - !registry_entry_is_empty()と同義ではない。
 *   Corrupted Entryは両Predicateでfalseとなる場合がある。
 */
static bool registry_entry_is_registered(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL == entry_->resource_name) {
        return false;
    }
    if(0 == entry_->reference_count) {
        return false;
    }
    return true;
}

// ============================================================
// Validation
// ============================================================
/*
 * Entryのcanonical validityを判定する。
 *
 * Validation Scope:
 * - NULL Entryはinvalidとする。
 * - Empty Stateの構造的条件を満たすEntryはvalidとする。
 * - Empty StateでないEntryはRegistered Stateの
 *   構造的条件を満たす必要がある。
 * - Registered Stateでは、Resource NameがGeneral Allocator管理下の
 *   current live allocationであることを確認する。
 * - Resource NameがvalidなChoco Stringであり、
 *   空文字列ではないことを確認する。
 * - Materialのcanonical validityを検証する。
 *
 * - Empty / Registeredのどちらにも該当しないEntryはinvalidとする。
 * - Reference CountとResourceの登録状態の矛盾を検出する。
 * - Empty StateではMaterialの格納値を検証しない。
 * - Resource Nameの内部bufferの整合性は
 *   Choco Stringのcanonical validatorに委譲する。
 *
 * - 本ValidatorはEntryの状態を変更しない。
 */
static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    if(registry_entry_is_empty(entry_)) {
        return true;
    }
    if(!registry_entry_is_registered(entry_)) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(entry_->resource_name)) {
        return false;
    }
    if(!choco_string_is_valid(entry_->resource_name)) {
        return false;
    }
    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }

    if(!untextured_mesh_material_is_valid(&entry_->material)) {
        return false;
    }

    return true;
}

// ============================================================
// Lookup
// ============================================================
/*
 * Resource Nameに対応するRegistered Entryを検索する。
 *
 * - Entry配列を先頭から走査し、
 *   Registered Stateの構造的条件を満たすEntryのみ検索する。
 * - Resource Nameの比較はChoco Stringへ委譲する。
 * - 発見した場合、対応するEntry indexをout_index_へ出力する。
 * - 未発見の場合はfalseを返し、out_index_を変更しない。
 * - NULL引数の場合もfalseを返す。
 *
 * Validation:
 * - 本HelperはRegistry全体やEntryのcanonical validationを行わない。
 * - 検索対象のRegistryと登録済みResourceについては、
 *   callerが成立済みのinternal contractを保証する。
 * - Entryのcorruptionを診断することは本Helperの責務ではない。
 * - Registered Stateを満たさないEntryは検索対象から除外する。
 *
 * - RegistryおよびReference Countを変更しない。
 */
static bool find_by_name(const untextured_mesh_material_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_material_count; ++i) {
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
