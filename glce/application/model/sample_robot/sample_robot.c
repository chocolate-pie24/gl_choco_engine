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

application_result_t sample_robot_create(const char* model_name_, vec3f_t root_translate_, vec3f_t root_rotation_, sample_robot_t** out_robot_) {
    // TODO: この中の処理は全てmodel_pipelineに移す
    (void)model_name_;

    return APPLICATION_SUCCESS;
}

void sample_robot_destroy(sample_robot_t** robot_) {

}

application_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_) {
    return APPLICATION_SUCCESS;
}

// NOTE: dummyの一時関数なので正式なエラー処理は一旦おいておく
application_result_t sample_robot_dummy_config_loader(render_model_config_t* out_config_) {
    application_result_t ret = APPLICATION_LIMIT_EXCEEDED;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    render_model_result_t ret_render_model = RENDER_MODEL_INVALID_ARGUMENT;

    size_t coordinate_count = 0;
    size_t untextured_mesh_count = 0;

    render_model_coordinate_config_t* coordinates = NULL;
    untextured_mesh_config_t* untextured_mesh_configs = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "sample_robot_dummy_config_loader", "out_config_")
    // out_config_のfieldのNULLチェックも必要だが、dummy helperなのでやらない

    coordinate_count = 7;
    ret_general_allocator = general_allocator_allocate(sizeof(render_model_coordinate_config_t) * coordinate_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&coordinates);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    untextured_mesh_count = 6;
    ret_general_allocator = general_allocator_allocate(sizeof(untextured_mesh_config_t) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&untextured_mesh_configs);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Base
    ret_render_model = render_model_config_coordinate_initialize(1, 0, vec3f_initialize(0.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[0]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Shoulder
    ret_render_model = render_model_config_coordinate_initialize(2, 1, vec3f_initialize(0.0f, 0.0f, 140.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[1]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Elbow
    ret_render_model = render_model_config_coordinate_initialize(3, 2, vec3f_initialize(150.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[2]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Wrist
    ret_render_model = render_model_config_coordinate_initialize(4, 3, vec3f_initialize(120.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[3]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Left finger
    ret_render_model = render_model_config_coordinate_initialize(5, 4, vec3f_initialize(36.0f, 18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[4]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Right finger
    ret_render_model = render_model_config_coordinate_initialize(6, 4, vec3f_initialize(36.0f, -18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[5]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // TCP(Tool Center Point)
    ret_render_model = render_model_config_coordinate_initialize(7, 4, vec3f_initialize(88.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[6]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("base_rotation_joint", "stl/robot_sample", "base_rotation_joint", "materials/robot_sample", "base_rotation_joint_mat", 1, &untextured_mesh_configs[0]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("elbow_link", "stl/robot_sample", "elbow_link", "materials/robot_sample", "elbow_link_mat", 3, &untextured_mesh_configs[1]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("left_finger", "stl/robot_sample", "left_finger", "materials/robot_sample", "left_finger_mat", 5, &untextured_mesh_configs[2]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("right_finger", "stl/robot_sample", "right_finger", "materials/robot_sample", "right_finger_mat", 6, &untextured_mesh_configs[3]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("shoulder_link", "stl/robot_sample", "shoulder_link", "materials/robot_sample", "shoulder_link_mat", 2, &untextured_mesh_configs[4]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_render_model = render_model_config_untextured_mesh_initialize("wrist_gripper_base", "stl/robot_sample", "wrist_gripper_base", "materials/robot_sample", "wrist_gripper_base_mat", 4, &untextured_mesh_configs[5]);
    if(RENDER_MODEL_SUCCESS != ret_render_model) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_untextured_mesh_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}
