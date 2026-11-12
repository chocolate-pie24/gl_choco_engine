// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/stl_viewer/stl_viewer.h"

#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/line_mesh_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/line_grid_geometry.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/demos/stl_viewer/core/stl_viewer_types.h"
#include "application/demos/stl_viewer/core/stl_viewer_err_utils.h"

#include "application/demos/sample_robot/model_instance/sample_robot.h"

typedef struct stl_viewer {
    untextured_mesh_t untextured_mesh;
    // line_mesh_t floor_grid_mesh;
    // untextured_mesh_t debug_axis_mesh;
    // line_mesh_t debug_aabb_mesh;

    mat4x4f_t model_matrix;
} stl_viewer_t;

stl_viewer_result_t stl_viewer_create(const char* asset_root_path_, const untextured_mesh_config_t* config_, renderer_frontend_t* renderer_frontend_, stl_viewer_t** out_viewer_) {
    stl_viewer_result_t ret = STL_VIEWER_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    stl_viewer_t* tmp_viewer = NULL;
    untextured_mesh_t tmp_untextured_mesh = { 0 };

    bool untextured_mesh_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(out_viewer_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_create", "out_viewer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_viewer_, ret, STL_VIEWER_BAD_OPERATION, stl_viewer_result_to_str(STL_VIEWER_BAD_OPERATION), "stl_viewer_create", "*out_viewer_")

    ret_general_allocator = general_allocator_allocate(sizeof(stl_viewer_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_viewer);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = stl_viewer_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = STL_VIEWER_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - general_allocator_allocate failed.", stl_viewer_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_untextured_mesh_import(renderer_frontend_, config_, asset_root_path_, &tmp_untextured_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = stl_viewer_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = STL_VIEWER_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_import failed.", stl_viewer_result_to_str(ret));
        goto cleanup;
    }
    untextured_mesh_imported = true;

    mat4f_identity(&tmp_viewer->model_matrix);
    tmp_viewer->untextured_mesh = tmp_untextured_mesh;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(tmp_viewer)) {
        ret = STL_VIEWER_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_create(%s) - Result validation failed for 'tmp_viewer'.", stl_viewer_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_viewer_ = tmp_viewer;
    tmp_viewer = NULL;

    ret = STL_VIEWER_SUCCESS;

cleanup:
    if(STL_VIEWER_DATA_CORRUPTED == ret || STL_VIEWER_SUCCESS == ret) {
        return ret;
    }
    if(untextured_mesh_imported) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_untextured_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = STL_VIEWER_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_release failed.", stl_viewer_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != tmp_viewer) {
        general_allocator_free((void**)&tmp_viewer, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, stl_viewer_t** viewer_) {
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    if(NULL == viewer_ || NULL == *viewer_ || NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(*viewer_)) {
        ERROR_MESSAGE("stl_viewer_destroy(%s) - Precondition validation failed for '*viewer_'.", stl_viewer_result_to_str(STL_VIEWER_DATA_CORRUPTED));
        return;
    }
#endif

    ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &(*viewer_)->untextured_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_release failed.", stl_viewer_result_to_str(STL_VIEWER_DATA_CORRUPTED));
        return;
    }

    general_allocator_free((void**)viewer_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

stl_viewer_result_t stl_viewer_render_packet_build(const stl_viewer_t* viewer_, render_packet_t* packet_) {
    stl_viewer_result_t ret = STL_VIEWER_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(viewer_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_render_packet_build", "viewer_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, STL_VIEWER_INVALID_ARGUMENT, stl_viewer_result_to_str(STL_VIEWER_INVALID_ARGUMENT), "stl_viewer_render_packet_build", "packet_")

    render_packet_deinitialize(packet_);
    ret_render_packet = render_packet_initialize(0, 0, 0, 1, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = stl_viewer_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = STL_VIEWER_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_render_packet_build(%s) - render_packet_initialize failed.", stl_viewer_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_untextured_mesh_push(&viewer_->untextured_mesh, &viewer_->model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = stl_viewer_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = STL_VIEWER_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_render_packet_build(%s) - render_packet_line_mesh_push failed.", stl_viewer_result_to_str(ret));
        goto cleanup;
    }

    ret = STL_VIEWER_SUCCESS;

cleanup:
    return ret;
}

bool stl_viewer_is_valid(const stl_viewer_t* viewer_) {
    if(NULL == viewer_) {
        return false;
    }
    return true;
}
