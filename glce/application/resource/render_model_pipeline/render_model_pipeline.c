// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/resource/render_model_pipeline/render_model_pipeline.h"

#include <stdalign.h>
#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/model_transform_tree.h"
#include "engine/systems/renderer/resources/render_model/config/render_model_config.h"
#include "engine/systems/renderer/resources/render_model/render_model.h"

#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

static application_result_t transform_tree_create_from_config(const render_model_config_t* config_, model_transform_tree_t** out_transform_tree_);

application_result_t render_model_import(const render_model_config_t* config_, renderer_frontend_t* renderer_frontend_, const char* executable_directory_, render_model_t** out_model_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    render_model_result_t ret_render_model = RENDER_MODEL_INVALID_ARGUMENT;

    render_model_t* tmp_model = NULL;
    fs_path_t* fs_path = NULL;

    size_t tmp_untextured_mesh_count = 0;
    untextured_mesh_t* tmp_untextured_meshes = NULL;
    uint16_t* tmp_untextured_mesh_coordinate_ids = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    size_t allocation_size = 0;
    uint16_t tmp_geometry_id = 0;
    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "render_model_import", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "render_model_import", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "render_model_import", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(out_model_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "render_model_import", "out_model_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_model_, ret, APPLICATION_BAD_OPERATION, application_result_to_str(APPLICATION_BAD_OPERATION), "render_model_import", "*out_model_")

    if(!size_mul_checked(sizeof(untextured_mesh_t), config_->untextured_mesh_count, &allocation_size)) {
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("render_model_import(%s) - allocation size overflow.", application_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)tmp_untextured_meshes);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("render_model_import(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    if(!size_mul_checked(sizeof(uint16_t), config_->untextured_mesh_count, &allocation_size)) {
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("render_model_import(%s) - allocation size overflow.", application_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)tmp_untextured_mesh_coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("render_model_import(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != config_->untextured_mesh_count; ++i) {
        // TODO: configにextension追加, configのgeometry_subdirectoryはexecutable_directory_からの相対パスにするため../../を追加する
        // TODO: geometry系でhelper functionに移す
        // TODO: transaction設計

        // geometry
        ret_fs_path = fs_path_create(&fs_path, executable_directory_, choco_string_c_str(config_->untextured_mesh_configs[i].geometry_subdirectory), choco_string_c_str(config_->untextured_mesh_configs[i].geometry_resource_name), "stl");
        if(FS_PATH_SUCCESS != ret_fs_path) {
            ret = application_result_convert_fs_path(ret_fs_path);
            ERROR_MESSAGE("untextured_mesh_import(%s) - fs_path_create failed.", application_result_to_str(ret));
            goto cleanup;
        }
        ret_renderer_frontend = renderer_frontend_untextured_mesh_geometry_import_from_file(renderer_frontend_, choco_string_c_str(config_->untextured_mesh_configs[i].geometry_resource_name), fs_path_fullpath_get(fs_path), &tmp_geometry_id);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("untextured_mesh_import(%s) - renderer_frontend_untextured_mesh_geometry_import_from_file failed.", application_result_to_str(ret));
            goto cleanup;
        }
        fs_path_destroy(&fs_path);

        // material
        ret_fs_path = fs_path_create(&fs_path, executable_directory_, choco_string_c_str(config_->untextured_mesh_configs[i].material_subdirectory), choco_string_c_str(config_->untextured_mesh_configs[i].material_resource_name), "untextured_mat");
        if(FS_PATH_SUCCESS != ret_fs_path) {
            ret = application_result_convert_fs_path(ret_fs_path);
            ERROR_MESSAGE("untextured_mesh_import(%s) - fs_path_create failed.", application_result_to_str(ret));
            goto cleanup;
        }
        ret_renderer_frontend = renderer_frontend_untextured_mesh_material_import_from_file(renderer_frontend_, choco_string_c_str(config_->untextured_mesh_configs[i].material_resource_name), fs_path_fullpath_get(fs_path), &tmp_material_id);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("untextured_mesh_import(%s) - renderer_frontend_untextured_mesh_geometry_import_from_file failed.", application_result_to_str(ret));
            goto cleanup;
        }
        fs_path_destroy(&fs_path);

        tmp_untextured_meshes[i].geometry_id = tmp_geometry_id;
        tmp_untextured_meshes[i].material_id = tmp_material_id;
        tmp_untextured_mesh_coordinate_ids[i] = config_->untextured_mesh_configs[i].coordinate_id;
    }

    ret = transform_tree_create_from_config(config_, &tmp_transform_tree);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_import(%s) - transform_tree_create_from_config failed.", application_result_to_str(ret));
        goto cleanup;
    }

    tmp_untextured_mesh_count = config_->untextured_mesh_count;

    ret_render_model = render_model_create(tmp_untextured_mesh_count, &tmp_untextured_meshes, &tmp_untextured_mesh_coordinate_ids, &tmp_transform_tree, &tmp_model);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: 正式な実行結果コード返還が必要だが、render_model自体をapplication側に移すことも想定しているため、とりあえず置いてく
        ERROR_MESSAGE("untextured_mesh_import(%s) - render_model_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    *out_model_ = tmp_model;
    ret = APPLICATION_SUCCESS;

cleanup:
    // TODO: cleanup, rollback
    return ret;
}

static application_result_t transform_tree_create_from_config(const render_model_config_t* config_, model_transform_tree_t** out_transform_tree_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;
    render_model_result_t ret_render_model = RENDER_MODEL_INVALID_ARGUMENT;

    ring_queue_t* parent_queue = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    size_t queue_capacity = 0;
    size_t added_count = 0;
    uint16_t current_parent = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "transform_tree_create_from_config", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "transform_tree_create_from_config", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, APPLICATION_BAD_OPERATION, application_result_to_str(APPLICATION_BAD_OPERATION), "transform_tree_create_from_config", "*out_transform_tree_")
    if(config_->coordinate_count > 0 && NULL == config_->coordinates) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Provided config_ is not valid.", application_result_to_str(ret));
        goto cleanup;
    }

    if(!size_add_checked(config_->coordinate_count, 1, &queue_capacity)) {  // RootのID分を追加したQueue容量
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Queue capacity overflow.", application_result_to_str(ret));
        goto cleanup;
    }
    ret_ring_queue = ring_queue_create(queue_capacity, sizeof(uint16_t), alignof(uint16_t), &parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = application_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = model_transform_tree_create(0, &tmp_transform_tree);  // Root Coordinate
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: 正式な実行結果コード返還が必要だが、render_model自体をapplication側に移すことも想定しているため、とりあえず置いてく
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Rootを探索Queueに追加
    current_parent = 0;
    ret_ring_queue = ring_queue_push(&current_parent, sizeof(uint16_t), alignof(uint16_t), parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = application_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", application_result_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_is_empty(parent_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(uint16_t), alignof(uint16_t), parent_queue, &current_parent);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = application_result_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_pop failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // 現在のParentを持つCoordinateを探索
        for(size_t i = 0; i != config_->coordinate_count; ++i) {
            const render_model_coordinate_config_t* coordinate = &config_->coordinates[i];
            if(current_parent != coordinate->parent_coordinate_id) {
                continue;
            }

            ret_render_model = model_transform_tree_add_child(tmp_transform_tree, current_parent, coordinate->coordinate_id, coordinate->translate, coordinate->rotation);
            if(RENDER_MODEL_SUCCESS != ret_render_model) {
                ret = APPLICATION_RUNTIME_ERROR;    // TODO: 正式な実行結果コード返還が必要だが、render_model自体をapplication側に移すことも想定しているため、とりあえず置いてく
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_add_child failed.", application_result_to_str(ret));
                goto cleanup;
            }

            ++added_count;

            // 追加したChildを次のParent候補としてQueueへpush
            ret_ring_queue = ring_queue_push(&coordinate->coordinate_id, sizeof(uint16_t), alignof(uint16_t), parent_queue);
            if(RING_QUEUE_SUCCESS != ret_ring_queue) {
                ret = application_result_convert_ring_queue(ret_ring_queue);
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", application_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    // 全CoordinateがRootから到達可能か検査
    if(added_count != config_->coordinate_count) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Unreachable coordinates detected.", application_result_to_str(ret));
        goto cleanup;
    }

    // Ownership Commit.
    *out_transform_tree_ = tmp_transform_tree;
    tmp_transform_tree = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        ring_queue_destroy(&parent_queue);
        model_transform_tree_destroy(&tmp_transform_tree);
    }

    return ret;
}
