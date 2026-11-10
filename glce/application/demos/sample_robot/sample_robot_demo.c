// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/sample_robot/sample_robot_demo.h"

#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/memory/general_allocator/general_allocator.h"

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
};

sample_robot_result_t sample_robot_demo_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, sample_robot_demo_t** out_demo_) {
    sample_robot_result_t ret = SAMPLE_ROBOT_INVALID_ARGUMENT;

    model_framework_result_t ret_model_framework = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    sample_robot_demo_t* tmp_demo = NULL;
    untextured_model_assembly_config_t* config = NULL;
    untextured_model_resource_map_t* tmp_resource_map = NULL;
    sample_robot_t* tmp_robot1 = NULL;
    sample_robot_t* tmp_robot2 = NULL;

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

    tmp_demo->resource_map = tmp_resource_map;
    tmp_demo->robot1 = tmp_robot1;
    tmp_demo->robot2 = tmp_robot2;

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

    ret = SAMPLE_ROBOT_SUCCESS;

cleanup:
    if(SAMPLE_ROBOT_DATA_CORRUPTED == ret || SAMPLE_ROBOT_SUCCESS == ret) {
        return ret;
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

    sample_robot_destroy(&(*demo_)->robot1);
    sample_robot_destroy(&(*demo_)->robot2);

    ret_model_framework = untextured_model_resource_map_destroy(renderer_frontend_, &(*demo_)->resource_map);
    if(MODEL_FRAMEWORK_SUCCESS != ret_model_framework) {
        ERROR_MESSAGE("sample_robot_demo_destroy(%s) - untextured_model_resource_map_destroy failed.", sample_robot_result_to_str(SAMPLE_ROBOT_DATA_CORRUPTED));
        return;
    }
    general_allocator_free((void**)demo_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

// TODO: 実装
bool sample_robot_demo_is_valid(const sample_robot_demo_t* demo_) {
    if(NULL == demo_) {
        return false;
    }
    return true;
}
