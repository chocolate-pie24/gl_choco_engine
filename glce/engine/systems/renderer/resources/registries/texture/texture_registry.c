// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/registries/texture/texture_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/textures/texture_cpu_resource.h"
#include "engine/systems/renderer/resources/textures/texture_gpu_resource.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_err_utils.h"

/*
 * Module Internal Contract
 *
 * Resource Identity:
 * - Texture IDはEntry配列のindexに対応する。
 * - IDとEntryの対応を管理する独立したmappingは持たない。
 * - 解放済みEntryのindexは、後の新規登録で再利用できる。
 *
 * Entry State:
 * - Entryの正常なstable stateはEmpty StateとRegistered Stateである。
 * - Empty StateではResource Name、CPU Texture、GPU Textureが
 *   すべてNULLであり、Reference Countは0となる。
 * - Registered Stateでは3つの所有ポインタがすべて非NULLであり、
 *   Reference Countは1以上となる。
 * - 3つのResourceは一組として登録・解放する。
 * - Entryは正常なstate transitionの途中を除き、
 *   Empty StateまたはRegistered Stateのいずれかを満たす。
 * - 両Stateの構造的条件は各State Predicateが判定し、
 *   canonical validityはregistry_entry_is_valid()が判定する。
 *
 * Reference Count:
 * - 新規登録によってEmpty StateからRegistered Stateへ遷移し、
 *   Reference Countを1に初期化する。
 * - Acquireでは登録済みEntryのReference Countを1増加させる。
 * - Release後もReference Countが残る場合、Resourceを維持する。
 * - 最後の利用権の返却では所有Resourceを破棄し、
 *   EntryをEmpty Stateへ戻す。
 * - Registry全体のdeinitializeではReference Countを消費せず、
 *   全Entryに対して終了処理を実行する。
 *
 * Resource Ownership:
 * - RegistryはResource Name、CPU Texture、GPU Textureを所有する。
 * - CPU TextureとGPU Textureは個別のReference Countを持たない。
 * - 各Resourceの破棄は対応するdestroy APIへ委譲する。
 * - GPU Textureが保持するBackend Contextはborrowであり、
 *   RegistryはBackend Context自体を所有しない。
 * - 登録済みGPU Textureの破棄が完了するまで、
 *   Backend Contextが利用可能であることをcallerが保証する。
 *
 * Destruction:
 * - Resource Name、CPU Texture、GPU Textureの順に破棄する。
 * - 各destroy APIはvoidであり、破棄失敗をResultとして返さない。
 * - 正常な内部状態では、破棄後のEntryはEmpty Stateとなる。
 * - 下位destroy APIが破棄を完了できなかった場合、
 *   本Moduleはその失敗をResultとして検出・伝播できない。
 *
 * Initialization:
 * - Subsystem Allocatorのzero-initialized storageを使用し、
 *   生成直後の全EntryをEmpty Stateとする。
 * - Registryの利用期間中、Entry配列の容量は変更しない。
 * - deinitializeが正常に完了するとmax_texture_countが0となり、
 *   Registryのlogical lifetimeが終了する。
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
 * Texture IDに対応する一つの登録Entryを表す。
 *
 * - resource_nameはRegistryが所有するResource Nameを指す。
 * - cpu_resourceはRegistryが所有するCPU Textureを指す。
 * - gpu_resourceはRegistryが所有するGPU Textureを指す。
 * - 3つのResourceは同一Textureに属する一組として管理する。
 * - reference_countは、このResourceの組に対して
 *   保持されている利用権の数を表す。
 * - Empty Stateではすべての所有ポインタがNULLとなり、
 *   reference_countは0となる。
 * - Registered Stateではすべての所有ポインタが非NULLとなり、
 *   reference_countは1以上となる。
 * - 各Stateの構造的条件およびcanonical validityは、
 *   対応するState PredicateとValidatorが定義する。
 */
typedef struct registry_entry {
    choco_string_t* resource_name;
    texture_gpu_resource_t* gpu_resource;
    texture_cpu_resource_t* cpu_resource;
    uint16_t reference_count;
} registry_entry_t;

/*
 * Texture Resourceの登録状態を管理するRegistry本体。
 *
 * - max_texture_countは登録数ではなくEntry配列の容量を表す。
 * - entriesはTexture IDによって直接参照するEntry配列である。
 * - 正常な利用可能状態では、max_texture_countは
 *   1以上UINT16_MAX以下、entriesはNULL以外となる。
 * - Registryの利用期間中、Entry配列のstorageは変更しない。
 * - deinitializeが正常に完了するとmax_texture_countが0となり、
 *   Registryのlogical lifetimeが終了する。
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
static bool registry_entry_is_empty(const registry_entry_t* entry_);
static bool registry_entry_is_registered(const registry_entry_t* entry_);

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
 * - allocator_とout_registry_のNULL、および*out_registry_の
 *   NULL初期化状態を全Build Modeで検査する。
 * - 最大登録数が1以上UINT16_MAX以下であることを確認する。
 * - size_mul_checked()によってEntry配列のサイズ計算に
 *   overflowが発生しないことを確認する。
 * - Subsystem Allocatorの内部整合性とallocationの成立条件は、
 *   下位Allocator APIのcontractとvalidationに委ねる。
 *
 * Construction:
 * - Allocation開始前にrollback pointを取得する。
 * - Registry本体とEntry配列をSubsystem Allocatorから確保する。
 * - zero-initialized storageによって全EntryがEmpty Stateとなる。
 *
 * Result Validation:
 * - DEBUG / TESTでは、生成したRegistry全体を
 *   canonical validationしてからcallerへ公開する。
 * - RELEASEでは、構築処理とSubsystem Allocatorの
 *   zero-initialization contractを信頼する。
 *
 * Failure:
 * - 回復可能な構築失敗ではAllocatorをrollbackし、
 *   構築前のallocation stateを復元する。
 * - rollback自体が失敗した場合はDATA_CORRUPTEDとする。
 * - DATA_CORRUPTEDの場合は通常のrollbackを行わない。
 * - 回復可能な失敗ではout_registry_を変更しない。
 *
 * Postconditions:
 * - Result validation済みのRegistryを出力するだけであり、
 *   出力後に追加のcanonical validationは実行しない。
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
    if(!size_mul_checked(max_texture_count_, sizeof(registry_entry_t), &array_size)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("texture_registry_create(%s) - array size overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

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
 * - registry_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、全EntryのOwned Resourceを破棄する前に
 *   Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼し、
 *   canonical validationを省略する。
 * - GPU Textureが借用するBackend Contextのlifetimeは、
 *   GPU Texture Resource側のcontractとcallerの責務とする。
 *
 * Deinitialization:
 * - Empty Stateを含む全Entryに対して
 *   registry_entry_deinitialize()を実行する。
 * - Reference Countの値によって終了処理を切り替えない。
 * - 各EntryのResource Name、CPU Texture、GPU Textureを破棄する。
 * - 全Entryの終了処理後、max_texture_countを0に設定する。
 *
 * Failure:
 * - 引数不正またはDEBUG / TESTでcanonical validationに
 *   失敗した場合、終了処理を開始しない。
 * - 各Resourceのdestroy APIはvoidであり、
 *   個別の破棄結果を取得・伝播しない。
 * - 下位destroy APIが内部で破棄を中断した場合、
 *   Registryは解放完了を確認できない。
 *
 * Postconditions:
 * - 正常終了後のRegistryは利用可能なstable object modelから
 *   外れるため、canonical validationを実行しない。
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
 * - registry_、resource_name_、out_exists_のNULLと、
 *   Resource Nameの空文字列を全Build Modeで検査する。
 * - Resource NameでEntry配列を検索するため、
 *   DEBUG / TESTではRegistry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 * - 登録名だけに限定したValidationも可能だが、
 *   既存Validatorとの重複実装を避け、
 *   Registry全体のcanonical validationを使用する。
 *
 * Lookup:
 * - Resource Nameの検索はfind_by_name()に委譲する。
 * - 未登録は正常な検索結果として扱う。
 * - 検索によってReference Countを変更しない。
 *
 * Result:
 * - 検索完了後にout_exists_へ登録状態を出力する。
 * - 失敗時はout_exists_を変更しない。
 * - Registryの状態を変更しないため、
 *   Result validationとPostconditions validationは行わない。
 */
resource_registry_result_t texture_registry_exists(const texture_registry_t* registry_, const char* resource_name_, bool* out_exists_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;
    bool exists = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_exists", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_exists", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_exists_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_exists", "out_exists_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_exists(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
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
 * - registry_のNULL、Texture IDの範囲、
 *   および対象EntryのRegistered Stateを全Build Modeで検査する。
 * - Texture IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - GPU Textureの内部状態をconsumeせず、
 *   既存ポインタをborrowとして返すだけなので、
 *   GPU Textureのcanonical validationも実行しない。
 *
 * Result:
 * - 戻り値はRegistryが所有するGPU Textureへのconst borrowである。
 * - Reference Countを変更せず、利用権を新たに取得しない。
 * - Registryの状態を変更しないため、
 *   Result validationとPostconditions validationは行わない。
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
    if(!registry_entry_is_registered(&registry_->entries[texture_id_])) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided texture_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].gpu_resource;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULL、Texture IDの範囲、
 *   および対象EntryのRegistered Stateを全Build Modeで検査する。
 * - Texture IDによって対象Entryへ直接アクセスし、
 *   Entry配列全体を走査しないため、
 *   Registry全体のcanonical validationは実行しない。
 * - CPU Textureの内部状態をconsumeせず、
 *   既存ポインタをborrowとして返すだけなので、
 *   CPU Textureのcanonical validationも実行しない。
 *
 * Result:
 * - 戻り値はRegistryが所有するCPU Textureへのconst borrowである。
 * - Reference Countを変更せず、利用権を新たに取得しない。
 * - Registryの状態を変更しないため、
 *   Result validationとPostconditions validationは行わない。
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
    if(!registry_entry_is_registered(&registry_->entries[texture_id_])) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided texture_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].cpu_resource;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_、resource_name_、out_texture_id_のNULLと、
 *   Resource Nameの空文字列を全Build Modeで検査する。
 * - Resource NameによってEntry配列を検索するため、
 *   DEBUG / TESTではRegistry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 *
 * Preflight:
 * - find_by_name()によって対象Resourceの登録状態を確認する。
 * - 未登録の場合はBAD_OPERATIONとして扱う。
 * - 対象EntryのindexがUINT16_MAX以下であることを確認する。
 * - Reference CountがUINT16_MAXに達している場合は
 *   LIMIT_EXCEEDEDとして扱う。
 * - これらの条件が成立するまでReference Countを変更しない。
 *
 * Commit:
 * - 対象EntryのReference Countを1増加させる。
 * - Resource Name、CPU Texture、GPU Texture、
 *   およびTexture IDとの対応は変更しない。
 *
 * Postconditions:
 * - Commitは検証済みReference Countに対する単一の加算であり、
 *   Registered Stateの構造を維持する。
 * - Commit後のcanonical validationは実行しない。
 * - Reference Countの加算後にTexture IDを出力する。
 *
 * Failure:
 * - Commit前の失敗ではRegistryと出力先を変更しない。
 * - DATA_CORRUPTEDの場合、Registryの内部整合性は保証しない。
 */
resource_registry_result_t texture_registry_acquire(texture_registry_t* registry_, const char* resource_name_, uint16_t* out_texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_index = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_acquire", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_acquire", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_acquire", "out_texture_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_acquire(%s) - provided resource name is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_acquire(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの登録状態確認
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_acquire(%s) - Provided resource name is not registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < tmp_index) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_acquire(%s) - Resource registry is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    if(registry_->entries[tmp_index].reference_count == UINT16_MAX) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("texture_registry_acquire(%s) - Reference count overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    registry_->entries[tmp_index].reference_count++;
    *out_texture_id_ = (uint16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - Registry、Resource Name、CPU / GPU Textureの所有Pointer、
 *   Texture IDの出力Pointerについて、NULL条件を検査する。
 * - CPU / GPU Textureの所有PointerがNULLの場合は
 *   BAD_OPERATIONとして扱う。
 * - Resource Nameの空文字列を全Build Modeで検査する。
 * - callerから受け取るCPU / GPU Textureについて、
 *   全Build Modeでcanonical validationする。
 * - invalidなCPU / GPU TextureはINVALID_ARGUMENTとして扱う。
 * - Resource Nameの重複検索と空きEntryの探索に先立ち、
 *   DEBUG / TESTではRegistry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 *
 * Preflight:
 * - Resource Nameが未登録であることを確認する。
 * - registry_entry_is_empty()によって空きEntryを探索する。
 * - 空きEntryが存在しない場合はLIMIT_EXCEEDEDとして扱う。
 * - Resource Nameの複製をCommit前に完了する。
 * - これらの処理が成功するまでRegistryとcallerの所有状態を
 *   変更しない。
 *
 * Commit:
 * - CPU Texture、GPU Texture、Resource Nameの
 *   所有ポインタを対象Entryに格納する。
 * - Reference Countを1に初期化する。
 * - Empty StateからRegistered Stateへの遷移を成立させる。
 *
 * Postconditions:
 * - DEBUG / TESTでは、登録後にRegistry全体をcanonical validationし、
 *   Commit後のstable stateが成立していることを確認する。
 * - RELEASEでは、PreconditionsとCommitのcontractを信頼し、
 *   canonical validationを省略する。
 * - caller側の所有ポインタのNULL初期化およびTexture IDの出力は、
 *   Postconditions validationの成功後に行う。
 *
 * Failure:
 * - Commit前の回復可能な失敗では、一時Resource Nameを破棄し、
 *   Registryとcaller側の所有状態を変更しない。
 * - DATA_CORRUPTED発生時は、通常のcleanupによる
 *   一時Resource Nameの破棄を行わない。
 * - Commit後のPostconditions validationで失敗した場合、
 *   Ownership Transferの完了状態およびRegistryの状態復元を
 *   保証しない。
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
    if(!texture_cpu_resource_is_valid(*cpu_resource_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_register(%s) - Provided *cpu_resource_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!texture_gpu_resource_is_valid(*gpu_resource_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_register(%s) - Provided *gpu_resource_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(find_by_name(registry_, resource_name_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_register(%s) - provided resource name is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(registry_entry_is_empty(&registry_->entries[i])) {
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
    registry_->entries[tmp_index].reference_count = 1;

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
 * - registry_のNULLを全Build Modeで検査する。
 * - DEBUG / TESTでは、Registry全体をcanonical validationする。
 * - RELEASEでは、成立済みのRegistry internal contractを信頼する。
 * - Texture IDの範囲と対象EntryのRegistered Stateを
 *   全Build Modeで直接検査する。
 * - 未登録EntryへのReleaseはBAD_OPERATIONとして扱う。
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
 * - Reference Countが2以上の場合はReference Countを
 *   1減少させ、Resourceの登録状態を維持する。
 * - Reference Countが1の場合はregistry_entry_deinitialize()へ
 *   委譲し、Resource Name、CPU Texture、GPU Textureを破棄する。
 * - 最後の利用権が返却された場合、正常な破棄処理によって
 *   対象EntryがEmpty Stateとなる。
 *
 * Postconditions:
 * - Reference Countの減算のみの場合、Registered Stateを維持する。
 * - 最後の利用権の返却では、下位destroy APIのcontractを信頼する。
 * - 破棄済みResourceの再検証や、Commit後の
 *   canonical validationは実行しない。
 *
 * Failure:
 * - Commit前の失敗ではRegistryを変更しない。
 * - 各Resourceのdestroy APIはvoidであるため、
 *   内部の破棄失敗をResultとして検出・伝播しない。
 * - 下位destroy APIが破棄を完了できなかった場合、
 *   EntryがEmpty Stateになったことは保証できない。
 */
resource_registry_result_t texture_registry_release(texture_registry_t* registry_, uint16_t texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_release", "registry_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_release(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!texture_id_is_in_range(registry_, texture_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_release(%s) - Provided texture_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!registry_entry_is_registered(&registry_->entries[texture_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_release(%s) - provided texture id entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(1 == registry_->entries[texture_id_].reference_count) {
        registry_entry_deinitialize(&registry_->entries[texture_id_]);
    } else {
        registry_->entries[texture_id_].reference_count--;
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
 * - RegistryのNULL、最大登録数の範囲、Entry配列のNULLを
 *   先に検査し、明らかに不正な構造を走査しない。
 * - 最大登録数を走査上限として全Entryを検証する。
 * - 各Entryのcanonical validityはregistry_entry_is_valid()へ
 *   委譲する。
 * - 各EntryはReference Countを含めて、
 *   Empty StateまたはRegistered Stateを満たす必要がある。
 * - Registered StateではResource Name、CPU Texture、
 *   GPU Textureのallocation-level validityを確認する。
 * - それぞれのOwned Resourceのcanonical validityは
 *   対応するResource ModuleのValidatorへ委譲する。
 *
 * Validation Limitations:
 * - Resource NameのEntry間一意性は検証しない。
 * - CPU / GPU Texture間の画像情報の一致は検証しない。
 * - Registry本体とEntry配列のallocation-level validityは
 *   検証しない。
 * - Allocator全体の内部状態は検証しない。
 *
 * - Registry本体とEntry配列のmemory readabilityおよび
 *   lifetimeはcallerのcontractとして扱う。
 * - Registryの状態を変更せず、追加のvalidationも実行しない。
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
/*
 * Private Helper Contract
 *
 * - Entryが所有するResource Name、CPU Texture、GPU Textureを
 *   この順に破棄する。
 * - 各Resourceの破棄は対応するdestroy APIへ委譲する。
 * - 破棄処理後、Reference Countを0に設定する。
 * - Empty StateのEntryにも適用できる。
 * - 正常に全Resourceが破棄された場合、EntryはEmpty Stateとなる。
 *
 * Preconditions:
 * - entry_のNULLを検査する。
 * - NULLの場合は処理せずreturnする。
 * - NULL以外のEntryについては、所有ポインタがそれぞれ
 *   Empty / Registered Stateのcontractを満たすことを
 *   callerが保証する。
 * - 本HelperはEntryのcanonical validationを実行しない。
 *
 * Failure:
 * - 各destroy APIはvoidであり、破棄結果を受け取らない。
 * - 下位destroy APIが破棄を中断した場合でも、
 *   Reference Countの0への設定は実行する。
 * - その場合、EntryのEmpty State成立は保証されない。
 * - 本Helperはrollbackを行わない。
 *
 * Postconditions:
 * - 正常な内部状態で実行された場合、
 *   すべての所有Resourceが破棄され、EntryはEmpty Stateとなる。
 * - 終了後のcanonical validationは実行しない。
 */
static void registry_entry_deinitialize(registry_entry_t* entry_) {
    if(NULL == entry_) {
        return;
    }
    choco_string_destroy(&entry_->resource_name);
    texture_cpu_resource_destroy(&entry_->cpu_resource);
    texture_gpu_resource_destroy(&entry_->gpu_resource);
    entry_->reference_count = 0;
}

// ============================================================
// State Queries
// ============================================================
/*
 * Texture IDがRegistryのEntry配列の範囲内にあることを判定する。
 *
 * - registry_がNULLの場合はfalseを返す。
 * - texture_id_がmax_texture_count未満であることを確認する。
 * - 対象Entryの登録状態やcanonical validityは検査しない。
 * - Texture ID = 0も有効なindexとして扱う。
 * - RegistryおよびEntryの状態を変更しない。
 */
static bool texture_id_is_in_range(const texture_registry_t* registry_, uint16_t texture_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_texture_count <= (size_t)texture_id_) {
        return false;
    }
    return true;
}

/*
 * EntryがEmpty Stateの構造的条件を満たすことを判定する。
 *
 * Empty State:
 * - Resource Name、CPU Texture、GPU Textureの
 *   すべての所有ポインタがNULLである。
 * - Reference Countが0である。
 *
 * - NULL EntryはEmpty Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Owned Resourceのcanonical validityは検査しない。
 * - 本Predicateは新規登録可能なEntryの探索にも使用する。
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
    if(NULL != entry_->cpu_resource) {
        return false;
    }
    if(NULL != entry_->gpu_resource) {
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
 * - Resource Name、CPU Texture、GPU Textureの
 *   すべての所有ポインタがNULLではない。
 * - Reference Countが1以上である。
 *
 * - NULL EntryはRegistered Stateとして扱わない。
 * - いずれかの条件が成立しない場合はfalseを返す。
 * - Owned Resourceのcanonical validityは検査しない。
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
    if(NULL == entry_->cpu_resource) {
        return false;
    }
    if(NULL == entry_->gpu_resource) {
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
 * - Registered Stateでは、Resource Name、CPU Texture、
 *   GPU TextureがGeneral Allocator管理下の
 *   current live allocationであることを確認する。
 * - Resource NameがvalidなChoco Stringであり、
 *   空文字列ではないことを確認する。
 * - CPU / GPU Textureのcanonical validityを検証する。
 *
 * Validation Responsibility:
 * - Resource Nameの内部整合性は
 *   Choco Stringのcanonical validatorに委譲する。
 * - CPU Textureの内部整合性は
 *   texture_cpu_resource_is_valid()に委譲する。
 * - GPU Textureの内部整合性は
 *   texture_gpu_resource_is_valid()に委譲する。
 * - Owned Pointerのallocation-level validityは、
 *   Child Validatorの呼び出しより前に確認する。
 *
 * Validation Limitations:
 * - 他EntryとのResource Nameの一意性は検証しない。
 * - CPU / GPU Texture間の画像情報の一致は検証しない。
 * - Registry本体とEntry配列の整合性は検証しない。
 *
 * - Reference CountとResourceの登録状態の矛盾を検出する。
 * - Entryの状態を変更しない。
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
/*
 * Resource Nameに対応するRegistered Entryを検索する。
 *
 * - Entry配列を先頭から走査する。
 * - Registered Stateの構造的条件を満たすEntryだけを検索する。
 * - Resource Nameの比較はChoco Stringへ委譲する。
 * - 発見した場合は対応するEntry indexをout_index_へ出力する。
 * - 未発見の場合はfalseを返し、out_index_を変更しない。
 * - NULL引数の場合もfalseを返す。
 *
 * Validation:
 * - 本HelperはRegistry全体やEntryのcanonical validationを行わない。
 * - Registryと登録済みResourceの内部整合性については、
 *   callerが成立済みのinternal contractを保証する。
 * - 登録状態の構造的判定以外に、Entryのcorruptionを
 *   診断することは本Helperの責務ではない。
 *
 * - Registryの登録状態およびReference Countを変更しない。
 */
static bool find_by_name(const texture_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
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
