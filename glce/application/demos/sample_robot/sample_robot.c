// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/sample_robot/sample_robot.h"

#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/camera/core/camera_types.h"
#include "engine/camera/flight_camera.h"

#include "engine/light/core/light_types.h"
#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

#include "engine/systems/event_system/core/engine_event_view.h"

#include "engine/systems/renderer/resources/geometries/line_mesh_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/line_grid_geometry.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/assembly/untextured_model/model_assembly_config.h"
#include "application/model_framework/assembly/untextured_model/model_resource_map.h"
#include "application/model_framework/coordinates/model_transform_tree.h"

#include "application/demos/core/scene_types.h"
#include "application/demos/core/scene_err_utils.h"

#include "application/demos/sample_robot/model_instance/sample_robot_model.h"

struct sample_robot {
    // Robot Models.
    untextured_model_resource_map_t* robot_resource_map;
    sample_robot_model_t* robot1;
    sample_robot_model_t* robot2;
    float robot_animation_time;

    // Floor Grid.
    line_mesh_t* floor_grid_mesh;
    mat4x4f_t floor_grid_model_matrix;

    // Camera.
    flight_camera_t* flight_camera;
    float camera_speed;
    mat4x4f_t view_matrix;
    mat4x4f_t projection_matrix;

    // Lighting.
    directional_light_t directional_light;
    ambient_light_t ambient_light;

    // Rendering.
    render_packet_t* render_packet;
};

static flight_camera_key_bind_t s_flight_camera_keybinds[FLIGHT_CAMERA_COMMAND_MAX];
static const float s_default_fovy = 45.0f;
static const float s_default_near_clip = 0.1f;
static const float s_default_far_clip = 3000.0f;

// Robot Animation.
static scene_result_t robot_animation_update(float delta_time_, sample_robot_t* sample_robot_);

// Camera Configuration.
static void flight_camera_keybinds_initialize(void);

// Floor Grid Resource.
static scene_result_t floor_grid_mesh_create(renderer_frontend_t* renderer_frontend_, line_mesh_t** out_mesh_);
static scene_result_t floor_grid_mesh_destroy(renderer_frontend_t* renderer_frontend_, line_mesh_t** mesh_);

// Render Packet.
static scene_result_t render_packet_build(const sample_robot_t* sample_robot_, render_packet_t* packet_);
static scene_result_t robot_draw_request_count_add(const untextured_model_resource_map_t* resource_map_, const sample_robot_model_t* robot_, size_t* io_untextured_mesh_count_, size_t* io_line_mesh_count_);
static scene_result_t robot_draw_requests_append(const untextured_model_resource_map_t* resource_map_, const sample_robot_model_t* robot_, render_packet_t* packet_);

// TODO: リファクタリング
scene_result_t sample_robot_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, sample_robot_t** out_sample_robot_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;
    scene_result_t ret_cleanup = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;
    light_result_t ret_light = LIGHT_INVALID_ARGUMENT;

    sample_robot_t* tmp_sample_robot = NULL;
    untextured_model_assembly_config_t* config = NULL;
    untextured_model_resource_map_t* tmp_resource_map = NULL;
    flight_camera_t* tmp_flight_camera = NULL;
    sample_robot_model_t* tmp_robot_model1 = NULL;
    sample_robot_model_t* tmp_robot_model2 = NULL;
    line_mesh_t* tmp_floor_grid = NULL;
    render_packet_t* tmp_render_packet = NULL;
    mat4x4f_t tmp_floor_grid_model_matrix = { 0 };

    // Debug Axis Configuration.
    const uint32_t segment_count = 32;
    const cone_t cone = { .height = 10.0f, .radius = 10.0f };
    const cylinder_t cylinder = { .height = 30.0f, .radius = 5.0f };

    float aspect = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_sample_robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_create", "out_sample_robot_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_sample_robot_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "sample_robot_create", "*out_sample_robot_")
    if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("sample_robot_create(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(sample_robot_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_sample_robot);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_create(16 * MIB, &tmp_render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - render_packet_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Model Assembly config.
    ret = sample_robot_model_dummy_config_loader(&config);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_model_dummy_config_loader failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Model Resource Map.
    ret_model_framework = untextured_model_resource_map_create(renderer_frontend_, "sample_robot_resource_map", config, asset_root_path_, vec4u8_initialize(255, 0, 0, 255), &cone, &cylinder, segment_count, &tmp_resource_map);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - model_resource_map_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    untextured_model_assembly_config_destroy(&config);

    // Camera.
    flight_camera_keybinds_initialize();
    aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
    ret_camera = flight_camera_create(s_flight_camera_keybinds, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip, &tmp_flight_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("sample_robot_create(%s) - flight_camera_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_view_matrix_get(tmp_flight_camera, &tmp_sample_robot->view_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("sample_robot_create(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_camera = flight_camera_perspective_matrix_get(tmp_flight_camera, &tmp_sample_robot->projection_matrix);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("sample_robot_create(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Lighting.
    // NOTE: 設定値の意味
    // direction: 正面から少し斜め上の光で、面の違いが見やすい
    // color: 白色光。色による影響を除外できる
    // intensity: 明るさの計算と結果を比較しやすい
    ret_light = directional_light_initialize(vec3f_initialize(0.4f, 0.8f, 1.0f), vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_sample_robot->directional_light);
    if(LIGHT_SUCCESS != ret_light) {
        ret = scene_result_convert_light(ret_light);
        ERROR_MESSAGE("sample_robot_create(%s) - directional_light_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret_light = ambient_light_initialize(vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_sample_robot->ambient_light);
    if(LIGHT_SUCCESS != ret_light) {
        ret = scene_result_convert_light(ret_light);
        ERROR_MESSAGE("sample_robot_create(%s) - ambient_light_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Sample Robot1.
    ret = sample_robot_model_create(tmp_resource_map, &tmp_robot_model1);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_model_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret = sample_robot_model_placement_set(vec3f_initialize(-135.0f, 0.0f, -600.0f), vec3f_initialize(270.0f, 0.0f, 0.0f), tmp_robot_model1);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_model_placement_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Sample Robot2.
    ret = sample_robot_model_create(tmp_resource_map, &tmp_robot_model2);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_model_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret = sample_robot_model_placement_set(vec3f_initialize(150.0f, 0.0f, -1000.0f), vec3f_initialize(270.0f, 0.0f, 0.0f), tmp_robot_model2);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_model_placement_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Floor Grid.
    ret = floor_grid_mesh_create(renderer_frontend_, &tmp_floor_grid);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_create(%s) - floor_grid_mesh_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_floor_grid_model_matrix);

    tmp_sample_robot->robot_resource_map = tmp_resource_map;
    tmp_sample_robot->flight_camera = tmp_flight_camera;
    tmp_sample_robot->robot1 = tmp_robot_model1;
    tmp_sample_robot->robot2 = tmp_robot_model2;
    tmp_sample_robot->floor_grid_mesh = tmp_floor_grid;
    tmp_sample_robot->floor_grid_model_matrix = tmp_floor_grid_model_matrix;
    tmp_sample_robot->render_packet = tmp_render_packet;
    tmp_sample_robot->robot_animation_time = 0.0f;
    tmp_sample_robot->camera_speed = 5.0f;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_is_valid(tmp_sample_robot)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_create(%s) - Result validation failed for 'tmp_sample_robot'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_sample_robot_ = tmp_sample_robot;
    tmp_sample_robot = NULL;
    tmp_resource_map = NULL;
    tmp_robot_model1 = NULL;
    tmp_robot_model2 = NULL;
    tmp_floor_grid = NULL;
    tmp_flight_camera = NULL;
    tmp_render_packet = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != renderer_frontend_ && NULL != tmp_floor_grid) {
        ret_cleanup = floor_grid_mesh_destroy(renderer_frontend_, &tmp_floor_grid);
        if(SCENE_DATA_CORRUPTED == ret_cleanup) {
            ERROR_MESSAGE("sample_robot_create(%s) - floor_grid_mesh_destroy failed.", scene_result_to_str(ret_cleanup));
            return ret_cleanup;
        }
    }
    if(NULL != tmp_robot_model2) {
        sample_robot_model_destroy(&tmp_robot_model2);
    }
    if(NULL != tmp_robot_model1) {
        sample_robot_model_destroy(&tmp_robot_model1);
    }
    if(NULL != tmp_render_packet) {
        render_packet_destroy(&tmp_render_packet);
    }
    if(NULL != tmp_flight_camera) {
        flight_camera_destroy(&tmp_flight_camera);
    }
    if(NULL != tmp_resource_map && NULL != renderer_frontend_) {
        ret_model_framework = untextured_model_resource_map_destroy(renderer_frontend_, &tmp_resource_map);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("sample_robot_create(%s) - untextured_model_resource_map_destroy failed.", scene_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != config) {
        untextured_model_assembly_config_destroy(&config);
    }
    if(NULL != tmp_sample_robot) {
        general_allocator_free((void**)&tmp_sample_robot, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void sample_robot_destroy(renderer_frontend_t* renderer_frontend_, sample_robot_t** sample_robot_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    if(NULL == sample_robot_ || NULL == *sample_robot_ || NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_is_valid(*sample_robot_)) {
        ERROR_MESSAGE("sample_robot_destroy(%s) - Precondition validation failed for '*sample_robot_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL != (*sample_robot_)->floor_grid_mesh) {
        ret = floor_grid_mesh_destroy(renderer_frontend_, &(*sample_robot_)->floor_grid_mesh);
        if(SCENE_DATA_CORRUPTED == ret) {
            ERROR_MESSAGE("sample_robot_destroy(%s) - floor_grid_mesh_destroy failed.", scene_result_to_str(ret));
            return;
        }
    }

    render_packet_destroy(&(*sample_robot_)->render_packet);
    sample_robot_model_destroy(&(*sample_robot_)->robot1);
    sample_robot_model_destroy(&(*sample_robot_)->robot2);
    flight_camera_destroy(&(*sample_robot_)->flight_camera);

    ret_model_framework = untextured_model_resource_map_destroy(renderer_frontend_, &(*sample_robot_)->robot_resource_map);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ERROR_MESSAGE("sample_robot_destroy(%s) - untextured_model_resource_map_destroy failed.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }

    general_allocator_free((void**)sample_robot_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

scene_result_t sample_robot_update(
    sample_robot_t* sample_robot_,
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

    IF_ARG_NULL_GOTO_CLEANUP(sample_robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_update", "sample_robot_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_update", "event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_is_valid(sample_robot_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_update(%s) - Precondition validation failed for 'sample_robot_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = robot_animation_update(delta_time_, sample_robot_);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_update(%s) - robot_animation_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != event_view_->keyboard_event_count; ++i) {
        ret_camera = flight_camera_command_update(sample_robot_->flight_camera, &event_view_->keyboard_events[i]);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("sample_robot_view_update(%s) - flight_camera_command_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret_camera = flight_camera_command_execute(sample_robot_->flight_camera, sample_robot_->camera_speed, delta_time_, &view_changed);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = scene_result_convert_camera(ret_camera);
        ERROR_MESSAGE("sample_robot_view_update(%s) - flight_camera_command_execute failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    if(window_resized_ && 0 < framebuffer_height_ && 0 < framebuffer_width_) { // window最小化等でframebuffer_heightが0の場合は視錐台の更新は行わない
        aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
        ret_camera = flight_camera_viewing_frustum_update(sample_robot_->flight_camera, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("sample_robot_view_update(%s) - flight_camera_viewing_frustum_update failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        projection_changed = true;
    }

    if(view_changed) {
        ret_camera = flight_camera_view_matrix_get(sample_robot_->flight_camera, &sample_robot_->view_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("sample_robot_draw(%s) - flight_camera_view_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    if(projection_changed) {
        ret_camera = flight_camera_perspective_matrix_get(sample_robot_->flight_camera, &sample_robot_->projection_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = scene_result_convert_camera(ret_camera);
            ERROR_MESSAGE("sample_robot_draw(%s) - flight_camera_perspective_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

scene_result_t sample_robot_draw(sample_robot_t* sample_robot_, renderer_frontend_t* renderer_frontend_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(sample_robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_draw", "sample_robot_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "sample_robot_draw", "renderer_frontend_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_is_valid(sample_robot_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_draw(%s) - Precondition validation failed for 'sample_robot_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = render_packet_build(sample_robot_, sample_robot_->render_packet);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_draw(%s) - render_packet_build failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_untextured_mesh_render_update(renderer_frontend_, &sample_robot_->view_matrix, &sample_robot_->projection_matrix, &sample_robot_->directional_light, &sample_robot_->ambient_light);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("sample_robot_draw(%s) - renderer_frontend_untextured_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_render_update(renderer_frontend_, &sample_robot_->view_matrix, &sample_robot_->projection_matrix);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("sample_robot_draw(%s) - renderer_frontend_line_mesh_render_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_draw_packet(sample_robot_->render_packet, renderer_frontend_);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("sample_robot_draw(%s) - renderer_frontend_draw_packet failed.", scene_result_to_str(ret));
            goto cleanup;
        }
        ERROR_MESSAGE("sample_robot_draw(%s) - renderer_frontend_draw_packet failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

// TODO: 実装
bool sample_robot_is_valid(const sample_robot_t* sample_robot_) {
    if(NULL == sample_robot_) {
        return false;
    }
    return true;
}

static scene_result_t robot_animation_update(float delta_time_, sample_robot_t* sample_robot_) {
    // Chat GPT Generated.
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    sample_robot_posture_t robot1_posture = { 0 };
    sample_robot_posture_t robot2_posture = { 0 };

    float next_animation_time = 0.0f;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(sample_robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_animation_update", "sample_robot_")

    // Prepare.
    next_animation_time = sample_robot_->robot_animation_time + delta_time_;

    robot1_posture = sample_robot_->robot1->posture;
    robot2_posture = sample_robot_->robot2->posture;

    // Robot 1.
    // Base: continuous rotation (30 deg/sec).
    robot1_posture.base_motor_angle = fmodf(30.0f * next_animation_time, 360.0f);

    // Shoulder: smooth oscillation (-45 to 0 deg).
    robot1_posture.shoulder_motor_angle = -22.5f + 22.5f * cosf(0.8f * next_animation_time);

    // Robot 2.
    // Wrist: continuous rotation (40 deg/sec).
    robot2_posture.wrist_motor_angle = fmodf(40.0f * next_animation_time, 360.0f);

    // Elbow: smooth oscillation (-45 to 45 deg).
    robot2_posture.elbow_motor_angle = 45.0f * sinf(0.6f * next_animation_time);

    // Gripper: symmetric opening and closing.
    robot2_posture.left_finger_angle = 22.5f * (1.0f - cosf(1.2f * next_animation_time));
    robot2_posture.right_finger_angle = -robot2_posture.left_finger_angle;

    // Update robot postures.
    ret = sample_robot_model_posture_update(&robot1_posture, sample_robot_->robot1);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_animation_update(%s) - sample_robot_model_posture_update failed for robot1.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = sample_robot_model_posture_update(&robot2_posture, sample_robot_->robot2);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_animation_update(%s) - sample_robot_model_posture_update failed for robot2.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    sample_robot_->robot_animation_time = next_animation_time;

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

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

static scene_result_t floor_grid_mesh_create(renderer_frontend_t* renderer_frontend_, line_mesh_t** out_mesh_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    line_vertex_t* tmp_vertices = NULL;
    size_t tmp_vertex_count = 0;
    line_mesh_t* tmp_mesh = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_create", "out_mesh_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_mesh_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "floor_grid_mesh_create", "*out_mesh_")

    ret_general_allocator = general_allocator_allocate(sizeof(line_mesh_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_mesh);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_geometry = line_grid_geometry_vertices_create(LINE_GRID_PLANE_XZ, vec4u8_initialize(128, 128, 128, 255), vec3f_initialize(-500.0f, 0.0f, -1300.0f), vec3f_initialize(500.0f, 0.0f, 100.0f), 50.0f, &tmp_vertices, &tmp_vertex_count);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = scene_result_convert_geometry(ret_geometry);
        if(GEOMETRY_INVALID_ARGUMENT == ret_geometry) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - line_grid_geometry_vertices_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_import_from_vertices(renderer_frontend_, "floor_grid_line", tmp_vertices, tmp_vertex_count, tmp_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - renderer_frontend_line_mesh_import_from_vertices failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    *out_mesh_ = tmp_mesh;
    tmp_mesh = NULL;
    general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_mesh) {
        general_allocator_free((void**)&tmp_mesh, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static scene_result_t floor_grid_mesh_destroy(renderer_frontend_t* renderer_frontend_, line_mesh_t** mesh_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "floor_grid_mesh_destroy", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(*mesh_, ret, SCENE_BAD_OPERATION, scene_result_to_str(SCENE_BAD_OPERATION), "floor_grid_mesh_destroy", "*mesh_")

    ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, *mesh_);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = scene_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_destroy(%s) - renderer_frontend_line_mesh_release failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    general_allocator_free((void**)mesh_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t render_packet_build(const sample_robot_t* sample_robot_, render_packet_t* packet_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    size_t untextured_mesh_count = 0;
    size_t line_mesh_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(sample_robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "render_packet_build", "sample_robot_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "render_packet_build", "packet_")

    ret = robot_draw_request_count_add(sample_robot_->robot_resource_map, sample_robot_->robot1, &untextured_mesh_count, &line_mesh_count);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - robot_draw_request_count_add failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret = robot_draw_request_count_add(sample_robot_->robot_resource_map, sample_robot_->robot2, &untextured_mesh_count, &line_mesh_count);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - robot_draw_request_count_add failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    line_mesh_count++;  // floor grid mesh

    render_packet_deinitialize(packet_);
    ret_render_packet = render_packet_initialize(line_mesh_count, 0, 0, untextured_mesh_count, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_initialize failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = robot_draw_requests_append(sample_robot_->robot_resource_map, sample_robot_->robot1, packet_);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - robot_draw_requests_append failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    ret = robot_draw_requests_append(sample_robot_->robot_resource_map, sample_robot_->robot2, packet_);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - robot_draw_requests_append failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_line_mesh_push(sample_robot_->floor_grid_mesh, &sample_robot_->floor_grid_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = scene_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_build(%s) - render_packet_line_mesh_push failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t robot_draw_request_count_add(const untextured_model_resource_map_t* resource_map_, const sample_robot_model_t* robot_, size_t* io_untextured_mesh_count_, size_t* io_line_mesh_count_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    size_t tmp_untextured_mesh_count = 0;
    size_t tmp_debug_axis_mesh_count = 0;
    size_t tmp_debug_aabb_mesh_count = 0;

    size_t result_untextured_mesh_count = 0;
    size_t result_line_mesh_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_request_count_add", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_request_count_add", "robot_")
    IF_ARG_NULL_GOTO_CLEANUP(io_untextured_mesh_count_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_request_count_add", "io_untextured_mesh_count_")
    IF_ARG_NULL_GOTO_CLEANUP(io_line_mesh_count_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_request_count_add", "io_line_mesh_count_")

    ret_model_framework = untextured_model_resource_map_untextured_mesh_count_get(resource_map_, &tmp_untextured_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_untextured_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(tmp_untextured_mesh_count, *io_untextured_mesh_count_, &result_untextured_mesh_count)) {
        ret = SCENE_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Untextured mesh count overflow.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = untextured_model_resource_map_debug_axis_mesh_count_get(resource_map_, &tmp_debug_axis_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_debug_axis_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(result_untextured_mesh_count, tmp_debug_axis_mesh_count, &result_untextured_mesh_count)) {
        ret = SCENE_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Untextured mesh count overflow.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_count_get(resource_map_, &tmp_debug_aabb_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_debug_aabb_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(tmp_debug_aabb_mesh_count, *io_line_mesh_count_, &result_line_mesh_count)) {
        ret = SCENE_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Line mesh count overflow.", scene_result_to_str(ret));
        goto cleanup;
    }

    *io_line_mesh_count_ = result_line_mesh_count;
    *io_untextured_mesh_count_ = result_untextured_mesh_count;

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

static scene_result_t robot_draw_requests_append(const untextured_model_resource_map_t* resource_map_, const sample_robot_model_t* robot_, render_packet_t* packet_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    size_t mesh_count = 0;
    uint16_t coordinate_id = 0;

    untextured_mesh_t untextured_mesh = { 0 };
    line_mesh_t line_mesh = { 0 };
    mat4x4f_t model_matrix = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_requests_append", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_requests_append", "robot_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SCENE_INVALID_ARGUMENT, scene_result_to_str(SCENE_INVALID_ARGUMENT), "robot_draw_requests_append", "packet_")

    // Untextured Mesh.
    ret_model_framework = untextured_model_resource_map_untextured_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_untextured_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SCENE_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_untextured_mesh_binding_get(resource_map_, (uint16_t)i, &untextured_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_untextured_mesh_binding_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_untextured_mesh_push(&untextured_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = scene_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_untextured_mesh_push failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    // Debug Axis.
    ret_model_framework = untextured_model_resource_map_debug_axis_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_axis_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SCENE_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_debug_axis_mesh_binding_get(resource_map_, (uint16_t)i, &untextured_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_axis_mesh_binding_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_untextured_mesh_push(&untextured_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = scene_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_untextured_mesh_push failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    // Debug AABB.
    ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_aabb_mesh_count_get failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SCENE_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_binding_get(resource_map_, (uint16_t)i, &line_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_aabb_mesh_binding_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_line_mesh_push(&line_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = scene_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SCENE_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_line_mesh_push failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}
