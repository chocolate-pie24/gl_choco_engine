// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/render_resources/untextured_mesh_render_resource.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/conversion/geometry_conversion.h"
#include "engine/systems/renderer/resources/shaders/untextured_mesh_shader.h"
#include "engine/systems/renderer/resources/materials/material_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/geometries/untextured_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_material_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resources/pipelines/materials/untextured_material_pipeline.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/render_resources/core/render_resource_err_utils.h"

struct untextured_mesh_render_resource {
    untextured_mesh_shader_t* shader;
    untextured_mesh_geometry_registry_t* geometry_registry;
    untextured_material_registry_t* material_registry;
};

static render_resource_result_t shader_create(const untextured_mesh_shader_config_t* config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_shader_t** out_shader_);
static bool is_valid_shallow(const untextured_mesh_render_resource_t* render_resource_);

render_resource_result_t untextured_mesh_render_resource_create(const untextured_mesh_shader_config_t* config_, size_t max_geometry_count_, size_t max_material_count_, renderer_backend_context_t* backend_context_, subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_render_resource_t** out_render_resource_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    untextured_mesh_render_resource_t* tmp_render_resource = NULL;
    untextured_mesh_shader_t* tmp_shader = NULL;
    untextured_mesh_geometry_registry_t* tmp_geometry_registry = NULL;
    untextured_material_registry_t* tmp_material_registry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "out_render_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_resource_, ret, RENDER_RESOURCE_BAD_OPERATION, render_resource_result_to_str(RENDER_RESOURCE_BAD_OPERATION), "untextured_mesh_render_resource_create", "*out_render_resource_")

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(untextured_mesh_render_resource_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_render_resource);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = render_resource_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - Failed to allocate untextured_mesh_render_resource_t instance.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret = shader_create(config_, backend_context_, executable_directory_, shader_dir_, &tmp_shader);
    if(RENDER_RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - shader_create failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_mesh_geometry_registry_create(max_geometry_count_, allocator_, &tmp_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = render_resource_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - untextured_mesh_geometry_registry_create failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_material_registry_create(max_material_count_, allocator_, &tmp_material_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = render_resource_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - untextured_material_registry_create failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    tmp_render_resource->shader = tmp_shader;
    tmp_render_resource->geometry_registry = tmp_geometry_registry;
    tmp_render_resource->material_registry = tmp_material_registry;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(tmp_render_resource)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - Postcondition validation failed for 'tmp_render_resource'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_render_resource_ = tmp_render_resource;
    tmp_render_resource = NULL;
    tmp_shader = NULL;
    tmp_geometry_registry = NULL;
    tmp_material_registry = NULL;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    if(RENDER_RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_geometry_registry && NULL != tmp_shader) {
            untextured_mesh_geometry_registry_deinitialize(tmp_geometry_registry, tmp_shader);
        }
        if(NULL != tmp_material_registry) {
            untextured_material_registry_deinitialize(tmp_material_registry);
        }
        if(NULL != tmp_shader) {
            untextured_mesh_shader_destroy(&tmp_shader);
        }
    }
    return ret;
}

void untextured_mesh_render_resource_deinitialize(untextured_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ERROR_MESSAGE("untextured_mesh_render_resource_deinitialize(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(RENDER_RESOURCE_DATA_CORRUPTED));
        return;
    }
#endif

    untextured_mesh_geometry_registry_deinitialize(render_resource_->geometry_registry, render_resource_->shader);
    untextured_material_registry_deinitialize(render_resource_->material_registry);
    untextured_mesh_shader_destroy(&render_resource_->shader);
}

render_resource_result_t untextured_mesh_render_resource_geometry_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_import_from_file", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_import_from_file", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_import_from_file(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_import_from_file(render_resource_->shader, render_resource_->geometry_registry, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_import_from_file(%s) - untextured_mesh_geometry_pipeline_import_from_file failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_import_from_file(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_geometry_release(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_release", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_release(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(render_resource_->shader, render_resource_->geometry_registry, geometry_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_release(%s) - untextured_mesh_geometry_pipeline_release failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_release(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_material_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_material_import_from_file", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_material_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_material_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_material_import_from_file", "out_material_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_material_import_from_file(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = untextured_material_pipeline_import_from_file(render_resource_->material_registry, resource_name_, resource_fullpath_, &tmp_material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("untextured_mesh_render_resource_material_import_from_file(%s) - untextured_material_pipeline_import_from_file failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_material_import_from_file(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_material_id_ = tmp_material_id;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_material_release(untextured_mesh_render_resource_t* render_resource_, uint16_t material_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_material_release", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_material_release(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = untextured_material_pipeline_release(render_resource_->material_registry, material_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("untextured_mesh_render_resource_material_release(%s) - untextured_material_pipeline_release failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_material_release(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_view_matrix_set(untextured_mesh_render_resource_t* render_resource_, const mat4x4f_t* view_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_view_matrix_set", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_view_matrix_set", "view_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_view_matrix_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_view_matrix_set(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_view_matrix_set(render_resource_->shader, view_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_view_matrix_set(%s) - untextured_mesh_shader_view_matrix_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_view_matrix_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_projection_matrix_set(untextured_mesh_render_resource_t* render_resource_, const mat4x4f_t* projection_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_projection_matrix_set", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_projection_matrix_set", "projection_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_projection_matrix_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_projection_matrix_set(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_projection_matrix_set(render_resource_->shader, projection_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_projection_matrix_set(%s) - untextured_mesh_shader_projection_matrix_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_projection_matrix_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_directional_light_direction_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t direction_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_directional_light_direction_set", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_direction_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_direction_set(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_directional_light_direction_set(render_resource_->shader, direction_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_direction_set(%s) - untextured_mesh_shader_directional_light_direction_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_direction_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_directional_light_color_intensity_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t color_, float intensity_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_directional_light_color_intensity_set", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_color_intensity_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_color_intensity_set(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_directional_light_color_intensity_set(render_resource_->shader, color_, intensity_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_color_intensity_set(%s) - untextured_mesh_shader_directional_light_color_intensity_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_directional_light_color_intensity_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_ambient_light_color_intensity_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t color_, float intensity_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_ambient_light_color_intensity_set", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_ambient_light_color_intensity_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_ambient_light_color_intensity_set(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_ambient_light_color_intensity_set(render_resource_->shader, color_, intensity_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_ambient_light_color_intensity_set(%s) - untextured_mesh_shader_ambient_light_color_intensity_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_ambient_light_color_intensity_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// NOTE: renderer_backend_vao_unbindを行う場合renderer_backend_contextが必要となる、ただ、実行しなくても良いので当面は実行しない
render_resource_result_t untextured_mesh_render_resource_draw(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;
    const untextured_material_t* material = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_draw", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_draw", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    material = untextured_material_registry_material_get(render_resource_->material_registry, material_id_);
    if(NULL == material) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_material_registry_material_get failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    draw_range = untextured_mesh_geometry_registry_draw_range_get(render_resource_->geometry_registry, geometry_id_);
    if(NULL == draw_range) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_mesh_geometry_registry_draw_range_get failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_mesh_shader_use failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_material_set(render_resource_->shader, material);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_mesh_shader_material_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_model_matrix_set(render_resource_->shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_mesh_shader_model_matrix_set failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_vao_bind(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_draw(%s) - untextured_mesh_shader_vao_bind failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    // TODO: renderer_backendにgl系APIを追加しOpenGL依存を消す
    glDrawArrays(GL_TRIANGLES, (GLint)draw_range->first_vertex_count, (GLint)draw_range->vertex_count);

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t untextured_mesh_render_resource_geometry_convert_to_aabb_3d(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb_3d = { 0 };
    const untextured_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_convert_to_aabb_3d", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_3d_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "untextured_mesh_render_resource_geometry_convert_to_aabb_3d", "out_aabb_3d_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_convert_to_aabb_3d(%s) - Precondition validation failed for 'render_resource_'.", render_resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    tmp_geometry = untextured_mesh_geometry_registry_geometry_get(render_resource_->geometry_registry, geometry_id_);
    if(NULL == tmp_geometry) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_convert_to_aabb_3d(%s) - untextured_mesh_geometry_registry_geometry_get failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_geometry = geometry_conversion_untextured_mesh_geometry_to_aabb_3d(tmp_geometry, &tmp_aabb_3d);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = render_resource_result_convert_geometry(ret_geometry);
        ERROR_MESSAGE("untextured_mesh_render_resource_geometry_convert_to_aabb_3d(%s) - geometry_conversion_untextured_mesh_geometry_to_aabb_3d failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    // aabb_3d_tのcanonical validator追加後にtmp_aabb_3dのvalidationを実行する

    *out_aabb_3d_ = tmp_aabb_3d;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

bool untextured_mesh_render_resource_is_valid(const untextured_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return false;
    }
    if(!is_valid_shallow(render_resource_)) {
        return false;
    }
    if(!untextured_mesh_geometry_registry_is_valid(render_resource_->geometry_registry)) {
        return false;
    }
    if(!untextured_material_registry_is_valid(render_resource_->material_registry)) {
        return false;
    }

    // TODO: untextured_mesh_shader_is_valid追加後にrender_resource_->shaderのvalidationを追加する
    return true;
}

static render_resource_result_t shader_create(const untextured_mesh_shader_config_t* config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_shader_t** out_shader_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    untextured_mesh_shader_t* tmp_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_shader_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_result_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "out_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_, ret, RENDER_RESOURCE_BAD_OPERATION, render_resource_result_to_str(RENDER_RESOURCE_BAD_OPERATION), "shader_create", "*out_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = render_resource_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("shader_create(%s) - fs_path_create failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = render_resource_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("shader_create(%s) - fs_path_create failed.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_create(backend_context_, fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), config_, &tmp_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_result_convert_shader(ret_shader);
        ERROR_MESSAGE("shader_create(%s) - Failed to create textured mesh shader.", render_resource_result_to_str(ret));
        goto cleanup;
    }

    *out_shader_ = tmp_shader;
    tmp_shader = NULL;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    if(RENDER_RESOURCE_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_shader) {
            untextured_mesh_shader_destroy(&tmp_shader);
        }
    }

    return ret;
}

static bool is_valid_shallow(const untextured_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return false;
    }
    if(NULL == render_resource_->geometry_registry) {
        return false;
    }
    if(NULL == render_resource_->material_registry) {
        return false;
    }
    if(NULL == render_resource_->shader) {
        return false;
    }
    return true;
}
