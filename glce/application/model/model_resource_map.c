// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model/model_resource_map.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/model/core/model_types.h"
#include "application/model/core/model_err_utils.h"
#include "application/model/model_assembly_config.h"
#include "application/model/model_transform_tree.h"

struct model_resource_map {
    // 個体生成用の初期Transform Tree
    model_transform_tree_t* transform_tree;

    size_t line_mesh_count;
    line_mesh_t* line_meshes;
    uint16_t* line_mesh_coordinate_ids;

    size_t point_mesh_count;
    point_mesh_t* point_meshes;
    uint16_t* point_mesh_coordinate_ids;

    size_t ui_mesh_count;
    ui_mesh_t* ui_meshes;
    uint16_t* ui_mesh_coordinate_ids;

    size_t untextured_mesh_count;
    untextured_mesh_t* untextured_meshes;
    uint16_t* untextured_mesh_coordinate_ids;
};

static model_result_t transform_tree_create_from_config(const model_assembly_config_t* config_, model_transform_tree_t** out_transform_tree_);

model_result_t model_resource_map_create(renderer_frontend_t* renderer_frontend_, const model_assembly_config_t* config_, const char* asset_root_path_, model_resource_map_t** out_resource_map_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    model_resource_map_t* tmp_resource_map = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;
    line_mesh_t* tmp_line_meshes = NULL;
    point_mesh_t* tmp_point_meshes = NULL;
    ui_mesh_t* tmp_ui_meshes = NULL;
    untextured_mesh_t* tmp_untextured_meshes = NULL;

    uint16_t* tmp_line_mesh_coordinate_ids = NULL;
    uint16_t* tmp_point_mesh_coordinate_ids = NULL;
    uint16_t* tmp_ui_mesh_coordinate_ids = NULL;
    uint16_t* tmp_untextured_mesh_coordinate_ids = NULL;

    size_t allocation_size = 0;
    size_t imported_untextured_mesh_count = 0;
    untextured_mesh_t tmp_mesh = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_create", "out_resource_map_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_resource_map_, ret, MODEL_BAD_OPERATION, model_result_to_str(MODEL_BAD_OPERATION), "model_resource_map_create", "*out_resource_map_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_assembly_config_is_valid(config_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_resource_map_create(%s) - Provided config_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_general_allocator = general_allocator_allocate(sizeof(model_resource_map_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_resource_map);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
        goto cleanup;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Line Mesh
    //////////////////////////////////////////////////////////////////////////////////////////
    if(0 != config_->line_mesh_count) {
        if(!size_mul_checked(sizeof(line_mesh_t), config_->line_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_line_meshes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(uint16_t), config_->line_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_line_mesh_coordinate_ids);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }
        // TODO: renderer_frontend_line_mesh_import
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Point Mesh
    //////////////////////////////////////////////////////////////////////////////////////////
    if(0 != config_->point_mesh_count) {
        if(!size_mul_checked(sizeof(point_mesh_t), config_->point_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_point_meshes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(uint16_t), config_->point_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_point_mesh_coordinate_ids);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }
        // TODO: renderer_frontend_point_mesh_import
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // UI Mesh
    //////////////////////////////////////////////////////////////////////////////////////////
    if(0 != config_->ui_mesh_count) {
        if(!size_mul_checked(sizeof(ui_mesh_t), config_->ui_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_ui_meshes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(uint16_t), config_->ui_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_ui_mesh_coordinate_ids);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }
        // TODO: renderer_frontend_point_mesh_import
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Untextured Mesh
    //////////////////////////////////////////////////////////////////////////////////////////
    if(0 != config_->untextured_mesh_count) {
        if(!size_mul_checked(sizeof(untextured_mesh_t), config_->untextured_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_untextured_meshes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(uint16_t), config_->untextured_mesh_count, &allocation_size)) {
            ret = MODEL_OVERFLOW;
            ERROR_MESSAGE("model_resource_map_create(%s) - allocation overflow.", model_result_to_str(ret));
            goto cleanup;
        }
        ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_untextured_mesh_coordinate_ids);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = model_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("model_resource_map_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
            goto cleanup;
        }

        for(size_t i = 0; i != config_->untextured_mesh_count; ++i) {
            ret_renderer_frontend = renderer_frontend_untextured_mesh_import(renderer_frontend_, &config_->untextured_mesh_configs[i], asset_root_path_, &tmp_mesh);
            if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
                ret = model_result_convert_renderer_frontend(ret_renderer_frontend);
                ERROR_MESSAGE("model_resource_map_create(%s) - renderer_frontend_untextured_mesh_import failed.", model_result_to_str(ret));
                goto cleanup;
            }

            tmp_untextured_meshes[i].geometry_id = tmp_mesh.geometry_id;
            tmp_untextured_meshes[i].material_id = tmp_mesh.material_id;

            imported_untextured_mesh_count++;
        }
        for(size_t i = 0; i != config_->untextured_mesh_count; ++i) {
            tmp_untextured_mesh_coordinate_ids[i] = config_->untextured_mesh_coordinate_ids[i];
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Transform Tree
    //////////////////////////////////////////////////////////////////////////////////////////
    ret = transform_tree_create_from_config(config_, &tmp_transform_tree);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("model_resource_map_create(%s) - transform_tree_create_from_config failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    tmp_resource_map->line_mesh_coordinate_ids = tmp_line_mesh_coordinate_ids;
    tmp_resource_map->line_mesh_count = config_->line_mesh_count;
    tmp_resource_map->line_meshes = tmp_line_meshes;
    tmp_resource_map->point_mesh_coordinate_ids = tmp_point_mesh_coordinate_ids;
    tmp_resource_map->point_mesh_count = config_->point_mesh_count;
    tmp_resource_map->point_meshes = tmp_point_meshes;
    tmp_resource_map->ui_mesh_coordinate_ids = tmp_ui_mesh_coordinate_ids;
    tmp_resource_map->ui_mesh_count = config_->ui_mesh_count;
    tmp_resource_map->ui_meshes = tmp_ui_meshes;
    tmp_resource_map->untextured_mesh_coordinate_ids = tmp_untextured_mesh_coordinate_ids;
    tmp_resource_map->untextured_mesh_count = config_->untextured_mesh_count;
    tmp_resource_map->untextured_meshes = tmp_untextured_meshes;
    tmp_resource_map->transform_tree = tmp_transform_tree;
    *out_resource_map_ = tmp_resource_map;

    tmp_transform_tree = NULL;
    tmp_line_meshes = NULL;
    tmp_point_meshes = NULL;
    tmp_ui_meshes = NULL;
    tmp_untextured_meshes = NULL;

    tmp_line_mesh_coordinate_ids = NULL;
    tmp_point_mesh_coordinate_ids = NULL;
    tmp_ui_mesh_coordinate_ids = NULL;
    tmp_untextured_mesh_coordinate_ids = NULL;

    ret = MODEL_SUCCESS;

cleanup:
    if(MODEL_DATA_CORRUPTED == ret || MODEL_SUCCESS == ret) {
        return ret;
    }

    if(NULL != tmp_untextured_meshes) {
        for(size_t i = 0; i != imported_untextured_mesh_count; ++i) {
            if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_untextured_meshes[i])) {
                ret = MODEL_DATA_CORRUPTED;
                ERROR_MESSAGE("model_resource_map_create(%s) - renderer_frontend_untextured_mesh_release failed.", model_result_to_str(ret));
                return ret;
            }
        }
    }

    // TODO: renderer_frontendへのpoint, ui, line meshのreleaseを追加したらrelease処理を追加する

    if(NULL != tmp_transform_tree) {
        model_transform_tree_destroy(&tmp_transform_tree);
    }
    if(NULL != tmp_untextured_mesh_coordinate_ids) {
        general_allocator_free((void**)&tmp_untextured_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_untextured_meshes) {
        general_allocator_free((void**)&tmp_untextured_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_resource_map) {
        general_allocator_free((void**)&tmp_resource_map, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

model_result_t model_resource_map_destroy(renderer_frontend_t* renderer_frontend_, model_resource_map_t** resource_map_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_destroy", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(*resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_destroy", "*resource_map_")

    // TODO: line_mesh release
    // TODO: point_mesh release
    // TODO: ui_mesh release
    for(size_t i = 0; i != (*resource_map_)->untextured_mesh_count; ++i) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &(*resource_map_)->untextured_meshes[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = MODEL_DATA_CORRUPTED;
            ERROR_MESSAGE("model_resource_map_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", model_result_to_str(ret));
            goto cleanup;
        }
    }

    if(NULL != (*resource_map_)->untextured_mesh_coordinate_ids) {
        general_allocator_free((void**)&(*resource_map_)->untextured_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != (*resource_map_)->untextured_meshes) {
        general_allocator_free((void**)&(*resource_map_)->untextured_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != (*resource_map_)->transform_tree) {
        model_transform_tree_destroy(&(*resource_map_)->transform_tree);
    }
    general_allocator_free((void**)resource_map_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

model_result_t model_resource_map_transform_tree_clone(const model_resource_map_t* resource_map_, model_transform_tree_t** out_transform_tree_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_transform_tree_clone", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_transform_tree_clone", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, MODEL_BAD_OPERATION, model_result_to_str(MODEL_BAD_OPERATION), "model_resource_map_transform_tree_clone", "*out_transform_tree_")

    ret = model_transform_tree_clone(resource_map_->transform_tree, out_transform_tree_);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("model_resource_map_transform_tree_clone(%s) - model_transform_tree_clone failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

model_result_t model_resource_map_untextured_mesh_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_get", "out_mesh_")
    if(mesh_index_ >= resource_map_->untextured_mesh_count) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_resource_map_untextured_mesh_get(%s) - Provided mesh_index_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = resource_map_->untextured_meshes[mesh_index_].geometry_id;
    out_mesh_->material_id = resource_map_->untextured_meshes[mesh_index_].material_id;

    return MODEL_SUCCESS;

cleanup:
    return ret;
}

model_result_t model_resource_map_untextured_mesh_coordinate_id_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, uint16_t* out_coordinate_id_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_coordinate_id_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_id_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_coordinate_id_get", "out_coordinate_id_")
    if(mesh_index_ >= resource_map_->untextured_mesh_count) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_resource_map_untextured_mesh_coordinate_id_get(%s) - Provided untextured_mesh_index_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    *out_coordinate_id_ = resource_map_->untextured_mesh_coordinate_ids[mesh_index_];

    return MODEL_SUCCESS;

cleanup:
    return ret;
}

model_result_t model_resource_map_untextured_mesh_count_get(const model_resource_map_t* resource_map_, size_t* out_count_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_count_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_count_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_resource_map_untextured_mesh_count_get", "out_count_")

    *out_count_ = resource_map_->untextured_mesh_count;

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

bool model_resource_map_is_valid(const model_resource_map_t* resource_map_) {
    if(NULL == resource_map_) {
        return false;
    }
    // TODO: validator作成
    return true;
}

static model_result_t transform_tree_create_from_config(const model_assembly_config_t* config_, model_transform_tree_t** out_transform_tree_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    ring_queue_t* parent_queue = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    size_t queue_capacity = 0;
    size_t added_count = 0;
    uint16_t current_parent = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "transform_tree_create_from_config", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "transform_tree_create_from_config", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, MODEL_BAD_OPERATION, model_result_to_str(MODEL_BAD_OPERATION), "transform_tree_create_from_config", "*out_transform_tree_")
    if(config_->coordinate_count > 0 && NULL == config_->coordinates) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Provided config_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    if(!size_add_checked(config_->coordinate_count, 1, &queue_capacity)) {  // RootのID分を追加したQueue容量
        ret = MODEL_OVERFLOW;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Queue capacity overflow.", model_result_to_str(ret));
        goto cleanup;
    }
    ret_ring_queue = ring_queue_create(queue_capacity, sizeof(uint16_t), alignof(uint16_t), &parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = model_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_create failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret = model_transform_tree_create(0, &tmp_transform_tree);  // Root Coordinate
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_create failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Rootを探索Queueに追加
    current_parent = 0;
    ret_ring_queue = ring_queue_push(&current_parent, sizeof(uint16_t), alignof(uint16_t), parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = model_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", model_result_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_is_empty(parent_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(uint16_t), alignof(uint16_t), parent_queue, &current_parent);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = model_result_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_pop failed.", model_result_to_str(ret));
            goto cleanup;
        }

        // 現在のParentを持つCoordinateを探索
        for(size_t i = 0; i != config_->coordinate_count; ++i) {
            const model_coordinate_config_t* coordinate = &config_->coordinates[i];
            if(current_parent != coordinate->parent_coordinate_id) {
                continue;
            }

            ret = model_transform_tree_add_child(tmp_transform_tree, current_parent, coordinate->coordinate_id, coordinate->translate, coordinate->rotation);
            if(MODEL_SUCCESS != ret) {
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_add_child failed.", model_result_to_str(ret));
                goto cleanup;
            }

            ++added_count;

            // 追加したChildを次のParent候補としてQueueへpush
            ret_ring_queue = ring_queue_push(&coordinate->coordinate_id, sizeof(uint16_t), alignof(uint16_t), parent_queue);
            if(RING_QUEUE_SUCCESS != ret_ring_queue) {
                ret = model_result_convert_ring_queue(ret_ring_queue);
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", model_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    // 全CoordinateがRootから到達可能か検査
    if(added_count != config_->coordinate_count) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Unreachable coordinates detected.", model_result_to_str(ret));
        goto cleanup;
    }

    // Ownership Commit.
    *out_transform_tree_ = tmp_transform_tree;
    tmp_transform_tree = NULL;

    ret = MODEL_SUCCESS;

cleanup:
    if(MODEL_DATA_CORRUPTED != ret) {
        ring_queue_destroy(&parent_queue);
        model_transform_tree_destroy(&tmp_transform_tree);
    }

    return ret;
}
