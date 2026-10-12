// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_pipelines/materials/untextured_material_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/loaders/untextured_material_config_loader.h"

#include "engine/systems/renderer/resources/material/material_types.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/material/untextured_material_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t untextured_material_pipeline_import_from_file(untextured_material_registry_t* material_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

    untextured_material_config_t material_config = { 0 };
    untextured_material_t material = { 0 };

    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(material_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_material_pipeline_import_from_file", "material_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_material_pipeline_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_material_pipeline_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_material_pipeline_import_from_file", "out_material_id_")

    ret_resource = untextured_material_config_loader_load(resource_fullpath_, &material_config);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_result_convert_resource(ret_resource);
        ERROR_MESSAGE("untextured_material_pipeline_import_from_file(%s) - untextured_material_config_loader_load failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    material.ambient = material_config.ambient;
    material.diffuse = material_config.diffuse;
    material.shininess = material_config.shininess;
    material.specular = material_config.specular;
    if(!untextured_material_is_valid(&material)) {
        ret = RESOURCE_PIPELINE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("untextured_material_pipeline_import_from_file(%s) - Invalid material.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_material_registry_register(material_registry_, resource_name_, &material, &tmp_material_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_material_pipeline_import_from_file(%s) - untextured_material_registry_register failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    *out_material_id_ = tmp_material_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

resource_pipeline_result_t untextured_material_pipeline_release(untextured_material_registry_t* material_registry_, uint16_t material_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(material_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_material_material_pipeline_release", "material_registry_")

    ret_resource_registry = untextured_material_registry_unregister(material_registry_, material_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_material_material_pipeline_release(%s) - untextured_material_registry_unregister failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
