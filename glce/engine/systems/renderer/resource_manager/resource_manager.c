// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_manager/resource_manager.h"

#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/registries/geometries/untextured_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_mesh_material_registry.h"
#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resources/pipelines/materials/untextured_mesh_material_pipeline.h"

#include "engine/systems/renderer/resource_manager/core/resource_manager_types.h"
#include "engine/systems/renderer/resource_manager/core/resource_manager_err_utils.h"

// TODO: DYNAMIC / STATICでそれぞれVBOを作る
struct resource_manager {
    // untextured mesh.
    untextured_mesh_geometry_registry_t* untextured_mesh_geometry_registry;
    untextured_mesh_material_registry_t* untextured_mesh_material_registry;
    vbo_manager_t* untextured_mesh_vbo_manager;
};

resource_manager_result_t resource_manager_create(const resource_manager_config_t* config_, subsystem_allocator_t* allocator_, renderer_backend_context_t* backend_context_, resource_manager_t** out_resource_manager_) {
    resource_manager_result_t ret = RESOURCE_MANAGER_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    subsystem_allocator_rollback_point_t rollback_point = { 0 };

    // untextured_mesh
    resource_manager_t* tmp_resource_manager = NULL;

    untextured_mesh_geometry_registry_t* tmp_untextured_mesh_geometry_registry = NULL;
    untextured_mesh_material_registry_t* tmp_untextured_mesh_material_registry = NULL;    // TODO: untextured_mesh_materialに名称変更を検討する
    vbo_manager_t* tmp_untextured_mesh_vbo_manager = NULL;

    bool rollback_point_created = false;
    vbo_manager_config_t vbo_manager_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_manager_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_create", "out_resource_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_resource_manager_, ret, RESOURCE_MANAGER_BAD_OPERATION, resource_manager_result_to_str(RESOURCE_MANAGER_BAD_OPERATION), "resource_manager_create", "*out_resource_manager_")

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_manager_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("resource_manager_create(%s) - subsystem_allocator_rollback_point_get failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }
    rollback_point_created = true;

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(resource_manager_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_resource_manager);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = resource_manager_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("resource_manager_create(%s) - Failed to allocate resource_manager_t instance.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    // untextured_mesh生成
    vbo_manager_config.base_align = config_->untextured_mesh_resource_config.vbo_config.base_align;
    vbo_manager_config.buffer_usage = config_->untextured_mesh_resource_config.vbo_config.buffer_usage;
    vbo_manager_config.max_allocation_count = config_->untextured_mesh_resource_config.vbo_config.max_allocation_count;
    vbo_manager_config.vbo_size = config_->untextured_mesh_resource_config.vbo_config.vbo_size;
    ret_buffer_manager = vbo_manager_create(backend_context_, &vbo_manager_config, &tmp_untextured_mesh_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = resource_manager_result_convert_buffer_manager(ret_buffer_manager);
        ERROR_MESSAGE("resource_manager_create(%s) - buffer manager create failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_mesh_material_registry_create(config_->untextured_mesh_resource_config.max_material_count, allocator_, &tmp_untextured_mesh_material_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_manager_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("resource_manager_create(%s) - untextured_mesh_material_registry_create failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_mesh_geometry_registry_create(config_->untextured_mesh_resource_config.max_geometry_count, allocator_, &tmp_untextured_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_manager_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("resource_manager_create(%s) - untextured_mesh_geometry_registry_create failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    tmp_resource_manager->untextured_mesh_geometry_registry = tmp_untextured_mesh_geometry_registry;
    tmp_resource_manager->untextured_mesh_material_registry = tmp_untextured_mesh_material_registry;
    tmp_resource_manager->untextured_mesh_vbo_manager = tmp_untextured_mesh_vbo_manager;

    // TODO: validation

    // Output.
    *out_resource_manager_ = tmp_resource_manager;
    tmp_untextured_mesh_geometry_registry = NULL;
    tmp_untextured_mesh_material_registry = NULL;
    tmp_untextured_mesh_vbo_manager = NULL;

    ret = RESOURCE_MANAGER_SUCCESS;

cleanup:
    if(RESOURCE_MANAGER_SUCCESS == ret || RESOURCE_MANAGER_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(NULL != tmp_untextured_mesh_geometry_registry && NULL != tmp_untextured_mesh_vbo_manager) {
        untextured_mesh_geometry_registry_deinitialize(tmp_untextured_mesh_geometry_registry, tmp_untextured_mesh_vbo_manager);
    }
    if(NULL != tmp_untextured_mesh_material_registry) {
        untextured_mesh_material_registry_deinitialize(tmp_untextured_mesh_material_registry);
    }
    if(NULL != tmp_untextured_mesh_vbo_manager) {
        vbo_manager_destroy(&tmp_untextured_mesh_vbo_manager);
    }
    if(rollback_point_created) {
        ret_subsystem_allocator = subsystem_allocator_rollback(allocator_, &rollback_point);
        if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
            ret = RESOURCE_MANAGER_DATA_CORRUPTED;
            ERROR_MESSAGE("resource_manager_create(%s) - subsystem_allocator rollback failed.", resource_manager_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

void resource_manager_deinitialize(resource_manager_t* resource_manager_) {
    if(NULL == resource_manager_) {
        return;
    }
    if(NULL != resource_manager_->untextured_mesh_geometry_registry && NULL != resource_manager_->untextured_mesh_vbo_manager) {
        untextured_mesh_geometry_registry_deinitialize(resource_manager_->untextured_mesh_geometry_registry, resource_manager_->untextured_mesh_vbo_manager);
    }
    if(NULL != resource_manager_->untextured_mesh_material_registry) {
        untextured_mesh_material_registry_deinitialize(resource_manager_->untextured_mesh_material_registry);
    }
    if(NULL != resource_manager_->untextured_mesh_vbo_manager) {
        vbo_manager_destroy(&resource_manager_->untextured_mesh_vbo_manager);
    }
}

const draw_range_t* resource_manager_untextured_mesh_draw_range_get(const resource_manager_t* resource_manager_, uint16_t geometry_id_) {
    if(NULL == resource_manager_) {
        return NULL;
    }
    return untextured_mesh_geometry_registry_draw_range_get(resource_manager_->untextured_mesh_geometry_registry, geometry_id_);
}

const untextured_mesh_material_t* resource_manager_untextured_mesh_material_get(const resource_manager_t* resource_manager_, uint16_t material_id_) {
    if(NULL == resource_manager_) {
        return NULL;
    }
    return untextured_mesh_material_registry_material_get(resource_manager_->untextured_mesh_material_registry, material_id_);
}

const renderer_backend_vbo_t* resource_manager_untextured_mesh_vbo_get(const resource_manager_t* resource_manager_) {
    if(NULL == resource_manager_) {
        return NULL;
    }
    return vbo_manager_vbo_get(resource_manager_->untextured_mesh_vbo_manager);
}

resource_manager_result_t resource_manager_untextured_mesh_geometry_import_from_file(resource_manager_t* resource_manager_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    resource_manager_result_t ret = RESOURCE_MANAGER_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_geometry_import_from_file", "resource_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_geometry_import_from_file", "out_geometry_id_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_import_from_file(resource_manager_->untextured_mesh_vbo_manager, resource_manager_->untextured_mesh_geometry_registry, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = resource_manager_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("resource_manager_untextured_mesh_geometry_import_from_file(%s) - untextured_mesh_geometry_pipeline_import_from_file failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_MANAGER_SUCCESS;

cleanup:
    return ret;
}

resource_manager_result_t resource_manager_untextured_mesh_geometry_release(resource_manager_t* resource_manager_, uint16_t geometry_id_) {
    resource_manager_result_t ret = RESOURCE_MANAGER_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_geometry_release", "resource_manager_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(resource_manager_->untextured_mesh_vbo_manager, resource_manager_->untextured_mesh_geometry_registry, geometry_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = resource_manager_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("resource_manager_untextured_mesh_geometry_release(%s) - untextured_mesh_geometry_pipeline_release failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_MANAGER_SUCCESS;

cleanup:
    return ret;
}

resource_manager_result_t resource_manager_untextured_mesh_material_import_from_file(resource_manager_t* resource_manager_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_) {
    resource_manager_result_t ret = RESOURCE_MANAGER_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_material_import_from_file", "resource_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_material_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_material_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_material_import_from_file", "out_material_id_")

    ret_resource_pipeline = untextured_mesh_material_pipeline_import_from_file(resource_manager_->untextured_mesh_material_registry, resource_name_, resource_fullpath_, &tmp_material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = resource_manager_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("resource_manager_untextured_mesh_material_import_from_file(%s) - untextured_mesh_material_pipeline_import_from_file failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    *out_material_id_ = tmp_material_id;

    ret = RESOURCE_MANAGER_SUCCESS;

cleanup:
    return ret;
}

resource_manager_result_t resource_manager_untextured_mesh_material_release(resource_manager_t* resource_manager_, uint16_t material_id_) {
    resource_manager_result_t ret = RESOURCE_MANAGER_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RESOURCE_MANAGER_INVALID_ARGUMENT, resource_manager_result_to_str(RESOURCE_MANAGER_INVALID_ARGUMENT), "resource_manager_untextured_mesh_material_release", "resource_manager_")

    ret_resource_pipeline = untextured_mesh_material_pipeline_release(resource_manager_->untextured_mesh_material_registry, material_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = resource_manager_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("resource_manager_untextured_mesh_material_release(%s) - untextured_mesh_material_pipeline_release failed.", resource_manager_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_MANAGER_SUCCESS;

cleanup:
    return ret;
}
