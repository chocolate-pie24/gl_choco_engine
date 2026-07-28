/** @ingroup renderer
 *
 * @file lit_mesh_geometry_registry.c
 * @author chocolate-pie24
 *
 * @brief 単色ライティング描画用ジオメトリレジストリAPIの実装
 *
 * @version 0.1
 * @date 2026-06-20
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
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

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_types.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

/**
 * @brief 単色ライティング描画用ジオメトリレジストリ内部構造体
 *
 */
struct lit_mesh_geometry_registry {
    size_t max_geometry_count;          /**< レジストリに登録可能な最大ジオメトリ数(0は許可しない. 単色ライティング描画を使用しなくても1以上にする) */

    // CPU resources
    lit_mesh_geometry_t** geometries;   /**< 登録されたジオメトリの複製へのポインタ配列。複製の所有権はレジストリが持つ */

    // GPU placement metadata
    vertex_buffer_range_t* vertex_ranges;
};

static bool geometry_id_is_valid(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_);
static bool internal_state_is_valid(const lit_mesh_geometry_registry_t* registry_);
static bool find_by_name(const lit_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_);

resource_registry_result_t lit_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, lit_mesh_geometry_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    lit_mesh_geometry_registry_t* tmp_registry = NULL;
    lit_mesh_geometry_t** tmp_geometry_array = NULL;

    vertex_buffer_range_t* tmp_vertex_ranges = NULL;

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

    if((SIZE_MAX / max_geometry_count_) < sizeof(lit_mesh_geometry_t*)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - Allocation size overflow while calculating geometry pointer array size. target=geometries, elem_type=lit_mesh_geometry_t*, elem_count=%zu, elem_size=%zu, size_max=%zu", resource_registry_rslt_to_str(ret), max_geometry_count_, sizeof(lit_mesh_geometry_t*), SIZE_MAX);
        goto cleanup;
    }
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(lit_mesh_geometry_t*) * max_geometry_count_, alignof(lit_mesh_geometry_t*), (void**)&tmp_geometry_array);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - Failed to allocate geometry pointer array. target=geometries, elem_type=lit_mesh_geometry_t*, elem_count=%zu, elem_size=%zu, bytes=%zu, align=%zu", resource_registry_rslt_to_str(ret), max_geometry_count_, sizeof(lit_mesh_geometry_t*), sizeof(lit_mesh_geometry_t*) * max_geometry_count_, alignof(lit_mesh_geometry_t*));
        goto cleanup;
    }

    if((SIZE_MAX / max_geometry_count_) < sizeof(vertex_buffer_range_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - Allocation size overflow while calculating vertex offset array size. target=vertex_offsets, elem_type=size_t, elem_count=%zu, elem_size=%zu, size_max=%zu", resource_registry_rslt_to_str(ret), max_geometry_count_, sizeof(vertex_buffer_range_t), SIZE_MAX);
        goto cleanup;
    }
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(vertex_buffer_range_t) * max_geometry_count_, alignof(vertex_buffer_range_t), (void**)&tmp_vertex_ranges);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("lit_mesh_geometry_registry_initialize(%s) - Failed to allocate vertex range array. target=vertex_ranges, elem_type=size_t, elem_count=%zu, elem_size=%zu, bytes=%zu, align=%zu", resource_registry_rslt_to_str(ret), max_geometry_count_, sizeof(vertex_buffer_range_t), sizeof(vertex_buffer_range_t) * max_geometry_count_, alignof(vertex_buffer_range_t));
        goto cleanup;
    }

    tmp_registry->max_geometry_count = max_geometry_count_;
    for(size_t i = 0; i != max_geometry_count_; ++i) {
        tmp_geometry_array[i] = NULL;
        memset(&tmp_vertex_ranges[i], 0, sizeof(vertex_buffer_range_t));
    }

    tmp_registry->geometries = tmp_geometry_array;
    tmp_registry->vertex_ranges = tmp_vertex_ranges;

    *out_registry_ = tmp_registry;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void lit_mesh_geometry_registry_deinitialize(lit_mesh_geometry_registry_t* registry_) {
    if(NULL == registry_) {
        return;
    }
    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        lit_mesh_geometry_destroy(&registry_->geometries[i]);  // registry_->geometries[i] == NULLになる
        memset(&registry_->vertex_ranges[i], 0, sizeof(vertex_buffer_range_t));
    }
}

bool lit_mesh_geometry_registry_find(const lit_mesh_geometry_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_) {
        // これは場合によっては起こりうる(かも)ので、メッセージは出さない
        return false;
    }
    if(NULL == name_) {
        // これは場合によっては起こりうる(かも)ので、メッセージは出さない
        return false;
    }
    if(!internal_state_is_valid(registry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_registry_find(%s) - Registry internal state check failed. operation=find, target=lit_mesh_geometry_registry_t, query_name='%s'", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), name_);
        return false;
    }

    return find_by_name(registry_, name_, &tmp_id);
}

const lit_mesh_geometry_t* lit_mesh_geometry_registry_geometry_get(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    if(NULL == registry_ || !internal_state_is_valid(registry_) || !geometry_id_is_valid(registry_, geometry_id_)) {
        return NULL;
    }

    return registry_->geometries[geometry_id_];
}

resource_registry_result_t lit_mesh_geometry_registry_id_get(const lit_mesh_geometry_registry_t* registry_, const char* name_, int16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_id_get", "out_geometry_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(internal_state_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "lit_mesh_geometry_registry_id_get", "registry_")

    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_id_get(%s) - Failed to get lit mesh geometry id. reason=not_registered, query_name='%s'", resource_registry_rslt_to_str(ret), name_);
        goto cleanup;
    }

    *out_geometry_id_ = (int16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

resource_registry_result_t lit_mesh_geometry_registry_vertex_buffer_range_get(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_, vertex_buffer_range_t* out_vertex_buffer_range_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_vertex_buffer_range_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_buffer_range_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_vertex_buffer_range_get", "out_vertex_buffer_range_")
    IF_ARG_FALSE_GOTO_CLEANUP(internal_state_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "lit_mesh_geometry_registry_vertex_buffer_range_get", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(geometry_id_is_valid(registry_, geometry_id_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_vertex_buffer_range_get", "geometry_id_")

    if(NULL == registry_->geometries[geometry_id_]) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_vertex_buffer_range_get(%s) - Failed to get lit mesh geometry draw range. reason=not_registered, geometry_id=%d", resource_registry_rslt_to_str(ret), geometry_id_);
        goto cleanup;
    }

    *out_vertex_buffer_range_ = registry_->vertex_ranges[geometry_id_];

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

resource_registry_result_t lit_mesh_geometry_registry_register(lit_mesh_geometry_registry_t* registry_, const lit_mesh_geometry_t* geometry_, const vertex_buffer_range_t* vertex_buffer_range_, int16_t* out_geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

    size_t unused_index = 0;
    size_t free_slot = 0;
    size_t vertex_count = 0;
    bool found_free_slot = false;
    const char* name = NULL;
    lit_mesh_geometry_t* cloned_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(internal_state_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "lit_mesh_geometry_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_range_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "vertex_buffer_range_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_buffer_range_->allocation_info.range_allocation.allocated_size, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "lit_mesh_geometry_registry_register", "vertex_buffer_range_->allocation_info.range_allocation.allocated_size")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_buffer_range_->draw_range.vertex_count, ret, RESOURCE_REGISTRY_BAD_OPERATION, resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION), "lit_mesh_geometry_registry_register", "vertex_buffer_range_->draw_range.vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_register", "out_geometry_id_")

    ret_resource = lit_mesh_geometry_vertex_count_get(geometry_, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_registry_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - lit_mesh_geometry_registry_register failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(vertex_count != vertex_buffer_range_->draw_range.vertex_count) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - lit_mesh_geometry_registry_register failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // 重複チェック
    name = lit_mesh_geometry_name_get(geometry_);
    if(NULL == name) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - Failed to register lit mesh geometry. reason=name_get_failed", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(find_by_name(registry_, name, &unused_index)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - Failed to register lit mesh geometry. reason=already_registered, geometry_name='%s'", resource_registry_rslt_to_str(ret), name);
        goto cleanup;
    }

    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(NULL == registry_->geometries[i]) {
            ret_resource = lit_mesh_geometry_clone(geometry_, &cloned_geometry);
            if(RESOURCE_SUCCESS != ret_resource) {
                ret = resource_registry_rslt_convert_resource(ret_resource);
                ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - Failed to register lit mesh geometry. reason=clone_failed, geometry_name='%s'", resource_registry_rslt_to_str(ret), name);
                goto cleanup;
            }
            found_free_slot = true;
            free_slot = i;
            break;
        }
    }

    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("lit_mesh_geometry_registry_register(%s) - Failed to register lit mesh geometry. reason=registry_full, geometry_name='%s', max_geometry_count=%zu", resource_registry_rslt_to_str(ret), name, registry_->max_geometry_count);
        goto cleanup;
    }

    registry_->vertex_ranges[free_slot] = *vertex_buffer_range_;
    registry_->geometries[free_slot] = cloned_geometry;
    *out_geometry_id_ = (int16_t)free_slot;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

resource_registry_result_t lit_mesh_geometry_registry_unregister(lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_unregister", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(internal_state_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "lit_mesh_geometry_registry_unregister", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(geometry_id_is_valid(registry_, geometry_id_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "lit_mesh_geometry_registry_unregister", "geometry_id_")

    if(NULL == registry_->geometries[geometry_id_]) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("lit_mesh_geometry_registry_unregister(%s) - Failed to unregister lit mesh geometry. reason=not_registered, geometry_id=%d", resource_registry_rslt_to_str(ret), geometry_id_);
        goto cleanup;
    }

    lit_mesh_geometry_destroy(&registry_->geometries[geometry_id_]);
    memset(&registry_->vertex_ranges[geometry_id_], 0, sizeof(vertex_buffer_range_t));

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief geometry_id_が有効な値かを判定する
 *
 * @param[in] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[in] geometry_id_ 判定対象ジオメトリid
 *
 * @retval true geometry_id_は正常
 * @retval false 以下のいずれか
 * - registry_内部データ不整合が発生している
 * - geometry_id_が0未満
 * - geometry_id_がregistry_->max_geometry_count以上
 */
static bool geometry_id_is_valid(const lit_mesh_geometry_registry_t* registry_, int16_t geometry_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(!internal_state_is_valid(registry_)) {
        return false;
    }
    if(geometry_id_ < 0 || registry_->max_geometry_count <= (size_t)geometry_id_) {
        return false;
    }
    return true;
}

/**
 * @brief registry_の内部データが正常かを判定する
 *
 * @param[in] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 *
 * @retval true 内部データ正常
 * @retval false 以下のいずれか
 * - registry_ == NULL
 * - registry_->max_geometry_countが未初期化で0
 * - registry_->max_geometry_countがint16_tの最大値を超過
 * - registry_->geometriesが未初期化でNULL
 * - registry_->vertex_rangesが未初期化でNULL
 */
static bool internal_state_is_valid(const lit_mesh_geometry_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_geometry_count || INT16_MAX < registry_->max_geometry_count) {
        return false;
    }
    if(NULL == registry_->geometries || NULL == registry_->vertex_ranges) {
        return false;
    }
    return true;
}

/**
 * @brief registry_にname_のジオメトリが登録されているか判定し、登録されている場合はそのインデックスを取得する
 *
 * @note 返り値がfalseの場合はout_index_の値は不変
 *
 * @param[in] registry_ 検索対象のレジストリ
 * @param[in] name_ 検索対象のジオメトリ名
 * @param[out] out_index_ 登録済みジオメトリのインデックス
 *
 * @retval true registry_にname_のジオメトリが登録されている
 * @retval false 以下のいずれか
 * - name_ == NULL
 * - registry_ == NULL
 * - out_index_ == NULL
 * - registry_内部データ不整合が発生している
 * - registry_にname_のジオメトリが見つからない
 */
static bool find_by_name(const lit_mesh_geometry_registry_t* registry_, const char* name_, size_t* out_index_) {
    const char* tmp_name = NULL;
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }
    if(!internal_state_is_valid(registry_)) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_geometry_count; ++i) {
        if(NULL != registry_->geometries[i]) {
            tmp_name = lit_mesh_geometry_name_get(registry_->geometries[i]);
            if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
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
