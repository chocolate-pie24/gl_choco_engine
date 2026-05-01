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

texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    texture_manager_t* tmp_manager = NULL;
    texture_t** tmp_cpu_resources = NULL;
    renderer_backend_texture_t** tmp_gpu_resources;

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