// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/scenes/scene_concretes/demos/sample_robot/model_instance/sample_robot_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/core/model_framework_err_utils.h"
#include "application/model_framework/coordinates/model_coordinate_config.h"
#include "application/model_framework/coordinates/model_transform_tree.h"
#include "application/model_framework/assembly/untextured_model/model_assembly_config.h"
#include "application/model_framework/assembly/untextured_model/model_resource_map.h"

#include "application/scenes/core/scene_types.h"
#include "application/scenes/core/scene_err_utils.h"

typedef enum {
    ROBOT_COORDINATE_BASE_YAW = 1,
    ROBOT_COORDINATE_SHOULDER_PITCH = 2,
    ROBOT_COORDINATE_ELBOW_PITCH = 3,
    ROBOT_COORDINATE_WRIST_ROLL = 4,
    ROBOT_COORDINATE_LEFT_FINGER = 5,
    ROBOT_COORDINATE_RIGHT_FINGER = 6,
    ROBOT_COORDINATE_TCP = 7,
    ROBOT_COORDINATE_TOOL_MOUNT = 8,
    ROBOT_COORDINATE_BASE_MOTOR_HOUSING = 9,
    ROBOT_COORDINATE_SHOULDER_MOTOR_MOUNT = 10,
    ROBOT_COORDINATE_ELBOW_MOTOR_MOUNT = 11,
    ROBOT_COORDINATE_WRIST_MOTOR_MOUNT = 12,
    ROBOT_COORDINATE_FIXED_FOUNDATION = 13,
    ROBOT_COORDINATE_ROOT = 14,
} robot_coordinate_id_t;

scene_result_t sample_robot_model_create(const untextured_model_resource_map_t* resource_map_, sample_robot_model_t** out_robot_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    sample_robot_model_t* tmp_robot = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    sample_robot_posture_t default_posture = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_robot_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_robot_, ret, SCENE_BAD_OPERATION);

    ret_general_allocator = general_allocator_allocate(sizeof(sample_robot_model_t), GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION, (void**)&tmp_robot);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_model_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = untextured_model_resource_map_transform_tree_clone(resource_map_, &tmp_transform_tree);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_create(%s) - model_resource_map_transform_tree_clone failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    sample_robot_model_posture_default_get(&default_posture);

    tmp_robot->posture = default_posture;
    tmp_robot->transform_tree = tmp_transform_tree;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_model_is_valid(tmp_robot)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("sample_robot_model_create(%s) - Result validation failed for 'tmp_robot'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_robot_ = tmp_robot;
    tmp_transform_tree = NULL;
    tmp_robot = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_transform_tree) {
        model_transform_tree_destroy(&tmp_transform_tree);
    }
    if(NULL != tmp_robot) {
        general_allocator_free((void**)&tmp_robot, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
    }
    return ret;
}

void sample_robot_model_destroy(sample_robot_model_t** robot_) {
    if(NULL == robot_ || NULL == *robot_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!sample_robot_model_is_valid(*robot_)) {
        ERROR_MESSAGE("sample_robot_model_destroy(%s) - Precondition validation failed for '*robot_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif
    model_transform_tree_destroy(&(*robot_)->transform_tree);
    general_allocator_free((void**)robot_, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
}

scene_result_t sample_robot_model_placement_set(vec3f_t translate_, vec3f_t rotation_, sample_robot_model_t* robot_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SCENE_INVALID_ARGUMENT);

    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_ROOT, rotation_);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_placement_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_model_framework = model_transform_tree_translate_set(robot_->transform_tree, ROBOT_COORDINATE_ROOT, translate_);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_placement_set(%s) - model_transform_tree_translate_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

// NOTE: ChatGPT Generated.
// NOTE: MotorとCoordinate IDのbindingはAssembly Configに従う。
// NOTE: 現状ではMotor AngleとLink Angleが一致すると仮定する。
// NOTE: 可動範囲等の調整はまだ行わない。
scene_result_t sample_robot_model_posture_set(const sample_robot_posture_t* posture_, sample_robot_model_t* robot_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    vec3f_t tmp_rotation = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(posture_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, SCENE_INVALID_ARGUMENT);

    // Base Motor (Yaw, Z-axis)
    tmp_rotation = vec3f_initialize(0.0f, 0.0f, posture_->base_motor_angle);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_BASE_YAW, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Shoulder Motor (Pitch, Y-axis)
    tmp_rotation = vec3f_initialize(0.0f, posture_->shoulder_motor_angle, 0.0f);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_SHOULDER_PITCH, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Elbow Motor (Pitch, Y-axis)
    tmp_rotation = vec3f_initialize(0.0f, posture_->elbow_motor_angle, 0.0f);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_ELBOW_PITCH, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Wrist Motor (Roll, X-axis)
    tmp_rotation = vec3f_initialize(posture_->wrist_motor_angle, 0.0f, 0.0f);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_WRIST_ROLL, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Left Finger
    tmp_rotation = vec3f_initialize(0.0f, 0.0f, posture_->left_finger_angle);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_LEFT_FINGER, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Right Finger
    tmp_rotation = vec3f_initialize(0.0f, 0.0f, posture_->right_finger_angle);
    ret_model_framework = model_transform_tree_rotation_set(robot_->transform_tree, ROBOT_COORDINATE_RIGHT_FINGER, tmp_rotation);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_posture_set(%s) - model_transform_tree_rotation_set failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    robot_->posture = *posture_;

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

void sample_robot_model_posture_default_get(sample_robot_posture_t* out_posture_) {
    if(NULL == out_posture_) {
        return;
    }
    out_posture_->base_motor_angle = 0.0f;
    out_posture_->shoulder_motor_angle = 0.0f;
    out_posture_->elbow_motor_angle = 0.0f;
    out_posture_->wrist_motor_angle = 0.0f;

    out_posture_->left_finger_angle = 0.0f;
    out_posture_->right_finger_angle = 0.0f;
}

// NOTE: ChatGPT Generated.
// NOTE: dummyの一時関数なので正式なエラー処理は一旦おいておく
scene_result_t sample_robot_model_dummy_config_loader(untextured_model_assembly_config_t** out_config_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    // ID 14はRobot全体の配置用座標。
    // ID 1はBase Yaw、9/13は固定側で、いずれも14の子にする。
    static const struct {
        uint16_t coordinate_id;
        uint16_t parent_coordinate_id;
        float translate[3];
        float rotation[3];
    } coordinate_definitions[] = {
        // coordinate_id,                        parent_coordinate_id,            translate,               rotation
        { ROBOT_COORDINATE_ROOT,                 0,                               { 0.0f, 0.0f, 0.0f },    { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_BASE_YAW,             ROBOT_COORDINATE_ROOT,           { 0.0f, 0.0f, 0.0f },    { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_SHOULDER_PITCH,       ROBOT_COORDINATE_BASE_YAW,       { 0.0f, 0.0f, 140.0f },  { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_ELBOW_PITCH,          ROBOT_COORDINATE_SHOULDER_PITCH, { 150.0f, 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_WRIST_ROLL,           ROBOT_COORDINATE_ELBOW_PITCH,    { 120.0f, 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_TOOL_MOUNT,           ROBOT_COORDINATE_WRIST_ROLL,     { 26.0f, 0.0f, 0.0f },   { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_LEFT_FINGER,          ROBOT_COORDINATE_TOOL_MOUNT,     { 10.0f, 18.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_RIGHT_FINGER,         ROBOT_COORDINATE_TOOL_MOUNT,     { 10.0f, -18.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_TCP,                  ROBOT_COORDINATE_TOOL_MOUNT,     { 62.0f, 0.0f, 0.0f },   { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_BASE_MOTOR_HOUSING,   ROBOT_COORDINATE_ROOT,           { 0.0f, 0.0f, 0.0f },    { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_FIXED_FOUNDATION,     ROBOT_COORDINATE_ROOT,           { 0.0f, 0.0f, 0.0f },    { 0.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_SHOULDER_MOTOR_MOUNT, ROBOT_COORDINATE_BASE_YAW,       { 0.0f, 0.0f, 140.0f },  { -90.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_ELBOW_MOTOR_MOUNT,    ROBOT_COORDINATE_SHOULDER_PITCH, { 150.0f, 0.0f, 0.0f },  { -90.0f, 0.0f, 0.0f } },
        { ROBOT_COORDINATE_WRIST_MOTOR_MOUNT,    ROBOT_COORDINATE_ELBOW_PITCH,    { 120.0f, 0.0f, 0.0f },  { 0.0f, 90.0f, 0.0f } },
    };

    // geometry名を共有する4つのmotor Meshは同じResourceをacquireする。
    // Mesh名は現在のuntextured_mesh_config_tには含まれない。
    static const struct {
        const char* geometry_name;
        const char* material_name;
        uint16_t coordinate_id;
    } untextured_mesh_definitions[] = {
        // geometry_name,     material_name,  coordinate_id
        { "base_motor_mount", "link_mat",     ROBOT_COORDINATE_BASE_YAW },
        { "base_support",     "link_mat",     ROBOT_COORDINATE_FIXED_FOUNDATION },
        { "motor",            "actuator_mat", ROBOT_COORDINATE_BASE_MOTOR_HOUSING },
        { "motor",            "actuator_mat", ROBOT_COORDINATE_SHOULDER_MOTOR_MOUNT },
        { "motor",            "actuator_mat", ROBOT_COORDINATE_ELBOW_MOTOR_MOUNT },
        { "motor",            "actuator_mat", ROBOT_COORDINATE_WRIST_MOTOR_MOUNT },
        { "shoulder_link",    "link_mat",     ROBOT_COORDINATE_SHOULDER_PITCH },
        { "elbow_link",       "link_mat",     ROBOT_COORDINATE_ELBOW_PITCH },
        { "wrist_link",       "link_mat",     ROBOT_COORDINATE_WRIST_ROLL },
        { "tool_interface",   "actuator_mat", ROBOT_COORDINATE_WRIST_ROLL },
        { "gripper_body",     "end_tool_mat", ROBOT_COORDINATE_TOOL_MOUNT },
        { "left_finger",      "end_tool_mat", ROBOT_COORDINATE_LEFT_FINGER },
        { "right_finger",     "end_tool_mat", ROBOT_COORDINATE_RIGHT_FINGER },
    };

    const size_t coordinate_count = sizeof(coordinate_definitions) / sizeof(coordinate_definitions[0]);
    const size_t untextured_mesh_count = sizeof(untextured_mesh_definitions) / sizeof(untextured_mesh_definitions[0]);

    untextured_model_assembly_config_t* tmp_assembly_config = NULL;
    model_coordinate_config_t* coordinates = NULL;
    untextured_mesh_config_t* untextured_mesh_configs = NULL;
    uint16_t* coordinate_ids = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_config_, ret, SCENE_BAD_OPERATION);

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(*coordinates) * coordinate_count, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION, (void**)&coordinates);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_model_dummy_config_loader(%s) - Failed to allocate coordinates.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(*untextured_mesh_configs) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION, (void**)&untextured_mesh_configs);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_model_dummy_config_loader(%s) - Failed to allocate mesh configs.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(*coordinate_ids) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION, (void**)&coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_model_dummy_config_loader(%s) - Failed to allocate mesh coordinate IDs.", scene_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != coordinate_count; ++i) {
        ret_model_framework = model_coordinate_config_initialize(
            coordinate_definitions[i].coordinate_id,
            coordinate_definitions[i].parent_coordinate_id,
            vec3f_initialize(coordinate_definitions[i].translate[0], coordinate_definitions[i].translate[1], coordinate_definitions[i].translate[2]),
            vec3f_initialize(coordinate_definitions[i].rotation[0], coordinate_definitions[i].rotation[1], coordinate_definitions[i].rotation[2]),
            &coordinates[i]);
        if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
            ret = scene_result_convert_model_framework(ret_model_framework);
            ERROR_MESSAGE("sample_robot_model_dummy_config_loader(%s) - model_coordinate_config_initialize failed (index = %zu).", scene_result_to_str(ret), i);
            goto cleanup;
        }
    }

    for(size_t i = 0; i != untextured_mesh_count; ++i) {
        coordinate_ids[i] = untextured_mesh_definitions[i].coordinate_id;
        untextured_mesh_configs[i].geometry_subdirectory = "stl/robot_sample";
        untextured_mesh_configs[i].geometry_resource_name = untextured_mesh_definitions[i].geometry_name;
        untextured_mesh_configs[i].geometry_extension = "stl";
        untextured_mesh_configs[i].material_subdirectory = "materials/robot_sample";
        untextured_mesh_configs[i].material_resource_name = untextured_mesh_definitions[i].material_name;
        untextured_mesh_configs[i].material_extension = "phong_mat";
    }

    // NOTE: coordinates, coordinate_ids, untextured_mesh_configsはtmp_assembly_configに所有権がmoveされるためNULLになる
    ret_model_framework = untextured_model_assembly_config_create(
        coordinate_count,
        &coordinates,
        untextured_mesh_count, &coordinate_ids, &untextured_mesh_configs,
        &tmp_assembly_config);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ret = scene_result_convert_model_framework(ret_model_framework);
        ERROR_MESSAGE("sample_robot_model_dummy_config_loader(%s) - model_assembly_config_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Output. Assembly Configが各配列のOwnershipを引き受けている。
    *out_config_ = tmp_assembly_config;
    tmp_assembly_config = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    // createが成功していれば上でOutputして終了する。以下は失敗時の局所Rollback。
    if(NULL != tmp_assembly_config) {
        untextured_model_assembly_config_destroy(&tmp_assembly_config);
    }
    if(NULL != coordinates) {
        general_allocator_free((void**)&coordinates, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
    }
    if(NULL != untextured_mesh_configs) {
        general_allocator_free((void**)&untextured_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
    }
    if(NULL != coordinate_ids) {
        general_allocator_free((void**)&coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_APPLICATION);
    }
    return ret;
}

// ChatGPT Generated
bool sample_robot_model_is_valid(const sample_robot_model_t* robot_) {
    if(NULL == robot_) {
        return false;
    }

    // Owned Transform Tree.
    if(NULL == robot_->transform_tree) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(robot_->transform_tree)) {
        return false;
    }
    if(!model_transform_tree_is_valid(robot_->transform_tree)) {
        return false;
    }

    // Robot Posture.
    if(!isfinite(robot_->posture.base_motor_angle) ||
       !isfinite(robot_->posture.shoulder_motor_angle) ||
       !isfinite(robot_->posture.elbow_motor_angle) ||
       !isfinite(robot_->posture.wrist_motor_angle) ||
       !isfinite(robot_->posture.left_finger_angle) ||
       !isfinite(robot_->posture.right_finger_angle)) {
        return false;
    }

    return true;
}
