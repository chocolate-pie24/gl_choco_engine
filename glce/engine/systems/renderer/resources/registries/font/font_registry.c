// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/registries/font/font_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/resources/textures/texture_gpu_resource.h"
#include "engine/systems/renderer/resources/fonts/font_gpu_resource.h"

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_err_utils.h"

// ============================================================
// Private Type Definitions
// ============================================================
typedef struct registry_entry {
    const font_atlas_asset_t* atlas_asset;
    texture_gpu_resource_t* gpu_resource;
    uint16_t reference_count;
} registry_entry_t;

struct font_registry {
    size_t max_font_count;
    registry_entry_t* entries;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Lifecycle
static void registry_entry_deinitialize(registry_entry_t* entry_);

// State Queries
static bool font_id_is_in_range(const font_registry_t* registry_, uint16_t font_id_);
static bool registry_entry_is_empty(const registry_entry_t* entry_);
static bool registry_entry_is_registered(const registry_entry_t* entry_);

// Validation
static bool registry_entry_is_valid(const registry_entry_t* entry_);

// Lookup
static bool find_by_atlas_asset(const font_registry_t* registry_, const font_atlas_asset_t* asset_, size_t* out_index_);

resource_registry_result_t font_registry_create(size_t max_font_count_, subsystem_allocator_t* allocator_, font_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    font_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entries = NULL;

    size_t array_size = 0;
    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_BAD_OPERATION);
    if(0 == max_font_count_ || UINT16_MAX < max_font_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("font_registry_create(%s) - Provided max_font_count_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }
    if(!size_mul_checked(max_font_count_, sizeof(registry_entry_t), &array_size)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("font_registry_create(%s) - array size overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("font_registry_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(font_registry_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_registry);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("font_registry_create(%s) - subsystem_allocator_allocate failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, array_size, SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_entries);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_registry_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("font_registry_create(%s) - allocation failed.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    tmp_registry->max_font_count = max_font_count_;
    tmp_registry->entries = tmp_entries;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(tmp_registry)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_create(%s) - Result validation failed.", resource_registry_result_to_str(ret));
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
            ERROR_MESSAGE("font_registry_create(%s) - subsystem_allocator rollback failed.", resource_registry_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

void font_registry_deinitialize(font_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("font_registry_deinitialize(%s) - provided registry_ is NULL.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ERROR_MESSAGE("font_registry_deinitialize(%s) - font_registry_t internal state is corrupted.", resource_registry_result_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif
    for(size_t i = 0; i != registry_->max_font_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i]);
    }
    registry_->max_font_count = 0;
}

resource_registry_result_t font_registry_exists(const font_registry_t* registry_, const font_atlas_asset_t* asset_, bool* out_exists_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;
    bool exists = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_exists_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_exists(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    exists = find_by_atlas_asset(registry_, asset_, &tmp_id);
    *out_exists_ = exists;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

const font_atlas_asset_t* font_registry_atlas_asset_get(const font_registry_t* registry_, uint16_t font_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("font_registry_atlas_asset_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!font_id_is_in_range(registry_, font_id_)) {
        ERROR_MESSAGE("font_registry_atlas_asset_get(%s) - provided font_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[font_id_])) {
        ERROR_MESSAGE("font_registry_atlas_asset_get(%s) - provided font_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[font_id_].atlas_asset;
}

const texture_gpu_resource_t* font_registry_gpu_resource_get(const font_registry_t* registry_, uint16_t font_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("font_registry_gpu_resource_get(%s) - provided registry_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!font_id_is_in_range(registry_, font_id_)) {
        ERROR_MESSAGE("font_registry_gpu_resource_get(%s) - provided font_id_ is not valid.", resource_registry_result_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!registry_entry_is_registered(&registry_->entries[font_id_])) {
        ERROR_MESSAGE("font_registry_gpu_resource_get(%s) - provided font_id_ is not registered.", resource_registry_result_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[font_id_].gpu_resource;
}

resource_registry_result_t font_registry_acquire(font_registry_t* registry_, const font_atlas_asset_t* asset_, uint16_t* out_font_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_index = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_font_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_acquire(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!find_by_atlas_asset(registry_, asset_, &tmp_index)) {   // リソースの登録状態確認
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("font_registry_acquire(%s) - Provided font atlas asset is not registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < tmp_index) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_acquire(%s) - Resource registry is corrupted.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    if(registry_->entries[tmp_index].reference_count == UINT16_MAX) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("font_registry_acquire(%s) - Reference count overflow.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    registry_->entries[tmp_index].reference_count++;
    *out_font_id_ = (uint16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

resource_registry_result_t font_registry_register(font_registry_t* registry_, const font_atlas_asset_t* asset_, texture_gpu_resource_t** gpu_resource_, uint16_t* out_font_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(gpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(*gpu_resource_, ret, RESOURCE_REGISTRY_BAD_OPERATION);
    IF_ARG_NULL_GOTO_CLEANUP(out_font_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_register(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!texture_gpu_resource_is_valid(*gpu_resource_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("font_registry_register(%s) - Provided *gpu_resource_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(find_by_atlas_asset(registry_, asset_, &tmp_index)) {   // リソースの重複チェック
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("font_registry_register(%s) - provided font atlas asset is already registered.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    for(size_t i = 0; i != registry_->max_font_count; ++i) {
        if(registry_entry_is_empty(&registry_->entries[i])) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("font_registry_register(%s) - free slot not found.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    // entry登録, 所有権移動commit
    registry_->entries[tmp_index].atlas_asset = asset_;
    registry_->entries[tmp_index].gpu_resource = *gpu_resource_;
    registry_->entries[tmp_index].reference_count = 1;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_register(%s) - Postcondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *gpu_resource_ = NULL;
    *out_font_id_ = (uint16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

resource_registry_result_t font_registry_release(font_registry_t* registry_, uint16_t font_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!font_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("font_registry_release(%s) - Precondition validation failed for 'registry_'.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!font_id_is_in_range(registry_, font_id_)) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("font_registry_release(%s) - Provided font_id_ is not valid.", resource_registry_result_to_str(ret));
        goto cleanup;
    }
    if(!registry_entry_is_registered(&registry_->entries[font_id_])) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("font_registry_release(%s) - provided font id entry is empty.", resource_registry_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(1 == registry_->entries[font_id_].reference_count) {
        registry_entry_deinitialize(&registry_->entries[font_id_]);
    } else {
        registry_->entries[font_id_].reference_count--;
    }

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

bool font_registry_is_valid(const font_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_font_count || UINT16_MAX < registry_->max_font_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_font_count; ++i) {
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
    entry_->atlas_asset = NULL;
    font_gpu_resource_destroy(&entry_->gpu_resource);
    entry_->reference_count = 0;
}

// ============================================================
// State Queries
// ============================================================
static bool font_id_is_in_range(const font_registry_t* registry_, uint16_t font_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_font_count <= (size_t)font_id_) {
        return false;
    }
    return true;
}

static bool registry_entry_is_empty(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL != entry_->atlas_asset) {
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

static bool registry_entry_is_registered(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }
    if(NULL == entry_->atlas_asset) {
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

    // 登録済みentryの検査
    if(!texture_gpu_resource_is_valid(entry_->gpu_resource)) {
        return false;
    }
    return true;
}

static bool find_by_atlas_asset(const font_registry_t* registry_, const font_atlas_asset_t* asset_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == registry_ || NULL == asset_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_font_count; ++i) {
        if(registry_entry_is_registered(&registry_->entries[i])) {
            if(registry_->entries[i].atlas_asset == asset_) {
                tmp_slot = i;
                found = true;
                break;
            }
        }
    }
    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}
