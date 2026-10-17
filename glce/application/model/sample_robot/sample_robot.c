// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model/sample_robot/sample_robot.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/config/render_model_config.h"
#include "engine/systems/renderer/resources/render_model/render_model.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

static application_result_t dummy_config_loader(render_model_config_t* out_config_);

application_result_t sample_robot_create(vec3f_t root_translate_, vec3f_t root_rotation_, sample_robot_t** out_robot_) {
    return APPLICATION_SUCCESS
}

void sample_robot_destroy(sample_robot_t** robot_) {

}

application_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_) {
    return APPLICATION_SUCCESS;
}

// typedef struct untextured_mesh_bind_config {
//     const char* mesh_name;
//     const char* geometry_resource_name;
//     const char* material_resource_name;

//     int16_t coordinate_id;
// } untextured_mesh_bind_config_t;

// typedef struct render_model_config {
//     size_t coordinate_count;
//     const render_model_coordinate_config_t* coordinates;

//     size_t line_mesh_count;
//     size_t point_mesh_count;
//     size_t ui_mesh_count;
//     size_t untextured_mesh_count;

//     const line_mesh_bind_config_t* line_mesh_bind_configs;
//     const point_mesh_bind_config_t* point_mesh_bind_configs;
//     const ui_mesh_bind_config_t* ui_mesh_bind_configs;
//     const untextured_mesh_bind_config_t* untextured_mesh_bind_configs;
// } render_model_config_t;
static application_result_t dummy_config_loader(render_model_config_t* out_config_) {
    application_result_t ret = APPLICATION_LIMIT_EXCEEDED;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    size_t coordinate_count = 0;
    size_t line_mesh_count = 0;
    size_t point_mesh_count = 0;
    size_t ui_mesh_count = 0;
    size_t untextured_mesh_count = 0;

    render_model_coordinate_config_t* coordinates = NULL;
    untextured_mesh_config_t* untextured_mesh_configs = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "dummy_config_loader", "out_config_")
    // out_config_のfieldのNULLチェックも必要だが、dummy helperなのでやらない

    coordinate_count = 7;
    ret_general_allocator = general_allocator_allocate(sizeof(render_model_coordinate_config_t) * coordinate_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&coordinates);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("dummy_config_loader(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    untextured_mesh_count = 6;
    ret_general_allocator = general_allocator_allocate(sizeof(untextured_mesh_config_t) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&untextured_mesh_configs);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("dummy_config_loader(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Base
    ret = render_model_config_coordinate_initialize(1, 0, vec3f_initialize(0.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[0]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Shoulder
    ret = render_model_config_coordinate_initialize(2, 1, vec3f_initialize(0.0f, 0.0f, 140.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[1]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Elbow
    ret = render_model_config_coordinate_initialize(3, 2, vec3f_initialize(150.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[2]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Wrist
    ret = render_model_config_coordinate_initialize(4, 3, vec3f_initialize(120.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[3]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Left finger
    ret = render_model_config_coordinate_initialize(5, 4, vec3f_initialize(36.0f, 18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[4]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Right finger
    ret = render_model_config_coordinate_initialize(6, 4, vec3f_initialize(36.0f, -18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[5]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // TCP(Tool Center Point)
    ret = render_model_config_coordinate_initialize(7, 4, vec3f_initialize(88.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[6]);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // dummy helperなのでエラー処理はサボる
// [untextured_mesh]
    // ret_choco_string = choco_string_create_from_c_string = name = base_rotation_joint
// geometry = base_rotation_joint
// material = base_rotation_joint_mat
// coordinate_id = 1
// [/untextured_mesh]

// [untextured_mesh]
// name = elbow_link
// geometry = elbow_link
// material = elbow_link_mat
// coordinate_id = 3
// [/untextured_mesh]

// [untextured_mesh]
// name = left_finger
// geometry = left_finger
// material = left_finger_mat
// coordinate_id = 5
// [/untextured_mesh]

// [untextured_mesh]
// name = right_finger
// geometry = right_finger
// material = right_finger_mat
// coordinate_id = 6
// [/untextured_mesh]

// [untextured_mesh]
// name = shoulder_link
// geometry = shoulder_link
// material = shoulder_link_mat
// coordinate_id = 2
// [/untextured_mesh]

// [untextured_mesh]
// name = wrist_gripper_base
// geometry = wrist_gripper_base
// material = wrist_gripper_base_mat
// coordinate_id = 4
// [/untextured_mesh]

cleanup:
    return ret;
}
