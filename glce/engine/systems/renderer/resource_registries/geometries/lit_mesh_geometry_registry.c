// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file lit_mesh_geometry_registry.c
 * @author chocolate-pie24
 *
 * @brief 単色ライティング描画用ジオメトリレジストリAPIの実装
 *
 * @date 2026-06-20
 *
 */
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

typedef struct registry_entry {
    choco_string_t* resource_name;
    lit_mesh_geometry_t* cpu_resource;
    vbo_range_t allocation_descriptor;
} registry_entry_t;

/**
 * @brief 単色ライティング描画用ジオメトリレジストリ内部構造体
 *
 */
struct lit_mesh_geometry_registry {
    size_t max_geometry_count;          /**< レジストリに登録可能な最大ジオメトリ数(0は許可しない. 単色ライティング描画を使用しなくても1以上にする) */
    registry_entry_t* entries;
};

static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* registry_entry_, lit_mesh_shader_t* shader_);
static bool registry_entry_is_empty(const registry_entry_t* registry_entry_);

static bool registry_entry_is_valid(const registry_entry_t* entry_);
static bool geometry_id_is_valid(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_);
static bool find_by_name(const lit_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_);

resource_registry_result_t lit_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, lit_mesh_geometry_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    lit_mesh_geometry_registry_t* tmp_registry = NULL;
    registry_entry_t* tmp_entry_array = NULL;

    size_t entry_array_size = 0;

    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_geometry_count_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_initialize", "max_geometry_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(INT16_MAX >= max_geometry_count_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_initialize", "max_geometry_count_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_initialize", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_initialize", "*out_registry_")

    // geometry_registry_tメモリ確保
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(lit_mesh_geometry_registry_t), alignof(lit_mesh_geometry_registry_t), (void**)&tmp_registry);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - Failed to allocate registry instance. target=lit_mesh_geometry_registry_t, bytes=%zu, align=%zu, max_geometry_count=%zu", resource_registry_rslt_to_str(ret), sizeof(lit_mesh_geometry_registry_t), alignof(lit_mesh_geometry_registry_t), max_geometry_count_);
        goto cleanup;
    }
    memset(tmp_registry, 0, sizeof(lit_mesh_geometry_registry_t));

    if((SIZE_MAX / max_geometry_count_) < sizeof(registry_entry_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - overflow.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    entry_array_size = sizeof(registry_entry_t) * max_geometry_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, entry_array_size, alignof(registry_entry_t), (void**)&tmp_entry_array);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - allocation failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_entry_array, 0, entry_array_size);

    tmp_registry->max_geometry_count = max_geometry_count_;
    tmp_registry->entries = tmp_entry_array;

    *out_registry_ = tmp_registry;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void lit_mesh_geometry_registry_deinitialize(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_) {
    if(NULL == registry_ || NULL == shader_) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_deinitialize(%s) - provided registry_ or shader_ is NULL.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_deinitialize(%s) - lit_mesh_geometry_registry_t internal state is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(!registry_entry_is_empty(&registry_->entries[i])) {
            if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[i], shader_)) {
                ERROR_MESSAGE("lit_mesh_geometry_registry_deinitialize(%s) - registry_entry_deinitialize failed.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
                return;
            }
        }
    }
    registry_->max_geometry_count = 0;
}

bool lit_mesh_geometry_registry_find(const lit_mesh_geometry_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_find(%s) - lit_mesh_geometry_registry_t internal state is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_find(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return false;
    }

    return find_by_name(registry_, name_, &tmp_id);
}

const lit_mesh_geometry_t* lit_mesh_geometry_registry_geometry_get(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_geometry_get(%s) - provided registry_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_geometry_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
    if(!geometry_id_is_valid(registry_, geometry_id_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_geometry_get(%s) - provided geometry_id_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[geometry_id_].cpu_resource;
}

resource_registry_result_t lit_mesh_geometry_registry_id_get(const lit_mesh_geometry_registry_t* registry_, const char* name_, int16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "out_geometry_id_")

    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_id_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_geometry_registry_id_get(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_id_get(%s) - find_by_name failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    *out_geometry_id_ = (int16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

const draw_range_t* lit_mesh_geometry_registry_draw_range_get(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_draw_range_get(%s) - provided registry_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_draw_range_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
    if(!geometry_id_is_valid(registry_, geometry_id_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_draw_range_get(%s) - provided geometry_id_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(registry_entry_is_empty(&registry_->entries[geometry_id_])) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_draw_range_get(%s) - no allocation.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return &registry_->entries[geometry_id_].allocation_descriptor.draw_range;
}

resource_registry_result_t lit_mesh_geometry_registry_register(lit_mesh_geometry_registry_t* registry_, const char* resource_name_, lit_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, int16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "*geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_range_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "vbo_range_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!lit_mesh_geometry_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // リソースの重複チェック
    if(find_by_name(registry_, resource_name_, &tmp_index)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - provided resource name is already registered.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // 空きスロット検索
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(NULL == registry_->entries[i].resource_name) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - free slot not found.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_registry_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // entry登録, 所有権移動commit(allocation_descriptorはスタック領域にメモリ確保されたローカル変数の場合があるため値コピー)
    registry_->entries[tmp_index].cpu_resource = *geometry_;
    registry_->entries[tmp_index].allocation_descriptor = *vbo_range_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    *geometry_ = NULL;
    memset(vbo_range_, 0, sizeof(vbo_range_t));
    tmp_name = NULL;

    *out_geometry_id_ = (int16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    if(RESOURCE_REGISTRY_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
    }
    return ret;
}

resource_registry_result_t lit_mesh_geometry_registry_unregister(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_, int16_t geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_unregister", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_unregister", "shader_")
    IF_ARG_FALSE_GOTO_CLEANUP(lit_mesh_geometry_registry_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "lit_mesh_geometry_registry_unregister", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(geometry_id_is_valid(registry_, geometry_id_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_unregister", "geometry_id_")

    if(NULL == registry_->entries[geometry_id_].resource_name) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_unregister(%s) - provided geometry id entry is empty.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    if(RESOURCE_REGISTRY_SUCCESS != registry_entry_deinitialize(&registry_->entries[geometry_id_], shader_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_unregister(%s) - registry_entry_deinitialize failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

bool lit_mesh_geometry_registry_is_valid(const lit_mesh_geometry_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_geometry_count || INT16_MAX < registry_->max_geometry_count) {
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

static resource_registry_result_t registry_entry_deinitialize(registry_entry_t* registry_entry_, lit_mesh_shader_t* shader_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    if(NULL == registry_entry_ || NULL == shader_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        goto cleanup;
    }

    if(SHADER_SUCCESS != lit_mesh_shader_vbo_free(shader_, &registry_entry_->allocation_descriptor)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("registry_entry_deinitialize(%s) - lit_mesh_shader_vbo_free failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    memset(&registry_entry_->allocation_descriptor, 0, sizeof(vbo_range_t));

    lit_mesh_geometry_destroy(&registry_entry_->cpu_resource);
    choco_string_destroy(&registry_entry_->resource_name);

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

static bool registry_entry_is_empty(const registry_entry_t* registry_entry_) {
    if(NULL == registry_entry_) {
        return false;
    }
    if(NULL != registry_entry_->cpu_resource || NULL != registry_entry_->resource_name) {
        return false;
    }
    if(0 != registry_entry_->allocation_descriptor.allocation_info.allocated_size) {
        return false;
    }
    if(0 != registry_entry_->allocation_descriptor.allocation_info.node_index) {
        return false;
    }
    if(0 != registry_entry_->allocation_descriptor.allocation_info.offset) {
        return false;
    }
    if(NULL != registry_entry_->allocation_descriptor.allocation_info.owner) {
        return false;
    }
    if(0 != registry_entry_->allocation_descriptor.draw_range.first_vertex_count) {
        return false;
    }
    if(0 != registry_entry_->allocation_descriptor.draw_range.vertex_count) {
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
    if(NULL == entry_->cpu_resource || NULL == entry_->resource_name) {
        return false;
    }

    // 登録済みentryの検査 TODO: cpu_resourceのvalidation APIを追加
    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }
    if(!vbo_range_is_valid(&entry_->allocation_descriptor)) {
        return false;
    }

    return true;
}

static bool geometry_id_is_valid(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(geometry_id_ < 0 || registry_->max_geometry_count <= (size_t)geometry_id_) {
        return false;
    }
    return true;
}

static bool find_by_name(const lit_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
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
