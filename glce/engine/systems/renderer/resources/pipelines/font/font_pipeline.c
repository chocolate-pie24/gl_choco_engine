// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/pipelines/font/font_pipeline.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/resources/fonts/core/font_types.h"
#include "engine/systems/renderer/resources/fonts/font_gpu_resource.h"
#include "engine/systems/renderer/resources/registries/font/font_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t font_pipeline_import(const renderer_backend_context_t* backend_context_, font_registry_t* registry_, int32_t texture_unit_index_, const font_atlas_asset_t* asset_, uint16_t* out_font_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    font_result_t ret_font = FONT_INVALID_ARGUMENT;

    texture_gpu_resource_t* gpu_resource = NULL;

    uint16_t tmp_font_id = 0;
    bool exists = false;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_font_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT);

    ret_resource_registry = font_registry_exists(registry_, asset_, &exists);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("font_pipeline_import(%s) - font_registry_exists failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    if(exists) {
        ret_resource_registry = font_registry_acquire(registry_, asset_, &tmp_font_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
            ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
            ERROR_MESSAGE("font_pipeline_import(%s) - font_registry_acquire failed.", resource_pipeline_result_to_str(ret));
            goto cleanup;
        }
    } else {
        ret_font = font_gpu_resource_create(backend_context_, texture_unit_index_, asset_, &gpu_resource);
        if(FONT_SUCCESS != ret_font) {
            ret = resource_pipeline_result_convert_font(ret_font);
            ERROR_MESSAGE("font_pipeline_import(%s) - font_gpu_resource_create failed.", resource_pipeline_result_to_str(ret));
            goto cleanup;
        }
        ret_resource_registry = font_registry_register(registry_, asset_, &gpu_resource, &tmp_font_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
            ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
            ERROR_MESSAGE("font_pipeline_import(%s) - font_registry_register failed.", resource_pipeline_result_to_str(ret));
            goto cleanup;
        }
    }

    *out_font_id_ = tmp_font_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        font_gpu_resource_destroy(&gpu_resource);
    }
    return ret;
}

resource_pipeline_result_t font_pipeline_release(font_registry_t* registry_, uint16_t font_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT);

    ret_resource_registry = font_registry_release(registry_, font_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("font_pipeline_release(%s) - font_registry_release failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
