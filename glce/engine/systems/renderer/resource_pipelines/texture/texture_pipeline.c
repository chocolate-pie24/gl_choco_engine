// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resource_pipelines/texture/texture_pipeline.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/texture/texture_cpu_resource.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/resource/loaders/bmp_loader.h"

#include "engine/systems/renderer/resources/texture/texture_gpu_resource_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

#include "engine/systems/renderer/resource_registries/texture/texture_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

static resource_pipeline_result_t bmp_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, size_t* out_pixel_data_size_, uint8_t** out_pixels_);
static resource_pipeline_result_t solid_color_texture_generate(uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, size_t* out_pixel_data_size_, uint8_t** out_pixels_);

resource_pipeline_result_t texture_pipeline_import_from_bmp(const renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int32_t texture_unit_index_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    texture_gpu_resource_result_t ret_gpu_resource = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    texture_cpu_resource_t* cpu_resource = NULL;
    texture_gpu_resource_t* gpu_resource = NULL;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL; // CPUリソースにmoveされるピクセルデータ
    const uint8_t* tmp_pixels2 = NULL;  // CPUリソースから借用するピクセルデータ
    uint16_t tmp_texture_id = 0;
    size_t tmp_pixel_data_size = 0;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "texture_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "texture_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "out_texture_id_")

    ret = bmp_load(texture_fullpath_, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixel_data_size, &tmp_pixels);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - bmp_load failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // CPU側リソース生成
    ret_resource = texture_cpu_resource_create(tmp_width, tmp_height, tmp_channel_count, tmp_pixel_data_size, &tmp_pixels, &cpu_resource);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_cpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    // texture_cpu_resource_createによってtmp_pixelsはNULLになっているためCPUリソースから借用
    ret_resource = texture_cpu_resource_pixel_get(cpu_resource, &tmp_pixels2);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_cpu_resource_pixel_get failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // GPU側リソース生成
    ret_gpu_resource = texture_gpu_resource_create(backend_context_, texture_unit_index_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, tmp_width, tmp_height, tmp_channel_count, tmp_pixels2, &gpu_resource);
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret_gpu_resource) {
        ret = resource_pipeline_rslt_convert_texture_gpu_resource(ret_gpu_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_gpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // Registry登録(gpu_resource, cpu_resourceはregistry側にmoveされる)
    ret_registry = texture_registry_register(texture_registry_, resource_name_, &gpu_resource, &cpu_resource, &tmp_texture_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_registry_register failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = tmp_texture_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, tmp_pixel_data_size, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
        texture_gpu_resource_destroy(&gpu_resource);
        texture_cpu_resource_destroy(&cpu_resource);
    }
    return ret;
}

resource_pipeline_result_t texture_pipeline_import_from_solid_color(const renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int32_t texture_unit_index_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    texture_gpu_resource_result_t ret_gpu_resource = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    texture_cpu_resource_t* cpu_resource = NULL;
    texture_gpu_resource_t* gpu_resource = NULL;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL; // CPUリソースにmoveされるピクセルデータ
    const uint8_t* tmp_pixels2 = NULL;  // CPUリソースから借用するピクセルデータ
    uint16_t tmp_texture_id = 0;
    size_t tmp_pixel_data_size = 0;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "texture_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "out_texture_id_")

    ret = solid_color_texture_generate(red_, green_, blue_, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixel_data_size, &tmp_pixels);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - solid_color_texture_generate failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // CPU側リソース生成
    ret_resource = texture_cpu_resource_create(tmp_width, tmp_height, tmp_channel_count, tmp_pixel_data_size, &tmp_pixels, &cpu_resource);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_cpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }
    // texture_cpu_resource_createによってtmp_pixelsはNULLになっているためCPUリソースから借用
    ret_resource = texture_cpu_resource_pixel_get(cpu_resource, &tmp_pixels2);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_cpu_resource_pixel_get failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // GPU側リソース生成
    ret_gpu_resource = texture_gpu_resource_create(backend_context_, texture_unit_index_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, tmp_width, tmp_height, tmp_channel_count, tmp_pixels2, &gpu_resource);
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret_gpu_resource) {
        ret = resource_pipeline_rslt_convert_texture_gpu_resource(ret_gpu_resource);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_gpu_resource_create failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // Registry登録(gpu_resource, cpu_resourceはregistry側にmoveされる)
    ret_registry = texture_registry_register(texture_registry_, resource_name_, &gpu_resource, &cpu_resource, &tmp_texture_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_registry_register failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = tmp_texture_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, tmp_pixel_data_size, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
        texture_gpu_resource_destroy(&gpu_resource);
        texture_cpu_resource_destroy(&cpu_resource);
    }
    return ret;
}

resource_pipeline_result_t texture_pipeline_release(texture_registry_t* texture_registry_, uint16_t texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_release", "texture_registry_")

    ret_registry = texture_registry_unregister(texture_registry_, texture_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("texture_pipeline_release(%s) - texture_registry_unregister failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

static resource_pipeline_result_t bmp_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, size_t* out_pixel_data_size_, uint8_t** out_pixels_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL;
    size_t tmp_pixel_data_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_channel_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixel_data_size_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_pixel_data_size_")

    ret_resource = bmp_loader_load(fullpath_, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixel_data_size, &tmp_pixels);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("bmp_load(%s) - Failed to load BMP file(%s).", resource_pipeline_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    *out_width_ = tmp_width;
    *out_height_ = tmp_height;
    *out_channel_count_ = tmp_channel_count;
    *out_pixels_ = tmp_pixels;
    *out_pixel_data_size_ = tmp_pixel_data_size;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

static resource_pipeline_result_t solid_color_texture_generate(uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, size_t* out_pixel_data_size_, uint8_t** out_pixels_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    const uint16_t tmp_width = 32;
    const uint16_t tmp_height = 32;
    const uint8_t tmp_channel_count = 3;
    const size_t pixel_size = (size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count;
    uint8_t* tmp_pixels = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_channel_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixel_data_size_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_pixel_data_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "*out_pixels_")

    ret_mem = memory_system_allocate(pixel_size, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_pipeline_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("solid_color_texture_generate(%s) - Failed to allocate memory for pixels.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0, ii = 0; i != (tmp_width * tmp_height); ++i, ii += 3) {
        tmp_pixels[ii] = red_;
        tmp_pixels[ii + 1] = green_;
        tmp_pixels[ii + 2] = blue_;
    }

    *out_width_ = tmp_width;
    *out_height_ = tmp_height;
    *out_channel_count_ = tmp_channel_count;
    *out_pixels_ = tmp_pixels;

    *out_pixel_data_size_ = pixel_size;

    tmp_pixels = NULL;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(NULL != tmp_pixels) {
        memory_system_free(tmp_pixels, pixel_size, MEMORY_TAG_TEXTURE);
        tmp_pixels = NULL;
    }
    return ret;
}
