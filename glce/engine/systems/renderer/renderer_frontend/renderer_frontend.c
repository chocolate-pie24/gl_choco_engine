// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>    // TODO: remove this.

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/io_utils/fs_path.h"

#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_mesh_material_registry.h"
#include "engine/systems/renderer/resources/shaders/untextured_mesh_shader.h"
#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resources/pipelines/materials/untextured_mesh_material_pipeline.h"

#include "engine/systems/renderer/resource_manager/core/resource_manager_types.h"
#include "engine/systems/renderer/resource_manager/resource_manager.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_err_utils.h"

// TODO: DYNAMIC / STATICでそれぞれVBOを作る
struct renderer_frontend {
    // untextured mesh.
    untextured_mesh_shader_t* untextured_mesh_shader;
};

static renderer_frontend_result_t frontend_untextured_mesh_shader_create(const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, const resource_manager_t* resource_manager_, untextured_mesh_shader_t** out_shader_);

renderer_frontend_result_t renderer_frontend_create(subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, resource_manager_t* resource_manager_, renderer_frontend_t** out_renderer_frontend_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    subsystem_allocator_rollback_point_t rollback_point = { 0 };
    renderer_frontend_t* tmp_renderer_frontend = NULL;
    const vbo_manager_t* vbo_manager = NULL;

    // untextured_mesh
    untextured_mesh_shader_t* tmp_untextured_mesh_shader = NULL;

    bool rollback_point_created = false;
    vbo_manager_config_t vbo_manager_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_create", "resource_manager_")
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

    ret = frontend_untextured_mesh_shader_create(executable_directory_, shader_dir_, backend_context_, resource_manager_, &tmp_untextured_mesh_shader);
    if(RENDERER_FRONTEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_frontend_create(%s) - frontend_untextured_mesh_shader_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    tmp_renderer_frontend->untextured_mesh_shader = tmp_untextured_mesh_shader;

    // TODO: validation

    // Output.
    *out_renderer_frontend_ = tmp_renderer_frontend;
    tmp_untextured_mesh_shader = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED != ret && RENDERER_FRONTEND_SUCCESS != ret) {
        if(NULL != tmp_untextured_mesh_shader && NULL != backend_context_) {
            untextured_mesh_shader_destroy(backend_context_, &tmp_untextured_mesh_shader);
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

void renderer_frontend_deinitialize(renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_) {
    if(NULL == renderer_frontend_ || NULL == backend_context_) {
        return;
    }
    if(NULL != renderer_frontend_->untextured_mesh_shader) {
        untextured_mesh_shader_destroy(backend_context_, &renderer_frontend_->untextured_mesh_shader);
    }
}

// NOTE: renderer_backend_vao_unbindを行う場合renderer_backend_contextが必要となる、ただ、実行しなくても良いので当面は実行しない
renderer_frontend_result_t renderer_frontend_untextured_mesh_draw(const resource_manager_t* resource_manager_, const renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;
    const untextured_mesh_material_t* material = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "resource_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "model_matrix_")

    material = resource_manager_untextured_mesh_material_get(resource_manager_, material_id_);
    if(NULL == material) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - resource_manager_untextured_mesh_material_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    draw_range = resource_manager_untextured_mesh_draw_range_get(resource_manager_, geometry_id_);
    if(NULL == draw_range) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_geometry_registry_draw_range_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_use(backend_context_, renderer_frontend_->untextured_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_material_set(backend_context_, renderer_frontend_->untextured_mesh_shader, material);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_material_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_model_matrix_set(backend_context_, renderer_frontend_->untextured_mesh_shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_model_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_vao_bind(backend_context_, renderer_frontend_->untextured_mesh_shader);
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

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_render_update(const renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, const directional_light_t* directional_light_, const ambient_light_t* ambient_light_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    vec4f_t world_light_direction = { 0 };
    vec4f_t view_light_direction4 = { 0 };
    vec3f_t view_light_direction3 = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "projection_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(directional_light_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "directional_light_")
    IF_ARG_NULL_GOTO_CLEANUP(ambient_light_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "ambient_light_")

    ret_shader = untextured_mesh_shader_use(backend_context_, renderer_frontend_->untextured_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_projection_matrix_set(backend_context_, renderer_frontend_->untextured_mesh_shader, projection_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_projection_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_view_matrix_set(backend_context_, renderer_frontend_->untextured_mesh_shader, view_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_view_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    world_light_direction = vec4f_initialize(directional_light_->direction.elem[0], directional_light_->direction.elem[1], directional_light_->direction.elem[2], 0.0f);
    view_light_direction4 = mat4f_vec4f_mul(view_matrix_, world_light_direction);
    view_light_direction3 = vec3f_initialize(view_light_direction4.elem[0], view_light_direction4.elem[1], view_light_direction4.elem[2]);
    ret_shader = untextured_mesh_shader_directional_light_direction_set(backend_context_, renderer_frontend_->untextured_mesh_shader, view_light_direction3);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_directional_light_direction_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_directional_light_color_intensity_set(backend_context_, renderer_frontend_->untextured_mesh_shader, directional_light_->color, directional_light_->intensity);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_directional_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_ambient_light_color_intensity_set(backend_context_, renderer_frontend_->untextured_mesh_shader, ambient_light_->color, ambient_light_->intensity);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_ambient_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

static renderer_frontend_result_t frontend_untextured_mesh_shader_create(const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, const resource_manager_t* resource_manager_, untextured_mesh_shader_t** out_shader_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    untextured_mesh_shader_t* tmp_shader = NULL;
    const renderer_backend_vbo_t* untextured_mesh_vbo = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_manager_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "frontend_untextured_mesh_shader_create", "resource_manager_")
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

    untextured_mesh_vbo = resource_manager_untextured_mesh_vbo_get(resource_manager_);
    if(NULL == untextured_mesh_vbo) {
        ret = RENDERER_FRONTEND_DATA_CORRUPTED;
        ERROR_MESSAGE("renderer_frontend_create(%s) - Failed to get untextured mesh VBO.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_create(fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), backend_context_, untextured_mesh_vbo, &tmp_shader);
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
            untextured_mesh_shader_destroy(backend_context_, &tmp_shader);
        }
    }

    return ret;
}
