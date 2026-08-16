/** @ingroup texture_system
 *
 * @file texture_manager.c
 * @author chocolate-pie24
 * @brief テクスチャリソース(CPU / GPU)管理システムモジュールAPI実装
 *
 * @version 0.1
 * @date 2026-05-18
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/texture_system/texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset
#include <stdalign.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/texture/texture.h"

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_texture.h"

/**
 * @brief テクスチャリソース(CPU / GPU)リソース管理システム構造体定義
 *
 */
struct texture_manager {
    int16_t max_texture_count;                      /**< システムで管理可能なテクスチャの最大値 */
    texture_cpu_resource_t** cpu_resources;         /**< CPU側テクスチャリソース配列 */
    renderer_backend_texture_t** gpu_resources;     /**< GPU側テクスチャリソース配列 */
};

static const char* const s_rslt_str_success = "SUCCESS";                        /**< 実行結果コード: TEXTURE_SYSTEM_SUCCESSの文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード: TEXTURE_SYSTEM_NO_MEMORYの文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード: TEXTURE_SYSTEM_RUNTIME_ERRORの文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード: TEXTURE_SYSTEM_INVALID_ARGUMENTの文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";          /**< 実行結果コード: TEXTURE_SYSTEM_DATA_CORRUPTEDの文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード: TEXTURE_SYSTEM_BAD_OPERATIONの文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                      /**< 実行結果コード: TEXTURE_SYSTEM_OVERFLOWの文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード: TEXTURE_SYSTEM_LIMIT_EXCEEDEDの文字列 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";        /**< 実行結果コード: TEXTURE_SYSTEM_FILE_OPEN_ERRORの文字列 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";      /**< 実行結果コード: TEXTURE_SYSTEM_FILE_CLOSE_ERRORの文字列 */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";        /**< 実行結果コード: TEXTURE_SYSTEM_FILE_READ_ERRORの文字列 */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";      /**< 実行結果コード: TEXTURE_SYSTEM_UNSUPPORTED_FILEの文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード: TEXTURE_SYSTEM_UNDEFINED_ERRORの文字列 */

static const char* tex_sys_rslt_to_str(texture_system_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_renderer_backend(renderer_backend_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_resource(resource_result_t rslt_);

texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    texture_manager_t* tmp_manager = NULL;
    texture_cpu_resource_t** tmp_cpu_resources = NULL;
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

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(texture_cpu_resource_t*) * (size_t)(max_texture_count_), alignof(texture_cpu_resource_t*), (void**)&tmp_cpu_resources);
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
        WARN_MESSAGE("texture_manager_deinitialize - Provided backend_context_ is not valid.");
        return;
    }
    if(NULL == texture_manager_) {
        WARN_MESSAGE("texture_manager_deinitialize - Provided texture_manager_ is not valid.");
        return;
    }
    if(0 >= texture_manager_->max_texture_count) {
        WARN_MESSAGE("texture_manager_deinitialize - Provided texture_manager_ is not initialized.");
        return;
    }
    if(NULL == texture_manager_->gpu_resources || NULL == texture_manager_->cpu_resources) {
        WARN_MESSAGE("texture_manager_deinitialize - Provided texture_manager_->gpu_resources or texture_manager_->cpu_resources is not initialized.");
        return;
    }
    for(int16_t i = 0; i != texture_manager_->max_texture_count; ++i) {
        // NOTE: 各destroy内でのWARNING出力抑制用にNULLチェックを行う
        if(NULL != texture_manager_->cpu_resources[i]) {
            texture_destroy(&texture_manager_->cpu_resources[i]);
        }
        if(NULL != texture_manager_->gpu_resources[i]) {
            renderer_backend_texture_destroy(backend_context_, &texture_manager_->gpu_resources[i]);
        }
    }
    texture_manager_->max_texture_count = 0;
    texture_manager_->cpu_resources = NULL;
    texture_manager_->gpu_resources = NULL;
}

texture_system_result_t texture_manager_register(renderer_backend_context_t* backend_context_, int32_t gpu_unit_num_, const char* texture_name_, texture_manager_t* texture_manager_, int16_t* out_texture_id_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    int16_t free_slot = INVALID_TEXTURE_ID;
    texture_cpu_resource_t* tmp_cpu_resource = NULL;
    renderer_backend_texture_t* tmp_gpu_resource = NULL;
    const uint8_t* texture_pixels = NULL;
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
    IF_ARG_FALSE_GOTO_CLEANUP(gpu_unit_num_ >= 0, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_register", "gpu_unit_num_")

    for(int16_t i = 0; i != texture_manager_->max_texture_count; ++i) {
        if(NULL == texture_manager_->cpu_resources[i] && NULL != texture_manager_->gpu_resources[i]) {
            ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
            ERROR_MESSAGE("texture_manager_register(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
            goto cleanup;
        } else if(NULL != texture_manager_->cpu_resources[i] && NULL == texture_manager_->gpu_resources[i]) {
            ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
            ERROR_MESSAGE("texture_manager_register(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
            goto cleanup;
        } else if(NULL == texture_manager_->cpu_resources[i] && NULL == texture_manager_->gpu_resources[i]) {
            if(INVALID_TEXTURE_ID == free_slot) {
                free_slot = i;
            }
        } else {
            const char* name = texture_name_get(texture_manager_->cpu_resources[i]);
            if(NULL == name) {
                // NOTE: cpu_resources[i] != NULL && gpu_resources[i] != NULLでname == NULLは破損
                ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
                ERROR_MESSAGE("texture_manager_register(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
                goto cleanup;
            } else if(choco_string_equal(texture_name_, name)) {
                ret = TEXTURE_SYSTEM_BAD_OPERATION;
                ERROR_MESSAGE("texture_manager_register(%s) - Provided texture name '%s' is already registered.", tex_sys_rslt_to_str(ret), texture_name_);
                goto cleanup;
            }
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
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to create texture cpu resource. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_renderer_backend = renderer_backend_texture_create(backend_context_, gpu_unit_num_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &tmp_gpu_resource);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = tex_sys_rslt_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to create texture gpu resource. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_load(tmp_cpu_resource, "assets/textures/", ".bmp");
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to load BMP file. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_get(tmp_cpu_resource, &texture_pixels);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to get texture pixels. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_size_get(tmp_cpu_resource, &width, &height, &channel_count);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to get pixel size. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_renderer_backend = renderer_backend_texture_bind(backend_context_, tmp_gpu_resource);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = tex_sys_rslt_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to bind texture. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_renderer_backend = renderer_backend_texture_pixel_upload(backend_context_, width, height, channel_count, texture_pixels);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = tex_sys_rslt_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to upload texture pixels. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        ret_resource = texture_pixel_unload(tmp_cpu_resource);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = tex_sys_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("texture_manager_register(%s) - Failed to unload texture pixels. texture name = '%s'.", tex_sys_rslt_to_str(ret), texture_name_);
            goto cleanup;
        }

        texture_manager_->cpu_resources[free_slot] = tmp_cpu_resource;
        texture_manager_->gpu_resources[free_slot] = tmp_gpu_resource;
        *out_texture_id_ = free_slot;
    }

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    if(TEXTURE_SYSTEM_SUCCESS != ret && NULL != backend_context_) {
        texture_destroy(&tmp_cpu_resource);
        renderer_backend_texture_destroy(backend_context_, &tmp_gpu_resource);
    }
    return ret;
}

texture_system_result_t texture_manager_unregister(renderer_backend_context_t* backend_context_, int16_t texture_id_, texture_manager_t* texture_manager_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister", "texture_manager_->gpu_resources")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ < texture_manager_->max_texture_count, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister", "texture_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ >= 0, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister", "texture_id_")

    if(NULL == texture_manager_->cpu_resources[texture_id_] && NULL != texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_manager_unregister(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    } else if(NULL != texture_manager_->cpu_resources[texture_id_] && NULL == texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_manager_unregister(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    } else if(NULL == texture_manager_->gpu_resources[texture_id_] || NULL == texture_manager_->cpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("texture_manager_unregister(%s) - Provided texture id '%d' is not registered.", tex_sys_rslt_to_str(ret), texture_id_);
        goto cleanup;
    }
    renderer_backend_texture_destroy(backend_context_, &texture_manager_->gpu_resources[texture_id_]);
    texture_destroy(&texture_manager_->cpu_resources[texture_id_]);

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

texture_system_result_t texture_manager_unregister_by_name(renderer_backend_context_t* backend_context_, const char* name_, texture_manager_t* texture_manager_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    int16_t id = INVALID_TEXTURE_ID;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister_by_name", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister_by_name", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister_by_name", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister_by_name", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_unregister_by_name", "texture_manager_->gpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_unregister_by_name", "name_")

    ret = texture_manager_texture_id_get(name_, texture_manager_, &id);
    if(TEXTURE_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("texture_manager_unregister_by_name(%s) - Failed to get texture id. texture name = '%s'.", tex_sys_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret = texture_manager_unregister(backend_context_, id, texture_manager_);
    if(TEXTURE_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("texture_manager_unregister_by_name(%s) - Failed to unregister texture. texture name = '%s'.", tex_sys_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

texture_system_result_t texture_manager_texture_id_get(const char* name_, const texture_manager_t* texture_manager_, int16_t* out_texture_id_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    int16_t ret_id = INVALID_TEXTURE_ID;
    bool found = false;

    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_texture_id_get", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_texture_id_get", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_texture_id_get", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_texture_id_get", "texture_manager_->gpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_texture_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_texture_id_get", "out_texture_id_")

    for(int16_t i = 0; i != texture_manager_->max_texture_count; ++i) {
        if(NULL == texture_manager_->cpu_resources[i] && NULL != texture_manager_->gpu_resources[i]) {
            ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
            ERROR_MESSAGE("texture_manager_texture_id_get(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
            goto cleanup;
        } else if(NULL != texture_manager_->cpu_resources[i] && NULL == texture_manager_->gpu_resources[i]) {
            ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
            ERROR_MESSAGE("texture_manager_texture_id_get(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
            goto cleanup;
        } else if(NULL != texture_manager_->cpu_resources[i]) {
            const char* tmp_texture_name = texture_name_get(texture_manager_->cpu_resources[i]);
            if(NULL == tmp_texture_name) {
                ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
                ERROR_MESSAGE("texture_manager_texture_id_get(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
                goto cleanup;
            } else if(choco_string_equal(name_, tmp_texture_name)) {
                ret_id = i;
                found = true;
                break;
            }
        }
    }
    if(!found) {
        // NOTE: カメラの存在確認に使用することも考慮し、ワーニング、エラーは出さない
        ret = TEXTURE_SYSTEM_BAD_OPERATION;
        goto cleanup;
    }

    *out_texture_id_ = ret_id;

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

texture_system_result_t texture_manager_gpu_resource_get(int16_t texture_id_, const texture_manager_t* texture_manager_, const renderer_backend_texture_t** out_gpu_resource_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;

    // NOTE: *out_gpu_resource_ != NULLのチェックはリソース再取得をする場合を考慮しチェック対象外とする
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get", "texture_manager_->gpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(out_gpu_resource_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "out_gpu_resource_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ < texture_manager_->max_texture_count, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_ >= 0, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get", "texture_id_")

    if(NULL == texture_manager_->cpu_resources[texture_id_] && NULL != texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_manager_gpu_resource_get(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    } else if(NULL != texture_manager_->cpu_resources[texture_id_] && NULL == texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_manager_gpu_resource_get(%s) - Texture manager data corrupted.", tex_sys_rslt_to_str(ret));
        goto cleanup;
    } else if(NULL == texture_manager_->gpu_resources[texture_id_]) {
        ret = TEXTURE_SYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("texture_manager_gpu_resource_get(%s) - Provided texture id '%d' not found.", tex_sys_rslt_to_str(ret), texture_id_);
        goto cleanup;
    }
    *out_gpu_resource_ = texture_manager_->gpu_resources[texture_id_];

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

texture_system_result_t texture_manager_gpu_resource_get_by_name(const char* name_, const texture_manager_t* texture_manager_, const renderer_backend_texture_t** out_gpu_resource_) {
    texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    int16_t id = INVALID_TEXTURE_ID;

    // NOTE: *out_gpu_resource_ != NULLのチェックはリソース再取得をする場合を考慮しチェック対象外とする
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get_by_name", "texture_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_manager_->max_texture_count > 0, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get_by_name", "texture_manager_->max_texture_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->cpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get_by_name", "texture_manager_->cpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(texture_manager_->gpu_resources, ret, TEXTURE_SYSTEM_BAD_OPERATION, tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION), "texture_manager_gpu_resource_get_by_name", "texture_manager_->gpu_resources")
    IF_ARG_NULL_GOTO_CLEANUP(out_gpu_resource_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get_by_name", "out_gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, TEXTURE_SYSTEM_INVALID_ARGUMENT, tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT), "texture_manager_gpu_resource_get_by_name", "name_")

    ret = texture_manager_texture_id_get(name_, texture_manager_, &id);
    if(TEXTURE_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("texture_manager_gpu_resource_get_by_name(%s) - Failed to get texture id. texture name = '%s'.", tex_sys_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret = texture_manager_gpu_resource_get(id, texture_manager_, out_gpu_resource_);
    if(TEXTURE_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("texture_manager_gpu_resource_get_by_name(%s) - Failed to get texture gpu resource. texture name = '%s'.", tex_sys_rslt_to_str(ret), name_);
        goto cleanup;
    }

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
    case TEXTURE_SYSTEM_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
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
    default:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    }
}

static texture_system_result_t tex_sys_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return TEXTURE_SYSTEM_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return TEXTURE_SYSTEM_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return TEXTURE_SYSTEM_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return TEXTURE_SYSTEM_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return TEXTURE_SYSTEM_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return TEXTURE_SYSTEM_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return TEXTURE_SYSTEM_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return TEXTURE_SYSTEM_OVERFLOW;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
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
    case RESOURCE_FILE_CLOSE_ERROR:
        return TEXTURE_SYSTEM_FILE_CLOSE_ERROR;
    case RESOURCE_UNSUPPORTED_FILE:
        return TEXTURE_SYSTEM_UNSUPPORTED_FILE;
    case RESOURCE_UNDEFINED_ERROR:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    default:
        return TEXTURE_SYSTEM_UNDEFINED_ERROR;
    }
}
