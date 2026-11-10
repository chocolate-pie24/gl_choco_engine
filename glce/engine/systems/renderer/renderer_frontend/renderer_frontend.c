// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>    // TODO: remove this.

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_path.h"

#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/geometries/conversion/geometry_conversion.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/shaders/line_mesh_shader.h"
#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/shaders/untextured_mesh_shader.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/resources/registries/geometries/line_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/pipelines/geometries/line_mesh_geometry_pipeline.h"

#include "engine/systems/renderer/resources/registries/geometries/untextured_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_mesh_material_registry.h"
#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resources/pipelines/materials/untextured_mesh_material_pipeline.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"
#include "engine/systems/renderer/render_packet/render_packet.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_err_utils.h"

/*
 * Module validation policy.
 * Renderer Frontendは、ResourceのSemantic Validationを各下位モジュールへ委譲する。
 * create()およびdeinitialize()では、Frontend全体のOwnership Closureを検証するため、DEBUG / TESTにおいてCanonical Validatorを実行する。
 * その他の公開APIでは、Frontend全体のCanonical Validatorを実行しない。
 * 各APIは自身のPrecondition、Orchestrationに固有の整合条件、RollbackおよびCommitの成立条件を検証する。
 * RELEASEでは、APIを安全かつ有界に実行するために必要な条件を検証し、内部Invariantの診断を目的とするCanonical Validationは実行しない。
 */

// 将来的にrenderer_frontend/render_resourcesに移しても良い(ただしapplicationには非公開とし、窓口はあくまでrenderer_frontendにする)
typedef struct {
    line_mesh_shader_t* shader;
    line_mesh_geometry_registry_t* geometry_registry;
    vbo_manager_t* vbo_manager;
} line_mesh_render_resource_t;

typedef struct {
    untextured_mesh_shader_t* shader;
    untextured_mesh_geometry_registry_t* geometry_registry;
    untextured_mesh_material_registry_t* material_registry;
    vbo_manager_t* vbo_manager;
} untextured_mesh_render_resource_t;

// TODO: DYNAMIC / STATICでそれぞれVBOを作る
struct renderer_frontend {
    renderer_backend_context_t* renderer_backend_context;

    line_mesh_render_resource_t* line_mesh_render_resource;
    untextured_mesh_render_resource_t* untextured_mesh_render_resource;
};

static renderer_frontend_result_t line_mesh_render_resource_create(subsystem_allocator_t* allocator_, const line_mesh_resource_config_t *config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, line_mesh_render_resource_t** out_render_resource_);
static void line_mesh_render_resource_deinitialize(renderer_backend_context_t* backend_context_, line_mesh_render_resource_t* render_resource_);

static renderer_frontend_result_t untextured_mesh_render_resource_create(subsystem_allocator_t* allocator_, const untextured_mesh_resource_config_t *config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_render_resource_t** out_render_resource_);
static void untextured_mesh_render_resource_deinitialize(renderer_backend_context_t* backend_context_, untextured_mesh_render_resource_t* render_resource_);

// Validators.
static bool untextured_mesh_render_resource_is_valid(const renderer_backend_context_t* backend_context_, const untextured_mesh_render_resource_t* render_resource_);
static bool line_mesh_render_resource_is_valid(const renderer_backend_context_t* backend_context_, const line_mesh_render_resource_t* render_resource_);

renderer_frontend_result_t renderer_frontend_create(const renderer_frontend_config_t* config_, subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, renderer_frontend_t** out_renderer_frontend_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    subsystem_allocator_rollback_point_t rollback_point = { 0 };
    renderer_frontend_t* tmp_renderer_frontend = NULL;
    renderer_backend_context_t* tmp_renderer_backend = NULL;

    line_mesh_render_resource_t* tmp_line_mesh_render_resource = NULL;
    untextured_mesh_render_resource_t* tmp_untextured_mesh_render_resource = NULL;

    bool rollback_point_created = false;

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

    ret_renderer_backend = renderer_backend_create(allocator_, &tmp_renderer_backend);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = renderer_frontend_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("renderer_frontend_create(%s) - Failed to create renderer backend.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = line_mesh_render_resource_create(allocator_, &config_->line_mesh_resource_config, tmp_renderer_backend, executable_directory_, shader_dir_, &tmp_line_mesh_render_resource);
    if(RENDERER_FRONTEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_frontend_create(%s) - line_mesh_render_resource_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = untextured_mesh_render_resource_create(allocator_, &config_->untextured_mesh_resource_config, tmp_renderer_backend, executable_directory_, shader_dir_, &tmp_untextured_mesh_render_resource);
    if(RENDERER_FRONTEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_frontend_create(%s) - untextured_mesh_render_resource_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    tmp_renderer_frontend->renderer_backend_context = tmp_renderer_backend;
    tmp_renderer_frontend->line_mesh_render_resource = tmp_line_mesh_render_resource;
    tmp_renderer_frontend->untextured_mesh_render_resource = tmp_untextured_mesh_render_resource;

    tmp_renderer_backend = NULL;
    tmp_line_mesh_render_resource = NULL;
    tmp_untextured_mesh_render_resource = NULL;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!renderer_frontend_is_valid(tmp_renderer_frontend)) {
        ret = RENDERER_FRONTEND_DATA_CORRUPTED;
        ERROR_MESSAGE("renderer_frontend_create(%s) - Result validation failed for 'tmp_renderer_frontend'.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_renderer_frontend_ = tmp_renderer_frontend;
    tmp_renderer_frontend = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED == ret || RENDERER_FRONTEND_SUCCESS == ret) {
        return ret;
    }

    if(NULL != tmp_line_mesh_render_resource && NULL != tmp_renderer_backend) {
        line_mesh_render_resource_deinitialize(tmp_renderer_backend, tmp_line_mesh_render_resource);
    }
    if(NULL != tmp_untextured_mesh_render_resource && NULL != tmp_renderer_backend) {
        untextured_mesh_render_resource_deinitialize(tmp_renderer_backend, tmp_untextured_mesh_render_resource);
    }
    if(NULL != tmp_renderer_backend) {
        renderer_backend_deinitialize(tmp_renderer_backend);
    }

    if(rollback_point_created) {
        ret_subsystem_allocator = subsystem_allocator_rollback(allocator_, &rollback_point);
        if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
            ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            ERROR_MESSAGE("renderer_frontend_create(%s) - subsystem_allocator rollback failed.", renderer_frontend_result_to_str(ret));
            goto cleanup;
        }
    }
    return ret;
}

void renderer_frontend_deinitialize(renderer_frontend_t* renderer_frontend_) {
    if(NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!renderer_frontend_is_valid(renderer_frontend_)) {
        ERROR_MESSAGE("renderer_frontend_deinitialize(%s) - Precondition validation failed for renderer_frontend_''.", renderer_frontend_result_to_str(RENDERER_FRONTEND_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL != renderer_frontend_->line_mesh_render_resource && NULL != renderer_frontend_->renderer_backend_context) {
        line_mesh_render_resource_deinitialize(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource);
    }
    if(NULL != renderer_frontend_->untextured_mesh_render_resource && NULL != renderer_frontend_->renderer_backend_context) {
        untextured_mesh_render_resource_deinitialize(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource);
    }
    if(NULL != renderer_frontend_->renderer_backend_context) {
        renderer_backend_deinitialize(renderer_frontend_->renderer_backend_context);
    }
}

renderer_frontend_result_t renderer_frontend_draw_packet(render_packet_t* render_packet_, renderer_frontend_t* renderer_frontend_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    bool line_mesh_read_completed = false;
    // bool point_mesh_read_completed = false;
    // bool ui_mesh_read_completed = false;
    bool untextured_mesh_read_completed = false;

    line_mesh_t tmp_line_mesh = { 0 };
    // point_mesh_t tmp_point_mesh = { 0 };
    // ui_mesh_t tmp_ui_mesh = { 0 };
    untextured_mesh_t tmp_untextured_mesh = { 0 };

    mat4x4f_t tmp_model_matrix = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_draw_packet", "render_packet_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_draw_packet", "renderer_frontend_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!renderer_frontend_is_valid(renderer_frontend_)) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - Precondition validation failed for 'renderer_frontend_'.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Line mesh.
    while(!line_mesh_read_completed) {
        ret_render_packet = render_packet_line_mesh_read_state_completed(render_packet_, &line_mesh_read_completed);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = renderer_frontend_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_line_mesh_read_state_completed failed.", renderer_frontend_result_to_str(ret));
            goto cleanup;
        }
        if(!line_mesh_read_completed) {
            if(!render_packet_line_mesh_read_next(render_packet_, &tmp_line_mesh, &tmp_model_matrix)) {
                ret = RENDERER_FRONTEND_DATA_CORRUPTED;
                ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_line_mesh_read_next failed.", renderer_frontend_result_to_str(ret));
                goto cleanup;
            }

            ret = renderer_frontend_line_mesh_draw(renderer_frontend_, &tmp_line_mesh, &tmp_model_matrix);
            if(RENDERER_FRONTEND_SUCCESS != ret) {
                if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret) {
                    ret = RENDERER_FRONTEND_DATA_CORRUPTED;
                }
                ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - renderer_frontend_line_mesh_draw failed.", renderer_frontend_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    // Point mesh.
    // while(!point_mesh_read_completed) {
    //     ret_render_packet = render_packet_point_mesh_read_state_completed(render_packet_, &point_mesh_read_completed);
    //     if(RENDER_PACKET_SUCCESS != ret_render_packet) {
    //         ret = renderer_frontend_result_convert_render_packet(ret_render_packet);
    //         if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
    //             ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //         }
    //         ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_point_mesh_read_state_completed failed.", renderer_frontend_result_to_str(ret));
    //         goto cleanup;
    //     }
    //     if(!point_mesh_read_completed) {
    //         if(!render_packet_point_mesh_read_next(render_packet_, &tmp_point_mesh, &tmp_model_matrix)) {
    //             ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //             ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_point_mesh_read_next failed.", renderer_frontend_result_to_str(ret));
    //             goto cleanup;
    //         }

    //         ret = renderer_frontend_point_mesh_draw(renderer_frontend_, &tmp_point_mesh, &tmp_model_matrix);
    //         if(RENDERER_FRONTEND_SUCCESS != ret) {
    //             if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret) {
    //                 ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //             }
    //             ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - renderer_frontend_point_mesh_draw failed.", renderer_frontend_result_to_str(ret));
    //             goto cleanup;
    //         }
    //     }
    // }

    // UI mesh.
    // while(!ui_mesh_read_completed) {
    //     ret_render_packet = render_packet_ui_mesh_read_state_completed(render_packet_, &ui_mesh_read_completed);
    //     if(RENDER_PACKET_SUCCESS != ret_render_packet) {
    //         ret = renderer_frontend_result_convert_render_packet(ret_render_packet);
    //         if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
    //             ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //         }
    //         ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_ui_mesh_read_state_completed failed.", renderer_frontend_result_to_str(ret));
    //         goto cleanup;
    //     }
    //     if(!ui_mesh_read_completed) {
    //         if(!render_packet_ui_mesh_read_next(render_packet_, &tmp_ui_mesh, &tmp_model_matrix)) {
    //             ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //             ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_ui_mesh_read_next failed.", renderer_frontend_result_to_str(ret));
    //             goto cleanup;
    //         }

    //         ret = renderer_frontend_ui_mesh_draw(renderer_frontend_, &tmp_ui_mesh, &tmp_model_matrix);
    //         if(RENDERER_FRONTEND_SUCCESS != ret) {
    //             if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret) {
    //                 ret = RENDERER_FRONTEND_DATA_CORRUPTED;
    //             }
    //             ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - renderer_frontend_ui_mesh_draw failed.", renderer_frontend_result_to_str(ret));
    //             goto cleanup;
    //         }
    //     }
    // }

    // Untextured mesh.
    while(!untextured_mesh_read_completed) {
        ret_render_packet = render_packet_untextured_mesh_read_state_completed(render_packet_, &untextured_mesh_read_completed);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = renderer_frontend_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_untextured_mesh_read_state_completed failed.", renderer_frontend_result_to_str(ret));
            goto cleanup;
        }

        if(!untextured_mesh_read_completed) {
            if(!render_packet_untextured_mesh_read_next(render_packet_, &tmp_untextured_mesh, &tmp_model_matrix)) {
                ret = RENDERER_FRONTEND_DATA_CORRUPTED;
                ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - render_packet_untextured_mesh_read_next failed.", renderer_frontend_result_to_str(ret));
                goto cleanup;
            }

            ret = renderer_frontend_untextured_mesh_draw(renderer_frontend_, &tmp_untextured_mesh, &tmp_model_matrix);
            if(RENDERER_FRONTEND_SUCCESS != ret) {
                if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret) {
                    ret = RENDERER_FRONTEND_DATA_CORRUPTED;
                }
                ERROR_MESSAGE("renderer_frontend_draw_packet(%s) - renderer_frontend_untextured_mesh_draw failed.", renderer_frontend_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_draw(renderer_frontend_t* renderer_frontend_, const untextured_mesh_t* mesh_, const mat4x4f_t* model_matrix_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;
    const untextured_mesh_material_t* material = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_draw", "model_matrix_")

    material = untextured_mesh_material_registry_material_get(renderer_frontend_->untextured_mesh_render_resource->material_registry, mesh_->material_id);
    if(NULL == material) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_material_registry_material_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    draw_range = untextured_mesh_geometry_registry_draw_range_get(renderer_frontend_->untextured_mesh_render_resource->geometry_registry, mesh_->geometry_id);
    if(NULL == draw_range) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_geometry_registry_draw_range_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_use(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_material_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, material);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_material_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_model_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_draw(%s) - untextured_mesh_shader_model_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_vao_bind(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader);
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

renderer_frontend_result_t renderer_frontend_untextured_mesh_render_update(renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, const directional_light_t* directional_light_, const ambient_light_t* ambient_light_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    vec4f_t world_light_direction = { 0 };
    vec4f_t view_light_direction4 = { 0 };
    vec3f_t view_light_direction3 = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "projection_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(directional_light_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "directional_light_")
    IF_ARG_NULL_GOTO_CLEANUP(ambient_light_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_render_update", "ambient_light_")

    ret_shader = untextured_mesh_shader_use(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_projection_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, projection_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_projection_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_view_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, view_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_view_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    world_light_direction = vec4f_initialize(directional_light_->direction.elem[0], directional_light_->direction.elem[1], directional_light_->direction.elem[2], 0.0f);
    view_light_direction4 = mat4f_vec4f_mul(view_matrix_, world_light_direction);
    view_light_direction3 = vec3f_initialize(view_light_direction4.elem[0], view_light_direction4.elem[1], view_light_direction4.elem[2]);
    ret_shader = untextured_mesh_shader_directional_light_direction_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, view_light_direction3);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_directional_light_direction_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_directional_light_color_intensity_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, directional_light_->color, directional_light_->intensity);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_directional_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_ambient_light_color_intensity_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource->shader, ambient_light_->color, ambient_light_->intensity);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_render_update(%s) - untextured_mesh_shader_ambient_light_color_intensity_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_import(renderer_frontend_t* renderer_frontend_, const untextured_mesh_config_t* config_, const char* asset_root_path_, untextured_mesh_t* out_mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;
    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    fs_path_t* fs_path = NULL;
    uint16_t tmp_geometry_id = 0;
    uint16_t tmp_material_id = 0;

    bool geometry_imported = false;
    bool material_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "out_mesh_")
    if('\0' == asset_root_path_[0]) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - Provided asset root path is not valid.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fs_path, asset_root_path_, choco_string_c_str(config_->geometry_subdirectory), choco_string_c_str(config_->geometry_resource_name), choco_string_c_str(config_->geometry_extension));
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    ret_resource_pipeline = untextured_mesh_geometry_pipeline_import_from_file(renderer_frontend_->untextured_mesh_render_resource->vbo_manager, renderer_frontend_->untextured_mesh_render_resource->geometry_registry, choco_string_c_str(config_->geometry_resource_name), fs_path_fullpath_get(fs_path), &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_geometry_pipeline_import_from_file failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    fs_path_destroy(&fs_path);
    geometry_imported = true;

    // material
    ret_fs_path = fs_path_create(&fs_path, asset_root_path_, choco_string_c_str(config_->material_subdirectory), choco_string_c_str(config_->material_resource_name), choco_string_c_str(config_->material_extension));
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    ret_resource_pipeline = untextured_mesh_material_pipeline_import_from_file(renderer_frontend_->untextured_mesh_render_resource->material_registry, choco_string_c_str(config_->material_resource_name), fs_path_fullpath_get(fs_path), &tmp_material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_material_pipeline_import_from_file failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    fs_path_destroy(&fs_path);
    material_imported = true;

    out_mesh_->geometry_id = tmp_geometry_id;
    out_mesh_->material_id = tmp_material_id;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED == ret || RENDERER_FRONTEND_SUCCESS == ret) {
        return ret;
    }
    if(NULL != fs_path) {
        fs_path_destroy(&fs_path);
    }
    if(geometry_imported) {
        ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->vbo_manager, renderer_frontend_->untextured_mesh_render_resource->geometry_registry, tmp_geometry_id);
        if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
            ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_geometry_pipeline_release failed.", renderer_frontend_result_to_str(ret));
            return ret;
        }
    }
    if(material_imported) {
        ret_resource_pipeline = untextured_mesh_material_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->material_registry, tmp_material_id);
        if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
            ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_material_pipeline_release failed.", renderer_frontend_result_to_str(ret));
            return ret;
        }
    }
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_import_from_vertices(renderer_frontend_t* renderer_frontend_, const char* geometry_resource_name_, const char* material_resource_name_, const point_normal_vertex_t* vertices_, size_t vertex_count_, const untextured_mesh_material_t* material_, untextured_mesh_t* out_mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;
    uint16_t tmp_material_id = 0;

    bool geometry_imported = false;
    bool material_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "geometry_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(material_resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "material_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(material_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "material_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_import", "out_mesh_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_import_from_vertices(renderer_frontend_->untextured_mesh_render_resource->vbo_manager, renderer_frontend_->untextured_mesh_render_resource->geometry_registry, geometry_resource_name_, vertices_, vertex_count_, &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_geometry_pipeline_import_from_vertices failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    geometry_imported = true;

    ret_resource_pipeline = untextured_mesh_material_pipeline_import_from_material(renderer_frontend_->untextured_mesh_render_resource->material_registry, material_resource_name_, material_, &tmp_material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_material_pipeline_import_from_material failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    material_imported = true;

    out_mesh_->geometry_id = tmp_geometry_id;
    out_mesh_->material_id = tmp_material_id;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED == ret || RENDERER_FRONTEND_SUCCESS == ret) {
        return ret;
    }
    if(geometry_imported) {
        ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->vbo_manager, renderer_frontend_->untextured_mesh_render_resource->geometry_registry, tmp_geometry_id);
        if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
            ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_geometry_pipeline_release failed.", renderer_frontend_result_to_str(ret));
            return ret;
        }
    }
    if(material_imported) {
        ret_resource_pipeline = untextured_mesh_material_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->material_registry, tmp_material_id);
        if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
            ret = RENDERER_FRONTEND_DATA_CORRUPTED;
            ERROR_MESSAGE("renderer_frontend_untextured_mesh_import(%s) - untextured_mesh_material_pipeline_release failed.", renderer_frontend_result_to_str(ret));
            return ret;
        }
    }
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_release(renderer_frontend_t* renderer_frontend_, const untextured_mesh_t* mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_release", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_release", "mesh_")

    ret_resource_pipeline = untextured_mesh_geometry_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->vbo_manager, renderer_frontend_->untextured_mesh_render_resource->geometry_registry, mesh_->geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_release(%s) - untextured_mesh_geometry_pipeline_release failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_pipeline = untextured_mesh_material_pipeline_release(renderer_frontend_->untextured_mesh_render_resource->material_registry, mesh_->material_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = RENDERER_FRONTEND_DATA_CORRUPTED; // GeometryのReleaseに成功したのにMaterialのReleaseに失敗するのはDATA_CORRUPTED
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_release(%s) - untextured_mesh_material_pipeline_release failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_untextured_mesh_local_aabb_get(const renderer_frontend_t* renderer_frontend_, const untextured_mesh_t* mesh_, aabb_3d_t* out_aabb_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;

    const untextured_mesh_geometry_t* tmp_geometry = NULL;
    aabb_3d_t tmp_aabb = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_local_aabb_get", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_local_aabb_get", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_untextured_mesh_local_aabb_get", "out_aabb_")

    tmp_geometry = untextured_mesh_geometry_registry_geometry_get(renderer_frontend_->untextured_mesh_render_resource->geometry_registry, mesh_->geometry_id);
    if(NULL == tmp_geometry) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_local_aabb_get(%s) - untextured_mesh_geometry_registry_geometry_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_geometry = geometry_conversion_untextured_mesh_geometry_to_aabb_3d(tmp_geometry, &tmp_aabb);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = renderer_frontend_result_convert_geometry(ret_geometry);
        ERROR_MESSAGE("renderer_frontend_untextured_mesh_local_aabb_get(%s) - geometry_conversion_untextured_mesh_geometry_to_aabb_3d failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    *out_aabb_ = tmp_aabb;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_line_mesh_draw(renderer_frontend_t* renderer_frontend_, const line_mesh_t* mesh_, const mat4x4f_t* model_matrix_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_draw", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_draw", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_draw", "model_matrix_")

    draw_range = line_mesh_geometry_registry_draw_range_get(renderer_frontend_->line_mesh_render_resource->geometry_registry, mesh_->geometry_id);
    if(NULL == draw_range) {
        ret = RENDERER_FRONTEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_frontend_line_mesh_draw(%s) - line_mesh_geometry_registry_draw_range_get failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_use(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_draw(%s) - line_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_model_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_draw(%s) - line_mesh_shader_model_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_vao_bind(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_draw(%s) - line_mesh_shader_vao_bind failed.", renderer_frontend_result_to_str(ret));
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
    glDrawArrays(GL_LINES, (GLint)draw_range->first_vertex_count, (GLsizei)draw_range->vertex_count);

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_line_mesh_render_update(renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_render_update", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_render_update", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_render_update", "projection_matrix_")

    ret_shader = line_mesh_shader_use(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_render_update(%s) - line_mesh_shader_use failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_projection_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader, projection_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_render_update(%s) - line_mesh_shader_projection_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_view_matrix_set(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource->shader, view_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("renderer_frontend_line_mesh_render_update(%s) - line_mesh_shader_view_matrix_set failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_line_mesh_import_from_vertices(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, line_mesh_t* out_mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_vertices", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_vertices", "out_mesh_")

    ret_resource_pipeline = line_mesh_geometry_pipeline_import_from_vertices(renderer_frontend_->line_mesh_render_resource->vbo_manager, renderer_frontend_->line_mesh_render_resource->geometry_registry, resource_name_, vertices_, vertex_count_, &tmp_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_line_mesh_import_from_vertices(%s) - line_mesh_geometry_pipeline_import_from_vertices failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = tmp_id;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_line_mesh_import_from_aabb(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const aabb_3d_t* aabb_, vec4u8_t color_, line_mesh_t* out_mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_aabb", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_aabb", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_aabb", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_import_from_aabb", "out_mesh_")

    ret_resource_pipeline = line_mesh_geometry_pipeline_import_from_aabb(renderer_frontend_->line_mesh_render_resource->vbo_manager, renderer_frontend_->line_mesh_render_resource->geometry_registry, resource_name_, aabb_, color_, &tmp_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_line_mesh_import_from_aabb(%s) - line_mesh_geometry_pipeline_import_from_vertices failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = tmp_id;

    ret = RENDERER_FRONTEND_SUCCESS;
cleanup:
    return ret;
}

renderer_frontend_result_t renderer_frontend_line_mesh_release(renderer_frontend_t* renderer_frontend_, const line_mesh_t* mesh_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_release", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "renderer_frontend_line_mesh_release", "mesh_")

    ret_resource_pipeline = line_mesh_geometry_pipeline_release(renderer_frontend_->line_mesh_render_resource->vbo_manager, renderer_frontend_->line_mesh_render_resource->geometry_registry, mesh_->geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = renderer_frontend_result_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("renderer_frontend_line_mesh_release(%s) - line_mesh_geometry_pipeline_release failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    return ret;
}

bool renderer_frontend_is_valid(const renderer_frontend_t* renderer_frontend_) {
    if(NULL == renderer_frontend_) {
        return false;
    }

    if(NULL == renderer_frontend_->renderer_backend_context ||
       NULL == renderer_frontend_->line_mesh_render_resource ||
       NULL == renderer_frontend_->untextured_mesh_render_resource) {
        return false;
    }

    // Owned Backend ContextのCanonical Validity
    if(!renderer_backend_is_valid(renderer_frontend_->renderer_backend_context)) {
        return false;
    }

    if(!line_mesh_render_resource_is_valid(renderer_frontend_->renderer_backend_context, renderer_frontend_->line_mesh_render_resource)) {
        return false;
    }

    if(!untextured_mesh_render_resource_is_valid(renderer_frontend_->renderer_backend_context, renderer_frontend_->untextured_mesh_render_resource)) {
        return false;
    }

    return true;
}

// NOTE: 引数のvalidityは呼び出し側で保証する
static renderer_frontend_result_t line_mesh_render_resource_create(subsystem_allocator_t* allocator_, const line_mesh_resource_config_t *config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, line_mesh_render_resource_t** out_render_resource_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;
    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    line_mesh_render_resource_t* tmp_render_resource = NULL;
    vbo_manager_t* tmp_vbo_manager = NULL;
    line_mesh_shader_t* tmp_shader = NULL;
    line_mesh_geometry_registry_t* tmp_registry = NULL;
    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    vbo_manager_config_t vbo_manager_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_render_resource_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "line_mesh_render_resource_create", "out_render_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_resource_, ret, RENDERER_FRONTEND_BAD_OPERATION, renderer_frontend_result_to_str(RENDERER_FRONTEND_BAD_OPERATION), "line_mesh_render_resource_create", "*out_render_resource_")

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(line_mesh_render_resource_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_render_resource);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = renderer_frontend_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - Failed to allocate renderer_frontend_t instance.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // VBO Manager.
    vbo_manager_config.base_align = config_->vbo_config.base_align;
    vbo_manager_config.buffer_usage = config_->vbo_config.buffer_usage;
    vbo_manager_config.max_allocation_count = config_->vbo_config.max_allocation_count;
    vbo_manager_config.vbo_size = config_->vbo_config.vbo_size;
    ret_buffer_manager = vbo_manager_create(backend_context_, &vbo_manager_config, &tmp_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = renderer_frontend_result_convert_buffer_manager(ret_buffer_manager);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - vbo_manager_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // Line Mesh Shader.
    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "line_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "line_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = line_mesh_shader_create(fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), backend_context_, vbo_manager_vbo_get(tmp_vbo_manager), &tmp_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - Failed to create line mesh shader.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    fs_path_destroy(&vertex_shader_path);
    fs_path_destroy(&fragment_shader_path);

    // Geometry Registry.
    ret_resource_registry = line_mesh_geometry_registry_create(config_->max_geometry_count, allocator_, &tmp_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = renderer_frontend_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("line_mesh_render_resource_create(%s) - line_mesh_geometry_registry_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    tmp_render_resource->geometry_registry = tmp_registry;
    tmp_render_resource->shader = tmp_shader;
    tmp_render_resource->vbo_manager = tmp_vbo_manager;
    *out_render_resource_ = tmp_render_resource;
    tmp_render_resource = NULL;
    tmp_registry = NULL;
    tmp_shader = NULL;
    tmp_vbo_manager = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED == ret || RENDERER_FRONTEND_SUCCESS == ret) {
        return ret;
    }

    // Registry Rollback.
    if(NULL != tmp_registry && NULL != tmp_vbo_manager) {
        line_mesh_geometry_registry_deinitialize(tmp_registry, tmp_vbo_manager);
    }

    // Shader Rollback.
    fs_path_destroy(&vertex_shader_path);
    fs_path_destroy(&fragment_shader_path);
    if(NULL != tmp_shader && NULL != backend_context_) {
        line_mesh_shader_destroy(backend_context_, &tmp_shader);
    }

    // VBO Manager Rollback.
    if(NULL != tmp_vbo_manager && NULL != backend_context_) {
        vbo_manager_destroy(&tmp_vbo_manager);
    }
    return ret;
}

static void line_mesh_render_resource_deinitialize(renderer_backend_context_t* backend_context_, line_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return;
    }
    if(NULL == backend_context_) {
        return;
    }

    if(NULL != render_resource_->geometry_registry && NULL != render_resource_->vbo_manager) {
        line_mesh_geometry_registry_deinitialize(render_resource_->geometry_registry, render_resource_->vbo_manager);
    }

    if(NULL != render_resource_->shader) {
        line_mesh_shader_destroy(backend_context_, &render_resource_->shader);
    }

    if(NULL != render_resource_->vbo_manager && NULL != backend_context_) {
        vbo_manager_destroy(&render_resource_->vbo_manager);
    }
}

static renderer_frontend_result_t untextured_mesh_render_resource_create(subsystem_allocator_t* allocator_, const untextured_mesh_resource_config_t *config_, renderer_backend_context_t* backend_context_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_render_resource_t** out_render_resource_) {
    renderer_frontend_result_t ret = RENDERER_FRONTEND_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;
    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    untextured_mesh_render_resource_t* tmp_render_resource = NULL;
    vbo_manager_t* tmp_vbo_manager = NULL;
    untextured_mesh_shader_t* tmp_shader = NULL;
    untextured_mesh_geometry_registry_t* tmp_geometry_registry = NULL;
    untextured_mesh_material_registry_t* tmp_material_registry = NULL;
    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    vbo_manager_config_t vbo_manager_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_render_resource_, ret, RENDERER_FRONTEND_INVALID_ARGUMENT, renderer_frontend_result_to_str(RENDERER_FRONTEND_INVALID_ARGUMENT), "untextured_mesh_render_resource_create", "out_render_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_resource_, ret, RENDERER_FRONTEND_BAD_OPERATION, renderer_frontend_result_to_str(RENDERER_FRONTEND_BAD_OPERATION), "untextured_mesh_render_resource_create", "*out_render_resource_")

    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(untextured_mesh_render_resource_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_render_resource);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = renderer_frontend_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - subsystem_allocator_allocate failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // VBO Manager.
    vbo_manager_config.base_align = config_->vbo_config.base_align;
    vbo_manager_config.buffer_usage = config_->vbo_config.buffer_usage;
    vbo_manager_config.max_allocation_count = config_->vbo_config.max_allocation_count;
    vbo_manager_config.vbo_size = config_->vbo_config.vbo_size;
    ret_buffer_manager = vbo_manager_create(backend_context_, &vbo_manager_config, &tmp_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = renderer_frontend_result_convert_buffer_manager(ret_buffer_manager);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - vbo_manager_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // Untextured Mesh Shader.
    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "untextured_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = renderer_frontend_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - fs_path_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    ret_shader = untextured_mesh_shader_create(fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), backend_context_, vbo_manager_vbo_get(tmp_vbo_manager), &tmp_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = renderer_frontend_result_convert_shader(ret_shader);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - Failed to create untextured mesh shader.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }
    fs_path_destroy(&vertex_shader_path);
    fs_path_destroy(&fragment_shader_path);

    // Geometry Registry.
    ret_resource_registry = untextured_mesh_geometry_registry_create(config_->max_geometry_count, allocator_, &tmp_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = renderer_frontend_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - untextured_mesh_geometry_registry_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    // Material Registry.
    ret_resource_registry = untextured_mesh_material_registry_create(config_->max_material_count, allocator_, &tmp_material_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = renderer_frontend_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_render_resource_create(%s) - untextured_mesh_material_registry_create failed.", renderer_frontend_result_to_str(ret));
        goto cleanup;
    }

    tmp_render_resource->geometry_registry = tmp_geometry_registry;
    tmp_render_resource->material_registry = tmp_material_registry;
    tmp_render_resource->shader = tmp_shader;
    tmp_render_resource->vbo_manager = tmp_vbo_manager;
    *out_render_resource_ = tmp_render_resource;
    tmp_render_resource = NULL;
    tmp_geometry_registry = NULL;
    tmp_material_registry = NULL;
    tmp_shader = NULL;
    tmp_vbo_manager = NULL;

    ret = RENDERER_FRONTEND_SUCCESS;

cleanup:
    if(RENDERER_FRONTEND_DATA_CORRUPTED == ret || RENDERER_FRONTEND_SUCCESS == ret) {
        return ret;
    }

    // Registry Rollback.
    if(NULL != tmp_geometry_registry && NULL != tmp_vbo_manager) {
        untextured_mesh_geometry_registry_deinitialize(tmp_geometry_registry, tmp_vbo_manager);
    }
    if(NULL != tmp_material_registry) {
        untextured_mesh_material_registry_deinitialize(tmp_material_registry);
    }

    // Shader Rollback.
    fs_path_destroy(&vertex_shader_path);
    fs_path_destroy(&fragment_shader_path);
    if(NULL != tmp_shader && NULL != backend_context_) {
        untextured_mesh_shader_destroy(backend_context_, &tmp_shader);
    }

    // VBO Manager Rollback.
    if(NULL != tmp_vbo_manager && NULL != backend_context_) {
        vbo_manager_destroy(&tmp_vbo_manager);
    }
    return ret;
}

static void untextured_mesh_render_resource_deinitialize(renderer_backend_context_t* backend_context_, untextured_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return;
    }
    if(NULL == backend_context_) {
        return;
    }

    if(NULL != render_resource_->geometry_registry && NULL != render_resource_->vbo_manager) {
        untextured_mesh_geometry_registry_deinitialize(render_resource_->geometry_registry, render_resource_->vbo_manager);
    }
    if(NULL != render_resource_->material_registry) {
        untextured_mesh_material_registry_deinitialize(render_resource_->material_registry);
    }

    if(NULL != render_resource_->shader) {
        untextured_mesh_shader_destroy(backend_context_, &render_resource_->shader);
    }

    if(NULL != render_resource_->vbo_manager && NULL != backend_context_) {
        vbo_manager_destroy(&render_resource_->vbo_manager);
    }
}

static bool line_mesh_render_resource_is_valid(const renderer_backend_context_t* backend_context_, const line_mesh_render_resource_t* render_resource_) {

    if(NULL == backend_context_ || NULL == render_resource_) {
        return false;
    }

    if(NULL == render_resource_->shader ||
       NULL == render_resource_->geometry_registry ||
       NULL == render_resource_->vbo_manager) {
        return false;
    }

    // General Allocator管理のOwned Resource
    if(!general_allocator_ptr_is_allocated(render_resource_->shader)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(render_resource_->vbo_manager)) {
        return false;
    }

    if(!vbo_manager_is_valid(render_resource_->vbo_manager)) {
        return false;
    }
    if(!line_mesh_shader_is_valid(backend_context_, render_resource_->shader)) {
        return false;
    }
    if(!line_mesh_geometry_registry_is_valid(render_resource_->geometry_registry)) {
        return false;
    }

    return true;
}

static bool untextured_mesh_render_resource_is_valid(const renderer_backend_context_t* backend_context_, const untextured_mesh_render_resource_t* render_resource_) {

    if(NULL == backend_context_ || NULL == render_resource_) {
        return false;
    }

    if(NULL == render_resource_->shader ||
       NULL == render_resource_->geometry_registry ||
       NULL == render_resource_->material_registry ||
       NULL == render_resource_->vbo_manager) {
        return false;
    }

    // General Allocator管理のOwned Resource
    if(!general_allocator_ptr_is_allocated(render_resource_->shader)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(render_resource_->vbo_manager)) {
        return false;
    }

    if(!vbo_manager_is_valid(render_resource_->vbo_manager)) {
        return false;
    }
    if(!untextured_mesh_shader_is_valid(backend_context_, render_resource_->shader)) {
        return false;
    }
    if(!untextured_mesh_geometry_registry_is_valid(render_resource_->geometry_registry)) {
        return false;
    }
    if(!untextured_mesh_material_registry_is_valid(render_resource_->material_registry)) {
        return false;
    }

    return true;
}
