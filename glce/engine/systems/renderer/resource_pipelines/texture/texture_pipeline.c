// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_pipelines/texture/texture_pipeline.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/texture/texture_cpu_resource.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/texture/texture_gpu_resource_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

#include "engine/systems/renderer/resource_registries/texture/texture_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

static resource_pipeline_result_t resolve_texture_source(const char* texture_name_, choco_string_t** out_texture_source_);
static resource_pipeline_result_t load_cpu_resource_pixels(texture_cpu_resource_t* cpu_resource_, const char* texture_name_);

resource_pipeline_result_t texture_pipeline_import_from_file(renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int32_t gpu_unit_num_, const char* texture_name_, int16_t* out_texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    texture_gpu_resource_result_t ret_gpu_resource = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    texture_cpu_resource_t* cpu_resource = NULL;
    texture_gpu_resource_t* gpu_resource = NULL;

    uint16_t texture_width = 0;
    uint16_t texture_height = 0;
    uint8_t texture_channel_count = 0;
    const uint8_t* texture_pixels = NULL;
    int16_t tmp_texture_id = 0;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_file", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_file", "texture_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_file", "texture_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_file", "out_texture_id_")
    if(!texture_registry_is_valid(texture_registry_)) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - provided texture registry is not valid.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == texture_name_[0]) {
        ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - provided texture_name_ is not valid.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // CPU側リソース生成
    ret_resource = texture_cpu_resource_create(&cpu_resource);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_cpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    ret = load_cpu_resource_pixels(cpu_resource, texture_name_);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - load_cpu_resource_pixels failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    ret_resource = texture_cpu_resource_pixel_size_get(cpu_resource, &texture_width, &texture_height, &texture_channel_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_cpu_resource_pixel_size_get failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    ret_resource = texture_cpu_resource_pixel_get(cpu_resource, &texture_pixels);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_cpu_resource_pixel_get failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // GPU側リソース生成
    ret_gpu_resource = texture_gpu_resource_create(backend_context_, gpu_unit_num_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &gpu_resource);
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret_gpu_resource) {
        ret = resource_pipeline_rslt_convert_texture_gpu_resource(ret_gpu_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_gpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    ret_gpu_resource = texture_gpu_resource_upload(backend_context_, gpu_resource, texture_width, texture_height, texture_channel_count, texture_pixels);
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret_gpu_resource) {
        ret = resource_pipeline_rslt_convert_texture_gpu_resource(ret_gpu_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_gpu_resource_upload failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // Registry登録(gpu_resource, cpu_resourceはregistry側にmoveされる)
    ret_registry = texture_registry_register(texture_registry_, texture_name_, &gpu_resource, &cpu_resource, &tmp_texture_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("texture_pipeline_import_from_file(%s) - texture_registry_register failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = tmp_texture_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        // backend_context_ == NULLの場合はtexture_gpu_resource_destroyはNo-opになるためチェック不要
        texture_gpu_resource_destroy(backend_context_, &gpu_resource);
        texture_cpu_resource_destroy(&cpu_resource);
    }
    return ret;
}

resource_pipeline_result_t texture_pipeline_release(renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int16_t texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_release", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_release", "texture_registry_")
    if(!texture_registry_is_valid(texture_registry_)) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_pipeline_release(%s) - provided texture registry is not valid.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_registry = texture_registry_unregister(texture_registry_, backend_context_, texture_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("texture_pipeline_release(%s) - texture_registry_unregister failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

static resource_pipeline_result_t resolve_texture_source(const char* texture_name_, choco_string_t** out_texture_source_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* texture_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(texture_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resolve_texture_source", "texture_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_source_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resolve_texture_source", "out_texture_source_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_source_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resolve_texture_source", "*out_texture_source_")

    ret_string = choco_string_default_create(&texture_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_pipeline_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("resolve_texture_source(%s) - choco_string_default_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    if(choco_string_equal("test_texture_red", texture_name_)) {
        ret_string = choco_string_copy_from_c_string(texture_name_, texture_source);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_pipeline_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("resolve_texture_source(%s) - choco_string_copy_from_c_string failed.", resource_pipeline_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_green", texture_name_)) {
        ret_string = choco_string_copy_from_c_string(texture_name_, texture_source);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_pipeline_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("resolve_texture_source(%s) - choco_string_copy_from_c_string failed.", resource_pipeline_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_blue", texture_name_)) {
        ret_string = choco_string_copy_from_c_string(texture_name_, texture_source);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_pipeline_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("resolve_texture_source(%s) - choco_string_copy_from_c_string failed.", resource_pipeline_rslt_to_str(ret));
            goto cleanup;
        }
    } else {
        // begin fs_pathができるまでの暫定コード
        ret_string = choco_string_copy_from_c_string("../assets/textures/", texture_source);
        ret_string = choco_string_concat_from_c_string(texture_name_, texture_source);
        ret_string = choco_string_concat_from_c_string(".bmp", texture_source);
        // end fs_pathができるまでの暫定コード
    }

    *out_texture_source_ = texture_source;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        choco_string_destroy(&texture_source);
    }
    return ret;
}

static resource_pipeline_result_t load_cpu_resource_pixels(texture_cpu_resource_t* cpu_resource_, const char* texture_name_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

    choco_string_t* texture_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(cpu_resource_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "load_cpu_resource_pixels", "cpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "load_cpu_resource_pixels", "texture_name_")

    ret = resolve_texture_source(texture_name_, &texture_source);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("load_cpu_resource_pixels(%s) - resolve_texture_source failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_resource = texture_cpu_resource_pixel_load(cpu_resource_, choco_string_c_str(texture_source));
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("load_cpu_resource_pixels(%s) - texture_cpu_resource_pixel_load failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    choco_string_destroy(&texture_source);
    return ret;
}
