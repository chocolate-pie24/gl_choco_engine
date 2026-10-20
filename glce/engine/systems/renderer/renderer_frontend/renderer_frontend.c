// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include <stddef.h>

#include <GL/glew.h>    // TODO: remove this.

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/renderer/config/renderer_config.h" // TODO: remove this.

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/registries/geometries/untextured_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_material_registry.h"
#include "engine/systems/renderer/resources/shaders/untextured_mesh_shader.h"
#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resources/pipelines/materials/untextured_material_pipeline.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_err_utils.h"

struct renderer_frontend {
    renderer_backend_context_t* renderer_backend_context;

    // untextured mesh.
    untextured_mesh_geometry_registry_t* untextured_mesh_geometry_registry;
    untextured_material_registry_t* untextured_mesh_material_registry;
    vbo_manager_t* untextured_mesh_vbo_manager;
    untextured_mesh_shader_t* untextured_mesh_shader;
};

static renderer_frontend_result_t frontend_untextured_mesh_shader_create(const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, vbo_manager_t* vbo_manager_, untextured_mesh_shader_t** out_shader_);

renderer_frontend_result_t renderer_frontend_create(const renderer_frontend_config_t* config_, subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, renderer_frontend_t** out_renderer_frontend_) {

    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    subsystem_allocator_rollback_point_t rollback_point = { 0 };
    renderer_frontend_t* tmp_renderer_frontend = NULL;
    renderer_backend_context_t* tmp_renderer_backend_context = NULL;

    // untextured_mesh
    untextured_mesh_shader_t* tmp_untextured_mesh_shader = NULL;
    untextured_mesh_geometry_registry_t* tmp_untextured_mesh_geometry_registry = NULL;
    untextured_material_registry_t* tmp_untextured_material_registry = NULL;    // TODO: untextured_mesh_materialに名称変更を検討する
    vbo_manager_t* tmp_untextured_mesh_vbo_manager = NULL;

    bool rollback_point_created = false;
    vbo_manager_config_t vbo_manager_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "out_renderer_frontend_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_renderer_frontend_, ret, RENDERER_FRONTEND_BAD_OPERATION, renderer_frontend_result_to_str(RENDERER_FRONTEND_BAD_OPERATION), "renderer_frontend_create", "*out_renderer_frontend_")

    // Prepare.
    ret_subsystem_allocator = subsystem_allocator_rollback_point_get(allocator_, &rollback_point);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = renderer_frontend_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("renderer_frontend_create(%s) - subsystem_allocator_rollback_point_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    rollback_point_created = true;

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(renderer_frontend_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_renderer_frontend);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = renderer_frontend_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("renderer_frontend_create(%s) - Failed to allocate renderer_frontend_t instance.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_create(allocator_, &tmp_renderer_backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = renderer_frontend_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("renderer_frontend_create(%s) - Failed to create renderer backend.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // untextured_mesh生成
    vbo_manager_config.base_align = config_->untextured_mesh.vbo.base_align;
    vbo_manager_config.buffer_usage = config_->untextured_mesh.vbo.buffer_usage;
    vbo_manager_config.max_allocation_count = config_->untextured_mesh.vbo.max_allocation_count;
    vbo_manager_config.vbo_size = config_->untextured_mesh.vbo.vbo_size;
    ret_buffer_manager = vbo_manager_create(tmp_renderer_backend_context, &vbo_manager_config, &tmp_untextured_mesh_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = renderer_frontend_result_convert_buffer_manager(ret_buffer_manager);
        ERROR_MESSAGE("renderer_frontend_create(%s) - buffer manager create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_material_registry_create(config_->untextured_mesh.max_material_count, allocator_, &tmp_untextured_material_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = renderer_frontend_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("renderer_frontend_create(%s) - untextured_material_registry_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_mesh_geometry_registry_create(config_->untextured_mesh.max_geometry_count, allocator_, &tmp_untextured_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = renderer_frontend_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("renderer_frontend_create(%s) - untextured_mesh_geometry_registry_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = frontend_untextured_mesh_shader_create(executable_directory_, shader_dir_, tmp_renderer_backend_context, tmp_untextured_mesh_vbo_manager, &tmp_untextured_mesh_shader);
    if(RENDERER_FRONTEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_frontend_create(%s) - frontend_untextured_mesh_shader_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    tmp_renderer_frontend->renderer_backend_context = tmp_renderer_backend_context;
    tmp_renderer_frontend->untextured_mesh_geometry_registry = tmp_untextured_mesh_geometry_registry;
    tmp_renderer_frontend->untextured_mesh_material_registry = tmp_untextured_material_registry;
    tmp_renderer_frontend->untextured_mesh_shader = tmp_untextured_mesh_shader;
    tmp_renderer_frontend->untextured_mesh_vbo_manager = tmp_untextured_mesh_vbo_manager;

    // TODO: validation

    // Output.
    *out_renderer_frontend_ = tmp_renderer_frontend;
    tmp_untextured_mesh_geometry_registry = NULL;
    tmp_untextured_material_registry = NULL;
    tmp_untextured_mesh_shader = NULL;
    tmp_untextured_mesh_vbo_manager = NULL;
    tmp_renderer_backend_context = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED != ret && RENDERER_FRONTEND_SUCCESS != ret) {
        if(NULL != tmp_untextured_mesh_shader) {
            untextured_mesh_shader_destroy(&tmp_untextured_mesh_shader);
        }
        if(NULL != tmp_untextured_mesh_geometry_registry && NULL != tmp_untextured_mesh_vbo_manager) {
            untextured_mesh_geometry_registry_deinitialize(tmp_untextured_mesh_geometry_registry, tmp_untextured_mesh_vbo_manager);
        }
        if(NULL != tmp_untextured_material_registry) {
            untextured_material_registry_deinitialize(tmp_untextured_material_registry);
        }
        if(NULL != tmp_untextured_mesh_vbo_manager) {
            vbo_manager_destroy(&tmp_untextured_mesh_vbo_manager);
        }
        if(NULL != tmp_renderer_backend_context) {
            renderer_backend_deinitialize(tmp_renderer_backend_context);
        }
        if(rollback_point_created) {
            ret_subsystem_allocator = subsystem_allocator_rollback(allocator_, &rollback_point);
            if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
                ret = RENDERER_FRONTEND_DATA_CORRUPTED;
                ERROR_MESSAGE("renderer_frontend_create(%s) - subsystem_allocator rollback failed.", renderer_frontend_result_to_str(ret));
                goto cleanup;
            }
        }
    }
    return ret;
}

void renderer_frontend_deinitialize(renderer_frontend_t* renderer_frontend_) {
    if(NULL == renderer_frontend_) {
        return;
    }
    if(NULL != renderer_frontend_->untextured_mesh_shader) {
        untextured_mesh_shader_destroy(&renderer_frontend_->untextured_mesh_shader);
    }
    if(NULL != renderer_frontend_->untextured_mesh_geometry_registry && NULL != renderer_frontend_->untextured_mesh_vbo_manager) {
        untextured_mesh_geometry_registry_deinitialize(renderer_frontend_->untextured_mesh_geometry_registry, renderer_frontend_->untextured_mesh_vbo_manager);
    }
    if(NULL != renderer_frontend_->untextured_mesh_material_registry) {
        untextured_material_registry_deinitialize(renderer_frontend_->untextured_mesh_material_registry);
    }
    if(NULL != renderer_frontend_->untextured_mesh_vbo_manager) {
        vbo_manager_destroy(&renderer_frontend_->untextured_mesh_vbo_manager);
    }
    if(NULL != renderer_frontend_->renderer_backend_context) {
        renderer_backend_deinitialize(renderer_frontend_->renderer_backend_context);
    }
}

// NOTE: renderer_backend_vao_unbindを行う場合renderer_backend_contextが必要となる、ただ、実行しなくても良いので当面は実行しない
renderer_frontend_result_t renderer_frontend_untextured_mesh_draw(renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;
    const untextured_material_t* material = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "model_matrix_")

    material = untextured_material_registry_material_get(renderer_frontend_->untextured_mesh_material_registry, material_id_);
    if(NULL == material) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_material_registry_material_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    draw_range = untextured_mesh_geometry_registry_draw_range_get(renderer_frontend_->untextured_mesh_geometry_registry, geometry_id_);
    if(NULL == draw_range) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_geometry_registry_draw_range_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_use(renderer_frontend_->untextured_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_material_set(renderer_frontend_->untextured_mesh_shader, material);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_material_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_model_matrix_set(renderer_frontend_->untextured_mesh_shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_model_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_vao_bind(renderer_frontend_->untextured_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_vao_bind failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // TODO: renderer_backendにgl系APIを追加しOpenGL依存を消す
    if(draw_range->first_vertex_count > (size_t)INT32_MAX || draw_range->vertex_count > (size_t)INT32_MAX) {
        ret = RENDERER_FRONTEND_LIMIT_EXCEEDED;
        goto cleanup;
    }
    if(draw_range->first_vertex_count > (SIZE_MAX - draw_range->vertex_count)) {
        ret = RENDERER_FRONTEND_OVERFLOW;
        goto cleanup;
    }
    glDrawArrays(GL_TRIANGLES, (GLint)draw_range->first_vertex_count, (GLsizei)draw_range->vertex_count);

    ret = RENDERER_FRONTEND_SUCCESS;;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_geometry_import_from_file(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_geometry_import_from_file", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_geometry_import_from_file", "out_geometry_id_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_import_from_file(renderer_frontend_->untextured_mesh_vbo_manager, renderer_frontend_->untextured_mesh_geometry_registry, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_geometry_import_from_file(%s) - untextured_mesh_geometry_pipeline_import_from_file failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_geometry_release(renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_geometry_release", "render_resource_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(renderer_frontend_->untextured_mesh_vbo_manager, renderer_frontend_->untextured_mesh_geometry_registry, geometry_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_geometry_release(%s) - untextured_mesh_geometry_pipeline_release failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_material_import_from_file(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_material_import_from_file", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_material_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_material_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_material_import_from_file", "out_material_id_")

    ret_resource_pipeline = untextured_material_pipeline_import_from_file(renderer_frontend_->untextured_mesh_material_registry, resource_name_, resource_fullpath_, &tmp_material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_material_import_from_file(%s) - untextured_material_pipeline_import_from_file failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    *out_material_id_ = tmp_material_id;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_material_release(renderer_frontend_t* renderer_frontend_, uint16_t material_id_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_material_release", "renderer_frontend_")

    ret_resource_pipeline = untextured_material_pipeline_release(renderer_frontend_->untextured_mesh_material_registry, material_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_material_release(%s) - untextured_material_pipeline_release failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_shader_view_matrix_set(renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_view_matrix_set", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_view_matrix_set", "view_matrix_")

    ret_shader = untextured_mesh_shader_view_matrix_set(renderer_frontend_->untextured_mesh_shader, view_matrix_, should_transpose_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_shader_view_matrix_set(%s) - untextured_mesh_shader_view_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_shader_projection_matrix_set(renderer_frontend_t* renderer_frontend_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_projection_matrix_set", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_projection_matrix_set", "projection_matrix_")

    ret_shader = untextured_mesh_shader_projection_matrix_set(renderer_frontend_->untextured_mesh_shader, projection_matrix_, should_transpose_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_shader_projection_matrix_set(%s) - untextured_mesh_shader_projection_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_shader_directional_light_direction_set(renderer_frontend_t* renderer_frontend_, vec3f_t direction_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_directional_light_direction_set", "renderer_frontend_")

    ret_shader = untextured_mesh_shader_directional_light_direction_set(renderer_frontend_->untextured_mesh_shader, direction_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_shader_directional_light_direction_set(%s) - untextured_mesh_shader_directional_light_direction_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_shader_directional_light_color_intensity_set(renderer_frontend_t* renderer_frontend_, vec3f_t color_, float intensity_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_directional_light_color_intensity_set", "renderer_frontend_")

    ret_shader = untextured_mesh_shader_directional_light_color_intensity_set(renderer_frontend_->untextured_mesh_shader, color_, intensity_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_shader_directional_light_color_intensity_set(%s) - untextured_mesh_shader_directional_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_shader_ambient_light_color_intensity_set(renderer_frontend_t* renderer_frontend_, vec3f_t color_, float intensity_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_shader_ambient_light_color_intensity_set", "renderer_frontend_")

    ret_shader = untextured_mesh_shader_ambient_light_color_intensity_set(renderer_frontend_->untextured_mesh_shader, color_, intensity_);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_shader_ambient_light_color_intensity_set(%s) - untextured_mesh_shader_ambient_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

static renderer_frontend_result_t frontend_untextured_mesh_shader_create(const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, vbo_manager_t* vbo_manager_, untextured_mesh_shader_t** out_shader_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    untextured_mesh_shader_t* tmp_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(out_shader_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "out_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_, ret, RENDERER_FRONTEND_BAD_OPERATION, renderer_frontend_result_to_str(RENDERER_FRONTEND_BAD_OPERATION), "frontend_untextured_mesh_shader_create", "*out_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("frontend_untextured_mesh_shader_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("frontend_untextured_mesh_shader_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_create(fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), backend_context_, vbo_manager_, &tmp_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("frontend_untextured_mesh_shader_create(%s) - Failed to create textured mesh shader.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    *out_shader_ = tmp_shader;
    tmp_shader = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_shader) {
            untextured_mesh_shader_destroy(&tmp_shader);
        }
    }

    return ret;
}
