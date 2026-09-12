// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/renderer/application_renderer.h"

#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/aabb_3d.h"
#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/renderer/config/renderer_config.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/render_resources/line_mesh_render_resource.h"
#include "engine/systems/renderer/render_resources/lit_mesh_render_resource.h"
#include "engine/systems/renderer/render_resources/point_mesh_render_resource.h"
#include "engine/systems/renderer/render_resources/ui_mesh_render_resource.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

struct application_renderer {
    renderer_backend_context_t* renderer_backend_context;

    line_mesh_render_resource_t* line_mesh_render_resource;
    lit_mesh_render_resource_t* lit_mesh_render_resource;
    point_mesh_render_resource_t* point_mesh_render_resource;
    ui_mesh_render_resource_t* ui_mesh_render_resource;
};

static bool is_valid_shallow(const application_renderer_t* application_renderer_);

application_result_t application_renderer_initialize(const renderer_config_t* renderer_config_, target_graphics_api_t target_api_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, application_renderer_t** out_application_renderer_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    application_renderer_t* tmp_application_renderer = NULL;
    renderer_backend_context_t* tmp_renderer_backend_context = NULL;
    line_mesh_render_resource_t* tmp_line_mesh_render_resource = NULL;
    lit_mesh_render_resource_t* tmp_lit_mesh_render_resource = NULL;
    point_mesh_render_resource_t* tmp_point_mesh_render_resource = NULL;
    ui_mesh_render_resource_t* tmp_ui_mesh_render_resource = NULL;

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

    // TODO: 128をconfigで与えるように変更
    ret_render_resource = lit_mesh_render_resource_initialize(&renderer_config_->lit_mesh_shader_config, 128, tmp_renderer_backend_context, allocator_, executable_directory_, shader_dir_, &tmp_lit_mesh_render_resource);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_initialize(%s) - lit_mesh_render_resource_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // TODO: 128をconfigで与えるように変更
    ret_render_resource = point_mesh_render_resource_initialize(&renderer_config_->point_mesh_shader_config, 128, tmp_renderer_backend_context, allocator_, executable_directory_, shader_dir_, &tmp_point_mesh_render_resource);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_initialize(%s) - point_mesh_render_resource_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // TODO: 128をconfigで与えるように変更
    ret_render_resource = ui_mesh_render_resource_initialize(&renderer_config_->ui_mesh_shader_config, 128, 128, tmp_renderer_backend_context, allocator_, executable_directory_, shader_dir_, &tmp_ui_mesh_render_resource);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_initialize(%s) - ui_mesh_render_resource_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_application_renderer->line_mesh_render_resource = tmp_line_mesh_render_resource;
    tmp_application_renderer->lit_mesh_render_resource = tmp_lit_mesh_render_resource;
    tmp_application_renderer->point_mesh_render_resource = tmp_point_mesh_render_resource;
    tmp_application_renderer->ui_mesh_render_resource = tmp_ui_mesh_render_resource;
    tmp_application_renderer->renderer_backend_context = tmp_renderer_backend_context;

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
    tmp_lit_mesh_render_resource = NULL;
    tmp_point_mesh_render_resource = NULL;
    tmp_ui_mesh_render_resource = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        if(NULL != tmp_line_mesh_render_resource) {
            line_mesh_render_resource_deinitialize(tmp_line_mesh_render_resource);
        }
        if(NULL != tmp_lit_mesh_render_resource) {
            lit_mesh_render_resource_deinitialize(tmp_lit_mesh_render_resource);
        }
        if(NULL != tmp_point_mesh_render_resource) {
            point_mesh_render_resource_deinitialize(tmp_point_mesh_render_resource);
        }
        if(NULL != tmp_ui_mesh_render_resource) {
            ui_mesh_render_resource_deinitialize(tmp_ui_mesh_render_resource);
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
    lit_mesh_render_resource_deinitialize(application_renderer_->lit_mesh_render_resource);
    point_mesh_render_resource_deinitialize(application_renderer_->point_mesh_render_resource);
    ui_mesh_render_resource_deinitialize(application_renderer_->ui_mesh_render_resource);
    renderer_backend_destroy(application_renderer_->renderer_backend_context);
}

application_result_t application_renderer_update(application_renderer_t* application_renderer_, bool view_dirty_, bool projection_dirty_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

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
        ret_render_resource = ui_mesh_render_resource_projection_matrix_set(application_renderer_->ui_mesh_render_resource, projection_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - ui_mesh_render_resource_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = line_mesh_render_resource_projection_matrix_set(application_renderer_->line_mesh_render_resource, projection_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - line_mesh_render_resource_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = lit_mesh_render_resource_projection_matrix_set(application_renderer_->lit_mesh_render_resource, projection_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_render_resource_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = point_mesh_render_resource_projection_matrix_set(application_renderer_->point_mesh_render_resource, projection_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_render_resource_projection_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(view_dirty_) {
        ret_render_resource = ui_mesh_render_resource_view_matrix_set(application_renderer_->ui_mesh_render_resource, view_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - ui_mesh_render_resource_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = line_mesh_render_resource_view_matrix_set(application_renderer_->line_mesh_render_resource, view_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - line_mesh_render_resource_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = lit_mesh_render_resource_view_matrix_set(application_renderer_->lit_mesh_render_resource, view_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - lit_mesh_render_resource_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_render_resource = point_mesh_render_resource_view_matrix_set(application_renderer_->point_mesh_render_resource, view_matrix_);
        if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
            ret = app_rslt_convert_render_resource(ret_render_resource);
            ERROR_MESSAGE("application_renderer_update(%s) - point_mesh_render_resource_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_line_mesh_geometry_import_from_vertices(application_renderer_t* application_renderer_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_vertices", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_vertices", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_vertices(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_geometry_import_from_vertices(application_renderer_->line_mesh_render_resource, resource_name_, vertices_, vertex_count_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_vertices(%s) - line_mesh_render_resource_geometry_import_from_vertices failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_vertices(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_line_mesh_geometry_import_from_aabb(application_renderer_t* application_renderer_, const char* resource_name_, const aabb_3d_t* aabb_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_aabb", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_aabb", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_aabb", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_import_from_aabb", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_aabb(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_geometry_import_from_aabb(application_renderer_->line_mesh_render_resource, resource_name_, aabb_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_aabb(%s) - line_mesh_render_resource_geometry_import_from_aabb failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_import_from_aabb(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_lit_mesh_geometry_import_from_file(application_renderer_t* application_renderer_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_import_from_file", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_import_from_file", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_import_from_file(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = lit_mesh_render_resource_geometry_import_from_file(application_renderer_->lit_mesh_render_resource, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_import_from_file(%s) - lit_mesh_render_resource_geometry_import_from_file failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_import_from_file(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_point_mesh_geometry_import_from_vertices(application_renderer_t* application_renderer_, const char* resource_name_, const point_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_geometry_import_from_vertices", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_geometry_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_geometry_import_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_geometry_import_from_vertices", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_import_from_vertices(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = point_mesh_render_resource_geometry_import_from_vertices(application_renderer_->point_mesh_render_resource, resource_name_, vertices_, vertex_count_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_import_from_vertices(%s) - point_mesh_render_resource_geometry_import_from_vertices failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_import_from_vertices(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_geometry_import_from_file(application_renderer_t* application_renderer_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_geometry_import_from_file", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_geometry_import_from_file", "out_geometry_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_import_from_file(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_geometry_import_from_file(application_renderer_->ui_mesh_render_resource, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_import_from_file(%s) - ui_mesh_render_resource_geometry_import_from_file failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_import_from_file(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_line_mesh_geometry_release(application_renderer_t* application_renderer_, uint16_t geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_geometry_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_geometry_release(application_renderer_->line_mesh_render_resource, geometry_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_release(%s) - line_mesh_render_resource_geometry_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_geometry_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_lit_mesh_geometry_release(application_renderer_t* application_renderer_, uint16_t geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = lit_mesh_render_resource_geometry_release(application_renderer_->lit_mesh_render_resource, geometry_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_release(%s) - lit_mesh_render_resource_geometry_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_point_mesh_geometry_release(application_renderer_t* application_renderer_, uint16_t geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_geometry_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = point_mesh_render_resource_geometry_release(application_renderer_->point_mesh_render_resource, geometry_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_release(%s) - point_mesh_render_resource_geometry_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_point_mesh_geometry_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_geometry_release(application_renderer_t* application_renderer_, uint16_t geometry_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_geometry_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_geometry_release(application_renderer_->ui_mesh_render_resource, geometry_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_release(%s) - ui_mesh_render_resource_geometry_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_geometry_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_texture_import_from_bmp(application_renderer_t* application_renderer_, int32_t texture_unit_index_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_texture_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_bmp", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_bmp", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_fullpath_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_bmp", "texture_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_bmp", "out_texture_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_bmp(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_texture_import_from_bmp(application_renderer_->ui_mesh_render_resource, texture_unit_index_, resource_name_, texture_fullpath_, &tmp_texture_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_bmp(%s) - ui_mesh_render_resource_geometry_import_from_file failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_bmp(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_texture_id_ = tmp_texture_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_texture_import_from_solid_color(application_renderer_t* application_renderer_, int32_t texture_unit_index_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_texture_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_solid_color", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_solid_color", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_import_from_solid_color", "out_texture_id_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_solid_color(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_texture_import_from_solid_color(application_renderer_->ui_mesh_render_resource, texture_unit_index_, resource_name_, red_, green_, blue_, &tmp_texture_id);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_solid_color(%s) - ui_mesh_render_resource_geometry_import_from_file failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_import_from_solid_color(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_texture_id_ = tmp_texture_id;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_texture_release(application_renderer_t* application_renderer_, uint16_t texture_id_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_texture_release", "application_renderer_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_release(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_texture_release(application_renderer_->ui_mesh_render_resource, texture_id_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_release(%s) - ui_mesh_render_resource_texture_release failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_renderer_is_valid(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_texture_release(%s) - Postcondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_line_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_, const uint8_t color_[4]) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_draw", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_draw", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(color_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_line_mesh_draw", "color_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_line_mesh_draw(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = line_mesh_render_resource_draw(application_renderer_->line_mesh_render_resource, geometry_id_, model_matrix_, color_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_line_mesh_draw(%s) - line_mesh_render_resource_draw failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_lit_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_draw", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_draw", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_draw(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = lit_mesh_render_resource_draw(application_renderer_->lit_mesh_render_resource, geometry_id_, model_matrix_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_lit_mesh_draw(%s) - lit_mesh_render_resource_draw failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_point_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_draw", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_point_mesh_draw", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_point_mesh_draw(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = point_mesh_render_resource_draw(application_renderer_->point_mesh_render_resource, geometry_id_, model_matrix_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_point_mesh_draw(%s) - point_mesh_render_resource_draw failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_ui_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, uint16_t texture_id_, const mat4x4f_t* model_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_draw", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_ui_mesh_draw", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_ui_mesh_draw(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = ui_mesh_render_resource_draw(application_renderer_->ui_mesh_render_resource, geometry_id_, texture_id_, model_matrix_);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_ui_mesh_draw(%s) - ui_mesh_render_resource_draw failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_renderer_lit_mesh_geometry_convert_to_aabb_3d(application_renderer_t* application_renderer_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    render_resource_result_t ret_render_resource = RENDER_RESOURCE_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb_3d = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(application_renderer_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_convert_to_aabb_3d", "application_renderer_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_3d_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_renderer_lit_mesh_geometry_convert_to_aabb_3d", "out_aabb_3d_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_renderer_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_convert_to_aabb_3d(%s) - Precondition validation failed for 'application_renderer_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_render_resource = lit_mesh_render_resource_geometry_convert_to_aabb_3d(application_renderer_->lit_mesh_render_resource, geometry_id_, &tmp_aabb_3d);
    if(RENDER_RESOURCE_SUCCESS != ret_render_resource) {
        ret = app_rslt_convert_render_resource(ret_render_resource);
        ERROR_MESSAGE("application_renderer_lit_mesh_geometry_convert_to_aabb_3d(%s) - lit_mesh_render_resource_geometry_convert_to_aabb_3d failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // aabb_3d_tのcanonical validator追加後にtmp_aabb_3dのvalidationを実行する

    *out_aabb_3d_ = tmp_aabb_3d;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

bool application_renderer_is_valid(const application_renderer_t* application_renderer_) {
    if(NULL == application_renderer_) {
        return false;
    }
    if(!is_valid_shallow(application_renderer_)) {
        return false;
    }
    if(!line_mesh_render_resource_is_valid(application_renderer_->line_mesh_render_resource)) {
        return false;
    }
    if(!lit_mesh_render_resource_is_valid(application_renderer_->lit_mesh_render_resource)) {
        return false;
    }
    if(!point_mesh_render_resource_is_valid(application_renderer_->point_mesh_render_resource)) {
        return false;
    }
    if(!ui_mesh_render_resource_is_valid(application_renderer_->ui_mesh_render_resource)) {
        return false;
    }
    return true;
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
    if(NULL == application_renderer_->lit_mesh_render_resource) {
        return false;
    }
    if(NULL == application_renderer_->point_mesh_render_resource) {
        return false;
    }
    if(NULL == application_renderer_->ui_mesh_render_resource) {
        return false;
    }
    return true;
}
