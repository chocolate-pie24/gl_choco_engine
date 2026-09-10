// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/renderer/application_renderer.h"

#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/io_utils/fs_path.h"

// NOTE: engine/systems/renderer/resources/shaders/core/shader_resource_types.hのincludeについて
// Applicationからengine内部headerを直接includeするのは本来layering違反であるが暫定的に許可する。
// 将来Renderer Frontendを導入し、Shaderおよびその内部型をApplicationから隠した時点で削除する予定。
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

#include "engine/systems/renderer/config/renderer_config.h"

#include "engine/systems/renderer/resources/shaders/ui_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/line_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/point_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vao.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/render_resources/line_mesh_render_resource.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

struct application_renderer {
    renderer_backend_context_t* renderer_backend_context;

    line_mesh_render_resource_t* line_mesh_render_resource;

    ui_mesh_shader_t* ui_mesh_shader;
    point_mesh_shader_t* point_mesh_shader;
    lit_mesh_shader_t* lit_mesh_shader;
};

static application_result_t app_lit_mesh_shader_create(const lit_mesh_shader_config_t* lit_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, lit_mesh_shader_t** out_lit_mesh_shader_);
static application_result_t app_point_mesh_shader_create(const point_mesh_shader_config_t* point_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, point_mesh_shader_t** out_point_mesh_shader_);
static application_result_t app_ui_mesh_shader_create(const ui_mesh_shader_config_t* ui_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, ui_mesh_shader_t** out_ui_mesh_shader_);

static bool is_valid_shallow(const application_renderer_t* application_renderer_);

application_result_t application_renderer_initialize(const renderer_config_t* renderer_config_, target_graphics_api_t target_api_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, application_renderer_t** out_application_renderer_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    application_renderer_t* tmp_application_renderer = NULL;
    renderer_backend_context_t* tmp_renderer_backend_context = NULL;
    line_mesh_render_resource_t* tmp_line_mesh_render_resource = NULL;
    lit_mesh_shader_t* tmp_lit_mesh_shader = NULL;
    point_mesh_shader_t* tmp_point_mesh_shader = NULL;
    ui_mesh_shader_t* tmp_ui_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_config_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_initialize", "renderer_config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_initialize", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_initialize", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_initialize", "out_application_renderer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_application_renderer_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "application_renderer_initialize", "*out_application_renderer_")

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(application_renderer_t), alignof(application_renderer_t), (void**)&tmp_application_renderer);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("application_renderer_initialize(%s) - Failed to allocate application_renderer_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_application_renderer, 0, sizeof(application_renderer_t));

    ret_renderer_backend = renderer_backend_initialize(allocator_, target_api_, &tmp_renderer_backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = app_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("application_renderer_initialize(%s) - Failed to initialize renderer backend.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // TODO: 128をconfigで与えるように変更
    ret_render_resource = line_mesh_render_resource_initialize(&renderer_config_->line_mesh_shader_config, 128, tmp_renderer_backend_context, allocator_, executable_directory_, shader_dir_, &tmp_line_mesh_render_resource);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_initialize(%s) - line_mesh_render_resource_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = app_lit_mesh_shader_create(&renderer_config_->lit_mesh_shader_config, tmp_renderer_backend_context, executable_directory_, shader_dir_, &tmp_lit_mesh_shader);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_renderer_initialize(%s) - app_lit_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = app_point_mesh_shader_create(&renderer_config_->point_mesh_shader_config, tmp_renderer_backend_context, executable_directory_, shader_dir_, &tmp_point_mesh_shader);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_renderer_initialize(%s) - app_point_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = app_ui_mesh_shader_create(&renderer_config_->ui_mesh_shader_config, tmp_renderer_backend_context, executable_directory_, shader_dir_, &tmp_ui_mesh_shader);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_renderer_initialize(%s) - app_ui_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_application_renderer->line_mesh_render_resource = tmp_line_mesh_render_resource;
    tmp_application_renderer->lit_mesh_shader = tmp_lit_mesh_shader;
    tmp_application_renderer->point_mesh_shader = tmp_point_mesh_shader;
    tmp_application_renderer->renderer_backend_context = tmp_renderer_backend_context;
    tmp_application_renderer->ui_mesh_shader = tmp_ui_mesh_shader;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(tmp_application_renderer)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_initialize(%s) - Postcondition validation failed for 'tmp_application_renderer'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_application_renderer_ = tmp_application_renderer;
    tmp_application_renderer = NULL;
    tmp_renderer_backend_context = NULL;
    tmp_line_mesh_render_resource = NULL;
    tmp_lit_mesh_shader = NULL;
    tmp_point_mesh_shader = NULL;
    tmp_ui_mesh_shader = NULL;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        if(NULL != tmp_line_mesh_render_resource) {
            line_mesh_render_resource_deinitialize(tmp_line_mesh_render_resource);
        }
        if(NULL != tmp_lit_mesh_shader) {
            lit_mesh_shader_destroy(&tmp_lit_mesh_shader);
        }
        if(NULL != tmp_point_mesh_shader) {
            point_mesh_shader_destroy(&tmp_point_mesh_shader);
        }
        if(NULL != tmp_ui_mesh_shader) {
            ui_mesh_shader_destroy(&tmp_ui_mesh_shader);
        }
        if(NULL != tmp_renderer_backend_context) {
            renderer_backend_destroy(tmp_renderer_backend_context);
        }
    }
    return ret;
}

void application_renderer_deinitialize(application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ERROR_MESSAGE("application_renderer_deinitialize(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(APPLICATION_DATA_CORRUPTED));
        return;
    }
#endif

    line_mesh_render_resource_deinitialize(application_renderer_->line_mesh_render_resource);
    lit_mesh_shader_destroy(&application_renderer_->lit_mesh_shader);
    point_mesh_shader_destroy(&application_renderer_->point_mesh_shader);
    ui_mesh_shader_destroy(&application_renderer_->ui_mesh_shader);
    renderer_backend_destroy(application_renderer_->renderer_backend_context);
}

application_result_t application_renderer_update(application_renderer_t* application_renderer_, bool view_dirty_, bool projection_dirty_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, bool should_transpose_view_matrix_, bool should_transpose_projection_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_update", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_update", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_update", "projection_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_update(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    if(projection_dirty_) {
        ret_shader = ui_mesh_shader_use(application_renderer_->ui_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - ui_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = ui_mesh_shader_projection_matrix_set(application_renderer_->ui_mesh_shader, projection_matrix_, should_transpose_projection_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - Failed to set projection matrix.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = line_mesh_render_resource_projection_matrix_set(application_renderer_->line_mesh_render_resource, projection_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - line_mesh_render_resource_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_use(application_renderer_->point_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_projection_matrix_set(application_renderer_->point_mesh_shader, projection_matrix_, should_transpose_projection_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_shader_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_use(application_renderer_->lit_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_projection_matrix_set(application_renderer_->lit_mesh_shader, projection_matrix_, should_transpose_projection_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_shader_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(view_dirty_) {
        ret_shader = ui_mesh_shader_use(application_renderer_->ui_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - ui_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = ui_mesh_shader_view_matrix_set(application_renderer_->ui_mesh_shader, view_matrix_, should_transpose_view_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - ui_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = line_mesh_render_resource_view_matrix_set(application_renderer_->line_mesh_render_resource, view_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - line_mesh_render_resource_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_use(application_renderer_->point_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_view_matrix_set(application_renderer_->point_mesh_shader, view_matrix_, should_transpose_view_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_use(application_renderer_->lit_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_view_matrix_set(application_renderer_->lit_mesh_shader, view_matrix_, should_transpose_view_matrix_);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_line_mesh_import_from_vertices(application_renderer_t* application_renderer_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_vertices", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_vertices", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_line_mesh_import_from_vertices(%s) - Provided resource_name_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == vertex_count_) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_line_mesh_import_from_vertices(%s) - Provided vertex_count_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_line_mesh_import_from_vertices(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_import_from_vertices(application_renderer_->line_mesh_render_resource, resource_name_, vertices_, vertex_count_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_line_mesh_import_from_vertices(%s) - line_mesh_render_resource_import_from_vertices failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_line_mesh_import_from_vertices(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_line_mesh_import_from_aabb(application_renderer_t* application_renderer_, const char* resource_name_, const aabb_3d_t* aabb_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_aabb", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_aabb", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_aabb", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_import_from_aabb", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_line_mesh_import_from_aabb(%s) - Provided resource_name_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_line_mesh_import_from_aabb(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_import_from_aabb(application_renderer_->line_mesh_render_resource, resource_name_, aabb_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("line_mesh_render_resource_import_from_aabb(%s) - line_mesh_render_resource_import_from_aabb failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_line_mesh_import_from_aabb(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;;

cleanup:
    return ret;
}

application_result_t application_line_mesh_release(application_renderer_t* application_renderer_, uint16_t geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_render_resource_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_release(application_renderer_->line_mesh_render_resource, geometry_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("line_mesh_render_resource_release(%s) - line_mesh_render_resource_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_render_resource_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_line_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_, const uint8_t color_[4]) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_draw", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_draw", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(color_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_line_mesh_draw", "color_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_line_mesh_draw(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_draw(application_renderer_->line_mesh_render_resource, geometry_id_, model_matrix_, color_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_line_mesh_draw(%s) - line_mesh_render_resource_draw failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_shader_use(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_shader_use", "application_renderer_")
    if(!application_renderer_shader_type_is_valid(shader_type_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_renderer_shader_use(%s) - Provided shader_type_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_shader_use(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        ret_shader = SHADER_UNDEFINED_ERROR;
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        ret_shader = lit_mesh_shader_use(application_renderer_->lit_mesh_shader);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        ret_shader = point_mesh_shader_use(application_renderer_->point_mesh_shader);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        ret_shader = ui_mesh_shader_use(application_renderer_->ui_mesh_shader);
        break;
    default:
        ret_shader = SHADER_UNDEFINED_ERROR;    // preconditionでvalidationを行っているのでinvalidなshader_typeはundefined error
        break;
    }
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_renderer_shader_use(%s) - shader_use failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_vao_bind(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_vao_bind", "application_renderer_")
    if(!application_renderer_shader_type_is_valid(shader_type_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_renderer_vao_bind(%s) - Provided shader_type_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_vao_bind(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        ret_shader = SHADER_UNDEFINED_ERROR;
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        ret_shader = lit_mesh_shader_vao_bind(application_renderer_->lit_mesh_shader);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        ret_shader = point_mesh_shader_vao_bind(application_renderer_->point_mesh_shader);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        ret_shader = ui_mesh_shader_vao_bind(application_renderer_->ui_mesh_shader);
        break;
    default:
        ret_shader = SHADER_UNDEFINED_ERROR;    // preconditionでvalidationを行っているのでinvalidなshader_typeはundefined error
        break;
    }
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_renderer_vao_bind(%s) - vao_bind failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_vao_unbind(application_renderer_t* application_renderer_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_vao_unbind", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_vao_unbind(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_vao_unbind(application_renderer_->renderer_backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = app_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("application_renderer_vao_unbind(%s) - vao_unbind failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_model_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_model_matrix_set", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_model_matrix_set", "model_matrix_")
    if(!application_renderer_shader_type_is_valid(shader_type_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_renderer_model_matrix_set(%s) - Provided shader_type_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_model_matrix_set(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        ret_shader = SHADER_UNDEFINED_ERROR;
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        ret_shader = lit_mesh_shader_model_matrix_set(application_renderer_->lit_mesh_shader, model_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        ret_shader = point_mesh_shader_model_matrix_set(application_renderer_->point_mesh_shader, model_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        ret_shader = ui_mesh_shader_model_matrix_set(application_renderer_->ui_mesh_shader, model_matrix_, should_transpose_);
        break;
    default:
        ret_shader = SHADER_UNDEFINED_ERROR;    // preconditionでvalidationを行っているのでinvalidなshader_typeはundefined error
        break;
    }
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_renderer_model_matrix_set(%s) - model_matrix_set failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_view_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_view_matrix_set", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_view_matrix_set", "view_matrix_")
    if(!application_renderer_shader_type_is_valid(shader_type_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_renderer_view_matrix_set(%s) - Provided shader_type_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_view_matrix_set(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        ret_shader = SHADER_UNDEFINED_ERROR;
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        ret_shader = lit_mesh_shader_view_matrix_set(application_renderer_->lit_mesh_shader, view_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        ret_shader = point_mesh_shader_view_matrix_set(application_renderer_->point_mesh_shader, view_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        ret_shader = ui_mesh_shader_view_matrix_set(application_renderer_->ui_mesh_shader, view_matrix_, should_transpose_);
        break;
    default:
        ret_shader = SHADER_UNDEFINED_ERROR;    // preconditionでvalidationを行っているのでinvalidなshader_typeはundefined error
        break;
    }
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_renderer_view_matrix_set(%s) - view_matrix_set failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_projection_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_projection_matrix_set", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_projection_matrix_set", "projection_matrix_")
    if(!application_renderer_shader_type_is_valid(shader_type_)) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_renderer_projection_matrix_set(%s) - Provided shader_type_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_projection_matrix_set(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        ret_shader = SHADER_UNDEFINED_ERROR;
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        ret_shader = lit_mesh_shader_projection_matrix_set(application_renderer_->lit_mesh_shader, projection_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        ret_shader = point_mesh_shader_projection_matrix_set(application_renderer_->point_mesh_shader, projection_matrix_, should_transpose_);
        break;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        ret_shader = ui_mesh_shader_projection_matrix_set(application_renderer_->ui_mesh_shader, projection_matrix_, should_transpose_);
        break;
    default:
        ret_shader = SHADER_UNDEFINED_ERROR;    // preconditionでvalidationを行っているのでinvalidなshader_typeはundefined error
        break;
    }
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_renderer_projection_matrix_set(%s) - projection_matrix_set failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

renderer_backend_context_t* application_renderer_renderer_backend_context_get(application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return NULL;
    }
    return application_renderer_->renderer_backend_context;
}

lit_mesh_shader_t* application_renderer_lit_mesh_shader_get(application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return NULL;
    }
    return application_renderer_->lit_mesh_shader;
}

point_mesh_shader_t* application_renderer_point_mesh_shader_get(application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return NULL;
    }
    return application_renderer_->point_mesh_shader;
}

ui_mesh_shader_t* application_renderer_ui_mesh_shader_get(application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return NULL;
    }
    return application_renderer_->ui_mesh_shader;
}

bool application_renderer_shader_type_is_valid(application_renderer_shader_type_t shader_type_) {
    switch(shader_type_) {
    case APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH:
        return true;
    case APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH:
        return true;
    case APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH:
        return true;
    case APPLICATION_RENDERER_SHADER_TYPE_UI_MESH:
        return true;
    default:
        return false;
    }
}

bool application_renderer_is_valid(const application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return false;
    }
    if(!is_valid_shallow(application_renderer_)) {
        return false;
    }
    // TODO: xxx_mesh_shader_is_valid追加
    return true;
}

static application_result_t app_lit_mesh_shader_create(const lit_mesh_shader_config_t* lit_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, lit_mesh_shader_t** out_lit_mesh_shader_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    lit_mesh_shader_t* tmp_lit_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_config_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_lit_mesh_shader_create", "lit_mesh_shader_config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_lit_mesh_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_lit_mesh_shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_lit_mesh_shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_lit_mesh_shader_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_lit_mesh_shader_create", "out_lit_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_lit_mesh_shader_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "app_lit_mesh_shader_create", "*out_lit_mesh_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "lit_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_lit_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "lit_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_lit_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_create(renderer_backend_context_, fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), lit_mesh_shader_config_, &tmp_lit_mesh_shader);
    if(SHADER_LINK_ERROR == ret_shader || SHADER_COMPILE_ERROR == ret_shader) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_lit_mesh_shader_create(%s) - lit_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    } else if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("app_lit_mesh_shader_create(%s) - Failed to create lit mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }

    *out_lit_mesh_shader_ = tmp_lit_mesh_shader;
    tmp_lit_mesh_shader = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_lit_mesh_shader) {
            lit_mesh_shader_destroy(&tmp_lit_mesh_shader);
        }
    }

    return ret;
}

static application_result_t app_point_mesh_shader_create(const point_mesh_shader_config_t* point_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, point_mesh_shader_t** out_point_mesh_shader_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    point_mesh_shader_t* tmp_point_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_config_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_point_mesh_shader_create", "point_mesh_shader_config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_point_mesh_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_point_mesh_shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_point_mesh_shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_point_mesh_shader_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_point_mesh_shader_create", "out_point_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_point_mesh_shader_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "app_point_mesh_shader_create", "*out_point_mesh_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "point_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_point_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "point_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_point_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = point_mesh_shader_create(renderer_backend_context_, fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), point_mesh_shader_config_, &tmp_point_mesh_shader);
    if(SHADER_LINK_ERROR == ret_shader || SHADER_COMPILE_ERROR == ret_shader) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_point_mesh_shader_create(%s) - point_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    } else if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("app_point_mesh_shader_create(%s) - Failed to create point mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }

    *out_point_mesh_shader_ = tmp_point_mesh_shader;
    tmp_point_mesh_shader = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_point_mesh_shader) {
            point_mesh_shader_destroy(&tmp_point_mesh_shader);
        }
    }

    return ret;
}

static application_result_t app_ui_mesh_shader_create(const ui_mesh_shader_config_t* ui_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, ui_mesh_shader_t** out_ui_mesh_shader_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    ui_mesh_shader_t* tmp_ui_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_config_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_ui_mesh_shader_create", "ui_mesh_shader_config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_ui_mesh_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_ui_mesh_shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_ui_mesh_shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ui_mesh_shader_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "app_ui_mesh_shader_create", "out_ui_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ui_mesh_shader_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "app_ui_mesh_shader_create", "*out_ui_mesh_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "ui_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_ui_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "ui_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = app_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("app_ui_mesh_shader_create(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = ui_mesh_shader_create(renderer_backend_context_, fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), ui_mesh_shader_config_, &tmp_ui_mesh_shader);
    if(SHADER_LINK_ERROR == ret_shader || SHADER_COMPILE_ERROR == ret_shader) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_ui_mesh_shader_create(%s) - ui_mesh_shader_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    } else if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("app_ui_mesh_shader_create(%s) - Failed to create ui mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }

    *out_ui_mesh_shader_ = tmp_ui_mesh_shader;
    tmp_ui_mesh_shader = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_ui_mesh_shader) {
            ui_mesh_shader_destroy(&tmp_ui_mesh_shader);
        }
    }

    return ret;
}

static bool is_valid_shallow(const application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return false;
    }
    if(NULL == application_renderer_->renderer_backend_context) {
        return false;
    }
    if(NULL == application_renderer_->line_mesh_render_resource) {
        return false;
    }
    if(NULL == application_renderer_->lit_mesh_shader) {
        return false;
    }
    if(NULL == application_renderer_->point_mesh_shader) {
        return false;
    }
    if(NULL == application_renderer_->ui_mesh_shader) {
        return false;
    }
    return true;
}
