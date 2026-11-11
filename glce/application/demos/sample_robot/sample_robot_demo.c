// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/sample_robot/sample_robot_demo.h"

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
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "engine/systems/renderer/render_packet/render_packet.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/assembly/untextured_model/model_assembly_config.h"
#include "application/model_framework/assembly/untextured_model/model_resource_map.h"
#include "application/model_framework/coordinates/model_transform_tree.h"

#include "application/demos/sample_robot/core/sample_robot_types.h"
#include "application/demos/sample_robot/core/sample_robot_err_utils.h"

#include "application/demos/sample_robot/model_instance/sample_robot.h"

struct sample_robot_demo {
    untextured_model_resource_map_t* resource_map;
    sample_robot_t* robot1;
    sample_robot_t* robot2;

    line_mesh_t* floor_grid_line_mesh;
    mat4x4f_t floor_grid_model_matrix;

    float animation_time;
};

static sample_robot_result_t floor_grid_mesh_create(renderer_frontend_t* renderer_frontend_, line_mesh_t** out_mesh_);
static sample_robot_result_t floor_grid_mesh_destroy(renderer_frontend_t* renderer_frontend_, line_mesh_t** mesh_);

static sample_robot_result_t robot_draw_request_count_add(const untextured_model_resource_map_t* resource_map_, const sample_robot_t* robot_, size_t* io_untextured_mesh_count_, size_t* io_line_mesh_count_);
static sample_robot_result_t robot_draw_requests_append(const untextured_model_resource_map_t* resource_map_, const sample_robot_t* robot_, render_packet_t* packet_);

sample_robot_result_t sample_robot_demo_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, sample_robot_demo_t** out_demo_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;
    sample_robot_result_t ret_cleanup = SAMPLE_ROBOT_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    sample_robot_demo_t* tmp_demo = NULL;
    untextured_model_assembly_config_t* config = NULL;
    untextured_model_resource_map_t* tmp_resource_map = NULL;
    sample_robot_t* tmp_robot1 = NULL;
    sample_robot_t* tmp_robot2 = NULL;
    line_mesh_t* tmp_floor_grid = NULL;
    mat4x4f_t tmp_floor_grid_model_matrix = { 0 };

    // Debug Axis Configuration.
    const uint32_t segment_count = 32;
    const cone_t cone = { .height = 10.0f, .radius = 10.0f };
    const cylinder_t cylinder = { .height = 30.0f, .radius = 5.0f };

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_demo_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_create", "out_demo_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_demo_, ret, SAMPLE_ROBOT_BAD_OPERATION, sample_robot_result_to_str(SAMPLE_ROBOT_BAD_OPERATION), "sample_robot_demo_create", "*out_demo_")

    ret_general_allocator = general_allocator_allocate(sizeof(sample_robot_demo_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_demo);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = sample_robot_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - general_allocator_allocate failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    // Model Assembly config.
    // TODO: untextured_model_assembly_config_loaderを実装し、dummy_loaderと入れ替え
    ret = sample_robot_dummy_config_loader(&config);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - sample_robot_dummy_config_loader failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    // Model Resource Map.
    ret_model_framework = untextured_model_resource_map_create(renderer_frontend_, "sample_robot_resource_map", config, asset_root_path_, vec4u8_initialize(255, 0, 0, 255), &cone, &cylinder, segment_count, &tmp_resource_map);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - model_resource_map_create failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    untextured_model_assembly_config_destroy(&config);

    // Sample Robot1.
    ret = sample_robot_create(tmp_resource_map, &tmp_robot1);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - sample_robot_create failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    ret = sample_robot_placement_set(vec3f_initialize(-135.0f, 0.0f, -600.0f), vec3f_initialize(270.0f, 0.0f, 0.0f), tmp_robot1);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - sample_robot_placement_set failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    // Sample Robot2.
    ret = sample_robot_create(tmp_resource_map, &tmp_robot2);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - sample_robot_create failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    ret = sample_robot_placement_set(vec3f_initialize(150.0f, 0.0f, -1000.0f), vec3f_initialize(270.0f, 0.0f, 0.0f), tmp_robot2);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - sample_robot_placement_set failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    // Floor Grid.
    ret = floor_grid_mesh_create(renderer_frontend_, &tmp_floor_grid);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_create(%s) - floor_grid_mesh_create failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    mat4f_identity(&tmp_floor_grid_model_matrix);

    tmp_demo->resource_map = tmp_resource_map;
    tmp_demo->robot1 = tmp_robot1;
    tmp_demo->robot2 = tmp_robot2;
    tmp_demo->floor_grid_line_mesh = tmp_floor_grid;
    tmp_demo->floor_grid_model_matrix = tmp_floor_grid_model_matrix;
    tmp_demo->animation_time = 0.0f;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_demo_is_valid(tmp_demo)) {
        ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_demo_create(%s) - Result validation failed for 'tmp_demo'.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_demo_ = tmp_demo;
    tmp_demo = NULL;
    tmp_resource_map = NULL;
    tmp_robot1 = NULL;
    tmp_robot2 = NULL;
    tmp_floor_grid = NULL;

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    if(SAMPLE_ROBOT_DATA_CORRUPTED == ret || SAMPLE_ROBOT_SUCCESS == ret) {
        return ret;
    }
    if(NULL != renderer_frontend_ && NULL != tmp_floor_grid) {
        ret_cleanup = floor_grid_mesh_destroy(renderer_frontend_, &tmp_floor_grid);
        if(SAMPLE_ROBOT_DATA_CORRUPTED == ret_cleanup) {
            ERROR_MESSAGE("sample_robot_demo_create(%s) - floor_grid_mesh_destroy failed.", sample_robot_result_to_str(ret_cleanup));
            return ret_cleanup;
        }
    }
    if(NULL != tmp_robot2) {
        sample_robot_destroy(&tmp_robot2);
    }
    if(NULL != tmp_robot1) {
        sample_robot_destroy(&tmp_robot1);
    }
    if(NULL != tmp_resource_map && NULL != renderer_frontend_) {
        ret_model_framework = untextured_model_resource_map_destroy(renderer_frontend_, &tmp_resource_map);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            ERROR_MESSAGE("sample_robot_demo_create(%s) - untextured_model_resource_map_destroy failed.", sample_robot_result_to_str(ret));
            return ret;
        }
    }
    if(NULL != config) {
        untextured_model_assembly_config_destroy(&config);
    }
    if(NULL != tmp_demo) {
        general_allocator_free((void**)&tmp_demo, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void sample_robot_demo_destroy(renderer_frontend_t* renderer_frontend_, sample_robot_demo_t** demo_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    if(NULL == demo_ || NULL == *demo_ || NULL == renderer_frontend_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_demo_is_valid(*demo_)) {
        ERROR_MESSAGE("sample_robot_demo_destroy(%s) - Precondition validation failed for '*demo_'.", sample_robot_result_to_str(SAMPLE_ROBOT_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL != (*demo_)->floor_grid_line_mesh) {
        ret = floor_grid_mesh_destroy(renderer_frontend_, &(*demo_)->floor_grid_line_mesh);
        if(SAMPLE_ROBOT_DATA_CORRUPTED == ret) {
            ERROR_MESSAGE("sample_robot_demo_destroy(%s) - floor_grid_mesh_destroy failed.", sample_robot_result_to_str(ret));
            return;
        }
    }

    sample_robot_destroy(&(*demo_)->robot1);
    sample_robot_destroy(&(*demo_)->robot2);

    ret_model_framework = untextured_model_resource_map_destroy(renderer_frontend_, &(*demo_)->resource_map);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ERROR_MESSAGE("sample_robot_demo_destroy(%s) - untextured_model_resource_map_destroy failed.", sample_robot_result_to_str(SAMPLE_ROBOT_DATA_CORRUPTED));
        return;
    }

    general_allocator_free((void**)demo_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

sample_robot_result_t sample_robot_demo_posture_update(float delta_time_, sample_robot_demo_t* demo_) {
    // Chat GPT Generated.
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    sample_robot_posture_t robot1_posture = { 0 };
    sample_robot_posture_t robot2_posture = { 0 };

    float next_animation_time = 0.0f;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(demo_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_posture_update", "demo_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_demo_is_valid(demo_)) {
        ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_demo_posture_update(%s) - Precondition validation failed for 'demo_'.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    next_animation_time = demo_->animation_time + delta_time_;

    robot1_posture = demo_->robot1->posture;
    robot2_posture = demo_->robot2->posture;

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
    ret = sample_robot_posture_update(&robot1_posture, demo_->robot1);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_posture_update(%s) - sample_robot_posture_update failed for robot1.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret = sample_robot_posture_update(&robot2_posture, demo_->robot2);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_posture_update(%s) - sample_robot_posture_update failed for robot2.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    demo_->animation_time = next_animation_time;

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    return ret;
}

sample_robot_result_t sample_robot_demo_render_packet_build(const sample_robot_demo_t* demo_, render_packet_t* packet_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    size_t untextured_mesh_count = 0;
    size_t line_mesh_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(demo_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_render_packet_build", "demo_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "sample_robot_demo_render_packet_build", "packet_")

    ret = robot_draw_request_count_add(demo_->resource_map, demo_->robot1, &untextured_mesh_count, &line_mesh_count);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - robot_draw_request_count_add failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    ret = robot_draw_request_count_add(demo_->resource_map, demo_->robot2, &untextured_mesh_count, &line_mesh_count);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - robot_draw_request_count_add failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    line_mesh_count++;  // floor grid mesh

    render_packet_deinitialize(packet_);
    ret_render_packet = render_packet_initialize(line_mesh_count, 0, 0, untextured_mesh_count, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = sample_robot_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - render_packet_initialize failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret = robot_draw_requests_append(demo_->resource_map, demo_->robot1, packet_);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - robot_draw_requests_append failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    ret = robot_draw_requests_append(demo_->resource_map, demo_->robot2, packet_);
    if(SAMPLE_ROBOT_SUCCESS != ret) {
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - robot_draw_requests_append failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret_render_packet = render_packet_line_mesh_push(demo_->floor_grid_line_mesh, &demo_->floor_grid_model_matrix, packet_);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = sample_robot_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("sample_robot_demo_render_packet_build(%s) - render_packet_line_mesh_push failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    return ret;
}

// TODO: 実装
bool sample_robot_demo_is_valid(const sample_robot_demo_t* demo_) {
    if(NULL == demo_) {
        return false;
    }
    return true;
}

static sample_robot_result_t floor_grid_mesh_create(renderer_frontend_t* renderer_frontend_, line_mesh_t** out_mesh_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    line_vertex_t* tmp_vertices = NULL;
    size_t tmp_vertex_count = 0;
    line_mesh_t* tmp_mesh = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "floor_grid_mesh_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "floor_grid_mesh_create", "out_mesh_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_mesh_, ret, SAMPLE_ROBOT_BAD_OPERATION, sample_robot_result_to_str(SAMPLE_ROBOT_BAD_OPERATION), "floor_grid_mesh_create", "*out_mesh_")

    ret_general_allocator = general_allocator_allocate(sizeof(line_mesh_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_mesh);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = sample_robot_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - general_allocator_allocate failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret_geometry = line_grid_geometry_vertices_create(LINE_GRID_PLANE_XZ, vec4u8_initialize(128, 128, 128, 255), vec3f_initialize(-500.0f, 0.0f, -1300.0f), vec3f_initialize(500.0f, 0.0f, 100.0f), 50.0f, &tmp_vertices, &tmp_vertex_count);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = sample_robot_result_convert_geometry(ret_geometry);
        if(GEOMETRY_INVALID_ARGUMENT == ret_geometry) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - line_grid_geometry_vertices_create failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_import_from_vertices(renderer_frontend_, "floor_grid_line", tmp_vertices, tmp_vertex_count, tmp_mesh);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = sample_robot_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_create(%s) - renderer_frontend_line_mesh_import_from_vertices failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    *out_mesh_ = tmp_mesh;
    tmp_mesh = NULL;
    general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    if(SAMPLE_ROBOT_DATA_CORRUPTED == ret || SAMPLE_ROBOT_SUCCESS == ret) {
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

static sample_robot_result_t floor_grid_mesh_destroy(renderer_frontend_t* renderer_frontend_, line_mesh_t** mesh_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "floor_grid_mesh_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "floor_grid_mesh_destroy", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(*mesh_, ret, SAMPLE_ROBOT_BAD_OPERATION, sample_robot_result_to_str(SAMPLE_ROBOT_BAD_OPERATION), "floor_grid_mesh_destroy", "*mesh_")

    ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, *mesh_);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = sample_robot_result_convert_renderer_frontend(ret_renderer_frontend);
        if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("floor_grid_mesh_destroy(%s) - renderer_frontend_line_mesh_release failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    general_allocator_free((void**)mesh_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    return ret;
}

static sample_robot_result_t robot_draw_request_count_add(const untextured_model_resource_map_t* resource_map_, const sample_robot_t* robot_, size_t* io_untextured_mesh_count_, size_t* io_line_mesh_count_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    size_t tmp_untextured_mesh_count = 0;
    size_t tmp_debug_axis_mesh_count = 0;
    size_t tmp_debug_aabb_mesh_count = 0;

    size_t result_untextured_mesh_count = 0;
    size_t result_line_mesh_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_request_count_add", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_request_count_add", "robot_")
    IF_ARG_NULL_GOTO_CLEANUP(io_untextured_mesh_count_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_request_count_add", "io_untextured_mesh_count_")
    IF_ARG_NULL_GOTO_CLEANUP(io_line_mesh_count_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_request_count_add", "io_line_mesh_count_")

    ret_model_framework = untextured_model_resource_map_untextured_mesh_count_get(resource_map_, &tmp_untextured_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_untextured_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(tmp_untextured_mesh_count, *io_untextured_mesh_count_, &result_untextured_mesh_count)) {
        ret = SAMPLE_ROBOT_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Untextured mesh count overflow.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = untextured_model_resource_map_debug_axis_mesh_count_get(resource_map_, &tmp_debug_axis_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_debug_axis_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(result_untextured_mesh_count, tmp_debug_axis_mesh_count, &result_untextured_mesh_count)) {
        ret = SAMPLE_ROBOT_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Untextured mesh count overflow.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_count_get(resource_map_, &tmp_debug_aabb_mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - untextured_model_resource_map_debug_aabb_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(tmp_debug_aabb_mesh_count, *io_line_mesh_count_, &result_line_mesh_count)) {
        ret = SAMPLE_ROBOT_OVERFLOW;
        ERROR_MESSAGE("robot_draw_request_count_add(%s) - Line mesh count overflow.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    *io_line_mesh_count_ = result_line_mesh_count;
    *io_untextured_mesh_count_ = result_untextured_mesh_count;

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    return ret;
}

static sample_robot_result_t robot_draw_requests_append(const untextured_model_resource_map_t* resource_map_, const sample_robot_t* robot_, render_packet_t* packet_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;

    size_t mesh_count = 0;
    uint16_t coordinate_id = 0;

    untextured_mesh_t untextured_mesh = { 0 };
    line_mesh_t line_mesh = { 0 };
    mat4x4f_t model_matrix = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_requests_append", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_requests_append", "robot_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, SAMPLE_ROBOT_INVALID_ARGUMENT, sample_robot_result_to_str(SAMPLE_ROBOT_INVALID_ARGUMENT), "robot_draw_requests_append", "packet_")

    // Untextured Mesh.
    ret_model_framework = untextured_model_resource_map_untextured_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_untextured_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SAMPLE_ROBOT_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_untextured_mesh_binding_get(resource_map_, (uint16_t)i, &untextured_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_untextured_mesh_binding_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_untextured_mesh_push(&untextured_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = sample_robot_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_untextured_mesh_push failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }
    }

    // Debug Axis.
    ret_model_framework = untextured_model_resource_map_debug_axis_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_axis_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SAMPLE_ROBOT_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_debug_axis_mesh_binding_get(resource_map_, (uint16_t)i, &untextured_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_axis_mesh_binding_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_untextured_mesh_push(&untextured_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = sample_robot_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_untextured_mesh_push failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }
    }

    // Debug AABB.
    ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_count_get(resource_map_, &mesh_count);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = sample_robot_result_convert_model_framework(ret_model_framework);
        if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
            ret = SAMPLE_ROBOT_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_aabb_mesh_count_get failed.", sample_robot_result_to_str(ret));
        goto cleanup;
    }
    if(UINT16_MAX < mesh_count) {
        ret = SAMPLE_ROBOT_LIMIT_EXCEEDED;
        ERROR_MESSAGE("robot_draw_requests_append(%s) - Mesh count limit exceeded.", sample_robot_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < mesh_count; ++i) {
        ret_model_framework = untextured_model_resource_map_debug_aabb_mesh_binding_get(resource_map_, (uint16_t)i, &line_mesh, &coordinate_id);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - untextured_model_resource_map_debug_aabb_mesh_binding_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_model_framework = model_transform_tree_world_matrix_get(robot_->transform_tree, coordinate_id, &model_matrix);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = sample_robot_result_convert_model_framework(ret_model_framework);
            if(MODEL_FRAMEWORK_INVALID_ARGUMENT == ret_model_framework) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - model_transform_tree_world_matrix_get failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }

        ret_render_packet = render_packet_line_mesh_push(&line_mesh, &model_matrix, packet_);
        if(RENDER_PACKET_SUCCESS != ret_render_packet) {
            ret = sample_robot_result_convert_render_packet(ret_render_packet);
            if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
                ret = SAMPLE_ROBOT_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("robot_draw_requests_append(%s) - render_packet_line_mesh_push failed.", sample_robot_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    return ret;
}
