// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_registries/texture/texture_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/texture/texture_cpu_resource.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

/*
 * Module Internal Contract
 *
 * Entry State:
 * - 各EntryのStable Stateは、EmptyとRegisteredの2種類とする。
 * - Empty Stateでは、resource_name、cpu_resource、gpu_resourceがすべてNULLとなる。
 * - Registered Stateでは、3フィールドすべてが非NULLとなる。
 * - 登録および解除では3フィールドを一つの論理的なEntryとして扱い、
 *   部分的な登録状態をStable Stateとして公開しない。
 *
 * Initialization:
 * - Subsystem Allocatorが確保領域をゼロ初期化するContractを利用し、
 *   create時にすべてのEntryをEmpty Stateとして初期化する。
 * - 利用可能期間中はEntry配列の容量を変更しない。
 * - deinitialize後はmax_texture_countが0となり、
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
 * Texture IDに対応する一つの登録Slotを表す。
 *
 * resource_nameを登録Resourceの検索キーとし、
 * CPU / GPU Resourceを同一TextureのResource Pairとして保持する。
 * 独立した登録状態フラグは持たず、3フィールドのNULL状態によって
 * Entryの状態を表現する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    texture_gpu_resource_t* gpu_resource;
    texture_cpu_resource_t* cpu_resource;
} registry_entry_t;

/*
 * Texture Registryの内部Storage Representation。
 *
 * max_texture_countは登録済みTexture数ではなくEntry配列の容量を表す。
 * entriesの配列IndexがTexture IDに対応する。
 */
struct texture_registry {
    size_t max_texture_count;
    registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static void registry_entry_deinitialize(registry_entry_t* entry_);

// State Queries
static bool texture_id_is_in_range(const texture_registry_t* registry_, uint16_t texture_id_);

// Validation
static bool registry_entry_is_valid(const registry_entry_t* entry_);

// Lookup
static bool find_by_name(const texture_registry_t* registry_, const char* name_, size_t* out_index_);

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
resource_registry_result_t texture_registry_create(size_t max_texture_count_, subsystem_allocator_t* allocator_, texture_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    texture_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entries = NULL;

    size_t array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_create", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "texture_registry_create", "*out_registry_")
    if(0 == max_texture_count_ || UINT16_MAX < max_texture_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_create(%s) - Provided max_texture_count_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }
    if((SIZE_MAX / max_texture_count_) < sizeof(registry_entry_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("texture_registry_create(%s) - array size overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    array_size = sizeof(registry_entry_t) * max_texture_count_;

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("texture_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(texture_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("texture_registry_create(%s) - Failed to allocate registry instance. target=texture_registry_create, bytes=%zu, max_texture_count=%zu", resource_registry_result_to_str(ret), sizeof(texture_registry_t), max_texture_count_);
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_entries);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("texture_registry_create(%s) - allocation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_texture_count = max_texture_count_;
    tmp_registry->entries = tmp_entries;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(tmp_registry)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_create(%s) - Result validation failed.", resource_registry_result_to_str(ret));
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
            ERROR_MESSAGE("texture_registry_create(%s) - subsystem_allocator rollback failed.", resource_registry_result_to_str(ret));
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
void texture_registry_deinitialize(texture_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_deinitialize(%s) - provided registry_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_deinitialize(%s) - texture_registry_t internal state is corrupted.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i]);
    }
    registry_->max_texture_count = 0;
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
bool texture_registry_exists(const texture_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("texture_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return false;
    }
#endif

    return find_by_name(registry_, name_, &tmp_id);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULL、Texture IDの範囲、対象Entryの
 *   Resource Nameが非NULLであることを全Buildで確認する。
 * - Resource Nameの文字列Bufferへのborrow取得は
 *   choco_string_c_str()へ委譲する。
 * - 本Operationでは文字列内容のSemanticをConsumeしないため、
 *   Resource NameのCanonical Validationは行わない。
 * - 文字列内容を実際にConsumeするOperationが、
 *   必要なValidation Responsibilityを持つ。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、既存Resource Nameへのborrowを返すため、
 *   追加のValidationは行わない。
 */
const char* texture_registry_name_get(const texture_registry_t* registry_, uint16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_id_is_in_range(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided texture_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == registry_->entries[texture_id_].resource_name) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided texture_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return choco_string_c_str(registry_->entries[texture_id_].resource_name);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULL、Texture IDの範囲、対象Entryの
 *   GPU Resourceが非NULLであることを全Buildで確認する。
 * - GPU Resourceの内部状態をConsumeせず、既存Pointerをborrowとして返すだけなので、
 *   GPU ResourceのCanonical Validationは行わない。
 * - GPU Resourceの内部状態に対するValidationは、
 *   それをConsumeするOperationの責務とする。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、既存Resourceへのborrowを返すため、
 *   追加のValidationは行わない。
 */
const texture_gpu_resource_t* texture_registry_gpu_resource_get(const texture_registry_t* registry_, uint16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_id_is_in_range(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided texture_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == registry_->entries[texture_id_].gpu_resource) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided texture_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].gpu_resource;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - RegistryのNULL、Texture IDの範囲、対象Entryの
 *   CPU Resourceが非NULLであることを全Buildで確認する。
 * - CPU Resourceの内部状態をConsumeせず、既存Pointerをborrowとして返すだけなので、
 *   CPU ResourceのCanonical Validationは行わない。
 * - CPU Resourceの内部状態に対するValidationは、
 *   それをConsumeするOperationの責務とする。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、既存Resourceへのborrowを返すため、
 *   追加のValidationは行わない。
 */
const texture_cpu_resource_t* texture_registry_cpu_resource_get(const texture_registry_t* registry_, uint16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_id_is_in_range(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided texture_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == registry_->entries[texture_id_].cpu_resource) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided texture_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].cpu_resource;
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
resource_registry_result_t texture_registry_id_get(const texture_registry_t* registry_, const char* name_, uint16_t* out_texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "out_texture_id_")
    if('\0' == name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_id_get(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_id_get(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_id_get(%s) - find_by_name failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_texture_id_ = (uint16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - Registry、Resource Name、CPU / GPU Resourceの所有Pointer、
 *   Texture IDの出力Pointerについて、Operationに必要なNULL条件を
 *   全Buildで確認する。
 * - Resource Nameが空文字列でないことを全Buildで確認する。
 * - CPU / GPU Resourceの内部状態はConsumeせず、所有権のみを移転するため、
 *   入力ResourceのCanonical Validationは行わない。
 * - find_by_name()で全EntryのResource Nameを走査するため、
 *   DEBUG / TESTではRegistryのCanonical Validityを確認する。
 * - Resource Name専用Validatorでも必要な検証は可能だが、
 *   既存Canonical Validatorとの重複や保守負担を避け、
 *   Validation Architectureの単純さを優先する。
 *
 * Preflight:
 * - Resource Nameの重複を確認し、既存登録との衝突を防ぐ。
 * - 3フィールドすべてがNULLのEntryを登録先として選び、
 *   既存Resourceの上書きを防ぐ。
 * - Resource Nameの複製をCommit前に完了し、
 *   回復可能な失敗でRegistryやCallerの所有状態を変更しない。
 *
 * Postconditions:
 * - DEBUG / TESTではCommit後のRegistry全体をCanonical Validationし、
 *   新たに登録したCPU / GPU Resourceを含むStable Stateを確認する。
 * - 検証失敗はDATA_CORRUPTEDとして扱い、通常のCleanupを行わない。
 */
resource_registry_result_t texture_registry_register(texture_registry_t* registry_, const char* resource_name_, texture_gpu_resource_t** gpu_resource_, texture_cpu_resource_t** cpu_resource_, uint16_t* out_texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(gpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(*gpu_resource_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "texture_registry_register", "*gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(cpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "cpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(*cpu_resource_, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "texture_registry_register", "*cpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "out_texture_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_register(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_register(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_register(%s) - provided resource name is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(NULL == registry_->entries[i].cpu_resource && NULL == registry_->entries[i].gpu_resource && NULL == registry_->entries[i].resource_name) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("texture_registry_register(%s) - free slot not found.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_choco_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_registry_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("texture_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    // entry登録, 所有権移動commit
    registry_->entries[tmp_index].cpu_resource = *cpu_resource_;
    registry_->entries[tmp_index].gpu_resource = *gpu_resource_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_register(%s) - Postcondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *cpu_resource_ = NULL;
    *gpu_resource_ = NULL;
    tmp_name = NULL;

    *out_texture_id_ = (uint16_t)tmp_index;

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
 * - RegistryのNULLとTexture IDの範囲を全Buildで確認する。
 * - 指定EntryのOwned Resourceを破棄するため、
 *   DEBUG / TESTでは対象EntryのCanonical Validityを確認する。
 * - 他のEntryの内部状態はConsumeしないため、
 *   Registry全体のCanonical Validationは行わない。
 * - Registry本体とEntry配列の構造的Validityは、
 *   成立済みInternal Contractとして信頼する。
 * - 3フィールドすべてが非NULLであることを全Buildで確認し、
 *   登録解除Operationが実行可能かを判定する。
 *   このNULLチェックはEntryのCanonical Validationを目的としない。
 *
 * Postconditions:
 * - 対象EntryはEmpty Stateへ遷移するが、破棄済みResourceの
 *   再検証は行わない。
 * - 登録解除は検証済みの対象Entryを破棄するOperationであり、
 *   追加のCanonical Validationは行わない。
 */
resource_registry_result_t texture_registry_unregister(texture_registry_t* registry_, uint16_t texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_unregister", "registry_")
    if(!texture_id_is_in_range(registry_, texture_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_unregister(%s) - Provided texture_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!registry_entry_is_valid(&registry_->entries[texture_id_])) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_unregister(%s) - Entry validation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL == registry_->entries[texture_id_].cpu_resource || NULL == registry_->entries[texture_id_].gpu_resource || NULL == registry_->entries[texture_id_].resource_name) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_unregister(%s) - provided texture_id_ entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    registry_entry_deinitialize(&registry_->entries[texture_id_]);

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - このAPI自身がRegistryのCanonical Validatorであるため、
 *   事前に別のCanonical Validationは実行しない。
 * - RegistryのNULL、最大登録数の許容範囲、Entry配列のNULLを
 *   確認した後、各EntryのCanonical Validationを実行する。
 * - Registry本体とEntry配列のMemory Readabilityおよび
 *   LifetimeはCallerのContractとして扱う。
 *
 * Result validation / Postconditions:
 * - Registryの状態を変更せず、Canonical Validityの判定結果を
 *   返すOperationであるため、追加のValidationは行わない。
 */
bool texture_registry_is_valid(const texture_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_texture_count || UINT16_MAX < registry_->max_texture_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(!registry_entry_is_valid(&registry_->entries[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================
// Lifecycle
// ============================================================
static void registry_entry_deinitialize(registry_entry_t* entry_) {
    if(NULL == entry_) {
        return;
    }
    choco_string_destroy(&entry_->resource_name);
    texture_cpu_resource_destroy(&entry_->cpu_resource);
    texture_gpu_resource_destroy(&entry_->gpu_resource);
}

// ============================================================
// State Queries
// ============================================================
static bool texture_id_is_in_range(const texture_registry_t* registry_, uint16_t texture_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_texture_count <= (size_t)texture_id_) {
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
 * - Empty Stateでは、resource_name、cpu_resource、gpu_resourceが
 *   すべてNULLであること。
 * - Registered Stateでは、3フィールドすべてが非NULLであること。
 * - Registered Stateの各Owned Pointerが、General Allocatorの
 *   有効なAllocationを参照していること。
 * - Resource NameがCanonical Validityを満たし、
 *   空文字列ではないこと。
 * - CPU / GPU ResourceがそれぞれのCanonical Validityを満たすこと。
 *
 * Validation Limitations:
 * - 他のEntryとのResource Nameの一意性は検証しない。
 * - Registry本体およびEntry配列の構造的Validityは検証しない。
 */
static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // 初期化直後の未使用entryは正常
    if(NULL == entry_->cpu_resource && NULL == entry_->gpu_resource && NULL == entry_->resource_name) {
        return true;
    }

    // 中途半端な初期化状態は異常(全NULL判定済みなので、いずれかがNULLの場合は部分初期化状態)
    if(NULL == entry_->cpu_resource || NULL == entry_->gpu_resource || NULL == entry_->resource_name) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(entry_->resource_name)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(entry_->cpu_resource)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(entry_->gpu_resource)) {
        return false;
    }

    // 登録済みentryの検査
    if(!choco_string_is_valid(entry_->resource_name)) {
        return false;
    }
    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }
    if(!texture_cpu_resource_is_valid(entry_->cpu_resource)) {
        return false;
    }
    if(!texture_gpu_resource_is_valid(entry_->gpu_resource)) {
        return false;
    }
    return true;
}

// ============================================================
// Lookup
// ============================================================
static bool find_by_name(const texture_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
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
