// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_err_utils.h"

// extensionは""を許容する場合があるかも知れないが、当面は引数は全て必須とする
mesh_result_t untextured_mesh_config_initialize(
    const char* geometry_subdirectory_,
    const char* geometry_resource_name_,
    const char* geometry_extension_,
    const char* material_subdirectory_,
    const char* material_resource_name_,
    const char* material_extension_,
    untextured_mesh_config_t* out_config_) {

    mesh_result_t ret = MESH_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_geometry_subdirectory = NULL;
    choco_string_t* tmp_geometry_resource_name = NULL;
    choco_string_t* tmp_geometry_extension = NULL;
    choco_string_t* tmp_material_subdirectory = NULL;
    choco_string_t* tmp_material_resource_name = NULL;
    choco_string_t* tmp_material_extension = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_subdirectory_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "geometry_subdirectory_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_resource_name_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "geometry_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_extension_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "geometry_extension_")
    IF_ARG_NULL_GOTO_CLEANUP(material_subdirectory_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "material_subdirectory_")
    IF_ARG_NULL_GOTO_CLEANUP(material_resource_name_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "material_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(material_extension_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "material_extension_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MESH_INVALID_ARGUMENT, mesh_result_to_str(MESH_INVALID_ARGUMENT), "untextured_mesh_config_initialize", "out_config_")
    if(NULL != out_config_->geometry_subdirectory || NULL != out_config_->geometry_resource_name || NULL != out_config_->geometry_extension) {
        ret = MESH_BAD_OPERATION;
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - Provided config_ is already initialized.", mesh_result_to_str(ret));
        goto cleanup;
    }
    if(NULL != out_config_->material_subdirectory || NULL != out_config_->material_resource_name || NULL != out_config_->material_extension) {
        ret = MESH_BAD_OPERATION;
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - Provided config_ is already initialized.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(geometry_subdirectory_, &tmp_geometry_subdirectory);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(geometry_resource_name_, &tmp_geometry_resource_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(geometry_extension_, &tmp_geometry_extension);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(material_subdirectory_, &tmp_material_subdirectory);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(material_resource_name_, &tmp_material_resource_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(material_extension_, &tmp_material_extension);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = mesh_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_config_initialize(%s) - choco_string_create_from_c_string failed.", mesh_result_to_str(ret));
        goto cleanup;
    }

    out_config_->geometry_subdirectory = tmp_geometry_subdirectory;
    out_config_->geometry_resource_name = tmp_geometry_resource_name;
    out_config_->geometry_extension = tmp_geometry_extension;

    out_config_->material_subdirectory = tmp_material_subdirectory;
    out_config_->material_resource_name = tmp_material_resource_name;
    out_config_->material_extension = tmp_material_extension;

    tmp_geometry_subdirectory = NULL;
    tmp_geometry_resource_name = NULL;
    tmp_geometry_extension = NULL;
    tmp_material_subdirectory = NULL;
    tmp_material_resource_name = NULL;
    tmp_material_extension = NULL;

    ret = MESH_SUCCESS;

cleanup:
    if(MESH_DATA_CORRUPTED == ret || MESH_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_geometry_subdirectory) {
        choco_string_destroy(&tmp_geometry_subdirectory);
    }
    if(NULL != tmp_geometry_resource_name) {
        choco_string_destroy(&tmp_geometry_resource_name);
    }
    if(NULL != tmp_geometry_extension) {
        choco_string_destroy(&tmp_geometry_extension);
    }

    if(NULL != tmp_material_subdirectory) {
        choco_string_destroy(&tmp_material_subdirectory);
    }
    if(NULL != tmp_material_resource_name) {
        choco_string_destroy(&tmp_material_resource_name);
    }
    if(NULL != tmp_material_extension) {
        choco_string_destroy(&tmp_material_extension);
    }
    return ret;
}

void untextured_mesh_config_deinitialize(untextured_mesh_config_t* config_) {
    if(NULL == config_) {
        return;
    }
    if(NULL != config_->geometry_subdirectory) {
        choco_string_destroy(&config_->geometry_subdirectory);
    }
    if(NULL != config_->geometry_resource_name) {
        choco_string_destroy(&config_->geometry_resource_name);
    }
    if(NULL != config_->geometry_extension) {
        choco_string_destroy(&config_->geometry_extension);
    }

    if(NULL != config_->material_subdirectory) {
        choco_string_destroy(&config_->material_subdirectory);
    }
    if(NULL != config_->material_resource_name) {
        choco_string_destroy(&config_->material_resource_name);
    }
    if(NULL != config_->material_extension) {
        choco_string_destroy(&config_->material_extension);
    }
}