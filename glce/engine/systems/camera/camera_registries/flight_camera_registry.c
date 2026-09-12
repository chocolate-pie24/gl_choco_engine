// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/camera/camera_registries/flight_camera_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/camera/flight_camera.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"
#include "engine/systems/camera/camera_registries/core/camera_registry_err_utils.h"

typedef struct registry_entry {
    choco_string_t* resource_name;
    flight_camera_t* flight_camera;
} registry_entry_t;

struct flight_camera_registry {
    size_t max_flight_camera_count;
    registry_entry_t* entries;
};

static camera_registry_result_t registry_entry_deinitialize(registry_entry_t* registry_entry_);
static bool registry_entry_is_empty(const registry_entry_t* registry_entry_);

static bool registry_entry_is_valid(const registry_entry_t* entry_);
static bool flight_camera_id_is_valid(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_);
static bool find_by_name(const flight_camera_registry_t* registry_, const char* name_, size_t* out_index_);

static bool is_valid_shallow(const flight_camera_registry_t* registry_);

camera_registry_result_t flight_camera_registry_initialize(size_t max_flight_camera_count_, linear_alloc_t* allocator_, flight_camera_registry_t** out_registry_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    flight_camera_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entry_array = NULL;

    size_t entry_array_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_initialize", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, CAMERA_REGISTRY_BAD_OPERATION, camera_registry_rslt_to_str(CAMERA_REGISTRY_BAD_OPERATION), "flight_camera_registry_initialize", "*out_registry_")
    if(0 == max_flight_camera_count_ || UINT16_MAX < max_flight_camera_count_) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_initialize(%s) - Provided max_flight_camera_count_ is not valid.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // flight_camera_registry_tメモリ確保
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(flight_camera_registry_t), alignof(flight_camera_registry_t), (void**)&tmp_registry);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = camera_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("flight_camera_registry_initialize(%s) - Failed to allocate registry instance. target=flight_camera_registry_t, bytes=%zu, align=%zu, max_flight_camera_count=%zu", camera_registry_rslt_to_str(ret), sizeof(flight_camera_registry_t), alignof(flight_camera_registry_t), max_flight_camera_count_);
        goto cleanup;
    }
    memset(tmp_registry, 0, sizeof(flight_camera_registry_t));

    if((SIZE_MAX / max_flight_camera_count_) < sizeof(registry_entry_t)) {
        ret = CAMERA_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("flight_camera_registry_initialize(%s) - overflow.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
    entry_array_size = sizeof(registry_entry_t) * max_flight_camera_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, entry_array_size, alignof(registry_entry_t), (void**)&tmp_entry_array);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = camera_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("flight_camera_registry_initialize(%s) - allocation failed.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_entry_array, 0, entry_array_size);

    tmp_registry->max_flight_camera_count = max_flight_camera_count_;
    tmp_registry->entries = tmp_entry_array;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(tmp_registry)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_initialize(%s) - tmp_registry is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_registry_ = tmp_registry;
    tmp_registry = NULL;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

// NOTE: このAPIを呼んだ後はmax_flight_camera_countが0になるためregistryは再利用不可となる。再利用を前提で初期化する場合はregistry_reset APIを追加する
void flight_camera_registry_deinitialize(flight_camera_registry_t* registry_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("flight_camera_registry_deinitialize(%s) - provided registry_ is NULL.", camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ERROR_MESSAGE("flight_camera_registry_deinitialize(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_rslt_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
        return;
    }
#endif
    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(!registry_entry_is_empty(&registry_->entries[i])) {
            if(CAMERA_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[i])) {
                ERROR_MESSAGE("flight_camera_registry_deinitialize(%s) - registry_entry_deinitialize failed.", camera_registry_rslt_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
                return;
            }
        }
    }
    registry_->max_flight_camera_count = 0;
}

bool flight_camera_registry_find(const flight_camera_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(registry_)) {
        ERROR_MESSAGE("flight_camera_registry_find(%s) - flight_camera_registry_t internal state is corrupted.", camera_registry_rslt_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
        return false;
    }
#endif
    if('\0' == name_[0]) {
        ERROR_MESSAGE("flight_camera_registry_find(%s) - provided resource name is not valid.", camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return false;
    }

    return find_by_name(registry_, name_, &tmp_id);
}

flight_camera_t* flight_camera_registry_flight_camera_get(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided registry_ is not valid.", camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(registry_)) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided registry_ is corrupted.", camera_registry_rslt_to_str(CAMERA_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
#endif
    if(!flight_camera_id_is_valid(registry_, flight_camera_id_)) {
        ERROR_MESSAGE("flight_camera_registry_flight_camera_get(%s) - provided flight_camera_id_ is not valid.", camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }

    return registry_->entries[flight_camera_id_].flight_camera;
}

camera_registry_result_t flight_camera_registry_id_get(const flight_camera_registry_t* registry_, const char* name_, uint16_t* out_flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_id_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_id_get", "out_flight_camera_id_")
    if('\0' == name_[0]) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - provided resource name is not valid.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - provided registry_ is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_id_get(%s) - find_by_name failed.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    *out_flight_camera_id_ = (uint16_t)tmp_id;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

camera_registry_result_t flight_camera_registry_register(flight_camera_registry_t* registry_, const char* resource_name_, flight_camera_t** flight_camera_, uint16_t* out_flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(*flight_camera_, ret, CAMERA_REGISTRY_BAD_OPERATION, camera_registry_rslt_to_str(CAMERA_REGISTRY_BAD_OPERATION), "flight_camera_registry_register", "*flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_id_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_register", "out_flight_camera_id_")
    if('\0' == resource_name_[0]) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided resource name is not valid.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_is_valid(*flight_camera_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided *flight_camera_ is not valid.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_valid_shallow(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided registry_ is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // リソースの重複チェック
    if(find_by_name(registry_, resource_name_, &tmp_index)) {
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - provided resource name is already registered.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // 空きスロット検索
    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(registry_entry_is_empty(&registry_->entries[i])) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = CAMERA_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - free slot not found.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = camera_registry_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("flight_camera_registry_register(%s) - choco_string_create_from_c_string failed.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    registry_->entries[tmp_index].flight_camera = *flight_camera_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    *flight_camera_ = NULL;
    tmp_name = NULL;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_register(%s) - registry_ is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_flight_camera_id_ = (uint16_t)tmp_index;

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    if(CAMERA_REGISTRY_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
    }
    return ret;
}

camera_registry_result_t flight_camera_registry_unregister(flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, CAMERA_REGISTRY_INVALID_ARGUMENT, camera_registry_rslt_to_str(CAMERA_REGISTRY_INVALID_ARGUMENT), "flight_camera_registry_unregister", "registry_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - provided registry_ is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(!flight_camera_id_is_valid(registry_, flight_camera_id_)) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - provided flight_camera_id_ is not valid.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == registry_->entries[flight_camera_id_].resource_name) {
        ret = CAMERA_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - provided flight_camera id entry is empty.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

    if(CAMERA_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[flight_camera_id_])) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - registry_entry_deinitialize failed.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!flight_camera_registry_is_valid(registry_)) {
        ret = CAMERA_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("flight_camera_registry_unregister(%s) - registry_ is corrupted.", camera_registry_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

bool flight_camera_registry_is_valid(const flight_camera_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(!is_valid_shallow(registry_)) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(!registry_entry_is_empty(&registry_->entries[i]) && !registry_entry_is_valid(&registry_->entries[i])) {
            return false;
        }
    }
    for(size_t i = 0; i != (registry_->max_flight_camera_count - 1); ++i) {
        for(size_t j = (i + 1); j != registry_->max_flight_camera_count; ++j) {
            if(!registry_entry_is_empty(&registry_->entries[i]) && !registry_entry_is_empty(&registry_->entries[j])) {
                if(choco_string_equal(choco_string_c_str(registry_->entries[i].resource_name), choco_string_c_str(registry_->entries[j].resource_name))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static camera_registry_result_t registry_entry_deinitialize(registry_entry_t* registry_entry_) {
    camera_registry_result_t ret = CAMERA_REGISTRY_INVALID_ARGUMENT;

    if(NULL == registry_entry_) {
        ret = CAMERA_REGISTRY_INVALID_ARGUMENT;
        goto cleanup;
    }

    flight_camera_destroy(&registry_entry_->flight_camera);
    choco_string_destroy(&registry_entry_->resource_name);

    ret = CAMERA_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

static bool registry_entry_is_empty(const registry_entry_t* registry_entry_) {
    if(NULL == registry_entry_) {
        return false;
    }
    if(NULL != registry_entry_->flight_camera || NULL != registry_entry_->resource_name) {
        return false;
    }
    return true;
}

static bool registry_entry_is_valid(const registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // 初期化直後の未使用entryは正常
    if(registry_entry_is_empty(entry_)) {
        return true;
    }
    if(NULL == entry_->flight_camera || NULL == entry_->resource_name) {
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

static bool flight_camera_id_is_valid(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(registry_->max_flight_camera_count <= (size_t)flight_camera_id_) {
        return false;
    }
    return true;
}

static bool find_by_name(const flight_camera_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_flight_camera_count; ++i) {
        if(NULL != registry_->entries[i].resource_name && choco_string_equal(choco_string_c_str(registry_->entries[i].resource_name), name_)) {
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

static bool is_valid_shallow(const flight_camera_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_flight_camera_count || UINT16_MAX < registry_->max_flight_camera_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    return true;
}
