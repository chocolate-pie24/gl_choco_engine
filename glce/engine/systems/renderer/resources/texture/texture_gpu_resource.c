#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_texture.h"

#include "engine/systems/renderer/resources/texture/texture_gpu_resource_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource_err_utils.h"

struct texture_gpu_resource {
    renderer_backend_texture_t* backend_texture;
    bool uploaded;
};

texture_gpu_resource_result_t texture_gpu_resource_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, texture_gpu_resource_t** out_texture_gpu_resource_) {
    texture_gpu_resource_result_t ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    texture_gpu_resource_t* tmp_texture_gpu_resource = NULL;

    bool texture_created = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_gpu_resource_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_create", "out_texture_gpu_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_gpu_resource_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_create", "*out_texture_gpu_resource_")

    ret_memory = memory_system_allocate(sizeof(texture_gpu_resource_t), MEMORY_TAG_RENDERER, (void**)&tmp_texture_gpu_resource);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = texture_gpu_resource_rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - memory_system_allocate failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_texture_gpu_resource, 0, sizeof(texture_gpu_resource_t));

    ret_renderer_backend = renderer_backend_texture_create(backend_context_, unit_num_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, &tmp_texture_gpu_resource->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_create failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_texture_gpu_resource->uploaded = false;

    texture_created = true;

    *out_texture_gpu_resource_ = tmp_texture_gpu_resource;

    ret = TEXTURE_GPU_RESOURCE_SUCCESS;

cleanup:
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret) {
        if(texture_created && NULL != backend_context_ && NULL != tmp_texture_gpu_resource) {
            renderer_backend_texture_destroy(backend_context_, &tmp_texture_gpu_resource->backend_texture);
        }
        if(NULL != tmp_texture_gpu_resource) {
            memory_system_free(tmp_texture_gpu_resource, sizeof(texture_gpu_resource_t), MEMORY_TAG_RENDERER);
            tmp_texture_gpu_resource = NULL;
        }
    }
    return ret;
}

void texture_gpu_resource_destroy(renderer_backend_context_t* backend_context_, texture_gpu_resource_t** texture_gpu_resource_) {
    if(NULL == texture_gpu_resource_) {
        return;
    }
    if(NULL == *texture_gpu_resource_) {
        return;
    }
    if(NULL == backend_context_) {
        // TODO: エラーメッセージにtexture_gpu_resource_が解放されないことを明記する
        ERROR_MESSAGE("texture_gpu_resource_destroy(%s) - Provided backend_context_ is NULL.", texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT));
        return;
    }
    renderer_backend_texture_destroy(backend_context_, &(*texture_gpu_resource_)->backend_texture);
    memory_system_free(*texture_gpu_resource_, sizeof(texture_gpu_resource_t), MEMORY_TAG_RENDERER);
    *texture_gpu_resource_ = NULL;
}

// NOTE: アップロード済みのtextureのみbindを許可、未アップロード状態の場合はBAD_OPERATION
texture_gpu_resource_result_t texture_gpu_resource_bind(const renderer_backend_context_t* backend_context_, const texture_gpu_resource_t* texture_gpu_resource_) {
    texture_gpu_resource_result_t ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_gpu_resource_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_bind", "texture_gpu_resource_")

    if(!texture_gpu_resource_is_valid(texture_gpu_resource_)) {
        ret = TEXTURE_GPU_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_bind(%s) - provided texture_gpu_resource_ is corrupted.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_gpu_resource_is_uploaded(texture_gpu_resource_)) {
        ret = TEXTURE_GPU_RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_gpu_resource_bind(%s) - provided texture_gpu_resource_ is not uploaded.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_bind(backend_context_, texture_gpu_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_bind(%s) - renderer_backend_texture_bind failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = TEXTURE_GPU_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

texture_gpu_resource_result_t texture_gpu_resource_unbind(const renderer_backend_context_t* backend_context_, const texture_gpu_resource_t* texture_gpu_resource_) {
    texture_gpu_resource_result_t ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_gpu_resource_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_unbind", "texture_gpu_resource_")

    if(!texture_gpu_resource_is_valid(texture_gpu_resource_)) {
        ret = TEXTURE_GPU_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_unbind(%s) - provided texture_gpu_resource_ is corrupted.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, texture_gpu_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_unbind(%s) - renderer_backend_texture_unbind failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = TEXTURE_GPU_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

texture_gpu_resource_result_t texture_gpu_resource_upload(const renderer_backend_context_t* backend_context_, texture_gpu_resource_t* texture_gpu_resource_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    texture_gpu_resource_result_t ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    bool texture_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_upload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_gpu_resource_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_upload", "texture_gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT, texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT), "texture_gpu_resource_upload", "pixels_")

    if(!texture_gpu_resource_is_valid(texture_gpu_resource_)) {
        ret = TEXTURE_GPU_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - provided texture_gpu_resource_ is corrupted.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(texture_gpu_resource_->uploaded) {
        ret = TEXTURE_GPU_RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - provided texture_gpu_resource_ is already uploaded.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    if(3 != channel_count_ && 4 != channel_count_) {
        ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - provided channel_count_ is not valid.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == width_ || 0 == height_) {
        ret = TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - provided width_ or height_ is not valid.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_bind(backend_context_, texture_gpu_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - renderer_backend_texture_bind failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    texture_bound = true;

    ret_renderer_backend = renderer_backend_texture_pixel_upload(backend_context_, width_, height_, channel_count_, pixels_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - renderer_backend_texture_pixel_upload failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, texture_gpu_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_gpu_resource_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_upload(%s) - renderer_backend_texture_unbind failed.", texture_gpu_resource_rslt_to_str(ret));
        goto cleanup;
    }
    texture_bound = false;

    texture_gpu_resource_->uploaded = true;

    ret = TEXTURE_GPU_RESOURCE_SUCCESS;

cleanup:
    if(TEXTURE_GPU_RESOURCE_SUCCESS != ret && texture_bound) {
        ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, texture_gpu_resource_->backend_texture);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("texture_gpu_resource_upload(%s) - renderer_backend_texture_unbind failed.", texture_gpu_resource_rslt_to_str(TEXTURE_GPU_RESOURCE_DATA_CORRUPTED));
            ret = TEXTURE_GPU_RESOURCE_DATA_CORRUPTED;
        }
    }
    return ret;
}

// TODO: renderer_backend_texture_tのvalidator追加
bool texture_gpu_resource_is_valid(const texture_gpu_resource_t* texture_gpu_resource_) {
    if(NULL == texture_gpu_resource_) {
        return false;
    }
    if(NULL == texture_gpu_resource_->backend_texture) {
        return false;
    }
    return true;
}

bool texture_gpu_resource_is_uploaded(const texture_gpu_resource_t* texture_gpu_resource_) {
    if(!texture_gpu_resource_is_valid(texture_gpu_resource_)) {
        return false;
    }
    return texture_gpu_resource_->uploaded;
}
