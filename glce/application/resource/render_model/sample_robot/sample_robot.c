// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/resource/render_model/sample_robot/sample_robot.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/config/render_model_config.h"
#include "engine/systems/renderer/resources/render_model/render_model.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

#include "application/resource/render_model_pipeline/render_model_pipeline.h"

application_result_t sample_robot_create(vec3f_t root_translate_, vec3f_t root_rotation_, renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, sample_robot_t** out_robot_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    sample_robot_t* tmp_robot = NULL;
    render_model_t* tmp_model = NULL;
    render_model_config_t config;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "sample_robot_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "sample_robot_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_robot_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "sample_robot_create", "out_robot_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_robot_, ret, APPLICATION_BAD_OPERATION, application_result_to_str(APPLICATION_BAD_OPERATION), "sample_robot_create", "*out_robot_")
    if('\0' == asset_root_path_[0]) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("sample_robot_create(%s) - Provided asset_root_path_ is not valid.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(sample_robot_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_robot);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_create(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = sample_robot_dummy_config_loader(root_translate_, root_rotation_, &config);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_create(%s) - sample_robot_dummy_config_loader failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = render_model_import(&config, renderer_frontend_, asset_root_path_, &tmp_model);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_create(%s) - render_model_import failed.", application_result_to_str(ret));
        goto cleanup;
    }
    render_model_config_deinitialize(&config);

    tmp_robot->posture_dirty = true;
    tmp_robot->posture.left_finger_angle = 0.0f;
    tmp_robot->posture.link1_angle = 0.0f;
    tmp_robot->posture.link2_angle = 0.0f;
    tmp_robot->posture.right_finger_angle = 0.0f;
    tmp_robot->render_model = tmp_model;
    tmp_model = NULL;

    *out_robot_ = tmp_robot;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED == ret || APPLICATION_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_model) {
        render_model_destroy(&tmp_model);
    }
    render_model_config_deinitialize(&config);
    general_allocator_free((void**)&tmp_robot, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    return ret;
}

void sample_robot_destroy(sample_robot_t** robot_) {
    if(NULL == robot_ || NULL == *robot_) {
        return;
    }
    render_model_destroy(&(*robot_)->render_model);
    general_allocator_free((void**)robot_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

// application_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_) {
//     return APPLICATION_SUCCESS;
// }

// NOTE: dummyの一時関数なので正式なエラー処理は一旦おいておく
application_result_t sample_robot_dummy_config_loader(vec3f_t root_translate_, vec3f_t root_rotation_, render_model_config_t* out_config_) {
    application_result_t ret = APPLICATION_LIMIT_EXCEEDED;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    render_model_result_t ret_render_model = RENDER_MODEL_INVALID_ARGUMENT;
    mesh_result_t ret_mesh = MESH_INVALID_ARGUMENT;

    size_t coordinate_count = 0;
    size_t untextured_mesh_count = 0;

    render_model_coordinate_config_t* coordinates = NULL;
    untextured_mesh_config_t* untextured_mesh_configs = NULL;
    uint16_t* coordinate_ids = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "sample_robot_dummy_config_loader", "out_config_")
    // out_config_のfieldのNULLチェックも必要だが、dummy helperなのでやらない

    //////////////////////////////////////////////////////////////////////////////////////////
    // Coordinate config
    //////////////////////////////////////////////////////////////////////////////////////////
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

    ret_general_allocator = general_allocator_allocate(sizeof(uint16_t) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Base
    ret_render_model = render_model_config_coordinate_initialize(1, 0, root_translate_, root_rotation_, &coordinates[0]);
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

    //////////////////////////////////////////////////////////////////////////////////////////
    // Mesh config
    //////////////////////////////////////////////////////////////////////////////////////////
    coordinate_ids[0] = 1;
    coordinate_ids[1] = 3;
    coordinate_ids[2] = 5;
    coordinate_ids[3] = 6;
    coordinate_ids[4] = 2;
    coordinate_ids[5] = 4;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "base_rotation_joint",
        "stl",
        "materials/robot_sample",
        "base_rotation_joint_mat",
        "untextured_mat",
        &untextured_mesh_configs[0]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "elbow_link",
        "stl",
        "materials/robot_sample",
        "elbow_link_mat",
        "untextured_mat",
        &untextured_mesh_configs[1]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "left_finger",
        "stl",
        "materials/robot_sample",
        "left_finger_mat",
        "untextured_mat",
        &untextured_mesh_configs[2]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "right_finger",
        "stl",
        "materials/robot_sample",
        "right_finger_mat",
        "untextured_mat",
        &untextured_mesh_configs[3]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "shoulder_link",
        "stl",
        "materials/robot_sample",
        "shoulder_link_mat",
        "untextured_mat",
        &untextured_mesh_configs[4]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "wrist_gripper_base",
        "stl",
        "materials/robot_sample",
        "wrist_gripper_base_mat",
        "untextured_mat",
        &untextured_mesh_configs[5]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }

    out_config_->coordinate_count = coordinate_count;
    out_config_->coordinates = coordinates;
    out_config_->line_mesh_configs = NULL;
    out_config_->line_mesh_coordinate_ids = NULL;
    out_config_->line_mesh_count = 0;
    out_config_->point_mesh_configs = NULL;
    out_config_->point_mesh_coordinate_ids = NULL;
    out_config_->point_mesh_count = 0;
    out_config_->ui_mesh_configs = NULL;
    out_config_->ui_mesh_coordinate_ids = NULL;
    out_config_->ui_mesh_count = 0;
    out_config_->untextured_mesh_configs = untextured_mesh_configs;
    out_config_->untextured_mesh_coordinate_ids = coordinate_ids;
    out_config_->untextured_mesh_count = untextured_mesh_count;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED == ret || APPLICATION_SUCCESS == ret) {
        return ret;
    }
    if(NULL != coordinates) {
        general_allocator_free((void**)&coordinates, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != untextured_mesh_configs) {
        general_allocator_free((void**)&untextured_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != coordinate_ids) {
        general_allocator_free((void**)&coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}
