#include "engine/systems/texture_system/texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset
#include <stdalign.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"
#include "engine/resource/texture/texture.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_texture.h"

/**
 * @brief カメラ管理システム内部状態管理構造体
 *
 */
struct texture_manager {
    int16_t max_texture_count;
    texture_t** cpu_resources;
    renderer_backend_texture_t** gpu_resources;
};

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

static const char* tex_sys_rslt_to_str(texture_system_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_renderer(renderer_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_resource(resource_result_t rslt_);

texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    texture_manager_t* tmp_manager = NULL;
    texture_t** tmp_cpu_resources = NULL;
    renderer_backend_texture_t** tmp_gpu_resources = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_initialize", "out_texture_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_initialize", "*out_texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < max_texture_count_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_initialize", "max_texture_count_")

    // Simulation.
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(texture_manager_t), alignof(texture_manager_t), (void**)&tmp_manager);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = tex_sys_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("texture_manager_initialize(%s) - Failed to allocate memory for texture manager.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_manager, 0, sizeof(texture_manager_t));
    tmp_manager->max_texture_count = max_texture_count_;

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(texture_t*) * (size_t)(max_texture_count_), alignof(texture_t*), (void**)&tmp_cpu_resources);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = tex_sys_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("texture_manager_initialize(%s) - Failed to allocate memory for cpu resources.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(renderer_backend_texture_t*) * (size_t)(max_texture_count_), alignof(renderer_backend_texture_t*), (void**)&tmp_gpu_resources);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = tex_sys_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("texture_manager_initialize(%s) - Failed to allocate memory for gpu resources.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_manager->cpu_resources = tmp_cpu_resources;
    tmp_manager->gpu_resources = tmp_gpu_resources;
    for(int16_t i = 0; i != max_texture_count_; ++i) {
        tmp_manager->cpu_resources[i] = NULL;
        tmp_manager->gpu_resources[i] = NULL;
    }

    // commit.
    *out_texture_manager_ = tmp_manager;

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void texture_manager_deinitialize(renderer_backend_context_t* backend_context_, texture_manager_t* texture_manager_) {
    if(NULL == backend_context_) {
        return;
    }
    if(NULL == texture_manager_) {
        return;
    }
    if(0 == texture_manager_->max_texture_count) {
        return;
    }
    for(int16_t i = 0; i != texture_manager_->max_texture_count; ++i) {
        texture_destroy(&texture_manager_->cpu_resources[i]);
        renderer_backend_texture_destroy(backend_context_, &texture_manager_->gpu_resources[i]);
    }
    texture_manager_->max_texture_count = 0;
}

// TODO: test_textureに対応させる
texture_system_result_t texture_manager_register(renderer_backend_context_t* backend_context_, int32_t gpu_unit_num_, const char* texture_name_, texture_manager_t* texture_manager_, int16_t* out_texture_id_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    int16_t free_slot = INVALID_TEXTURE_ID;
    texture_t* tmp_cpu_resource = NULL;
    renderer_backend_texture_t* tmp_gpu_resource = NULL;
    uint8_t* texture_pixels = NULL;
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_register", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_name_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_register", "texture_name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_register", "texture_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_register", "out_texture_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_register", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_register", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_register", "texture_manager_->gpu_resources")

    for(int16_t i = 0; i != texture_manager_->max_texture_count; ++i) {
        if(NULL == texture_manager_->cpu_resources[i] && NULL == texture_manager_->gpu_resources[i]) {
            if(INVALID_TEXTURE_ID == free_slot) {
                free_slot = i;
            }
        } else if(choco_string_equal(texture_name_, texture_name_get(texture_manager_->cpu_resources[i]))) {
            ret = TEXTURE_SYSTEM_BAD_OPERATION;
            ERROR_MESSAGE("texture_manager_register(%s) - Provided texture name '%s' is already registered.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }
    }
    if(INVALID_TEXTURE_ID == free_slot) {
        ret = TEXTURE_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("texture_manager_register(%s) - Texture manager has no free slot.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    } else {
        ret_resource = texture_create(texture_name_, &tmp_cpu_resource);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to create texture cpu resource. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_renderer = renderer_backend_texture_create(backend_context_, gpu_unit_num_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &tmp_gpu_resource);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = tex_sys_rslt_convert_renderer(ret_renderer);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to create texture gpu resource. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_load(tmp_cpu_resource, "assets/textures/", ".bmp");
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to load BMP file. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_get(tmp_cpu_resource, &texture_pixels);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to get texture pixels. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_size_get(tmp_cpu_resource, &width, &height, &channel_count);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to get pixel size. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_renderer = renderer_backend_texture_pixel_upload(backend_context_, tmp_gpu_resource, width, height, channel_count, texture_pixels);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = tex_sys_rslt_convert_resource(ret_renderer);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to upload texture pixels. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_unload(tmp_cpu_resource);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to unload texture pixels. texture name = '%s'.", ret, texture_name_);
            goto cleanup;
        }

        texture_manager_->cpu_resources[free_slot] = tmp_cpu_resource;
        texture_manager_->gpu_resources[free_slot] = tmp_gpu_resource;
        *out_texture_id_ = free_slot;
    }

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    if(TEXTURE_SYSTEM_SUCCESS != ret) {
        texture_destroy(&tmp_cpu_resource);
        renderer_backend_texture_destroy(backend_context_, &tmp_gpu_resource);
    }
    return ret;
}

texture_system_result_t texture_manager_gpu_resource_get(int16_t texture_id_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->gpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(out_gpu_resource_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "out_gpu_resource_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ < texture_manager_->max_texture_count, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ >= 0, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_id_")

    if(NULL == texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("texture_manager_gpu_resource_get(%s) - Provided texture id '%d' not found.", tex_sys_rslt_to_str(ret), texture_id_);
        goto cleanup;
    }
    *out_gpu_resource_ = texture_manager_->gpu_resources[texture_id_];

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static const char* tex_sys_rslt_to_str(texture_system_result_t rslt_) {
    switch(rslt_) {
    case TEXTURE_SYSTEM_SUCCESS:
        return s_rslt_str_success;
    case TEXTURE_SYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case TEXTURE_SYSTEM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case TEXTURE_SYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case TEXTURE_SYSTEM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case TEXTURE_SYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case TEXTURE_SYSTEM_OVERFLOW:
        return s_rslt_str_overflow;
    case TEXTURE_SYSTEM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case TEXTURE_SYSTEM_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case TEXTURE_SYSTEM_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case TEXTURE_SYSTEM_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case TEXTURE_SYSTEM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static texture_system_result_t tex_sys_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return TEXTURE_SYSTEM_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return TEXTURE_SYSTEM_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return TEXTURE_SYSTEM_INVALID_ARGUMENT;
    }
}

static texture_system_result_t tex_sys_rslt_convert_renderer(renderer_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_SUCCESS:
        return TEXTURE_SYSTEM_SUCCESS;
    case RENDERER_INVALID_ARGUMENT:
        return TEXTURE_SYSTEM_INVALID_ARGUMENT;
    case RENDERER_RUNTIME_ERROR:
        return TEXTURE_SYSTEM_RUNTIME_ERROR;
    case RENDERER_NO_MEMORY:
        return TEXTURE_SYSTEM_NO_MEMORY;
    case RENDERER_SHADER_COMPILE_ERROR:
        return TEXTURE_SYSTEM_RUNTIME_ERROR;
    case RENDERER_SHADER_LINK_ERROR:
        return TEXTURE_SYSTEM_RUNTIME_ERROR;
    case RENDERER_LIMIT_EXCEEDED:
        return TEXTURE_SYSTEM_LIMIT_EXCEEDED;
    case RENDERER_BAD_OPERATION:
        return TEXTURE_SYSTEM_BAD_OPERATION;
    case RENDERER_DATA_CORRUPTED:
        return TEXTURE_SYSTEM_DATA_CORRUPTED;
    case RENDERER_UNDEFINED_ERROR:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    default:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    }
}

static texture_system_result_t tex_sys_rslt_convert_resource(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return TEXTURE_SYSTEM_SUCCESS;
    case RESOURCE_NO_MEMORY:
        return TEXTURE_SYSTEM_NO_MEMORY;
    case RESOURCE_RUNTIME_ERROR:
        return TEXTURE_SYSTEM_RUNTIME_ERROR;
    case RESOURCE_INVALID_ARGUMENT:
        return TEXTURE_SYSTEM_INVALID_ARGUMENT;
    case RESOURCE_DATA_CORRUPTED:
        return TEXTURE_SYSTEM_DATA_CORRUPTED;
    case RESOURCE_BAD_OPERATION:
        return TEXTURE_SYSTEM_BAD_OPERATION;
    case RESOURCE_OVERFLOW:
        return TEXTURE_SYSTEM_OVERFLOW;
    case RESOURCE_LIMIT_EXCEEDED:
        return TEXTURE_SYSTEM_LIMIT_EXCEEDED;
    case RESOURCE_FILE_OPEN_ERROR:
        return TEXTURE_SYSTEM_FILE_OPEN_ERROR;
    case RESOURCE_FILE_READ_ERROR:
        return TEXTURE_SYSTEM_FILE_READ_ERROR;
    case RESOURCE_UNSUPPORTED_FILE:
        return TEXTURE_SYSTEM_UNSUPPORTED_FILE;
    case RESOURCE_UNDEFINED_ERROR:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    default:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    }
}
