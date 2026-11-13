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

#include "engine/camera/flight_camera.h"    // TODO: remove this.

#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

#include "engine/systems/event_system/core/engine_event_view.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/line_mesh_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/line_grid_geometry.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/demos/core/scene_types.h"
#include "application/demos/core/scene_err_utils.h"

typedef struct stl_viewer {
    untextured_mesh_t untextured_mesh;
    // line_mesh_t floor_grid_mesh;
    // untextured_mesh_t debug_axis_mesh;
    // line_mesh_t debug_aabb_mesh;
    mat4x4f_t model_matrix;

    // Camera(Temporary)
    flight_camera_t* flight_camera;
    float camera_speed;
    mat4x4f_t view_matrix;
    mat4x4f_t projection_matrix;

    // Lighting.
    directional_light_t directional_light;
    ambient_light_t ambient_light;

    // Rendering.
    render_packet_t* render_packet;
} stl_viewer_t;

static flight_camera_key_bind_t s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MAX];
static const float s_default_fovy = 45.0f;
static const float s_default_near_clip = 0.1f;
static const float s_default_far_clip = 3000.0f;

// Camera Configuration.
static void flight_camera_keybinds_initialize(void);

// Render Packet.
static scene_result_t render_packet_build(const stl_viewer_t* stl_viewer_, render_packet_t* packet_);

scene_result_t stl_viewer_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, stl_viewer_t** out_viewer_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;
    light_result_t ret_light = LIGHT_INVALID_ARGUMENT;

    stl_viewer_t* tmp_stl_viewer = NULL;
    flight_camera_t* tmp_flight_camera = NULL;  // Temporary
    render_packet_t* tmp_render_packet = NULL;

    untextured_mesh_t tmp_untextured_mesh = { 0 };
    untextured_mesh_config_t stl_viewer_mesh_config = { 0 };

    bool untextured_mesh_imported = false;

    // Debug Axis Configuration.
    // const uint32_t segment_count = 32;
    // const cone_t cone = { .height = 10.0f, .radius = 10.0f };
    // const cylinder_t cylinder = { .height = 30.0f, .radius = 5.0f };

    float aspect = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(out_viewer_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_create", "out_viewer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_viewer_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "stl_viewer_create", "*out_viewer_")
    if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("stl_viewer_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

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

    // Camera.
    flight_camera_keybinds_initialize();
    aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
    ret_camera = flight_camera_create(s_flight_camera_keybinds, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip, &tmp_flight_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("stl_viewer_create(%s) - flight_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_view_matrix_get(tmp_flight_camera, &tmp_stl_viewer->view_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("stl_viewer_create(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_perspective_matrix_get(tmp_flight_camera, &tmp_stl_viewer->projection_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("stl_viewer_create(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }

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

    stl_viewer_mesh_config.geometry_subdirectory = "stl/demo_stl_viewer";
    stl_viewer_mesh_config.geometry_resource_name = "utah_teapot_ascii";
    stl_viewer_mesh_config.geometry_extension = "stl";
    stl_viewer_mesh_config.material_subdirectory = "materials/demo_stl_viewer";
    stl_viewer_mesh_config.material_resource_name = "utah_teapot";
    stl_viewer_mesh_config.material_extension = "untextured_mat";
    ret_renderer_frontend = renderer_frontend_untextured_mesh_import(renderer_frontend_, &stl_viewer_mesh_config, asset_root_path_, &tmp_untextured_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_import failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    untextured_mesh_imported = true;

    mat4f_identity(&tmp_stl_viewer->model_matrix);
    tmp_stl_viewer->untextured_mesh = tmp_untextured_mesh;
    tmp_stl_viewer->camera_speed = 5.0f;
    tmp_stl_viewer->render_packet = tmp_render_packet;
    tmp_stl_viewer->flight_camera = tmp_flight_camera;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(tmp_stl_viewer)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_create(%s) - Result validation failed for 'tmp_stl_viewer'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_viewer_ = tmp_stl_viewer;
    tmp_stl_viewer = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(untextured_mesh_imported) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_untextured_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("stl_viewer_create(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != tmp_render_packet) {
        render_packet_destroy(&tmp_render_packet);
    }
    if(NULL != tmp_flight_camera) {
        flight_camera_destroy(&tmp_flight_camera);
    }
    if(NULL != tmp_stl_viewer) {
        general_allocator_free((void**)&tmp_stl_viewer, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, stl_viewer_t** stl_viewer_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    if(NULL == stl_viewer_ || NULL == *stl_viewer_ || NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(*stl_viewer_)) {
        ERROR_MESSAGE("stl_viewer_destroy(%s) - Precondition validation failed for '*stl_viewer_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif

    ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &(*stl_viewer_)->untextured_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", scene_result_to_str(ret));
        if(SCENE_DATA_CORRUPTED == ret) {
            return;
        }
    }

    render_packet_destroy(&(*stl_viewer_)->render_packet);
    flight_camera_destroy(&(*stl_viewer_)->flight_camera);

    general_allocator_free((void**)stl_viewer_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

scene_result_t stl_viewer_update(
    stl_viewer_t* stl_viewer_,
    const engine_event_view_t* event_view_,
    float delta_time_,
    bool window_resized_,
    int framebuffer_width_,
    int framebuffer_height_) {

    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    float aspect = 0.0f;
    bool projection_changed = false;
    bool view_changed = false;

    IF_ARG_NULL_GOTO_CLEANUP(stl_viewer_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_update", "stl_viewer_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_update", "event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(stl_viewer_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_update(%s) - Precondition validation failed for 'stl_viewer_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    for(size_t i = 0; i != event_view_->keyboard_event_count; ++i) {
        ret_camera = flight_camera_command_update(stl_viewer_->flight_camera, &event_view_->keyboard_events[i]);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - flight_camera_command_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret_camera = flight_camera_command_execute(stl_viewer_->flight_camera, stl_viewer_->camera_speed, delta_time_, &view_changed);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("stl_viewer_update(%s) - flight_camera_command_execute failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    if(window_resized_ && 0 < framebuffer_height_ && 0 < framebuffer_width_) { // window最小化等でframebuffer_heightが0の場合は視錐台の更新は行わない
        aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
        ret_camera = flight_camera_viewing_frustum_update(stl_viewer_->flight_camera, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - flight_camera_viewing_frustum_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        projection_changed = true;
    }

    if(view_changed) {
        ret_camera = flight_camera_view_matrix_get(stl_viewer_->flight_camera, &stl_viewer_->view_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    if(projection_changed) {
        ret_camera = flight_camera_perspective_matrix_get(stl_viewer_->flight_camera, &stl_viewer_->projection_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("stl_viewer_update(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

scene_result_t stl_viewer_draw(stl_viewer_t* stl_viewer_, renderer_frontend_t* renderer_frontend_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_viewer_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_draw", "stl_viewer_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "stl_viewer_draw", "renderer_frontend_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!stl_viewer_is_valid(stl_viewer_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_viewer_(%s) - Precondition validation failed for 'stl_viewer_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = render_packet_build(stl_viewer_, stl_viewer_->render_packet);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("stl_viewer_draw(%s) - render_packet_build failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_untextured_mesh_render_update(renderer_frontend_, &stl_viewer_->view_matrix, &stl_viewer_->projection_matrix, &stl_viewer_->directional_light, &stl_viewer_->ambient_light);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_untextured_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // ret_renderer_frontend = renderer_frontend_line_mesh_render_update(renderer_frontend_, &stl_viewer_->view_matrix, &stl_viewer_->projection_matrix);
    // if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
    //     ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
    //     ERROR_MESSAGE("stl_viewer_draw(%s) - renderer_frontend_line_mesh_render_update failed.", scene_result_to_str(ret));
    //     goto cleanup;
    // }

    ret_renderer_frontend = renderer_frontend_draw_packet(stl_viewer_->render_packet, renderer_frontend_);
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

bool stl_viewer_is_valid(const stl_viewer_t* viewer_) {
    if(NULL == viewer_) {
        return false;
    }
    return true;
}

// TODO: remove this.
static void flight_camera_keybinds_initialize(void) {
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].key = KEY_W;         // カメラ前進コマンド(キーバインド: KEY_W)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].key = KEY_S;        // カメラ後進コマンド(キーバインド: KEY_S)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].key = KEY_D;           // カメラ右移動コマンド(キーバインド: KEY_D)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].key = KEY_A;            // カメラ左移動コマンド(キーバインド: KEY_A)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_UP].key = KEY_E;              // カメラ上方向移動コマンド(キーバインド: KEY_E)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].key = KEY_Q;            // カメラ下方向移動コマンド(キーバインド: KEY_Q)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].key = KEY_UP;      // カメラピッチ方向(+)回転コマンド(キーバインド: KEY_UP)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].key = KEY_DOWN;   // カメラピッチ方向(-)回転コマンド(キーバインド: KEY_DOWN)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].key = KEY_LEFT;      // カメラヨー方向(+)回転コマンド(キーバインド: KEY_LEFT)
    s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].key = KEY_RIGHT;    // カメラヨー方向(-)回転コマンド(キーバインド: KEY_RIGHT)
}

static scene_result_t render_packet_build(const stl_viewer_t* viewer_, render_packet_t* packet_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(viewer_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "render_packet_build", "viewer_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "render_packet_build", "packet_")

    render_packet_deinitialize(packet_);
    ret_render_packet = render_packet_initialize(0, 0, 0, 1, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_untextured_mesh_push(&viewer_->untextured_mesh, &viewer_->model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_untextured_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}
