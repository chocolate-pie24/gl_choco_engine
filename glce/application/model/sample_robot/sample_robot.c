// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model/sample_robot/sample_robot.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"
#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "application/model/core/model_types.h"
#include "application/model/core/model_err_utils.h"
#include "application/model/model_resource_map.h"
#include "application/model/model_transform_tree.h"
#include "application/model/model_assembly_config.h"

model_result_t sample_robot_create(const model_resource_map_t* resource_map_, sample_robot_t** out_robot_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    sample_robot_t* tmp_robot = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    sample_robot_posture_t default_posture = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_create", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_robot_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_create", "out_robot_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_robot_, ret, MODEL_BAD_OPERATION, model_result_to_str(MODEL_BAD_OPERATION), "sample_robot_create", "*out_robot_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_resource_map_is_valid(resource_map_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("sample_robot_create(%s) - Provided resource_map_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_general_allocator = general_allocator_allocate(sizeof(sample_robot_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_robot);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_create(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret = model_resource_map_transform_tree_clone(resource_map_, &tmp_transform_tree);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_create(%s) - model_resource_map_transform_tree_clone failed.", model_result_to_str(ret));
        goto cleanup;
    }

    sample_robot_posture_default_get(&default_posture);

    tmp_robot->posture = default_posture;
    tmp_robot->transform_tree = tmp_transform_tree;
    *out_robot_ = tmp_robot;
    tmp_transform_tree = NULL;
    tmp_robot = NULL;

    ret = MODEL_SUCCESS;

cleanup:
    if(MODEL_DATA_CORRUPTED == ret || MODEL_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_transform_tree) {
        model_transform_tree_destroy(&tmp_transform_tree);
    }
    if(NULL != tmp_robot) {
        general_allocator_free((void**)&tmp_robot, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void sample_robot_destroy(sample_robot_t** robot_) {
    if(NULL == robot_ || NULL == *robot_) {
        return;
    }
    model_transform_tree_destroy(&(*robot_)->transform_tree);
    general_allocator_free((void**)robot_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

model_result_t sample_robot_placement_set(vec3f_t translate_, vec3f_t rotation_, sample_robot_t* robot_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_placement_set", "robot_")

    // Base: Coordinate ID = 1
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 1, rotation_);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_placement_set(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret = model_transform_tree_translate_set(robot_->transform_tree, 1, translate_);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_placement_set(%s) - model_transform_tree_translate_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

// NOTE: linkとcoordinate idのbindingはconfigを見ながら手動で書く
// NOTE: 可動範囲等の調整はまだ行わない
// TODO: 座標変換処理が途中で失敗した場合のrollback
model_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    vec3f_t tmp_rotation = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(posture_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_posture_update", "posture_")
    IF_ARG_NULL_GOTO_CLEANUP(robot_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_posture_update", "robot_")

    // | Coordinate ID | File                            | Model part                   | Local origin              | Motion axis |
    // |---------------|---------------------------------|------------------------------|---------------------------|-------------|
    // | 1             | `01_base_rotation_joint.stl`    | Base with support pedestal   | Base yaw center at origin | Z           |
    // | 2             | `02_shoulder_link.stl`          | Link 1                       | Shoulder pitch hinge      | Y           |
    // | 3             | `03_elbow_link.stl`             | Link 2                       | Elbow pitch hinge         | Y           |
    // | 4             | `04_wrist_gripper_base.stl`     | Link 3 tip / gripper base    | Wrist roll center         | X           |
    // | 5             | `05_left_finger.stl`            | Left finger: **one cuboid**  | Left hinge, on rear face  | Z           |
    // | 6             | `06_right_finger.stl`           | Right finger: **one cuboid** | Right hinge, on rear face | Z           |

    // Link1
    tmp_rotation = vec3f_initialize(0.0f, posture_->link1_angle, 0.0f);
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 2, tmp_rotation);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_posture_update(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Link2
    tmp_rotation = vec3f_initialize(0.0f, posture_->link2_angle, 0.0f);
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 3, tmp_rotation);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_posture_update(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Link3
    tmp_rotation = vec3f_initialize(posture_->gripper_angle, 0.0f, 0.0f);
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 4, tmp_rotation);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_posture_update(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Left finger
    tmp_rotation = vec3f_initialize(0.0f, 0.0f, posture_->left_finger_angle);
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 5, tmp_rotation);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_posture_update(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Right finger
    tmp_rotation = vec3f_initialize(0.0f, 0.0f, posture_->right_finger_angle);
    ret = model_transform_tree_rotation_set(robot_->transform_tree, 6, tmp_rotation);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_posture_update(%s) - model_transform_tree_rotation_set failed.", model_result_to_str(ret));
        goto cleanup;
    }

    robot_->posture = *posture_;

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

void sample_robot_posture_default_get(sample_robot_posture_t* posture_) {
    if(NULL == posture_) {
        return;
    }
    posture_->left_finger_angle = 0.0f;
    posture_->gripper_angle = 0.0f;
    posture_->link1_angle = 0.0f;
    posture_->link2_angle = 0.0f;
    posture_->right_finger_angle = 0.0f;
}

// NOTE: dummyの一時関数なので正式なエラー処理は一旦おいておく
model_result_t sample_robot_dummy_config_loader(model_assembly_config_t** out_config_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    mesh_result_t ret_mesh = MESH_INVALID_ARGUMENT;

    size_t coordinate_count = 0;
    size_t untextured_mesh_count = 0;
    size_t created_config_count = 0;

    model_assembly_config_t* tmp_assembly_config = NULL;
    model_coordinate_config_t* coordinates = NULL;
    untextured_mesh_config_t* untextured_mesh_configs = NULL;
    uint16_t* coordinate_ids = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "sample_robot_dummy_config_loader", "out_config_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_config_, ret, MODEL_BAD_OPERATION, model_result_to_str(MODEL_BAD_OPERATION), "sample_robot_dummy_config_loader", "*out_config_")

    //////////////////////////////////////////////////////////////////////////////////////////
    // Coordinate config
    //////////////////////////////////////////////////////////////////////////////////////////
    coordinate_count = 7;
    ret_general_allocator = general_allocator_allocate(sizeof(model_coordinate_config_t) * coordinate_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&coordinates);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
        goto cleanup;
    }

    untextured_mesh_count = 6;
    ret_general_allocator = general_allocator_allocate(sizeof(untextured_mesh_config_t) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&untextured_mesh_configs);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(uint16_t) * untextured_mesh_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - general_allocator_allocate failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Base
    ret = model_coordinate_config_initialize(1, 0, vec3f_initialize(0.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[0]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - render_model_config_coordinate_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Shoulder
    ret = model_coordinate_config_initialize(2, 1, vec3f_initialize(0.0f, 0.0f, 140.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[1]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Elbow
    ret = model_coordinate_config_initialize(3, 2, vec3f_initialize(150.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[2]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Wrist
    ret = model_coordinate_config_initialize(4, 3, vec3f_initialize(120.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[3]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Left finger
    ret = model_coordinate_config_initialize(5, 4, vec3f_initialize(36.0f, 18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[4]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // Right finger
    ret = model_coordinate_config_initialize(6, 4, vec3f_initialize(36.0f, -18.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[5]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // TCP(Tool Center Point)
    ret = model_coordinate_config_initialize(7, 4, vec3f_initialize(88.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f), &coordinates[6]);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - model_coordinate_config_initialize failed.", model_result_to_str(ret));
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
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "elbow_link",
        "stl",
        "materials/robot_sample",
        "elbow_link_mat",
        "untextured_mat",
        &untextured_mesh_configs[1]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "left_finger",
        "stl",
        "materials/robot_sample",
        "left_finger_mat",
        "untextured_mat",
        &untextured_mesh_configs[2]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "right_finger",
        "stl",
        "materials/robot_sample",
        "right_finger_mat",
        "untextured_mat",
        &untextured_mesh_configs[3]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "shoulder_link",
        "stl",
        "materials/robot_sample",
        "shoulder_link_mat",
        "untextured_mat",
        &untextured_mesh_configs[4]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret_mesh = untextured_mesh_config_initialize(
        "stl/robot_sample",
        "wrist_gripper_base",
        "stl",
        "materials/robot_sample",
        "wrist_gripper_base_mat",
        "untextured_mat",
        &untextured_mesh_configs[5]);
    if(MESH_SUCCESS != ret_mesh) {
        ret = model_result_convert_mesh(ret_mesh);
        ERROR_MESSAGE("sample_robot_dummy_config_loader(%s) - untextured_mesh_config_initialize failed.", model_result_to_str(ret));
        goto cleanup;
    }
    created_config_count++;

    ret = model_assembly_config_create(
        coordinate_count,
        &coordinates,
        0, NULL, NULL,
        0, NULL, NULL,
        0, NULL, NULL,
        untextured_mesh_count, &coordinate_ids, &untextured_mesh_configs, &tmp_assembly_config);
    if(MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("model_assembly_config_create(%s) - model_assembly_config_create failed.", model_result_to_str(ret));
        goto cleanup;
    }

    // NOTE: ユーザーアプリケーション領域なのでvalidationは行わない
    *out_config_ = tmp_assembly_config;

    ret = MODEL_SUCCESS;

cleanup:
    if(MODEL_DATA_CORRUPTED == ret || MODEL_SUCCESS == ret) {
        return ret;
    }
    if(NULL != untextured_mesh_configs) {
        for(size_t i = 0; i != created_config_count; ++i) {
            untextured_mesh_config_deinitialize(&untextured_mesh_configs[i]);
        }
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
    if(NULL != tmp_assembly_config) {
        general_allocator_free((void**)&tmp_assembly_config, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}
