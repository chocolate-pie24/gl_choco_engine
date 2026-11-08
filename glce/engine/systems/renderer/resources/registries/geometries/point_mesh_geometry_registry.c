// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/registries/geometries/point_mesh_geometry_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/geometries/point_mesh_geometry.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Resource Identity:
 * - Geometry IDはEntry配列のindexに対応する。
 * - IDとEntryの対応を管理する独立したmappingは持たない。
 * - 解放済みEntryのindexは、後の新規登録で再利用できる。
 *
 * Entry State:
 * - Entryの正常なstable stateはEmpty StateとRegistered Stateである。
 * - Empty Stateでは、所有ResourceとVBO Allocation Descriptorが
 *   空状態であり、Reference Countは0となる。
 * - Registered Stateでは、Resource Name、CPU Geometry、
 *   VBO Allocation Descriptorを一組のResourceとして保持し、
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
 * - 最後の利用権の返却では所有Resourceを解放し、
 *   EntryをEmpty Stateへ戻す。
 * - Registry全体のdeinitializeではReference Countを消費せず、
 *   各登録Resourceを一度だけ解放する。
 *
 * Resource Ownership:
 * - RegistryはResource NameとCPU Geometryを所有する。
 * - 対応するVBO Allocationの解放責務を持つ。
 * - VBO Manager本体は所有せず、解放操作時にborrowする。
 * - VBO Allocationの解放にはvbo_manager_free()を使用する。
 * - CPU GeometryとResource Nameの解放には、それぞれの
 *   Resource Moduleが提供するdestroy APIを使用する。
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
 * Point Mesh Geometryの登録情報を保持するEntry。
 *
 * - resource_nameはRegistryが所有するResource Nameを指す。
 * - cpu_resourceはRegistryが所有するCPU Geometryを指す。
 * - allocation_descriptorは対応するVBO Allocationの
 *   Descriptorを値として保持する。
 * - reference_countは、このEntryのResourceに対して
 *   保持されている利用権の数を表す。
 * - Empty Stateではreference_countは0となる。
 * - Registered Stateではreference_countは1以上となる。
 * - 各Stateの構造的成立条件およびcanonical validityは、
 *   対応するState PredicateとValidatorが定義する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    point_mesh_geometry_t* cpu_resource;
    vbo_range_t allocation_descriptor;
    uint16_t reference_count;
} registry_entry_t;

/*
 * Point Mesh Geometryの登録状態を管理するRegistry本体。
 *
 * - max_geometry_countはEntry配列の容量を表す。
 * - entriesはGeometry IDによって直接参照するEntry配列である。
 * - 正常な利用可能状態では、max_geometry_countは
 *   1以上UINT16_MAX以下、entriesはNULL以外となる。
 * - Registryの利用期間中、Entry配列のstorageは変更しない。
 * - deinitializeが正常に完了するとmax_geometry_countが0となり、
 *   Registryのlogical lifetimeが終了する。
 */
struct point_mesh_geometry_registry {
    size_t max_geometry_count;          /**< レジストリに登録可能な最大ジオメトリ数(0は許可しない. 点描画を使用しなくても1以上にする) */
    registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* entry_, vbo_manager_t* vbo_manager_);

// State Queries
static bool geometry_id_is_in_range(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);
static bool registry_entry_is_empty(const registry_entry_t* entry_);
static bool registry_entry_is_registered(const registry_entry_t* entry_);

// Validation
static bool registry_entry_is_valid(const registry_entry_t* entry_);

// Lookup
static bool find_by_name(const point_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_);

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
resource_registry_result_t point_mesh_geometry_registry_create(size_t max_geometry_count_, subsystem_allocator_t* allocator_, point_mesh_geometry_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    point_mesh_geometry_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entry_array = NULL;

    size_t entry_array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_create", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "point_mesh_geometry_registry_create", "*out_registry_")
    if(0 == max_geometry_count_ || UINT16_MAX < max_geometry_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - Provided max_geometry_count_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }
    if((SIZE_MAX / max_geometry_count_) < sizeof(registry_entry_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    entry_array_size = sizeof(registry_entry_t) * max_geometry_count_;

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // geometry_registry_tメモリ確保
    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(point_mesh_geometry_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - Failed to allocate registry instance. target=point_mesh_geometry_registry_t, bytes=%zu, max_geometry_count=%zu", resource_registry_result_to_str(ret), sizeof(point_mesh_geometry_registry_t), max_geometry_count_);
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, entry_array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_entry_array);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - allocation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_geometry_count = max_geometry_count_;
    tmp_registry->entries = tmp_entry_array;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(tmp_registry)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - Result validation failed.", resource_registry_result_to_str(ret));
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
            ERROR_MESSAGE("point_mesh_geometry_registry_create(%s) - subsystem_allocator rollback failed.", resource_registry_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_とvbo_manager_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、登録済みEntryのowned resourceを安全に
 *   解放できることを確認するため、Registry全体を
 *   canonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 * - VBO Managerの内部整合性とVBO Allocationの解放条件は、
 *   VBO Manager側のAPI contractとvalidationに委ねる。
 *
 * Deinitialization:
 * - Empty StateでないEntryに対して
 *   registry_entry_deinitialize()を実行する。
 * - Reference Countの値によって解放処理を切り替えない。
 * - 各登録ResourceのVBO Allocation、CPU Geometry、
 *   Resource Nameを一度だけ解放する。
 * - 全Entryの終了処理が成功した後、
 *   max_geometry_countを0に設定する。
 *
 * Failure:
 * - VBO Allocationの解放に失敗した場合は、
 *   Registryの内部所有関係を維持できない可能性があるため、
 *   DATA_CORRUPTED相当として処理を中断する。
 * - 解放失敗後は、残りのEntryの通常解放を継続しない。
 * - 本APIはvoidであり、失敗をResultとして返さない。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なstable object modelから
 *   外れるため、通常のcanonical validationを実行しない。
 */
void point_mesh_geometry_registry_deinitialize(point_mesh_geometry_registry_t* registry_, vbo_manager_t* vbo_manager_) {
    if(NULL == registry_ || NULL == vbo_manager_) {
        ERROR_MESSAGE("point_mesh_geometry_registry_deinitialize(%s) - provided registry_ or vbo_manager_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("point_mesh_geometry_registry_deinitialize(%s) - point_mesh_geometry_registry_t internal state is corrupted.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(!registry_entry_is_empty(&registry_->entries[i])) {
            if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[i], vbo_manager_)) {
                ERROR_MESSAGE("point_mesh_geometry_registry_deinitialize(%s) - registry_entry_deinitialize failed.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
                return;
            }
        }
    }
    registry_->max_geometry_count = 0;
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
resource_registry_result_t point_mesh_geometry_registry_exists(const point_mesh_geometry_registry_t* registry_, const char* resource_name_, bool* out_exists_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;
    bool exists = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_exists", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_exists", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_exists_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_exists", "out_exists_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
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
 * - registry_のNULL、Geometry IDの範囲、
 *   および対象EntryのRegistered Stateを直接検査する。
 * - 指定IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - CPU Geometry自体のcanonical validationも実行せず、
 *   登録時に確立されたinternal contractを信頼する。
 *
 * Result:
 * - 戻り値はRegistryが所有するCPU Geometryへの
 *   読み取り専用borrowである。
 * - Reference Countを変更せず、利用権を新たに取得しない。
 * - 新しいstateを構築・変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
const point_mesh_geometry_t* point_mesh_geometry_registry_geometry_get(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("point_mesh_geometry_registry_geometry_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!geometry_id_is_in_range(registry_, geometry_id_)) {
        ERROR_MESSAGE("point_mesh_geometry_registry_geometry_get(%s) - provided geometry_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[geometry_id_])) {
        ERROR_MESSAGE("point_mesh_geometry_registry_geometry_get(%s) - provided geometry_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[geometry_id_].cpu_resource;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Geometry IDの範囲、
 *   および対象EntryのRegistered Stateを直接検査する。
 * - 指定IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - VBO Range自体のcanonical validationも実行せず、
 *   登録時に確立されたinternal contractを信頼する。
 *
 * Result:
 * - 戻り値はRegistry内部のVBO Allocation Descriptorに
 *   格納されたDraw Rangeへの読み取り専用borrowである。
 * - Reference Countを変更せず、利用権を新たに取得しない。
 * - 新しいstateを構築・変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
const draw_range_t* point_mesh_geometry_registry_draw_range_get(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("point_mesh_geometry_registry_draw_range_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!geometry_id_is_in_range(registry_, geometry_id_)) {
        ERROR_MESSAGE("point_mesh_geometry_registry_draw_range_get(%s) - provided geometry_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[geometry_id_])) {
        ERROR_MESSAGE("point_mesh_geometry_registry_draw_range_get(%s) - no allocation.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return &registry_->entries[geometry_id_].allocation_descriptor.draw_range;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、resource_name_、out_geometry_id_のNULLと、
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
 * - Resource Name、CPU Geometry、VBO Allocation Descriptor、
 *   およびGeometry IDとの対応は変更しない。
 *
 * Postconditions:
 * - Commitは検証済みのReference Countに対する単一の加算であり、
 *   Registered Stateの構造を維持する。
 * - Commit後のcanonical validationは行わない。
 * - Commit後にGeometry IDを出力する。
 *
 * Failure:
 * - Commit前の失敗ではRegistryと出力先を変更しない。
 * - DATA_CORRUPTEDではRegistryの内部整合性を保証しない。
 */
resource_registry_result_t point_mesh_geometry_registry_acquire(point_mesh_geometry_registry_t* registry_, const char* resource_name_, uint16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_index = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_acquire", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_acquire", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_acquire", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_acquire(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_acquire(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの登録状態確認
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_registry_acquire(%s) - Provided resource name is not registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < tmp_index) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_acquire(%s) - Resource registry is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    if(registry_->entries[tmp_index].reference_count == UINT16_MAX) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("point_mesh_geometry_registry_acquire(%s) - Reference count overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    registry_->entries[tmp_index].reference_count++;
    *out_geometry_id_ = (uint16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - 入力ポインタのNULLとResource Nameの空文字列を検査する。
 * - callerから受け取るCPU GeometryとVBO Rangeは
 *   Engine API Trust Boundaryを越えるResourceであるため、
 *   全Build Modeでそれぞれcanonical validationする。
 * - invalidなCPU GeometryまたはVBO Rangeは
 *   INVALID_ARGUMENTとして扱う。
 * - 既存の登録名を検索し、Entry配列から登録可能なslotを
 *   探索するため、DEBUG / TESTではRegistry全体を
 *   canonical validationする。
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
 * - CPU Geometryの所有ポインタを対象Entryに格納する。
 * - VBO Allocation Descriptorを対象Entryに値コピーする。
 * - Resource Nameを対象Entryに格納する。
 * - Reference Countを1に初期化する。
 * - Empty StateからRegistered Stateへの遷移を成立させる。
 *
 * Postconditions:
 * - DEBUG / TESTでは、登録後にRegistry全体をcanonical validationし、
 *   Commit後のstable stateが成立していることを確認する。
 * - RELEASEでは、PreconditionsおよびCommitのcontractを信頼し、
 *   canonical validationを省略する。
 * - caller側の所有ポインタとDescriptorの初期化、およびID出力は、
 *   Postconditions validationの成功後に行う。
 *
 * Failure:
 * - Commit前の回復可能な失敗では、生成済みの一時Resource Nameを
 *   破棄し、Registryとcaller側の所有状態を変更しない。
 * - DATA_CORRUPTED発生時は内部状態の整合性を信頼できないため、
 *   cleanupによる通常の状態復元を試みない。
 * - Commit後のPostconditions validationで失敗した場合も
 *   DATA_CORRUPTEDとなり、所有権移転状態の復元は保証しない。
 */
resource_registry_result_t point_mesh_geometry_registry_register(point_mesh_geometry_registry_t* registry_, const char* resource_name_, point_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "*geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_range_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "vbo_range_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_register", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!point_mesh_geometry_is_valid(*geometry_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - provided *geometry_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!vbo_range_is_valid(vbo_range_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - provided *vbo_range_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - provided resource name is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(registry_entry_is_empty(&registry_->entries[i])) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - free slot not found.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_choco_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_registry_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    // entry登録, 所有権移動commit(allocation_descriptorはスタック領域にメモリ確保されたローカル変数の場合があるため値コピー)
    registry_->entries[tmp_index].cpu_resource = *geometry_;
    registry_->entries[tmp_index].allocation_descriptor = *vbo_range_;
    registry_->entries[tmp_index].resource_name = tmp_name;
    registry_->entries[tmp_index].reference_count = 1;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_register(%s) - Postcondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *geometry_ = NULL;
    memset(vbo_range_, 0, sizeof(vbo_range_t));
    tmp_name = NULL;
    *out_geometry_id_ = (uint16_t)tmp_index;

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
 * - registry_とvbo_manager_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 * - Geometry IDの範囲と対象EntryのRegistered Stateを
 *   全Build Modeで直接検査する。
 * - 未登録EntryへのReleaseはBAD_OPERATIONとして扱う。
 * - VBO Managerの内部整合性とVBO Allocationの解放条件は、
 *   VBO Manager側のAPI contractとvalidationに委ねる。
 *
 * Validation Scope:
 * - ReleaseはRegistryの状態を変更するため、DEBUG / TESTでは
 *   診断範囲を広げてRegistry全体をcanonical validationする。
 * - Operationの対象は単一Entryであるため、
 *   Registry全体の検査は実行上必須の条件ではない。
 * - Validation Costが問題となる場合は、対象Entryのみの
 *   canonical validationへ変更する選択肢がある。
 *
 * Commit:
 * - Reference Countが2以上の場合は、Reference Countを
 *   1減少させ、Resourceの登録状態を維持する。
 * - Reference Countが1の場合はregistry_entry_deinitialize()へ委譲し、
 *   所有Resourceを解放してEntryをEmpty Stateへ戻す。
 * - VBO Allocationの解放をCPU Geometryおよび
 *   Resource Nameの破棄より先に実行する。
 *
 * Postconditions:
 * - Reference Countの減算のみの場合、Registered Stateを維持する。
 * - 最後の利用権の返却では、下位helperのcontractによって
 *   Empty Stateが成立することを信頼する。
 * - Commit後のcanonical validationは行わない。
 *
 * Failure:
 * - Commit前の失敗ではRegistryを変更しない。
 * - VBO Allocationの解放に失敗した場合は、
 *   対象Entryの正常な終了処理を完了できないため、
 *   DATA_CORRUPTEDとして処理を中断する。
 * - DATA_CORRUPTEDでは対象Entryの状態復元を保証しない。
 */
resource_registry_result_t point_mesh_geometry_registry_release(point_mesh_geometry_registry_t* registry_, vbo_manager_t* vbo_manager_, uint16_t geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_release", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_release", "vbo_manager_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_release(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!geometry_id_is_in_range(registry_, geometry_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_release(%s) - Provided geometry_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!registry_entry_is_registered(&registry_->entries[geometry_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_registry_release(%s) - provided geometry_id_ entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(1 == registry_->entries[geometry_id_].reference_count) {
        if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[geometry_id_], vbo_manager_)) {
            ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
            ERROR_MESSAGE("point_mesh_geometry_registry_release(%s) - registry_entry_deinitialize failed.", resource_registry_result_to_str(ret));
            goto cleanup;
        }
    } else {
        registry_->entries[geometry_id_].reference_count--;
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
 * - Registered Stateでは、Resource NameとCPU Geometryの
 *   allocation-level validity、各Resourceのcanonical validity、
 *   およびVBO Rangeのcanonical validityを確認する。
 * - Resource Nameの一意性、CPU GeometryとVBO Rangeの
 *   相互整合性、VBO Allocationの実在性、
 *   Registry本体とEntry配列のallocation-level validity、
 *   およびAllocatorの内部状態は検査対象に含めない。
 *
 * - Validator自体はRegistryを変更しない。
 * - Result validationやPostconditions validationは行わない。
 */
bool point_mesh_geometry_registry_is_valid(const point_mesh_geometry_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_geometry_count || UINT16_MAX < registry_->max_geometry_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
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
 * - Registered StateのEntryが所有するResourceを解放する。
 * - VBO Allocation、CPU Geometry、Resource Nameの順で解放する。
 * - VBO Allocationの解放はvbo_manager_free()へ委譲する。
 * - VBO Allocationの解放に成功した後、
 *   Allocation Descriptorをゼロ初期化する。
 * - CPU GeometryとResource Nameをそれぞれdestroyし、
 *   Reference Countを0に設定する。
 * - 正常終了時にはEntryがEmpty Stateとなる。
 *
 * Preconditions:
 * - entry_とvbo_manager_のNULLを検査する。
 * - 対象EntryはRegistered Stateであり、
 *   所有Resourceのinternal contractが成立していることを
 *   callerが保証する。
 * - VBO Managerは対象Allocationを管理する有効なobjectであることを
 *   callerが保証する。
 * - 本HelperはEntryのcanonical validationを実行せず、
 *   callerが成立済みのcontractを信頼する。
 *
 * Failure:
 * - vbo_manager_free()が失敗した場合、
 *   DATA_CORRUPTEDを返して処理を中断する。
 * - VBO Allocationの解放失敗後は、CPU Geometryと
 *   Resource Nameの破棄を行わない。
 * - 本Helperはrollbackを行わない。
 *
 * Postconditions:
 * - 成功時にはEntryの所有Resourceが解放され、
 *   Empty Stateが成立する。
 * - 成功後のcanonical validationは行わない。
 */
static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* entry_, vbo_manager_t* vbo_manager_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    if(NULL == entry_ || NULL == vbo_manager_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        goto cleanup;
    }

    ret_buffer_manager = vbo_manager_free(vbo_manager_, &entry_->allocation_descriptor.allocation_info);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("registry_entry_deinitialize(%s) - vbo_manager_free failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    memset(&entry_->allocation_descriptor, 0, sizeof(vbo_range_t));

    point_mesh_geometry_destroy(&entry_->cpu_resource);
    choco_string_destroy(&entry_->resource_name);
    entry_->reference_count = 0;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// State Queries
// ============================================================
/*
 * Geometry IDがRegistryのEntry配列の範囲内にあることを判定する。
 *
 * - registry_がNULLの場合はfalseを返す。
 * - geometry_id_がmax_geometry_count未満であることを確認する。
 * - 対象Entryの登録状態やcanonical validityは検査しない。
 * - Geometry ID = 0も有効なindexとして扱う。
 * - RegistryおよびEntryの状態を変更しない。
 */
static bool geometry_id_is_in_range(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_geometry_count <= (size_t)geometry_id_) {
        return false;
    }
    return true;
}

/*
 * EntryがEmpty Stateの構造的条件を満たすことを判定する。
 *
 * Empty State:
 * - Resource NameとCPU GeometryのポインタがNULLである。
 * - VBO Allocation Descriptorのallocation_infoと
 *   draw_rangeの各Fieldがゼロ状態である。
 * - Reference Countが0である。
 *
 * - NULL EntryはEmpty Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Resource Name、CPU Geometry、VBO Rangeの
 *   canonical validityは検査しない。
 * - 本Predicateは新規登録可能なEntryの判定にも使用する。
 *
 * - !registry_entry_is_registered()と同義ではない。
 *   Corrupted Entryは両Predicateでfalseとなる場合がある。
 */
static bool registry_entry_is_empty(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL != entry_->resource_name || NULL != entry_->cpu_resource) {
        return false;
    }
    if(0 != entry_->allocation_descriptor.allocation_info.allocated_size) {
        return false;
    }
    if(0 != entry_->allocation_descriptor.allocation_info.node_index) {
        return false;
    }
    if(0 != entry_->allocation_descriptor.allocation_info.offset) {
        return false;
    }
    if(NULL != entry_->allocation_descriptor.allocation_info.owner) {
        return false;
    }
    if(0 != entry_->allocation_descriptor.draw_range.first_vertex_count) {
        return false;
    }
    if(0 != entry_->allocation_descriptor.draw_range.vertex_count) {
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
 * - Resource NameとCPU GeometryのポインタがNULLではない。
 * - VBO Allocation Sizeが0ではない。
 * - VBO AllocationのownerがNULLではない。
 * - Draw RangeのVertex Countが0ではない。
 * - Reference Countが1以上である。
 *
 * - NULL EntryはRegistered Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Resource Name、CPU Geometry、VBO Rangeの
 *   canonical validityは本Predicateの検証対象に含まない。
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
    if(NULL == entry_->resource_name || NULL == entry_->cpu_resource) {
        return false;
    }
    if(0 == entry_->allocation_descriptor.allocation_info.allocated_size) {
        return false;
    }
    if(NULL == entry_->allocation_descriptor.allocation_info.owner) {
        return false;
    }
    if(0 == entry_->allocation_descriptor.draw_range.vertex_count) {
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
 * - CPU GeometryがGeneral Allocator管理下の
 *   current live allocationであることを確認する。
 * - CPU GeometryとVBO Rangeのcanonical validityを検証する。
 *
 * - Empty / Registeredのどちらにも該当しないEntryはinvalidとする。
 * - Reference CountとResourceの登録状態の矛盾を検出する。
 * - Resource Nameの内部bufferの整合性は
 *   Choco Stringのcanonical validatorに委譲する。
 * - CPU Geometryの内部整合性は
 *   Point Mesh Geometryのcanonical validatorに委譲する。
 * - VBO Allocation Descriptorの内部整合性は
 *   vbo_range_is_valid()に委譲する。
 *
 * - 本ValidatorはEntryの状態を変更しない。
 */
static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // Empty State.
    if(registry_entry_is_empty(entry_)) {
        return true;
    }
    if(!registry_entry_is_registered(entry_)) {
        return false;
    }

    // Registered State.
    if(NULL == entry_->resource_name || NULL == entry_->cpu_resource) {
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

    if(!general_allocator_ptr_is_allocated(entry_->cpu_resource)) {
        return false;
    }
    if(!point_mesh_geometry_is_valid(entry_->cpu_resource)) {
        return false;
    }

    if(!vbo_range_is_valid(&entry_->allocation_descriptor)) {
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
static bool find_by_name(const point_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
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
