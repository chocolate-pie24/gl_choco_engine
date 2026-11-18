// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/scenes/scene_concretes/demos/stl_viewer/stl_viewer.h"

#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"

#include "engine/containers/choco_string.h"

#include "engine/camera/orbit_camera.h"

#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

#include "engine/systems/event_system/core/engine_event_view.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/line_mesh_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/line_grid_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/untextured_arrow_geometry.h"

#include "engine/systems/renderer/resources/materials/material_types.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/scenes/core/scene_types.h"
#include "application/scenes/core/scene_err_utils.h"

typedef struct stl_viewer {
    // 描画対象STLモデル
    untextured_mesh_t model_mesh;
    mat4x4f_t model_matrix;

    // Debug Axis / Debug AABB.
    untextured_mesh_t debug_axis_meshes[3];
    line_mesh_t debug_aabb_mesh;

    // Floor Grid.
    line_mesh_t floor_grid_mesh;
    mat4x4f_t floor_grid_model_matrix;  // NOTE: 将来、Camera操作とは独立してModelを移動・回転する可能性がある。
                                        //       Floor GridをModelの変換から独立させるため、専用のModel Matrixを保持する。

    // Camera
    orbit_camera_t* orbit_camera;
    mat4x4f_t view_matrix;
    mat4x4f_t projection_matrix;
    float near_clip;
    float far_clip;

    // Lighting.
    directional_light_t directional_light;
    ambient_light_t ambient_light;

    // Rendering.
    render_packet_t* render_packet;
    bool show_debug_geometry;
} stl_viewer_t;

// Scene Configuration (Rendering Model).
static const untextured_mesh_config_t s_model_mesh_config = {
    .geometry_subdirectory = "stl/demo_stl_viewer",
    .geometry_resource_name = "utah_teapot_ascii",
    .geometry_extension = "stl",
    .material_subdirectory = "materials/demo_stl_viewer",
    .material_resource_name = "utah_teapot",
    .material_extension = "untextured_mat",
};

// Scene Configuration(View)
static const float s_default_fovy_degrees = 45.0f;

// Vtable Functions.
static scene_result_t stl_viewer_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, void** out_scene_concrete_);
static void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, void** scene_concrete_);
static scene_result_t stl_viewer_update(void* scene_concrete_, const engine_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_);
static scene_result_t stl_viewer_draw(void* scene_concrete_, renderer_frontend_t* renderer_frontend_);
static bool stl_viewer_is_valid(const void* scene_concrete_);

// Resource Initializer.
static scene_result_t model_mesh_import(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, untextured_mesh_t* out_mesh_, aabb_3d_t* out_model_aabb_);
static scene_result_t debug_axis_mesh_initialize(renderer_frontend_t* renderer_frontend_, const char* src_resource_name_, const aabb_3d_t* aabb_, untextured_mesh_t out_mesh_[3]);
static scene_result_t debug_axis_resource_name_create(const char* source_resource_name_, const char* resource_category_, const char* axis_name_, choco_string_t** out_string_);
static scene_result_t floor_grid_mesh_initialize(renderer_frontend_t* renderer_frontend_, const aabb_3d_t* aabb_, line_mesh_t* out_mesh_);
static scene_result_t debug_aabb_mesh_initialize(renderer_frontend_t* renderer_frontend_, const char* src_resource_name_, const aabb_3d_t* model_aabb_, line_mesh_t* out_mesh_);

// Camera.
static scene_result_t viewer_camera_create(const aabb_3d_t* model_aabb_, int framebuffer_width_, int framebuffer_height_, mat4x4f_t* out_view_matrix_, mat4x4f_t* out_projection_matrix_, float* out_near_clip_, float* out_far_clip_, orbit_camera_t** out_orbit_camera_);
static scene_result_t initial_camera_parameters_calculate(const aabb_3d_t* aabb_, float fovy_degrees_, vec3f_t* out_target_, float* out_distance_, float* out_near_clip_, float* out_far_clip_);

// Render Packet.
static scene_result_t render_packet_build(stl_viewer_t* stl_viewer_);

// Scene Vtable.
static const scene_vtable_t s_stl_viewer_vtable = {
    .scene_create = stl_viewer_create,
    .scene_destroy = stl_viewer_destroy,
    .scene_draw = stl_viewer_draw,
    .scene_update = stl_viewer_update,
    .scene_is_valid = stl_viewer_is_valid,
};

const scene_vtable_t* stl_viewer_vtable_get(void) {
    return &s_stl_viewer_vtable;
}

static scene_result_t stl_viewer_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, void** out_scene_concrete_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;
    light_result_t ret_light = LIGHT_INVALID_ARGUMENT;

    stl_viewer_t* tmp_stl_viewer = NULL;
    orbit_camera_t* tmp_orbit_camera = NULL;
    render_packet_t* tmp_render_packet = NULL;

    aabb_3d_t model_aabb = { 0 };
    line_mesh_t tmp_floor_grid_mesh = { 0 };
    line_mesh_t tmp_debug_aabb_mesh = { 0 };
    untextured_mesh_t tmp_debug_axis_mesh[3];
    untextured_mesh_t tmp_model_mesh = { 0 };
    mat4x4f_t tmp_floor_grid_model_matrix = { 0 };

    bool model_mesh_imported = false;
    bool floor_grid_mesh_imported = false;
    bool debug_aabb_mesh_imported = false;
    bool debug_axis_mesh_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(out_scene_concrete_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "out_scene_concrete_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_scene_concrete_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "stl_viewer_create", "*out_scene_concrete_")
    if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("stl_viewer_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Viewer / Render Packet.
    ret_general_allocator = general_allocator_allocate(sizeof(stl_viewer_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_stl_viewer);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_render_packet = render_packet_create(16 * MIB, &tmp_render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - render_packet_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Model Resource.
    ret = model_mesh_import(renderer_frontend_, asset_root_path_, &tmp_model_mesh, &model_aabb);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - model_mesh_import failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    model_mesh_imported = true;

    // Camera.
    ret = viewer_camera_create(&model_aabb, framebuffer_width_, framebuffer_height_, &tmp_stl_viewer->view_matrix, &tmp_stl_viewer->projection_matrix, &tmp_stl_viewer->near_clip, &tmp_stl_viewer->far_clip, &tmp_orbit_camera);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - viewer_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Floor Grid.
    ret = floor_grid_mesh_initialize(renderer_frontend_, &model_aabb, &tmp_floor_grid_mesh);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - floor_grid_mesh_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_floor_grid_model_matrix);
    floor_grid_mesh_imported = true;

    // Debug Geometry.
    ret = debug_aabb_mesh_initialize(renderer_frontend_, s_model_mesh_config.geometry_resource_name, &model_aabb, &tmp_debug_aabb_mesh);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - debug_aabb_mesh_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    debug_aabb_mesh_imported = true;

    ret = debug_axis_mesh_initialize(renderer_frontend_, s_model_mesh_config.geometry_resource_name, &model_aabb, tmp_debug_axis_mesh);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - debug_axis_mesh_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    debug_axis_mesh_imported = true;

    // Lighting.
    // NOTE: 設定値の意味
    // direction: 正面から少し斜め上の光で、面の違いが見やすい
    // color: 白色光。色による影響を除外できる
    // intensity: 明るさの計算と結果を比較しやすい
    ret_light = directional_light_initialize(vec3f_initialize(0.4f, 0.8f, 1.0f), vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_stl_viewer->directional_light);
    if(LIGHT_SUCCESS != ret_light) {
        ret = scene_result_convert_light(ret_light);
        ERROR_MESSAGE("stl_viewer_create(%s) - directional_light_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_light = ambient_light_initialize(vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_stl_viewer->ambient_light);
    if(LIGHT_SUCCESS != ret_light) {
        ret = scene_result_convert_light(ret_light);
        ERROR_MESSAGE("stl_viewer_create(%s) - ambient_light_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    mat4f_identity(&tmp_stl_viewer->model_matrix);
    tmp_stl_viewer->model_mesh = tmp_model_mesh;
    tmp_stl_viewer->render_packet = tmp_render_packet;
    tmp_stl_viewer->orbit_camera = tmp_orbit_camera;
    tmp_stl_viewer->floor_grid_mesh = tmp_floor_grid_mesh;
    tmp_stl_viewer->floor_grid_model_matrix = tmp_floor_grid_model_matrix;
    tmp_stl_viewer->debug_aabb_mesh = tmp_debug_aabb_mesh;
    tmp_stl_viewer->debug_axis_meshes[0] = tmp_debug_axis_mesh[0];
    tmp_stl_viewer->debug_axis_meshes[1] = tmp_debug_axis_mesh[1];
    tmp_stl_viewer->debug_axis_meshes[2] = tmp_debug_axis_mesh[2];
    tmp_stl_viewer->show_debug_geometry = true;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(tmp_stl_viewer)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_create(%s) - Result validation failed for 'tmp_stl_viewer'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_scene_concrete_ = (void*)tmp_stl_viewer;
    tmp_stl_viewer = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != renderer_frontend_ && debug_aabb_mesh_imported) {
        ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, &tmp_debug_aabb_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_line_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != renderer_frontend_ && floor_grid_mesh_imported) {
        ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, &tmp_floor_grid_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_line_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != renderer_frontend_ && debug_axis_mesh_imported) {
        for(size_t i = 0; i != 3; ++i) {
            ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_debug_axis_mesh[i]);
            if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
                ret = SCENE_DATA_CORRUPTED;
                ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
                return ret;
            }
        }
    }
    if(NULL != renderer_frontend_ && model_mesh_imported) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_model_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != tmp_render_packet) {
        render_packet_destroy(&tmp_render_packet);
    }
    if(NULL != tmp_orbit_camera) {
        orbit_camera_destroy(&tmp_orbit_camera);
    }
    if(NULL != tmp_stl_viewer) {
        general_allocator_free((void**)&tmp_stl_viewer, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, void** scene_concrete_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    if(NULL == scene_concrete_ || NULL == *scene_concrete_ || NULL == renderer_frontend_) {
        return;
    }
    stl_viewer_t* tmp_stl_viewer = (stl_viewer_t*)(*scene_concrete_);

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(*scene_concrete_)) {
        ERROR_MESSAGE("stl_viewer_destroy(%s) - Precondition validation failed for '*scene_concrete_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif

    ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, &tmp_stl_viewer->debug_aabb_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_destroy(%s) - renderer_frontend_line_mesh_release failed.", scene_result_to_str(ret));
        if(SCENE_DATA_CORRUPTED == ret) {
            return;
        }
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, &tmp_stl_viewer->floor_grid_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
        if(SCENE_DATA_CORRUPTED == ret) {
            return;
        }
    }

    for(size_t i = 0; i != 3; ++i) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_stl_viewer->debug_axis_meshes[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("stl_viewer_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
            if(SCENE_DATA_CORRUPTED == ret) {
                return;
            }
        }
    }

    ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_stl_viewer->model_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
        if(SCENE_DATA_CORRUPTED == ret) {
            return;
        }
    }

    render_packet_destroy(&tmp_stl_viewer->render_packet);
    orbit_camera_destroy(&tmp_stl_viewer->orbit_camera);

    general_allocator_free((void**)scene_concrete_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

static scene_result_t stl_viewer_update(
    void* scene_concrete_,
    const engine_event_view_t* event_view_,
    float delta_time_,
    bool window_resized_,
    int framebuffer_width_,
    int framebuffer_height_) {
    (void)delta_time_;

    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    stl_viewer_t* stl_viewer = NULL;

    float aspect = 0.0f;
    bool projection_changed = false;
    bool view_changed = false;

    IF_ARG_NULL_GOTO_CLEANUP(scene_concrete_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_update", "scene_concrete_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_update", "event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(scene_concrete_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_update(%s) - Precondition validation failed for 'scene_concrete_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    stl_viewer = (stl_viewer_t*)scene_concrete_;

    for(size_t i = 0; i != event_view_->keyboard_event_count; ++i) {
        if(KEY_D == event_view_->keyboard_events[i].key && event_view_->keyboard_events[i].event_args.pressed) {
            stl_viewer->show_debug_geometry = !stl_viewer->show_debug_geometry;
        }
    }

    for(size_t i = 0; i != event_view_->mouse_event_count; ++i) {
        ret_camera = orbit_camera_command_update(stl_viewer->orbit_camera, &event_view_->mouse_events[i]);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - orbit_camera_command_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret_camera = orbit_camera_command_execute(stl_viewer->orbit_camera, &view_changed);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("stl_viewer_update(%s) - orbit_camera_command_execute failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    if(window_resized_ && 0 < framebuffer_height_ && 0 < framebuffer_width_) { // window最小化等でframebuffer_heightが0の場合は視錐台の更新は行わない
        aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
        ret_camera = orbit_camera_viewing_frustum_update(stl_viewer->orbit_camera, s_default_fovy_degrees, aspect, stl_viewer->near_clip, stl_viewer->far_clip);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - orbit_camera_viewing_frustum_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        projection_changed = true;
    }

    if(view_changed) {
        ret_camera = orbit_camera_view_matrix_get(stl_viewer->orbit_camera, &stl_viewer->view_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - orbit_camera_view_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    if(projection_changed) {
        ret_camera = orbit_camera_perspective_matrix_get(stl_viewer->orbit_camera, &stl_viewer->projection_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - orbit_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t stl_viewer_draw(void* scene_concrete_, renderer_frontend_t* renderer_frontend_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    stl_viewer_t* stl_viewer = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(scene_concrete_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_draw", "scene_concrete_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_draw", "renderer_frontend_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(scene_concrete_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_draw(%s) - Precondition validation failed for 'scene_concrete_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    stl_viewer = (stl_viewer_t*)scene_concrete_;

    ret = render_packet_build(stl_viewer);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_draw(%s) - render_packet_build failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_untextured_mesh_render_update(renderer_frontend_, &stl_viewer->view_matrix, &stl_viewer->projection_matrix, &stl_viewer->directional_light, &stl_viewer->ambient_light);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_untextured_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_render_update(renderer_frontend_, &stl_viewer->view_matrix, &stl_viewer->projection_matrix);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_line_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_draw_packet(stl_viewer->render_packet, renderer_frontend_);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_draw_packet failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_draw_packet failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static bool stl_viewer_is_valid(const void* scene_concrete_) {
    // Chat GPT Generated.
    if(NULL == scene_concrete_) {
        return false;
    }

    const stl_viewer_t* stl_viewer = (const stl_viewer_t*)scene_concrete_;

    if(!general_allocator_ptr_is_allocated(stl_viewer)) {
        return false;
    }

    // Owned Camera.
    if(NULL == stl_viewer->orbit_camera) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(stl_viewer->orbit_camera)) {
        return false;
    }
    if(!orbit_camera_is_valid(stl_viewer->orbit_camera)) {
        return false;
    }

    // Owned Render Packet.
    if(NULL == stl_viewer->render_packet) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(stl_viewer->render_packet)) {
        return false;
    }
    if(!render_packet_is_valid(stl_viewer->render_packet)) {
        return false;
    }

    // Camera Clip.
    if(!isfinite(stl_viewer->near_clip) || !isfinite(stl_viewer->far_clip)) {
        return false;
    }
    if(stl_viewer->near_clip <= 0.0f || stl_viewer->far_clip <= stl_viewer->near_clip) {
        return false;
    }

    // Matrices.
    for(size_t i = 0; i != 16; ++i) {
        if(!isfinite(stl_viewer->model_matrix.elem[i]) ||
           !isfinite(stl_viewer->floor_grid_model_matrix.elem[i]) ||
           !isfinite(stl_viewer->view_matrix.elem[i]) ||
           !isfinite(stl_viewer->projection_matrix.elem[i])) {
            return false;
        }
    }

    // Lighting.
    if(!directional_light_is_valid(&stl_viewer->directional_light)) {
        return false;
    }
    if(!ambient_light_is_valid(&stl_viewer->ambient_light)) {
        return false;
    }

    return true;
}

static scene_result_t model_mesh_import(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, untextured_mesh_t* out_mesh_, aabb_3d_t* out_model_aabb_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb = { 0 };
    untextured_mesh_t tmp_mesh = { 0 };

    bool mesh_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "model_mesh_import", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "model_mesh_import", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "model_mesh_import", "out_mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(out_model_aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "model_mesh_import", "out_model_aabb_")

    ret_renderer_frontend = renderer_frontend_untextured_mesh_import(renderer_frontend_, &s_model_mesh_config, asset_root_path_, &tmp_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("model_mesh_import(%s) - renderer_frontend_untextured_mesh_import failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mesh_imported = true;

    ret_renderer_frontend = renderer_frontend_untextured_mesh_local_aabb_get(renderer_frontend_, &tmp_mesh, &tmp_aabb);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("model_mesh_import(%s) - renderer_frontend_untextured_mesh_local_aabb_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_mesh_ = tmp_mesh;
    *out_model_aabb_ = tmp_aabb;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(mesh_imported) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("model_mesh_import(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    return ret;
}

static scene_result_t debug_axis_mesh_initialize(renderer_frontend_t* renderer_frontend_, const char* src_resource_name_, const aabb_3d_t* aabb_, untextured_mesh_t out_mesh_[3]) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;

    choco_string_t* tmp_geometry_resource_name = NULL;
    choco_string_t* tmp_material_resource_name = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    untextured_mesh_t tmp_mesh[3];
    const char* axis_names[3] = { "x", "y", "z" };
    const vec3f_t axis_rotations_degrees[3] = { vec3f_initialize(0.0f, 90.0f, 0.0f), vec3f_initialize(-90.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f) };
    const untextured_mesh_material_t axis_materials[3] = {
        {
            .diffuse = vec3f_initialize(1.0f, 0.0f, 0.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        },
        {
            .diffuse = vec3f_initialize(0.0f, 1.0f, 0.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        },
        {
            .diffuse = vec3f_initialize(0.0f, 0.0f, 1.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        }
    };
    size_t tmp_vertex_count = 0;
    size_t imported_mesh_count = 0;

    float max_aabb_extent = 0.0f;
    float axis_length = 0.0f;
    vec3f_t aabb_length = { 0 };

    cylinder_t cylinder = { 0 };
    cone_t cone = { 0 };
    mat4x4f_t axis_rotation_matrix = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_mesh_initialize", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(src_resource_name_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_mesh_initialize", "src_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_mesh_initialize", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_mesh_initialize", "out_mesh_")
    if('\0' == src_resource_name_[0]) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - Provided src_resource_name_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != 3; ++i) {
        aabb_length.elem[i] = aabb_->max.elem[i] - aabb_->min.elem[i];
        max_aabb_extent = (max_aabb_extent < aabb_length.elem[i]) ? aabb_length.elem[i] : max_aabb_extent;
    }
    if(!(max_aabb_extent > 0.0f)) {
        ret = SCENE_RUNTIME_ERROR;
        ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - Provided aabb_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }
    axis_length = max_aabb_extent * 0.7f;   // Debug Axisの長さをモデルAABBの最大辺長の70%とし、モデルの大きさに応じて表示サイズを調整する。

    cylinder.height = axis_length * 0.8f;
    cone.height = axis_length * 0.2f;

    cylinder.radius = axis_length * 0.01f;
    cone.radius = cylinder.radius * 2.0f;

    // Generate Geometry, Mesh Import.
    for(size_t i = 0; i != 3; ++i) {    // X, Y, Z各軸ループ
        ret = debug_axis_resource_name_create(src_resource_name_, "geometry", axis_names[i], &tmp_geometry_resource_name);
        if(SCENE_SUCCESS != ret) {
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - debug_axis_resource_name_create failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret = debug_axis_resource_name_create(src_resource_name_, "material", axis_names[i], &tmp_material_resource_name);
        if(SCENE_SUCCESS != ret) {
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - debug_axis_resource_name_create failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        tmp_vertex_count = 0;
        ret_geometry = untextured_arrow_geometry_vertices_create(16, &cone, &cylinder, &tmp_vertices, &tmp_vertex_count);
        if(GEOMETRY_SUCCESS != ret_geometry) {
            ret = scene_result_convert_geometry(ret_geometry);
            if(GEOMETRY_INVALID_ARGUMENT == ret_geometry) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - untextured_arrow_geometry_vertices_create failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        mat4f_rot_xyz(CHOCO_DEG_TO_RAD(axis_rotations_degrees[i].elem[0]), CHOCO_DEG_TO_RAD(axis_rotations_degrees[i].elem[1]), CHOCO_DEG_TO_RAD(axis_rotations_degrees[i].elem[2]), &axis_rotation_matrix);
        if(!point_normal_vertices_transform(tmp_vertices, tmp_vertex_count, &axis_rotation_matrix)) {
            ret = SCENE_RUNTIME_ERROR;
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - point_normal_vertices_transform failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_renderer_frontend = renderer_frontend_untextured_mesh_import_from_vertices(renderer_frontend_, choco_string_c_str(tmp_geometry_resource_name), choco_string_c_str(tmp_material_resource_name), tmp_vertices, tmp_vertex_count, &axis_materials[i], &tmp_mesh[imported_mesh_count]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
            if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - renderer_frontend_untextured_mesh_import_from_vertices failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        imported_mesh_count++;

        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        choco_string_destroy(&tmp_geometry_resource_name);
        choco_string_destroy(&tmp_material_resource_name);
    }

    out_mesh_[0] = tmp_mesh[0];
    out_mesh_[1] = tmp_mesh[1];
    out_mesh_[2] = tmp_mesh[2];

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    for(size_t i = 0; i != imported_mesh_count; ++i) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_mesh[i])) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("debug_axis_mesh_initialize(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != tmp_geometry_resource_name) {
        choco_string_destroy(&tmp_geometry_resource_name);
    }

    if(NULL != tmp_material_resource_name) {
        choco_string_destroy(&tmp_material_resource_name);
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

// NOTE:
// - *out_string_ = source_resource_name_ + "_debug_axis_" + resource_category_ + "_" + axis_name_
// - resource_category_ = material or geometry
// - axis_name_ = x or y or z
static scene_result_t debug_axis_resource_name_create(const char* source_resource_name_, const char* resource_category_, const char* axis_name_, choco_string_t** out_string_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(source_resource_name_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_resource_name_create", "source_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_category_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_resource_name_create", "resource_category_")
    IF_ARG_NULL_GOTO_CLEANUP(axis_name_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_resource_name_create", "axis_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_axis_resource_name_create", "out_string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_string_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "debug_axis_resource_name_create", "*out_string_")

    ret_choco_string = choco_string_create_from_c_string(source_resource_name_, &tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_resource_name_create(%s) - choco_string_create_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_debug_axis_", tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_resource_name_create(%s) - choco_string_concat_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string(resource_category_, tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_resource_name_create(%s) - choco_string_concat_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_", tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_resource_name_create(%s) - choco_string_concat_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string(axis_name_, tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_resource_name_create(%s) - choco_string_concat_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_string_ = tmp_string;
    tmp_string = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_string) {
        choco_string_destroy(&tmp_string);
    }
    return ret;
}

static scene_result_t floor_grid_mesh_initialize(renderer_frontend_t* renderer_frontend_, const aabb_3d_t* aabb_, line_mesh_t* out_mesh_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;

    line_vertex_t* tmp_vertices = NULL;
    size_t tmp_vertex_count = 0;
    line_mesh_t tmp_mesh = { 0 };

    float max_aabb_extent = 0.0f;
    vec3f_t aabb_length = { 0 };
    vec3f_t center = { 0 };
    vec3f_t grid_min = { 0 };
    vec3f_t grid_max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_initialize", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_initialize", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_initialize", "out_mesh_")

    for(size_t i = 0; i != 2; ++i) {    // XY平面へのGrid生成なので、zは除外
        aabb_length.elem[i] = aabb_->max.elem[i] - aabb_->min.elem[i];
        max_aabb_extent = (max_aabb_extent < aabb_length.elem[i]) ? aabb_length.elem[i] : max_aabb_extent;
    }
    if(!(max_aabb_extent > 0.0f)) {
        ret = SCENE_RUNTIME_ERROR;
        ERROR_MESSAGE("floor_grid_mesh_initialize(%s) - Provided aabb is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    center.elem[0] = (aabb_->max.elem[0] + aabb_->min.elem[0]) / 2.0f;
    center.elem[1] = (aabb_->max.elem[1] + aabb_->min.elem[1]) / 2.0f;
    center.elem[2] = aabb_->min.elem[2];
    for(size_t i = 0; i != 2; ++i) {
        grid_min.elem[i] = center.elem[i] - (max_aabb_extent / 2.0f);
        grid_max.elem[i] = center.elem[i] + (max_aabb_extent / 2.0f);
    }
    grid_min.elem[2] = center.elem[2];
    grid_max.elem[2] = center.elem[2];

    // STLモデルのZ軸を鉛直方向とし、
    // モデルAABBのZ最小値を通るXY平面にFloor Gridを配置する。
    ret_geometry = line_grid_geometry_vertices_create(LINE_GRID_PLANE_XY, vec4u8_initialize(128, 128, 128, 255), grid_min, grid_max, max_aabb_extent / 10.0f, &tmp_vertices, &tmp_vertex_count);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = scene_result_convert_geometry(ret_geometry);
        if(GEOMETRY_INVALID_ARGUMENT == ret_geometry) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_initialize(%s) - line_grid_geometry_vertices_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_import_from_vertices(renderer_frontend_, "floor_grid_line", tmp_vertices, tmp_vertex_count, &tmp_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_initialize(%s) - renderer_frontend_line_mesh_import_from_vertices failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_mesh_ = tmp_mesh;
    general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static scene_result_t debug_aabb_mesh_initialize(renderer_frontend_t* renderer_frontend_, const char* src_resource_name_, const aabb_3d_t* model_aabb_, line_mesh_t* out_mesh_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    choco_string_t* tmp_resource_name = NULL;
    line_mesh_t tmp_debug_aabb_mesh = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_aabb_mesh_initialize", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(src_resource_name_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_aabb_mesh_initialize", "src_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(model_aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_aabb_mesh_initialize", "model_aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "debug_aabb_mesh_initialize", "out_mesh_")

    ret_choco_string = choco_string_create_from_c_string(src_resource_name_, &tmp_resource_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_aabb_mesh_initialize(%s) - choco_string_create_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_debug_aabb", tmp_resource_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = scene_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_aabb_mesh_initialize(%s) - choco_string_concat_from_c_string failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_import_from_aabb(renderer_frontend_, choco_string_c_str(tmp_resource_name), model_aabb_, vec4u8_initialize(255, 0, 0, 0), &tmp_debug_aabb_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("debug_aabb_mesh_initialize(%s) - renderer_frontend_line_mesh_import_from_aabb failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    choco_string_destroy(&tmp_resource_name);

    *out_mesh_ = tmp_debug_aabb_mesh;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_resource_name) {
        choco_string_destroy(&tmp_resource_name);
    }
    return ret;
}

static scene_result_t viewer_camera_create(const aabb_3d_t* model_aabb_, int framebuffer_width_, int framebuffer_height_, mat4x4f_t* out_view_matrix_, mat4x4f_t* out_projection_matrix_, float* out_near_clip_, float* out_far_clip_, orbit_camera_t** out_orbit_camera_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    orbit_camera_t* tmp_orbit_camera = NULL;  // Temporary

    mat4x4f_t tmp_view_matrix = { 0 };
    mat4x4f_t tmp_projection_matrix = { 0 };

    vec3f_t tmp_target = { 0 };
    float tmp_distance = 0.0f;
    float tmp_near_clip = 0.0f;
    float tmp_far_clip = 0.0f;

    float aspect = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(model_aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "model_aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_view_matrix_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "out_view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(out_projection_matrix_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "out_projection_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(out_near_clip_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "out_near_clip_")
    IF_ARG_NULL_GOTO_CLEANUP(out_far_clip_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "out_far_clip_")
    IF_ARG_NULL_GOTO_CLEANUP(out_orbit_camera_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "viewer_camera_create", "out_orbit_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_orbit_camera_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "viewer_camera_create", "*out_orbit_camera_")
    if(framebuffer_width_ <= 0 || framebuffer_height_ <= 0) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("viewer_camera_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = initial_camera_parameters_calculate(model_aabb_, s_default_fovy_degrees, &tmp_target, &tmp_distance, &tmp_near_clip, &tmp_far_clip);
    if(SCENE_SUCCESS != ret) {
        ERROR_MESSAGE("viewer_camera_create(%s) - initial_camera_parameters_calculate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
    ret_camera = orbit_camera_create(s_default_fovy_degrees, aspect, tmp_near_clip, tmp_far_clip, tmp_target, tmp_distance, &tmp_orbit_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("viewer_camera_create(%s) - orbit_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = orbit_camera_view_matrix_get(tmp_orbit_camera, &tmp_view_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("viewer_camera_create(%s) - orbit_camera_view_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = orbit_camera_perspective_matrix_get(tmp_orbit_camera, &tmp_projection_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("viewer_camera_create(%s) - orbit_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_view_matrix_ = tmp_view_matrix;
    *out_projection_matrix_ = tmp_projection_matrix;
    *out_near_clip_ = tmp_near_clip;
    *out_far_clip_ = tmp_far_clip;
    *out_orbit_camera_ = tmp_orbit_camera;
    tmp_orbit_camera = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_orbit_camera) {
        orbit_camera_destroy(&tmp_orbit_camera);
    }
    return ret;
}

// NOTE: 現在のCamera Distance計算は垂直FOVのみを使用している。
// - 縦長Windowでは水平FOVが垂直FOVより狭くなるため、
//   Bounding Sphere全体が描画範囲に収まらない可能性がある。
// - 将来的にはAspect Ratioを考慮し、水平・垂直FOVのうち
//   狭い方を基準にCamera Distanceを決定する。
// - Window Resize時のCamera Distance再調整も併せて検討する。
// - 現時点ではPendingとする。
static scene_result_t initial_camera_parameters_calculate(const aabb_3d_t* aabb_, float fovy_degrees_, vec3f_t* out_target_, float* out_distance_, float* out_near_clip_, float* out_far_clip_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    vec3f_t tmp_target = { 0 };
    vec3f_t aabb_extent = { 0 };
    float radius = 0.0f;
    float tmp_distance = 0.0f;
    float tmp_near_clip = 0.0f;
    float tmp_far_clip = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "initial_camera_parameters_calculate", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_target_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "initial_camera_parameters_calculate", "out_target_")
    IF_ARG_NULL_GOTO_CLEANUP(out_distance_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "initial_camera_parameters_calculate", "out_distance_")
    IF_ARG_NULL_GOTO_CLEANUP(out_near_clip_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "initial_camera_parameters_calculate", "out_near_clip_")
    IF_ARG_NULL_GOTO_CLEANUP(out_far_clip_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "initial_camera_parameters_calculate", "out_far_clip_")

    // AABBを包含するbounding sphereの半径を求める
    for(size_t i = 0; i != 3; ++i) {
        tmp_target.elem[i] = aabb_->min.elem[i] + (aabb_->max.elem[i] - aabb_->min.elem[i]) * 0.5f;
        aabb_extent.elem[i] = aabb_->max.elem[i] - aabb_->min.elem[i];
    }
    radius = vec3f_length(aabb_extent) * 0.5f;

    // NOTE: 視点, 中視点, 視点からbounding sphereへの接線によりできる直角三角形の斜辺がdistance
    tmp_distance = 1.2f * radius / (sinf(CHOCO_DEG_TO_RAD(fovy_degrees_ / 2.0f))); // 1.2fは固定の余白係数

    // NOTE: Bounding Sphere全体が描画範囲に収まるようにNear/Far Clipを設定する。
    // - Sphereの手前端はdistance - radius、奥端はdistance + radius。
    // - 描画範囲に余裕を持たせるため、radiusを1.5倍してNear/Farを計算する。
    // - Near Clipは正値が必要なので、radiusの1%を下限とする。
    tmp_near_clip = (0.01f * radius < (tmp_distance - 1.5f * radius)) ? (tmp_distance - 1.5f * radius) : 0.01f * radius;
    tmp_far_clip = 1.5f * radius + tmp_distance;

    *out_distance_ = tmp_distance;
    *out_target_ = tmp_target;
    *out_near_clip_ = tmp_near_clip;
    *out_far_clip_ = tmp_far_clip;

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t render_packet_build(stl_viewer_t* stl_viewer_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_viewer_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "render_packet_build", "stl_viewer_")
    if(NULL == stl_viewer_->render_packet) {
        ret = SCENE_BAD_OPERATION;
        ERROR_MESSAGE("render_packet_build(%s) - Provided stl_viewer_ is not initialized.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Line: 2 (Floor Grid, Debug AABB)
    // Point: 0, UI: 0
    // Untextured: 4 (Model, Debug Axis X/Y/Z)
    render_packet_deinitialize(stl_viewer_->render_packet);
    ret_render_packet = render_packet_initialize(2, 0, 0, 4, stl_viewer_->render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_untextured_mesh_push(&stl_viewer_->model_mesh, &stl_viewer_->model_matrix, stl_viewer_->render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_untextured_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_line_mesh_push(&stl_viewer_->floor_grid_mesh, &stl_viewer_->floor_grid_model_matrix, stl_viewer_->render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_line_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    if(stl_viewer_->show_debug_geometry) {
        for(size_t i = 0; i != 3; ++i) {
            ret_render_packet = render_packet_untextured_mesh_push(&stl_viewer_->debug_axis_meshes[i], &stl_viewer_->model_matrix, stl_viewer_->render_packet);
            if(RENDER_PACKET_SUCCESS != ret_render_packet) {
                ret = scene_result_convert_render_packet(ret_render_packet);
                if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                    ret = SCENE_DATA_CORRUPTED;
                }
                ERROR_MESSAGE("render_packet_build(%s) - render_packet_untextured_mesh_push failed.", scene_result_to_str(ret));
                goto cleanup;
            }
        }

        ret_render_packet = render_packet_line_mesh_push(&stl_viewer_->debug_aabb_mesh, &stl_viewer_->model_matrix, stl_viewer_->render_packet);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = scene_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_build(%s) - render_packet_line_mesh_push failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}
