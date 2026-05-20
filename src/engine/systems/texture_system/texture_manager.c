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

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"
#include "engine/resource/texture/texture.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_texture.h"

/**
 * @brief テクスチャリソース(CPU / GPU)リソース管理システム構造体定義
 *
 */
struct texture_manager {
    int16_t max_texture_count;                      /**< システムで管理可能なテクスチャの最大値 */
    texture_t** cpu_resources;                      /**< CPU側テクスチャリソース配列 */
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
static texture_system_result_t tex_sys_rslt_convert_renderer(renderer_result_t rslt_);
static texture_system_result_t tex_sys_rslt_convert_resource(resource_result_t rslt_);

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "test_controller.h"

#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/memory/test_linear_allocator.h"

#include "engine/systems/texture_system/test_texture_manager.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/test_context_texture.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/test_renderer_backend_context.h"

#include "engine/resource/texture/test_texture.h"

// texture_manager用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_texture_manager_initialize;               /**< texture_manager_initialize()テスト設定 */
static test_call_control_t s_test_config_texture_manager_register;                 /**< texture_manager_register()テスト設定 */
static test_call_control_t s_test_config_texture_manager_unregister;               /**< texture_manager_unregister()テスト設定 */
static test_call_control_t s_test_config_texture_manager_unregister_by_name;       /**< texture_manager_unregister_by_name()テスト設定 */
static test_call_control_t s_test_config_texture_manager_texture_id_get;           /**< texture_manager_texture_id_get()テスト設定 */
static test_call_control_t s_test_config_texture_manager_gpu_resource_get;         /**< texture_manager_gpu_resource_get()テスト設定 */
static test_call_control_t s_test_config_texture_manager_gpu_resource_get_by_name; /**< texture_manager_gpu_resource_get_by_name()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_texture_manager_initialize(void);
static void test_texture_manager_deinitialize(void);
static void test_texture_manager_register(void);
static void test_texture_manager_unregister(void);
static void test_texture_manager_unregister_by_name(void);
static void test_texture_manager_texture_id_get(void);
static void test_texture_manager_gpu_resource_get(void);
static void test_texture_manager_gpu_resource_get_by_name(void);
static void test_tex_sys_rslt_to_str(void);
static void test_tex_sys_rslt_convert_linear_alloc(void);
static void test_tex_sys_rslt_convert_renderer(void);
static void test_tex_sys_rslt_convert_resource(void);
#endif

texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_) {
#ifdef TEST_BUILD
    s_test_config_texture_manager_initialize.call_count++;
    if(s_test_config_texture_manager_initialize.fail_on_call != 0) {
        if(s_test_config_texture_manager_initialize.call_count == s_test_config_texture_manager_initialize.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_initialize.forced_result;
        }
    }
#endif
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
        // NOTE: texture_destroy, renderer_backend_texture_destroyはNULLを渡されたら何もしないのでチェック不要
        texture_destroy(&texture_manager_->cpu_resources[i]);
        renderer_backend_texture_destroy(backend_context_, &texture_manager_->gpu_resources[i]);
    }
    texture_manager_->max_texture_count = 0;
    texture_manager_->cpu_resources = NULL;
    texture_manager_->gpu_resources = NULL;
}

texture_system_result_t texture_manager_register(renderer_backend_context_t* backend_context_, int32_t gpu_unit_num_, const char* texture_name_, texture_manager_t* texture_manager_, int16_t* out_texture_id_) {
#ifdef TEST_BUILD
    s_test_config_texture_manager_register.call_count++;
    if(s_test_config_texture_manager_register.fail_on_call != 0) {
        if(s_test_config_texture_manager_register.call_count == s_test_config_texture_manager_register.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_register.forced_result;
        }
    }
#endif
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

        ret_renderer = renderer_backend_texture_create(backend_context_, gpu_unit_num_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &tmp_gpu_resource);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = tex_sys_rslt_convert_renderer(ret_renderer);
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

        ret_renderer = renderer_backend_texture_pixel_upload(backend_context_, tmp_gpu_resource, width, height, channel_count, texture_pixels);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = tex_sys_rslt_convert_renderer(ret_renderer);
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
#ifdef TEST_BUILD
    s_test_config_texture_manager_unregister.call_count++;
    if(s_test_config_texture_manager_unregister.fail_on_call != 0) {
        if(s_test_config_texture_manager_unregister.call_count == s_test_config_texture_manager_unregister.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_unregister.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_texture_manager_unregister_by_name.call_count++;
    if(s_test_config_texture_manager_unregister_by_name.fail_on_call != 0) {
        if(s_test_config_texture_manager_unregister_by_name.call_count == s_test_config_texture_manager_unregister_by_name.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_unregister_by_name.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_texture_manager_texture_id_get.call_count++;
    if(s_test_config_texture_manager_texture_id_get.fail_on_call != 0) {
        if(s_test_config_texture_manager_texture_id_get.call_count == s_test_config_texture_manager_texture_id_get.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_texture_id_get.forced_result;
        }
    }
#endif
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
            const char* name = texture_name_get(texture_manager_->cpu_resources[i]);
            if(NULL != name && choco_string_equal(name_, name)) {
                ret_id = i;
                found = true;
                break;
            }
        }
    }
    if(!found) {
        ret = TEXTURE_SYSTEM_BAD_OPERATION;
        WARN_MESSAGE("texture_manager_texture_id_get(%s) - Provided texture name '%s' not found.", tex_sys_rslt_to_str(ret), name_);
        goto cleanup;
    }

    *out_texture_id_ = ret_id;

    ret = TEXTURE_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

texture_system_result_t texture_manager_gpu_resource_get(int16_t texture_id_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_) {
#ifdef TEST_BUILD
    s_test_config_texture_manager_gpu_resource_get.call_count++;
    if(s_test_config_texture_manager_gpu_resource_get.fail_on_call != 0) {
        if(s_test_config_texture_manager_gpu_resource_get.call_count == s_test_config_texture_manager_gpu_resource_get.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_gpu_resource_get.forced_result;
        }
    }
#endif
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

texture_system_result_t texture_manager_gpu_resource_get_by_name(const char* name_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_) {
#ifdef TEST_BUILD
    s_test_config_texture_manager_gpu_resource_get_by_name.call_count++;
    if(s_test_config_texture_manager_gpu_resource_get_by_name.fail_on_call != 0) {
        if(s_test_config_texture_manager_gpu_resource_get_by_name.call_count == s_test_config_texture_manager_gpu_resource_get_by_name.fail_on_call) {
            return (texture_system_result_t)s_test_config_texture_manager_gpu_resource_get_by_name.forced_result;
        }
    }
#endif
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

#ifdef TEST_BUILD
void NO_COVERAGE test_texture_manager_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_register_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_register.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_register.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_unregister_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_unregister.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_unregister.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_unregister_by_name_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_unregister_by_name.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_unregister_by_name.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_texture_id_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_texture_id_get.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_texture_id_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_gpu_resource_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_gpu_resource_get.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_gpu_resource_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_gpu_resource_get_by_name_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_manager_gpu_resource_get_by_name.fail_on_call = config_->fail_on_call;
    s_test_config_texture_manager_gpu_resource_get_by_name.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_manager_config_reset(void) {
    test_call_control_reset(&s_test_config_texture_manager_initialize);
    test_call_control_reset(&s_test_config_texture_manager_register);
    test_call_control_reset(&s_test_config_texture_manager_unregister);
    test_call_control_reset(&s_test_config_texture_manager_unregister_by_name);
    test_call_control_reset(&s_test_config_texture_manager_texture_id_get);
    test_call_control_reset(&s_test_config_texture_manager_gpu_resource_get);
    test_call_control_reset(&s_test_config_texture_manager_gpu_resource_get_by_name);
}

void NO_COVERAGE test_texture_manager(void) {
    test_texture_manager_initialize();
    test_texture_manager_deinitialize();
    test_texture_manager_register();
    test_texture_manager_unregister();
    test_texture_manager_unregister_by_name();
    test_texture_manager_texture_id_get();
    test_texture_manager_gpu_resource_get();
    test_texture_manager_gpu_resource_get_by_name();
    test_tex_sys_rslt_to_str();
    test_tex_sys_rslt_convert_linear_alloc();
    test_tex_sys_rslt_convert_renderer();
    test_tex_sys_rslt_convert_resource();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_initialize(void) {
    {
        // texture_manager_initialize() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        linear_alloc_t* allocator = (linear_alloc_t*)(uintptr_t)0x1U;
        texture_manager_t* manager = NULL;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_initialize_config_set(&config);

        ret = texture_manager_initialize(
            2,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(NULL == manager);

        test_texture_manager_config_reset();
    }
    {
        // allocator_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t* manager = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_initialize(
            2,
            NULL,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == manager);

        test_texture_manager_config_reset();
    }
    {
        // out_texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        linear_alloc_t* allocator = (linear_alloc_t*)(uintptr_t)0x1U;

        test_texture_manager_config_reset();

        ret = texture_manager_initialize(
            2,
            allocator,
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);

        test_texture_manager_config_reset();
    }
    {
        // *out_texture_manager_ != NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        linear_alloc_t* allocator = (linear_alloc_t*)(uintptr_t)0x1U;
        texture_manager_t* manager = (texture_manager_t*)(uintptr_t)0x1234U;

        test_texture_manager_config_reset();

        ret = texture_manager_initialize(
            2,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((texture_manager_t*)(uintptr_t)0x1234U == manager);

        test_texture_manager_config_reset();
    }
    {
        // max_texture_count_ == 0 -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        linear_alloc_t* allocator = (linear_alloc_t*)(uintptr_t)0x1U;
        texture_manager_t* manager = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_initialize(
            0,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == manager);

        test_texture_manager_config_reset();
    }
    {
        // max_texture_count_ < 0 -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        linear_alloc_t* allocator = (linear_alloc_t*)(uintptr_t)0x1U;
        texture_manager_t* manager = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_initialize(
            -1,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == manager);

        test_texture_manager_config_reset();
    }
    {
        // 1回目の linear_allocator_allocate() が失敗
        // texture_manager_t 本体の確保失敗 -> TEXTURE_SYSTEM_NO_MEMORY
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;
        texture_manager_t* manager = NULL;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        linear_allocator_preinit(&allocator_memory_requirement, &allocator_align_requirement);

        ret_memory = memory_system_allocate(
            allocator_memory_requirement,
            MEMORY_TAG_SYSTEM,
            (void**)&allocator
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator);

        ret_memory = memory_system_allocate(
            allocator_pool_size,
            MEMORY_TAG_SYSTEM,
            &allocator_pool
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator_pool);

        ret_linear = linear_allocator_init(
            allocator,
            allocator_pool_size,
            allocator_pool
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_allocate_config_set(&config);

        ret = texture_manager_initialize(
            2,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_NO_MEMORY == ret);
        assert(NULL == manager);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 2回目の linear_allocator_allocate() が失敗
        // cpu_resources 配列の確保失敗 -> TEXTURE_SYSTEM_NO_MEMORY
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;
        texture_manager_t* manager = NULL;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        linear_allocator_preinit(&allocator_memory_requirement, &allocator_align_requirement);

        ret_memory = memory_system_allocate(
            allocator_memory_requirement,
            MEMORY_TAG_SYSTEM,
            (void**)&allocator
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator);

        ret_memory = memory_system_allocate(
            allocator_pool_size,
            MEMORY_TAG_SYSTEM,
            &allocator_pool
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator_pool);

        ret_linear = linear_allocator_init(
            allocator,
            allocator_pool_size,
            allocator_pool
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_call_control_reset(&config);
        config.fail_on_call = 2U;
        config.forced_result = (int)LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_allocate_config_set(&config);

        ret = texture_manager_initialize(
            2,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_NO_MEMORY == ret);
        assert(NULL == manager);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 3回目の linear_allocator_allocate() が失敗
        // gpu_resources 配列の確保失敗 -> TEXTURE_SYSTEM_NO_MEMORY
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;
        texture_manager_t* manager = NULL;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        linear_allocator_preinit(&allocator_memory_requirement, &allocator_align_requirement);

        ret_memory = memory_system_allocate(
            allocator_memory_requirement,
            MEMORY_TAG_SYSTEM,
            (void**)&allocator
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator);

        ret_memory = memory_system_allocate(
            allocator_pool_size,
            MEMORY_TAG_SYSTEM,
            &allocator_pool
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator_pool);

        ret_linear = linear_allocator_init(
            allocator,
            allocator_pool_size,
            allocator_pool
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_call_control_reset(&config);
        config.fail_on_call = 3U;
        config.forced_result = (int)LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_allocate_config_set(&config);

        ret = texture_manager_initialize(
            2,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_NO_MEMORY == ret);
        assert(NULL == manager);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: manager / cpu_resources / gpu_resources を確保し、全スロットをNULL初期化する
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;
        texture_manager_t* manager = NULL;

        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        linear_allocator_preinit(&allocator_memory_requirement, &allocator_align_requirement);

        ret_memory = memory_system_allocate(
            allocator_memory_requirement,
            MEMORY_TAG_SYSTEM,
            (void**)&allocator
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator);

        ret_memory = memory_system_allocate(
            allocator_pool_size,
            MEMORY_TAG_SYSTEM,
            &allocator_pool
        );
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);
        assert(NULL != allocator_pool);

        ret_linear = linear_allocator_init(
            allocator,
            allocator_pool_size,
            allocator_pool
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        ret = texture_manager_initialize(
            3,
            allocator,
            &manager
        );

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert(NULL != manager);
        assert(3 == manager->max_texture_count);
        assert(NULL != manager->cpu_resources);
        assert(NULL != manager->gpu_resources);

        assert(NULL == manager->cpu_resources[0]);
        assert(NULL == manager->cpu_resources[1]);
        assert(NULL == manager->cpu_resources[2]);

        assert(NULL == manager->gpu_resources[0]);
        assert(NULL == manager->gpu_resources[1]);
        assert(NULL == manager->gpu_resources[2]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_deinitialize(void) {
    {
        // backend_context_ == NULL -> no-op
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        texture_manager_deinitialize(NULL, &manager);

        assert(2 == manager.max_texture_count);
        assert(cpu_resources == manager.cpu_resources);
        assert(gpu_resources == manager.gpu_resources);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> no-op
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;

        test_texture_manager_config_reset();

        texture_manager_deinitialize(backend_context, NULL);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> no-op
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        texture_manager_deinitialize(backend_context, &manager);

        assert(0 == manager.max_texture_count);
        assert(cpu_resources == manager.cpu_resources);
        assert(gpu_resources == manager.gpu_resources);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> no-op
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        texture_manager_deinitialize(backend_context, &manager);

        assert(2 == manager.max_texture_count);
        assert(NULL == manager.cpu_resources);
        assert(gpu_resources == manager.gpu_resources);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> no-op
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        texture_manager_deinitialize(backend_context, &manager);

        assert(2 == manager.max_texture_count);
        assert(cpu_resources == manager.cpu_resources);
        assert(NULL == manager.gpu_resources);

        test_texture_manager_config_reset();
    }
    {
        // 正常系: 空のmanagerを未初期化状態へ戻す
        // NOTE: renderer_backend_context_t は opaque なので renderer_backend_initialize() で生成する
        texture_system_result_t ret_tex_sys = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;

        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[4096U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;

        ret_linear = linear_allocator_init(
            allocator,
            sizeof(pool_storage),
            pool_storage
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_texture_manager_config_reset();

        ret_renderer = renderer_backend_initialize(
            allocator,
            GRAPHICS_API_GL33,
            &backend_context
        );
        assert(RENDERER_SUCCESS == ret_renderer);
        assert(NULL != backend_context);

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        texture_manager_deinitialize(backend_context, &manager);

        assert(0 == manager.max_texture_count);
        assert(NULL == manager.cpu_resources);
        assert(NULL == manager.gpu_resources);

        assert(NULL == cpu_resources[0]);
        assert(NULL == cpu_resources[1]);
        assert(NULL == cpu_resources[2]);

        assert(NULL == gpu_resources[0]);
        assert(NULL == gpu_resources[1]);
        assert(NULL == gpu_resources[2]);

        renderer_backend_destroy(backend_context);

        (void)ret_tex_sys;

        test_texture_manager_config_reset();
    }
    {
        // 正常系: CPU resourceを含むmanagerを破棄し、未初期化状態へ戻す
        // NOTE: GPU resource は NULL のため、有効な backend_context でも renderer_backend_texture_destroy() は no-op
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;

        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[4096U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;

        ret_linear = linear_allocator_init(
            allocator,
            sizeof(pool_storage),
            pool_storage
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_renderer = renderer_backend_initialize(
            allocator,
            GRAPHICS_API_GL33,
            &backend_context
        );
        assert(RENDERER_SUCCESS == ret_renderer);
        assert(NULL != backend_context);

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        texture_manager_deinitialize(backend_context, &manager);

        assert(0 == manager.max_texture_count);
        assert(NULL == manager.cpu_resources);
        assert(NULL == manager.gpu_resources);

        assert(NULL == cpu_resources[0]);
        assert(NULL == cpu_resources[1]);

        assert(NULL == gpu_resources[0]);
        assert(NULL == gpu_resources[1]);

        renderer_backend_destroy(backend_context);
        memory_system_destroy();

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_register(void) {
#define TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT()                                      \
    do {                                                                                  \
        ret_memory = memory_system_create();                                              \
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);                                      \
                                                                                          \
        linear_allocator_preinit(&allocator_memory_requirement, &allocator_align_requirement); \
                                                                                          \
        ret_memory = memory_system_allocate(                                              \
            allocator_memory_requirement,                                                  \
            MEMORY_TAG_SYSTEM,                                                             \
            (void**)&allocator                                                             \
        );                                                                                 \
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);                                       \
        assert(NULL != allocator);                                                         \
                                                                                           \
        ret_memory = memory_system_allocate(                                               \
            allocator_pool_size,                                                           \
            MEMORY_TAG_SYSTEM,                                                             \
            &allocator_pool                                                                \
        );                                                                                 \
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);                                       \
        assert(NULL != allocator_pool);                                                     \
                                                                                           \
        ret_linear = linear_allocator_init(                                                \
            allocator,                                                                     \
            allocator_pool_size,                                                           \
            allocator_pool                                                                 \
        );                                                                                 \
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);                                        \
                                                                                           \
        ret_renderer = renderer_backend_initialize(                                        \
            allocator,                                                                     \
            GRAPHICS_API_GL33,                                                             \
            &backend_context                                                               \
        );                                                                                 \
        assert(RENDERER_SUCCESS == ret_renderer);                                          \
        assert(NULL != backend_context);                                                    \
    } while(false)

#define TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT()                                   \
    do {                                                                                  \
        if(NULL != backend_context) {                                                     \
            renderer_backend_destroy(backend_context);                                    \
            backend_context = NULL;                                                       \
        }                                                                                 \
        memory_system_destroy();                                                          \
        test_texture_manager_config_reset();                                              \
        test_texture_config_reset();                                                      \
        test_renderer_backend_context_config_reset();                                     \
        test_linear_allocator_config_reset();                                             \
        test_choco_memory_config_reset();                                                 \
    } while(false)

    {
        // texture_manager_register() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_register_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(123 == texture_id);
        assert(1U == s_test_config_texture_manager_register.call_count);

        test_texture_manager_config_reset();
    }
    {
        // backend_context_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_register(
            NULL,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);
        assert(1U == s_test_config_texture_manager_register.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_name_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            NULL,
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            NULL,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // out_texture_id_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // cpu_resources[i] == NULL && gpu_resources[i] != NULL -> TEXTURE_SYSTEM_DATA_CORRUPTED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1U == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // cpu_resources[i] != NULL && gpu_resources[i] == NULL -> TEXTURE_SYSTEM_DATA_CORRUPTED
        // NOTE: この分岐では texture_name_get() に到達しないため、CPU resource はダミーでよい
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        cpu_resources[0] = (texture_t*)(uintptr_t)0x1U;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(123 == texture_id);
        assert((texture_t*)(uintptr_t)0x1U == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // 登録済みの同名 texture が存在する -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);
        assert(NULL != cpu_resources[0]);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1U == gpu_resources[0]);

        texture_destroy(&cpu_resources[0]);
        assert(NULL == cpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // 空きスロットがない -> TEXTURE_SYSTEM_LIMIT_EXCEEDED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        ret_resource = texture_create("test_texture_blue", &cpu_resources[1]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[1]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;
        gpu_resources[1] = (renderer_backend_texture_t*)(uintptr_t)0x2U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_green",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_LIMIT_EXCEEDED == ret);
        assert(123 == texture_id);

        texture_destroy(&cpu_resources[0]);
        texture_destroy(&cpu_resources[1]);
        assert(NULL == cpu_resources[0]);
        assert(NULL == cpu_resources[1]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_create() が RESOURCE_NO_MEMORY を返す -> TEXTURE_SYSTEM_NO_MEMORY
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_NO_MEMORY;
        test_texture_create_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_NO_MEMORY == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // renderer_backend_texture_create() が RENDERER_NO_MEMORY を返す -> TEXTURE_SYSTEM_NO_MEMORY
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_NO_MEMORY;
        test_renderer_backend_texture_create_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_NO_MEMORY == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_pixel_load() が RESOURCE_FILE_OPEN_ERROR を返す -> TEXTURE_SYSTEM_FILE_OPEN_ERROR
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_create_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_FILE_OPEN_ERROR;
        test_texture_pixel_load_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_FILE_OPEN_ERROR == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_pixel_get() が RESOURCE_RUNTIME_ERROR を返す -> TEXTURE_SYSTEM_RUNTIME_ERROR
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_create_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_RUNTIME_ERROR;
        test_texture_pixel_get_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_pixel_size_get() が RESOURCE_DATA_CORRUPTED を返す -> TEXTURE_SYSTEM_DATA_CORRUPTED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_create_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_DATA_CORRUPTED;
        test_texture_pixel_size_get_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // renderer_backend_texture_pixel_upload() が RENDERER_RUNTIME_ERROR を返す
        // -> TEXTURE_SYSTEM_RUNTIME_ERROR
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_create_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_RUNTIME_ERROR;
        test_renderer_backend_texture_pixel_upload_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // texture_pixel_unload() が RESOURCE_FILE_CLOSE_ERROR を返す -> TEXTURE_SYSTEM_FILE_CLOSE_ERROR
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_create_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_SUCCESS;
        test_renderer_backend_texture_pixel_upload_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_FILE_CLOSE_ERROR;
        test_texture_pixel_unload_config_set(&config);

        ret = texture_manager_register(
            backend_context,
            0,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_FILE_CLOSE_ERROR == ret);
        assert(123 == texture_id);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }
    {
        // gpu_unit_num_ < 0 -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        renderer_result_t ret_renderer = RENDERER_UNDEFINED_ERROR;

        renderer_backend_context_t* backend_context = NULL;
        linear_alloc_t* allocator = NULL;
        void* allocator_pool = NULL;
        size_t allocator_memory_requirement = 0U;
        size_t allocator_align_requirement = 0U;
        size_t allocator_pool_size = 4096U;

        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_renderer_backend_context_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT();

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_register(
            backend_context,
            -1,
            "test_texture_red",
            &manager,
            &texture_id
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);

        assert(NULL == cpu_resources[0]);
        assert(NULL == cpu_resources[1]);
        assert(NULL == gpu_resources[0]);
        assert(NULL == gpu_resources[1]);

        assert(1U == s_test_config_texture_manager_register.call_count);

        TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT();
    }

#undef TEST_TEXTURE_MANAGER_SETUP_BACKEND_CONTEXT
#undef TEST_TEXTURE_MANAGER_TEARDOWN_BACKEND_CONTEXT
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_unregister(void) {
    // NOTE: 現状ではrenderer_backend_texture_destroyをNo-opにできないため、成功経路を踏めない, 成功経路は実際の実行で確認することにする
    {
        // texture_manager_unregister() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_unregister_config_set(&config);

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // backend_context_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            NULL,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_id_ >= texture_manager_->max_texture_count -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            2,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_id_ < 0 -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            -1,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[texture_id_] == NULL && gpu_resources[texture_id_] != NULL
        // -> TEXTURE_SYSTEM_DATA_CORRUPTED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(NULL == cpu_resources[0]);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1U == gpu_resources[0]);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[texture_id_] != NULL && gpu_resources[texture_id_] == NULL
        // -> TEXTURE_SYSTEM_DATA_CORRUPTED
        // NOTE: この分岐では texture_destroy() / renderer_backend_texture_destroy() に到達しないため、
        // cpu resource はダミーポインタでよい
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        cpu_resources[0] = (texture_t*)(uintptr_t)0x1U;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert((texture_t*)(uintptr_t)0x1U == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // 指定IDが未登録 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        cpu_resources[0] = NULL;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister(
            backend_context,
            0,
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(NULL == cpu_resources[0]);
        assert(NULL == gpu_resources[0]);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_unregister_by_name(void) {
    {
        // texture_manager_unregister_by_name() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_unregister_by_name_config_set(&config);

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // backend_context_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            NULL,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // name_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            NULL,
            &manager
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() が TEXTURE_SYSTEM_BAD_OPERATION を返す -> そのまま伝播
        // 登録済みtextureがないため name は見つからない
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);
        assert(1U == s_test_config_texture_manager_texture_id_get.call_count);
        assert(0U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() が TEXTURE_SYSTEM_DATA_CORRUPTED を返す -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);
        assert(1U == s_test_config_texture_manager_texture_id_get.call_count);
        assert(0U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_texture_id_get_config_set(&config);

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);
        assert(1U == s_test_config_texture_manager_texture_id_get.call_count);
        assert(0U == s_test_config_texture_manager_unregister.call_count);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() 成功後、
        // texture_manager_unregister() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x5678U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_unregister_config_set(&config);

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_red",
            &manager
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(NULL != cpu_resources[0]);
        assert((renderer_backend_texture_t*)(uintptr_t)0x5678U == gpu_resources[0]);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);
        assert(1U == s_test_config_texture_manager_texture_id_get.call_count);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        texture_destroy(&cpu_resources[0]);
        assert(NULL == cpu_resources[0]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 成功系: 名前からIDを解決し、texture_manager_unregister() に委譲する
        // NOTE: texture_manager_unregister() は失敗注入で SUCCESS を返させるため、
        // 実際の破棄処理は行われない
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_context_t* backend_context =
            (renderer_backend_context_t*)(uintptr_t)0x1U;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_blue", &cpu_resources[2]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[2]);

        gpu_resources[2] = (renderer_backend_texture_t*)(uintptr_t)0x9999U;

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_SUCCESS;
        test_texture_manager_unregister_config_set(&config);

        ret = texture_manager_unregister_by_name(
            backend_context,
            "test_texture_blue",
            &manager
        );

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert(NULL != cpu_resources[2]);
        assert((renderer_backend_texture_t*)(uintptr_t)0x9999U == gpu_resources[2]);
        assert(1U == s_test_config_texture_manager_unregister_by_name.call_count);
        assert(1U == s_test_config_texture_manager_texture_id_get.call_count);
        assert(1U == s_test_config_texture_manager_unregister.call_count);

        texture_destroy(&cpu_resources[2]);
        assert(NULL == cpu_resources[2]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_texture_id_get(void) {
    {
        // texture_manager_texture_id_get() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_texture_id_get_config_set(&config);

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        int16_t texture_id = 123;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", NULL, &texture_id);

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // name_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get(NULL, &manager, &texture_id);

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // out_texture_id_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, NULL);

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[i] == NULL && gpu_resources[i] != NULL -> TEXTURE_SYSTEM_DATA_CORRUPTED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[i] != NULL && gpu_resources[i] == NULL -> TEXTURE_SYSTEM_DATA_CORRUPTED
        // NOTE: この分岐では texture_name_get() に到達しないため、ダミーポインタでよい
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        cpu_resources[0] = (texture_t*)(uintptr_t)0x1U;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // 登録済みtextureがない -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        int16_t texture_id = 123;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        test_texture_manager_config_reset();
    }
    {
        // 登録済みtextureはあるが、名前が一致しない -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_texture_id_get("test_texture_green", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert(123 == texture_id);

        texture_destroy(&cpu_resources[0]);
        assert(NULL == cpu_resources[0]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 0番目のtexture名に一致する
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_texture_id_get("test_texture_red", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert(0 == texture_id);

        texture_destroy(&cpu_resources[0]);
        assert(NULL == cpu_resources[0]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 空スロットを挟んで2番目のtexture名に一致する
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        int16_t texture_id = 123;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_blue", &cpu_resources[2]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[2]);

        gpu_resources[2] = (renderer_backend_texture_t*)(uintptr_t)0x2U;

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_texture_id_get("test_texture_blue", &manager, &texture_id);

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert(2 == texture_id);

        texture_destroy(&cpu_resources[2]);
        assert(NULL == cpu_resources[2]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_gpu_resource_get(void) {
    {
        // texture_manager_gpu_resource_get() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_gpu_resource_get_config_set(&config);

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            NULL,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // out_gpu_resource_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);

        test_texture_manager_config_reset();
    }
    {
        // texture_id_ >= texture_manager_->max_texture_count -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            2,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_id_ < 0 -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            -1,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[texture_id_] == NULL && gpu_resources[texture_id_] != NULL
        // -> TEXTURE_SYSTEM_DATA_CORRUPTED
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // cpu_resources[texture_id_] != NULL && gpu_resources[texture_id_] == NULL
        // -> TEXTURE_SYSTEM_DATA_CORRUPTED
        // NOTE: texture_manager_gpu_resource_get() では cpu resource の中身を参照しないため、
        // ダミーポインタでよい
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        cpu_resources[0] = (texture_t*)(uintptr_t)0x1U;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // 指定IDが未登録 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        cpu_resources[0] = NULL;
        gpu_resources[0] = NULL;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            0,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // 正常系: 指定IDのGPU resourceを取得する
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        cpu_resources[1] = (texture_t*)(uintptr_t)0x1U;
        gpu_resources[1] = (renderer_backend_texture_t*)(uintptr_t)0x5678U;

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get(
            1,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x5678U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_manager_gpu_resource_get_by_name(void) {
    {
        // texture_manager_gpu_resource_get_by_name() 冒頭で強制的に
        // TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_gpu_resource_get_by_name_config_set(&config);

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            NULL,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->max_texture_count <= 0 -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 0;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->cpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = NULL;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_->gpu_resources == NULL -> TEXTURE_SYSTEM_BAD_OPERATION
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = NULL;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // out_gpu_resource_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            NULL
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);

        test_texture_manager_config_reset();
    }
    {
        // name_ == NULL -> TEXTURE_SYSTEM_INVALID_ARGUMENT
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            NULL,
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() が TEXTURE_SYSTEM_BAD_OPERATION を返す
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_BAD_OPERATION == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() が TEXTURE_SYSTEM_DATA_CORRUPTED を返す
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;

        cpu_resources[0] = NULL;
        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x1U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_DATA_CORRUPTED == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;
        test_call_control_t config = {0};

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_texture_manager_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_texture_id_get_config_set(&config);

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        test_texture_manager_config_reset();
    }
    {
        // texture_manager_texture_id_get() 成功後、
        // texture_manager_gpu_resource_get() 冒頭で強制的に TEXTURE_SYSTEM_RUNTIME_ERROR を返させる
        // -> そのまま伝播
        texture_system_result_t ret = TEXTURE_SYSTEM_SUCCESS;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[2] = {NULL};
        renderer_backend_texture_t* gpu_resources[2] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_red", &cpu_resources[0]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[0]);

        gpu_resources[0] = (renderer_backend_texture_t*)(uintptr_t)0x5678U;

        manager.max_texture_count = 2;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)TEXTURE_SYSTEM_RUNTIME_ERROR;
        test_texture_manager_gpu_resource_get_config_set(&config);

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_red",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_RUNTIME_ERROR == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x1234U == out_gpu_resource);

        texture_destroy(&cpu_resources[0]);
        assert(NULL == cpu_resources[0]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 成功系: 名前からIDを解決し、GPU resourceを取得する
        texture_system_result_t ret = TEXTURE_SYSTEM_INVALID_ARGUMENT;
        texture_manager_t manager = {0};
        texture_t* cpu_resources[3] = {NULL};
        renderer_backend_texture_t* gpu_resources[3] = {NULL};
        renderer_backend_texture_t* out_gpu_resource =
            (renderer_backend_texture_t*)(uintptr_t)0x1234U;
        resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_memory = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_memory);

        ret_resource = texture_create("test_texture_blue", &cpu_resources[2]);
        assert(RESOURCE_SUCCESS == ret_resource);
        assert(NULL != cpu_resources[2]);

        gpu_resources[2] = (renderer_backend_texture_t*)(uintptr_t)0x9999U;

        manager.max_texture_count = 3;
        manager.cpu_resources = cpu_resources;
        manager.gpu_resources = gpu_resources;

        ret = texture_manager_gpu_resource_get_by_name(
            "test_texture_blue",
            &manager,
            &out_gpu_resource
        );

        assert(TEXTURE_SYSTEM_SUCCESS == ret);
        assert((renderer_backend_texture_t*)(uintptr_t)0x9999U == out_gpu_resource);

        texture_destroy(&cpu_resources[2]);
        assert(NULL == cpu_resources[2]);

        memory_system_destroy();
        test_texture_manager_config_reset();
        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_tex_sys_rslt_to_str(void) {
    assert(0 == strcmp("SUCCESS", tex_sys_rslt_to_str(TEXTURE_SYSTEM_SUCCESS)));
    assert(0 == strcmp("NO_MEMORY", tex_sys_rslt_to_str(TEXTURE_SYSTEM_NO_MEMORY)));
    assert(0 == strcmp("RUNTIME_ERROR", tex_sys_rslt_to_str(TEXTURE_SYSTEM_RUNTIME_ERROR)));
    assert(0 == strcmp("INVALID_ARGUMENT", tex_sys_rslt_to_str(TEXTURE_SYSTEM_INVALID_ARGUMENT)));
    assert(0 == strcmp("DATA_CORRUPTED", tex_sys_rslt_to_str(TEXTURE_SYSTEM_DATA_CORRUPTED)));
    assert(0 == strcmp("BAD_OPERATION", tex_sys_rslt_to_str(TEXTURE_SYSTEM_BAD_OPERATION)));
    assert(0 == strcmp("OVERFLOW", tex_sys_rslt_to_str(TEXTURE_SYSTEM_OVERFLOW)));
    assert(0 == strcmp("LIMIT_EXCEEDED", tex_sys_rslt_to_str(TEXTURE_SYSTEM_LIMIT_EXCEEDED)));
    assert(0 == strcmp("FILE_OPEN_ERROR", tex_sys_rslt_to_str(TEXTURE_SYSTEM_FILE_OPEN_ERROR)));
    assert(0 == strcmp("FILE_CLOSE_ERROR", tex_sys_rslt_to_str(TEXTURE_SYSTEM_FILE_CLOSE_ERROR)));
    assert(0 == strcmp("FILE_READ_ERROR", tex_sys_rslt_to_str(TEXTURE_SYSTEM_FILE_READ_ERROR)));
    assert(0 == strcmp("UNSUPPORTED_FILE", tex_sys_rslt_to_str(TEXTURE_SYSTEM_UNSUPPORTED_FILE)));
    assert(0 == strcmp("UNDEFINED_ERROR", tex_sys_rslt_to_str(TEXTURE_SYSTEM_UNDEFINED_ERROR)));

    assert(0 == strcmp("UNDEFINED_ERROR", tex_sys_rslt_to_str((texture_system_result_t)99999)));
}

// Generated by ChatGPT
static void NO_COVERAGE test_tex_sys_rslt_convert_linear_alloc(void) {
    assert(TEXTURE_SYSTEM_SUCCESS == tex_sys_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS));
    assert(TEXTURE_SYSTEM_NO_MEMORY == tex_sys_rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY));
    assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == tex_sys_rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT));

    assert(TEXTURE_SYSTEM_UNDEFINED_ERROR == tex_sys_rslt_convert_linear_alloc((linear_allocator_result_t)99999));
}

// Generated by ChatGPT
static void NO_COVERAGE test_tex_sys_rslt_convert_renderer(void) {
    assert(TEXTURE_SYSTEM_SUCCESS == tex_sys_rslt_convert_renderer(RENDERER_SUCCESS));
    assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == tex_sys_rslt_convert_renderer(RENDERER_INVALID_ARGUMENT));
    assert(TEXTURE_SYSTEM_RUNTIME_ERROR == tex_sys_rslt_convert_renderer(RENDERER_RUNTIME_ERROR));
    assert(TEXTURE_SYSTEM_NO_MEMORY == tex_sys_rslt_convert_renderer(RENDERER_NO_MEMORY));
    assert(TEXTURE_SYSTEM_RUNTIME_ERROR == tex_sys_rslt_convert_renderer(RENDERER_SHADER_COMPILE_ERROR));
    assert(TEXTURE_SYSTEM_RUNTIME_ERROR == tex_sys_rslt_convert_renderer(RENDERER_SHADER_LINK_ERROR));
    assert(TEXTURE_SYSTEM_LIMIT_EXCEEDED == tex_sys_rslt_convert_renderer(RENDERER_LIMIT_EXCEEDED));
    assert(TEXTURE_SYSTEM_BAD_OPERATION == tex_sys_rslt_convert_renderer(RENDERER_BAD_OPERATION));
    assert(TEXTURE_SYSTEM_DATA_CORRUPTED == tex_sys_rslt_convert_renderer(RENDERER_DATA_CORRUPTED));
    assert(TEXTURE_SYSTEM_UNDEFINED_ERROR == tex_sys_rslt_convert_renderer(RENDERER_UNDEFINED_ERROR));

    assert(TEXTURE_SYSTEM_UNDEFINED_ERROR == tex_sys_rslt_convert_renderer((renderer_result_t)99999));
}

// Generated by ChatGPT
static void NO_COVERAGE test_tex_sys_rslt_convert_resource(void) {
    assert(TEXTURE_SYSTEM_SUCCESS == tex_sys_rslt_convert_resource(RESOURCE_SUCCESS));
    assert(TEXTURE_SYSTEM_NO_MEMORY == tex_sys_rslt_convert_resource(RESOURCE_NO_MEMORY));
    assert(TEXTURE_SYSTEM_RUNTIME_ERROR == tex_sys_rslt_convert_resource(RESOURCE_RUNTIME_ERROR));
    assert(TEXTURE_SYSTEM_INVALID_ARGUMENT == tex_sys_rslt_convert_resource(RESOURCE_INVALID_ARGUMENT));
    assert(TEXTURE_SYSTEM_DATA_CORRUPTED == tex_sys_rslt_convert_resource(RESOURCE_DATA_CORRUPTED));
    assert(TEXTURE_SYSTEM_BAD_OPERATION == tex_sys_rslt_convert_resource(RESOURCE_BAD_OPERATION));
    assert(TEXTURE_SYSTEM_OVERFLOW == tex_sys_rslt_convert_resource(RESOURCE_OVERFLOW));
    assert(TEXTURE_SYSTEM_LIMIT_EXCEEDED == tex_sys_rslt_convert_resource(RESOURCE_LIMIT_EXCEEDED));
    assert(TEXTURE_SYSTEM_FILE_OPEN_ERROR == tex_sys_rslt_convert_resource(RESOURCE_FILE_OPEN_ERROR));
    assert(TEXTURE_SYSTEM_FILE_CLOSE_ERROR == tex_sys_rslt_convert_resource(RESOURCE_FILE_CLOSE_ERROR));
    assert(TEXTURE_SYSTEM_FILE_READ_ERROR == tex_sys_rslt_convert_resource(RESOURCE_FILE_READ_ERROR));
    assert(TEXTURE_SYSTEM_UNSUPPORTED_FILE == tex_sys_rslt_convert_resource(RESOURCE_UNSUPPORTED_FILE));
    assert(TEXTURE_SYSTEM_UNDEFINED_ERROR == tex_sys_rslt_convert_resource(RESOURCE_UNDEFINED_ERROR));

    assert(TEXTURE_SYSTEM_UNDEFINED_ERROR == tex_sys_rslt_convert_resource((resource_result_t)99999));
}
#endif
