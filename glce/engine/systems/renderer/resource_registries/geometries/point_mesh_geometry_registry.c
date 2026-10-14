// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/geometry/point_mesh_geometry.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/point_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Resource Identity:
 * - Geometry IDはEntry配列のindexに対応する。
 * - IDとEntryの対応を管理する独立したmappingは持たない。
 *
 * Registration State:
 * - EntryはEmpty StateまたはRegistered Stateを持つ。
 * - Empty StateのEntryは新規登録に利用できる。
 * - Registered StateのEntryは、Resource Name、CPU Geometry、
 *   VBO Allocation Descriptorを一組の登録Resourceとして扱う。
 * - 登録解除後のEntryはEmpty Stateへ戻り、再利用できる。
 * - 各Stateのcanonical validityはregistry_entry_is_valid()が定義する。
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
 * - EntryはEmpty StateまたはRegistered Stateを取り、
 *   各状態の成立条件はregistry_entry_is_valid()が定義する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    point_mesh_geometry_t* cpu_resource;
    vbo_range_t allocation_descriptor;
} registry_entry_t;

/*
 * Point Mesh Geometryの登録状態を管理するRegistry本体。
 *
 * - max_geometry_countはEntry配列の容量を表す。
 * - entriesはGeometry IDによって直接参照するEntry配列である。
 * - 正常な利用可能状態では、max_geometry_countは
 *   1以上UINT16_MAX以下、entriesはNULL以外となる。
 * - deinitialize後はmax_geometry_countが0となり、
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
static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* entry_, point_mesh_shader_t* shader_);

// State Queries
static bool geometry_id_is_in_range(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);
static bool registry_entry_is_empty(const registry_entry_t* entry_);

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
 * Result validation:
 * - DEBUG / TESTでは、構築したRegistry全体をcanonical validationし、
 *   callerへ公開する前にEmpty Stateからなるstable object modelの
 *   成立を確認する。
 * - RELEASEでは、zero-initialized storageと構築処理のcontractを
 *   信頼し、canonical validationを省略する。
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
 * - registry_とshader_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、登録済みEntryのowned resourceを安全に
 *   解放できることを確認するため、Registry全体を
 *   canonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 * - Shaderの内部整合性およびVBO Allocationの解放条件は、
 *   Point Mesh Shader側のAPI contractとvalidationに委ねる。
 *
 * Failure:
 * - VBO Allocationの解放に失敗した場合は、
 *   Registryの内部所有関係を維持できない可能性があるため
 *   DATA_CORRUPTED相当として処理を中断する。
 * - DATA_CORRUPTED確定後は、残りのEntryの通常解放を継続しない。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なstable object modelから
 *   外れるため、通常のcanonical validationを実行しない。
 */
void point_mesh_geometry_registry_deinitialize(point_mesh_geometry_registry_t* registry_, point_mesh_shader_t* shader_) {
    if(NULL == registry_ || NULL == shader_) {
        ERROR_MESSAGE("point_mesh_geometry_registry_deinitialize(%s) - provided registry_ or shader_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
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
            if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[i], shader_)) {
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
 * - registry_とname_のNULL、および空のResource Nameを検査する。
 * - DEBUG / TESTでは、Entry配列と登録名を走査する前に
 *   Registry全体のcanonical validationを行い、
 *   内部表現の整合性を確認する。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * - このAPIは登録状態を変更せず、検索結果のみを返すため、
 *   Result validationとPostconditions validationは行わない。
 */
bool point_mesh_geometry_registry_exists(const point_mesh_geometry_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("point_mesh_geometry_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("point_mesh_geometry_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return false;
    }
#endif

    return find_by_name(registry_, name_, &tmp_id);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Geometry IDの範囲、
 *   および対象Entryの登録状態を直接検査する。
 * - 指定IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - CPU Geometry自体のcanonical validationも実行せず、
 *   登録時に確立されたinternal contractを信頼する。
 *
 * - 戻り値は既存CPU Geometryへの読み取り専用borrowであり、
 *   新しいstateを構築・変更しないため、
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
    if(registry_entry_is_empty(&registry_->entries[geometry_id_])) {
        ERROR_MESSAGE("point_mesh_geometry_registry_geometry_get(%s) - provided geometry_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[geometry_id_].cpu_resource;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、name_、out_geometry_id_のNULL、
 *   および空のResource Nameを検査する。
 * - Resource Nameによる検索ではEntry配列全体を走査するため、
 *   DEBUG / TESTでは検索前にRegistry全体をcanonical validationし、
 *   Entry配列と登録名の内部表現の整合性を確認する。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 *
 * Result validation:
 * - 検索結果のIDはRegistryのEntry配列のindexから導出され、
 *   成立済みの容量contractによってuint16_tの範囲内に収まるため、
 *   追加のcanonical validationを行わない。
 *
 * Postconditions:
 * - Registryの登録状態は変更せず、確定したIDを出力するだけなので、
 *   Postconditions validationは行わない。
 */
resource_registry_result_t point_mesh_geometry_registry_id_get(const point_mesh_geometry_registry_t* registry_, const char* name_, uint16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_id_get", "out_geometry_id_")
    if('\0' == name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_id_get(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_id_get(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_registry_id_get(%s) - find_by_name failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_geometry_id_ = (uint16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Geometry IDの範囲、
 *   および対象Entryの登録状態を直接検査する。
 * - 指定IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - VBO Range自体のcanonical validationも実行せず、
 *   登録時に確立されたinternal contractを信頼する。
 *
 * - 戻り値は既存VBO Allocation Descriptor内のDraw Rangeへの
 *   読み取り専用borrowであり、新しいstateを構築・変更しないため、
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
    if(registry_entry_is_empty(&registry_->entries[geometry_id_])) {
        ERROR_MESSAGE("point_mesh_geometry_registry_draw_range_get(%s) - no allocation.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return &registry_->entries[geometry_id_].allocation_descriptor.draw_range;
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
 * - Resource Nameが未登録であり、Empty StateのEntryが
 *   存在することを確認する。
 * - 登録可能なslotが確定するまで、Registryの状態を変更しない。
 *
 * Postconditions:
 * - DEBUG / TESTでは、CPU Geometry、VBO Range、Resource Nameを
 *   Entryへ登録した後にRegistry全体をcanonical validationし、
 *   Commit後のstable stateが成立していることを確認する。
 * - RELEASEでは、PreconditionsおよびCommitのcontractを信頼し、
 *   canonical validationを省略する。
 * - caller側の所有ポインタとDescriptorの初期化、およびID出力は
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
 * - registry_とshader_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、登録済みEntryのowned resourceを安全に
 *   解放できることを確認するため、Registry全体を
 *   canonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 * - Geometry IDの範囲と対象Entryの登録状態は、
 *   誤ったEntryの参照や登録解除を防ぐため、
 *   全Build Modeで直接検査する。
 * - Shaderの内部整合性とVBO Allocationの解放条件は、
 *   Point Mesh Shader側のAPI contractとvalidationに委ねる。
 *
 * Failure:
 * - VBO Allocationの解放に失敗した場合は、
 *   対象Entryの正常な登録解除を完了できないため、
 *   DATA_CORRUPTEDとして処理を中断する。
 * - 解放失敗後の対象Entryについて、
 *   通常の状態復元や残りのResource破棄は試みない。
 *
 * Postconditions:
 * - 正常終了時、対象EntryはResourceのlifecycle contractに従って
 *   Empty Stateへ遷移する。
 * - 解放完了後のEntryを同じ条件で再検証する必要はないため、
 *   Registry全体のPostconditions validationは行わない。
 */
resource_registry_result_t point_mesh_geometry_registry_unregister(point_mesh_geometry_registry_t* registry_, point_mesh_shader_t* shader_, uint16_t geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_unregister", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "point_mesh_geometry_registry_unregister", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!point_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_unregister(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!geometry_id_is_in_range(registry_, geometry_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_registry_unregister(%s) - Provided geometry_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(registry_entry_is_empty(&registry_->entries[geometry_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_registry_unregister(%s) - provided geometry_id_ entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[geometry_id_], shader_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_registry_unregister(%s) - registry_entry_deinitialize failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * - Registryのcanonical validityを診断する公開Validatorであり、
 *   Build Modeによって検査内容を変更しない。
 * - RegistryのNULL、最大登録数の範囲、Entry配列ポインタを
 *   先に検査し、明らかに不正な構造を走査しない。
 * - 最大登録数を走査上限として、全Entryのcanonical validityを
 *   確認する。
 * - 各Entryについて、Empty StateまたはRegistered Stateの
 *   いずれかが成立していることを要求する。
 * - 登録済みEntryではResource Name、CPU Geometry、
 *   VBO Rangeのcanonical validityを確認する。
 * - Resource Nameの一意性、CPU GeometryとVBO Range間の
 *   相互整合性、VBO Allocationの実在性、
 *   backing storageのallocation-level validityは
 *   検査対象に含めない。
 *
 * - Validator自体はRegistryを変更せず、Result validationや
 *   Postconditions validationは行わない。
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
static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* entry_, point_mesh_shader_t* shader_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    if(NULL == entry_ || NULL == shader_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        goto cleanup;
    }

    if(SHADER_SUCCESS != point_mesh_shader_vbo_free(shader_, &entry_->allocation_descriptor)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("registry_entry_deinitialize(%s) - point_mesh_shader_vbo_free failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    memset(&entry_->allocation_descriptor, 0, sizeof(vbo_range_t));

    point_mesh_geometry_destroy(&entry_->cpu_resource);
    choco_string_destroy(&entry_->resource_name);

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// State Queries
// ============================================================
static bool geometry_id_is_in_range(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_geometry_count <= (size_t)geometry_id_) {
        return false;
    }
    return true;
}

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
    return true;
}

// ============================================================
// Validation
// ============================================================
static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // Empty State.
    if(registry_entry_is_empty(entry_)) {
        return true;
    }

    // Registered State.
    if(NULL == entry_->resource_name || NULL == entry_->cpu_resource) {
        return false;
    }
    if(!choco_string_is_valid(entry_->resource_name)) {
        return false;
    }
    if(0 == choco_string_length(entry_->resource_name)) {
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
static bool find_by_name(const point_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
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
