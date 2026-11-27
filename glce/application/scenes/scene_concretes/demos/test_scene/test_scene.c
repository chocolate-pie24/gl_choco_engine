// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/scenes/scene_concretes/demos/test_scene/test_scene.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

#include "engine/camera/flight_camera.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

#include "engine/systems/renderer/resources/geometries/point_mesh_geometry.h"
#include "engine/systems/renderer/resources/geometries/ui_mesh_geometry.h"

#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/scenes/core/scene_types.h"
#include "application/scenes/core/scene_err_utils.h"

typedef struct {
    ui_mesh_t large_icon_ui_mesh;
    ui_mesh_t small_icon_ui_mesh;
    ui_mesh_t solid_color_ui_mesh_large;
    ui_mesh_t solid_color_ui_mesh_small;

    mat4x4f_t large_icon_ui_mesh_model_matrix;
    mat4x4f_t small_icon_ui_mesh_model_matrix;
    mat4x4f_t solid_color_ui_mesh_large_model_matrix;
    mat4x4f_t solid_color_ui_mesh_small_model_matrix;

    // Point Mesh.
    point_mesh_t point_mesh;
    mat4x4f_t point_mesh_model_matrix;

    // Camera.
    flight_camera_t* flight_camera;
    float camera_speed;
    mat4x4f_t view_matrix;
    mat4x4f_t projection_matrix;

    // Rendering.
    render_packet_t* render_packet;
} test_scene_t;

static const float s_default_fovy_degrees = 45.0f;
static const float s_default_near_clip = 0.1f;
static const float s_default_far_clip = 3000.0f;

// ChatGPT Generated
static const ui_mesh_config_t s_ui_mesh_configs[4] = {
    // Large Icon: Frog
    {
        .geometry_subdirectory = "geometries",
        .geometry_resource_name = "large_icon",
        .geometry_extension = "ui_geom",
        .texture_subdirectory = "textures",
        .texture_resource_name = "frog_512",
        .texture_extension = "bmp",
    },
    // Small Icon: Rabbit
    {
        .geometry_subdirectory = "geometries",
        .geometry_resource_name = "small_icon",
        .geometry_extension = "ui_geom",
        .texture_subdirectory = "textures",
        .texture_resource_name = "rabbit_512",
        .texture_extension = "bmp",
    },
    // Solid Color: Large (Amber)
    {
        .geometry_subdirectory = "geometries",
        .geometry_resource_name = "large_icon",
        .geometry_extension = "ui_geom",
        .texture_subdirectory = NULL,
        .texture_resource_name = "test_solid_amber",
        .texture_extension = NULL,
    },
    // Solid Color: Small (Turquoise)
    {
        .geometry_subdirectory = "geometries",
        .geometry_resource_name = "small_icon",
        .geometry_extension = "ui_geom",
        .texture_subdirectory = NULL,
        .texture_resource_name = "test_solid_turquoise",
        .texture_extension = NULL,
    },
};

// Vtable Functions.
static scene_result_t test_scene_create(renderer_frontend_t* renderer_frontend_, const void* scene_config_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, void** out_scene_concrete_);
static void test_scene_destroy(renderer_frontend_t* renderer_frontend_, void** scene_concrete_);
static scene_result_t test_scene_update(void* scene_concrete_, const platform_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_);
static scene_result_t test_scene_draw(void* scene_concrete_, renderer_frontend_t* renderer_frontend_);
static bool test_scene_is_valid(const void* scene_concrete_);

// Camera.
static scene_result_t test_scene_camera_create(const flight_camera_key_bind_t* camera_key_binds_, int framebuffer_width_, int framebuffer_height_, mat4x4f_t* out_view_matrix_, mat4x4f_t* out_projection_matrix_, flight_camera_t** out_flight_camera_);

// Render Packet.
static scene_result_t render_packet_build(const test_scene_t* test_scene_, render_packet_t* packet_);

static scene_result_t point_vertices_create(point_vertex_t** out_vertices_, size_t* out_vertex_count_);
static void point_vertices_destroy(point_vertex_t** vertices_);

static const scene_vtable_t s_test_scene_vtable = {
    .scene_create = test_scene_create,
    .scene_destroy = test_scene_destroy,
    .scene_draw = test_scene_draw,
    .scene_update = test_scene_update,
    .scene_is_valid = test_scene_is_valid,
};

const scene_vtable_t* test_scene_vtable_get(void) {
    return &s_test_scene_vtable;
}

static scene_result_t test_scene_create(renderer_frontend_t* renderer_frontend_, const void* scene_config_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, void** out_scene_concrete_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    const test_scene_config_t* scene_config = NULL;
    test_scene_t* tmp_test_scene = NULL;
    point_vertex_t* tmp_test_vertices = NULL;
    flight_camera_t* tmp_flight_camera = NULL;
    render_packet_t* tmp_render_packet = NULL;

    size_t tmp_vertex_count = 0;

    bool point_mesh_imported = false;
    bool large_icon_ui_mesh_imported = false;
    bool small_icon_ui_mesh_imported = false;
    bool solid_color_ui_mesh_large_imported = false;
    bool solid_color_ui_mesh_small_imported = false;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(scene_config_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_scene_concrete_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_scene_concrete_, ret, SCENE_BAD_OPERATION);
    if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("test_scene_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(test_scene_t), GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION, (void**)&tmp_test_scene);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("test_scene_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_create(16 * MIB, &tmp_render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("test_scene_create(%s) - render_packet_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Camera.
    scene_config = (const test_scene_config_t*)scene_config_;
    ret = test_scene_camera_create(scene_config->flight_camera_keybinds, framebuffer_width_, framebuffer_height_, &tmp_test_scene->view_matrix, &tmp_test_scene->projection_matrix, &tmp_flight_camera);
    if(SCENE_SUCCESS != ret) {
        ERROR_MESSAGE("test_scene_create(%s) - test_scene_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Point Vertex
    ret = point_vertices_create(&tmp_test_vertices, &tmp_vertex_count);
    if(SCENE_SUCCESS != ret) {
        ERROR_MESSAGE("test_scene_create(%s) - point_vertices_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_test_scene->point_mesh_model_matrix);
    ret_renderer_frontend = renderer_frontend_point_mesh_import_from_vertices(renderer_frontend_, "test_points", tmp_test_vertices, tmp_vertex_count, &tmp_test_scene->point_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_point_mesh_import_from_vertices failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    point_vertices_destroy(&tmp_test_vertices);
    point_mesh_imported = true;

    // Textures.
    ret_renderer_frontend = renderer_frontend_ui_mesh_import_from_file(renderer_frontend_, &s_ui_mesh_configs[0], asset_root_path_, 0, &tmp_test_scene->large_icon_ui_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_import_from_file failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_test_scene->large_icon_ui_mesh_model_matrix);
    mat4f_translation(vec3f_initialize(0.0f, 0.0f, 0.0f), &tmp_test_scene->large_icon_ui_mesh_model_matrix);
    large_icon_ui_mesh_imported = true;

    ret_renderer_frontend = renderer_frontend_ui_mesh_import_from_file(renderer_frontend_, &s_ui_mesh_configs[1], asset_root_path_, 0, &tmp_test_scene->small_icon_ui_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_import_from_file failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_test_scene->small_icon_ui_mesh_model_matrix);
    mat4f_translation(vec3f_initialize(3.0f, 0.0f, 0.0f), &tmp_test_scene->small_icon_ui_mesh_model_matrix);
    small_icon_ui_mesh_imported = true;

    ret_renderer_frontend = renderer_frontend_ui_mesh_import_from_solid_color(renderer_frontend_, &s_ui_mesh_configs[2], asset_root_path_, 232, 155, 69, 0, &tmp_test_scene->solid_color_ui_mesh_large);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_import_from_solid_color failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_test_scene->solid_color_ui_mesh_large_model_matrix);
    mat4f_translation(vec3f_initialize(0.0f, 3.0f, 0.0f), &tmp_test_scene->solid_color_ui_mesh_large_model_matrix);
    solid_color_ui_mesh_large_imported = true;

    ret_renderer_frontend = renderer_frontend_ui_mesh_import_from_solid_color(renderer_frontend_, &s_ui_mesh_configs[3], asset_root_path_, 69, 184, 176, 0, &tmp_test_scene->solid_color_ui_mesh_small);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_import_from_solid_color failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_test_scene->solid_color_ui_mesh_small_model_matrix);
    mat4f_translation(vec3f_initialize(3.0f, 3.0f, 0.0f), &tmp_test_scene->solid_color_ui_mesh_small_model_matrix);
    solid_color_ui_mesh_small_imported = true;

    tmp_test_scene->flight_camera = tmp_flight_camera;
    tmp_test_scene->camera_speed = 10.0f;
    tmp_test_scene->render_packet = tmp_render_packet;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!test_scene_is_valid(tmp_test_scene)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_create(%s) - Result validation failed for 'tmp_test_scene'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_scene_concrete_ = (void*)tmp_test_scene;
    scene_config = NULL;
    tmp_test_scene = NULL;
    tmp_test_vertices = NULL;
    tmp_flight_camera = NULL;
    tmp_render_packet = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(large_icon_ui_mesh_imported) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_ui_mesh_release(renderer_frontend_, &tmp_test_scene->large_icon_ui_mesh)) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(small_icon_ui_mesh_imported) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_ui_mesh_release(renderer_frontend_, &tmp_test_scene->small_icon_ui_mesh)) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(solid_color_ui_mesh_large_imported) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_ui_mesh_release(renderer_frontend_, &tmp_test_scene->solid_color_ui_mesh_large)) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(solid_color_ui_mesh_small_imported) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_ui_mesh_release(renderer_frontend_, &tmp_test_scene->solid_color_ui_mesh_small)) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(point_mesh_imported) {
        if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_point_mesh_release(renderer_frontend_, &tmp_test_scene->point_mesh)) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("test_scene_create(%s) - renderer_frontend_point_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }

    if(NULL != tmp_test_vertices) {
        point_vertices_destroy(&tmp_test_vertices);
    }
    if(NULL != tmp_render_packet) {
        render_packet_destroy(&tmp_render_packet);
    }
    if(NULL != tmp_flight_camera) {
        flight_camera_destroy(&tmp_flight_camera);
    }
    if(NULL != tmp_test_scene) {
        general_allocator_free((void**)&tmp_test_scene, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
    }
    return ret;
}

static void test_scene_destroy(renderer_frontend_t* renderer_frontend_, void** scene_concrete_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    if(NULL == scene_concrete_ || NULL == *scene_concrete_ || NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!test_scene_is_valid(*scene_concrete_)) {
        ERROR_MESSAGE("test_scene_destroy(%s) - Precondition validation failed for 'scene_concrete_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif
    test_scene_t* test_scene = (test_scene_t*)(*scene_concrete_);

    ret_renderer_frontend = renderer_frontend_point_mesh_release(renderer_frontend_, &test_scene->point_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_destroy(%s) - renderer_frontend_point_mesh_release failed.", scene_result_to_str(ret));
        return;
    }

    ret_renderer_frontend = renderer_frontend_ui_mesh_release(renderer_frontend_, &test_scene->large_icon_ui_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_destroy(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
        return;
    }
    ret_renderer_frontend = renderer_frontend_ui_mesh_release(renderer_frontend_, &test_scene->small_icon_ui_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_destroy(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
        return;
    }
    ret_renderer_frontend = renderer_frontend_ui_mesh_release(renderer_frontend_, &test_scene->solid_color_ui_mesh_large);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_destroy(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
        return;
    }
    ret_renderer_frontend = renderer_frontend_ui_mesh_release(renderer_frontend_, &test_scene->solid_color_ui_mesh_small);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_destroy(%s) - renderer_frontend_ui_mesh_release failed.", scene_result_to_str(ret));
        return;
    }

    render_packet_destroy(&test_scene->render_packet);
    flight_camera_destroy(&test_scene->flight_camera);

    general_allocator_free((void**)scene_concrete_, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
}

static scene_result_t test_scene_update(void* scene_concrete_, const platform_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    test_scene_t* test_scene = NULL;

    float aspect = 0.0f;
    bool projection_changed = false;
    bool view_changed = false;

    IF_ARG_NULL_GOTO_CLEANUP(scene_concrete_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, SCENE_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!test_scene_is_valid(scene_concrete_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_update(%s) - Precondition validation failed for 'scene_concrete_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    test_scene = (test_scene_t*)scene_concrete_;

    for(size_t i = 0; i != event_view_->keyboard_event_count; ++i) {
        ret_camera = flight_camera_command_update(test_scene->flight_camera, &event_view_->keyboard_events[i]);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("test_scene_update(%s) - flight_camera_command_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret_camera = flight_camera_command_execute(test_scene->flight_camera, test_scene->camera_speed, delta_time_, &view_changed);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("test_scene_update(%s) - flight_camera_command_execute failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    if(window_resized_ && 0 < framebuffer_height_ && 0 < framebuffer_width_) { // window最小化等でframebuffer_heightが0の場合は視錐台の更新は行わない
        aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
        ret_camera = flight_camera_viewing_frustum_update(test_scene->flight_camera, s_default_fovy_degrees, aspect, s_default_near_clip, s_default_far_clip);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("test_scene_update(%s) - flight_camera_viewing_frustum_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        projection_changed = true;
    }

    if(view_changed) {
        ret_camera = flight_camera_view_matrix_get(test_scene->flight_camera, &test_scene->view_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("test_scene_update(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    if(projection_changed) {
        ret_camera = flight_camera_perspective_matrix_get(test_scene->flight_camera, &test_scene->projection_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("test_scene_update(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t test_scene_draw(void* scene_concrete_, renderer_frontend_t* renderer_frontend_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    test_scene_t* test_scene = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(scene_concrete_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!test_scene_is_valid(scene_concrete_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("test_scene_draw(%s) - Precondition validation failed for 'scene_concrete_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    test_scene = (test_scene_t*)scene_concrete_;

    ret = render_packet_build(test_scene, test_scene->render_packet);
    if(SCENE_SUCCESS != ret) {
        ERROR_MESSAGE("test_scene_draw(%s) - render_packet_build failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_point_mesh_render_update(renderer_frontend_, &test_scene->view_matrix, &test_scene->projection_matrix);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_draw(%s) - renderer_frontend_point_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_ui_mesh_render_update(renderer_frontend_, &test_scene->view_matrix, &test_scene->projection_matrix);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_draw(%s) - renderer_frontend_ui_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_draw_packet(test_scene->render_packet, renderer_frontend_);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("test_scene_draw(%s) - renderer_frontend_draw_packet failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static bool test_scene_is_valid(const void* scene_concrete_) {
    if(NULL == scene_concrete_) {
        return false;
    }
    return true;
}

static scene_result_t test_scene_camera_create(const flight_camera_key_bind_t* camera_key_binds_, int framebuffer_width_, int framebuffer_height_, mat4x4f_t* out_view_matrix_, mat4x4f_t* out_projection_matrix_, flight_camera_t** out_flight_camera_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    flight_camera_t* tmp_flight_camera = NULL;

    float aspect = 0.0f;
    mat4x4f_t tmp_view_matrix = { 0 };
    mat4x4f_t tmp_projection_matrix = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_key_binds_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_view_matrix_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_projection_matrix_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_flight_camera_, ret, SCENE_BAD_OPERATION);
    if(framebuffer_width_ <= 0 || framebuffer_height_ <= 0) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("test_scene_camera_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
    ret_camera = flight_camera_create(camera_key_binds_, s_default_fovy_degrees, aspect, s_default_near_clip, s_default_far_clip, &tmp_flight_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("test_scene_camera_create(%s) - flight_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_view_matrix_get(tmp_flight_camera, &tmp_view_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("test_scene_camera_create(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_perspective_matrix_get(tmp_flight_camera, &tmp_projection_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("test_scene_camera_create(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_flight_camera_ = tmp_flight_camera;
    *out_projection_matrix_ = tmp_projection_matrix;
    *out_view_matrix_ = tmp_view_matrix;
    tmp_flight_camera = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_flight_camera) {
        flight_camera_destroy(&tmp_flight_camera);
    }
    return ret;
}

static scene_result_t render_packet_build(const test_scene_t* test_scene_, render_packet_t* packet_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    const size_t point_mesh_count = 1;

    IF_ARG_NULL_GOTO_CLEANUP(test_scene_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SCENE_INVALID_ARGUMENT);

    render_packet_deinitialize(packet_);
    ret_render_packet = render_packet_initialize(0, point_mesh_count, 4, 0, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_point_mesh_push(&test_scene_->point_mesh, &test_scene_->point_mesh_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_point_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_ui_mesh_push(&test_scene_->large_icon_ui_mesh, &test_scene_->large_icon_ui_mesh_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_ui_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_ui_mesh_push(&test_scene_->small_icon_ui_mesh, &test_scene_->small_icon_ui_mesh_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_ui_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_ui_mesh_push(&test_scene_->solid_color_ui_mesh_large, &test_scene_->solid_color_ui_mesh_large_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_ui_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_ui_mesh_push(&test_scene_->solid_color_ui_mesh_small, &test_scene_->solid_color_ui_mesh_small_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_ui_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

// テスト用頂点データ生成処理(Chat GPT Generated)
static scene_result_t point_vertices_create(point_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    enum { POINTS_PER_AXIS = 9 };

    const size_t vertex_count = POINTS_PER_AXIS * POINTS_PER_AXIS * POINTS_PER_AXIS;
    const float spacing = 0.5f;
    const float center = (float)(POINTS_PER_AXIS - 1) * 0.5f;

    point_vertex_t* tmp_vertices = NULL;
    size_t vertex_index = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, SCENE_BAD_OPERATION);

    if(0 != *out_vertex_count_) {
        ret = SCENE_BAD_OPERATION;
        ERROR_MESSAGE("point_vertices_create(%s) - Output vertex count is not zero.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(
        sizeof(*tmp_vertices) * vertex_count,
        GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY,
        (void**)&tmp_vertices);

    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("point_vertices_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t z = 0; z < POINTS_PER_AXIS; ++z) {
        for(size_t y = 0; y < POINTS_PER_AXIS; ++y) {
            for(size_t x = 0; x < POINTS_PER_AXIS; ++x) {
                point_vertex_t* vertex = &tmp_vertices[vertex_index];

                vertex->position = vec3f_initialize(
                    ((float)x - center) * spacing,
                    ((float)y - center) * spacing,
                    ((float)z - center) * spacing);

                vertex->color = vec4u8_initialize(
                    (uint8_t)(x * 255 / (POINTS_PER_AXIS - 1)),
                    (uint8_t)(y * 255 / (POINTS_PER_AXIS - 1)),
                    (uint8_t)(z * 255 / (POINTS_PER_AXIS - 1)),
                    255);

                ++vertex_index;
            }
        }
    }

    // Commit.
    *out_vertices_ = tmp_vertices;
    *out_vertex_count_ = vertex_count;
    tmp_vertices = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
    }
    return ret;
}

static void point_vertices_destroy(point_vertex_t** vertices_) {
    if(NULL == vertices_ || NULL == *vertices_) {
        return;
    }
    general_allocator_free((void**)vertices_, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
}
