// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file line_mesh_shader.c
 * @author chocolate-pie24
 * @brief 線分描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIの実装
 *
 * @date 2026-05-28
 *
 */
#include "engine/systems/renderer/resources/shaders/line_mesh_shader.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/config/renderer_config.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vao.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_err_utils.h"
#include "engine/systems/renderer/resources/shaders/core/shader_program_builder.h"

// TODO: テスト(line_mesh_shaderは今後も拡張されるため、テストはまだ行わない)
// TODO: DYNAMIC / STATICでそれぞれVBOを作る

/**
 * @brief 線分描画用シェーダーリソース構造体
 * @note 本構造体はshader programだけでなく、line描画用のVAO/VBOとバッファ書き込み状態も保持する
 *
 */
struct line_mesh_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */
    int32_t color_location;                 /**< 色情報のユニフォーム変数Location */

    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */

    renderer_backend_vao_t* vao;            /**< 線分描画シェーダー用VAO */

    vbo_manager_t* vbo_manager;
    vbo_manager_config_t vbo_config;
};

// validation
static bool line_mesh_shader_is_initialized(const line_mesh_shader_t* line_mesh_shader_);

shader_result_t line_mesh_shader_create(line_mesh_shader_t** out_line_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    line_mesh_shader_t* tmp_line_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_create", "out_line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_create", "*out_line_mesh_shader_")

    // line shader構造体インスタンス生成
    ret_memory = memory_system_allocate(sizeof(line_mesh_shader_t), MEMORY_TAG_RENDERER, (void**)&tmp_line_mesh_shader);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = shader_rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("line_mesh_shader_create(%s) - Failed to allocate memory for tmp_line_mesh_shader.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_line_mesh_shader, 0, sizeof(line_mesh_shader_t));

    *out_line_mesh_shader_ = tmp_line_mesh_shader;

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

void line_mesh_shader_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t** line_mesh_shader_) {
    if(NULL == line_mesh_shader_) {
        WARN_MESSAGE("line_mesh_shader_destroy - Provided line_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == *line_mesh_shader_) {
        WARN_MESSAGE("line_mesh_shader_destroy - Provided *line_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == backend_context_) {
        WARN_MESSAGE("line_mesh_shader_destroy - Provided backend_context_ is not valid.");
        return;
    }
    line_mesh_shader_vao_vbo_destroy(backend_context_, *line_mesh_shader_);
    if(NULL != (*line_mesh_shader_)->shader) {
        renderer_backend_shader_destroy(backend_context_, &(*line_mesh_shader_)->shader);
    }
    memory_system_free(*line_mesh_shader_, sizeof(line_mesh_shader_t), MEMORY_TAG_RENDERER);
    *line_mesh_shader_ = NULL;
}

shader_result_t line_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const char* file_path_, const char* name_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp_shader = NULL;
    int32_t tmp_model_matrix_location = 0;
    int32_t tmp_view_matrix_location = 0;
    int32_t tmp_projection_matrix_location = 0;
    int32_t tmp_color_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "name_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_program_initialize", "line_mesh_shader_->shader")

    // シェーダープログラムビルド
    ret = shader_program_builder_create_from_files(backend_context_, file_path_, name_, &tmp_shader);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to build shader program.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_model_matrix", &tmp_model_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get model matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_view_matrix", &tmp_view_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get view matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_projection_matrix", &tmp_projection_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get projection matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_line_color", &tmp_color_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get color location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    line_mesh_shader_->model_matrix_location = tmp_model_matrix_location;
    line_mesh_shader_->view_matrix_location = tmp_view_matrix_location;
    line_mesh_shader_->projection_matrix_location = tmp_projection_matrix_location;
    line_mesh_shader_->color_location = tmp_color_location;
    line_mesh_shader_->shader = tmp_shader;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret && NULL != tmp_shader) {
        renderer_backend_shader_destroy(backend_context_, &tmp_shader);
    }
    return ret;
}

shader_result_t line_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const line_mesh_shader_config_t* config_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;
    vbo_manager_config_t vbo_config = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_create", "line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->vao, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_vbo_create", "vao")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->vbo_manager, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_vbo_create", "line_mesh_shader_->vbo_manager")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_create", "config_")
    if(!line_mesh_shader_config_is_valid(config_)) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_mesh_shader_vbo_initialize(%s) - Provided config_ is not valid.", shader_rslt_to_str(SHADER_INVALID_ARGUMENT));
        goto cleanup;
    }

    vbo_config.base_align = alignof(float);
    vbo_config.buffer_usage = config_->buffer_usage;
    vbo_config.max_allocation_count = config_->max_allocation_count;
    vbo_config.vbo_size = config_->vbo_size;

    ret_buff_mgr = vbo_manager_create(backend_context_, &vbo_config, &tmp_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("line_mesh_shader_vbo_initialize(%s) - buffer manager create failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    line_mesh_shader_->vbo_config = vbo_config;
    line_mesh_shader_->vbo_manager = tmp_vbo_manager;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret) {
        if(NULL != tmp_vbo_manager) {
            vbo_manager_destroy(&tmp_vbo_manager, backend_context_);
        }
    }

    return ret;
}

shader_result_t line_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;
    shader_result_t ret_cleanup = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    bool vao_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vao_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vao_initialize", "line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->vao, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_vao_initialize", "vao")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->vbo_manager, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_vao_initialize", "line_mesh_shader_->vbo_manager")

    ret_renderer_backend = renderer_backend_vao_create(backend_context_, &line_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to create line vao.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret_renderer_backend = renderer_backend_vao_bind(backend_context_, line_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to bind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret_buff_mgr = vbo_manager_bind(line_mesh_shader_->vbo_manager, backend_context_);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to bind vertex buffer.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vao_attribute_set(backend_context_, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(line_vertex_t), offsetof(line_vertex_t, position));  // 頂点座標(layout = 0)
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to set vertex array attribute.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_buff_mgr = vbo_manager_unbind(backend_context_);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to unbind vertex buffer.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    ret_renderer_backend = renderer_backend_vao_unbind(backend_context_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to unbind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret) {
        if(vbo_bound) {
            ret_buff_mgr = vbo_manager_unbind(backend_context_);
            if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
                ret_cleanup = shader_rslt_convert_buffer_manager(ret_buff_mgr);
                ERROR_MESSAGE("line_mesh_shader_vao_initialize failed. %s", shader_rslt_to_str(ret_cleanup));
                ret = SHADER_DATA_CORRUPTED;
            }
        }
        if(vao_bound) {
            ret_renderer_backend = renderer_backend_vao_unbind(backend_context_);
            if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
                ret_cleanup = shader_rslt_convert_renderer_backend(ret_renderer_backend);
                ERROR_MESSAGE("line_mesh_shader_vao_initialize failed. %s", shader_rslt_to_str(ret_cleanup));
                ret = SHADER_DATA_CORRUPTED;
            }
        }
        if(vao_created) {
            renderer_backend_vao_destroy(backend_context_, &line_mesh_shader_->vao);
        }
    }
    return ret;
}

void line_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_) {
    if(NULL == backend_context_) {
        WARN_MESSAGE("line_mesh_shader_vao_vbo_destroy - Provided backend_context_ is not valid.");
        return;
    }
    if(NULL == line_mesh_shader_) {
        WARN_MESSAGE("line_mesh_shader_vao_vbo_destroy - Provided line_mesh_shader_ is not valid.");
        return;
    }
    if(NULL != line_mesh_shader_->vbo_manager) {
        vbo_manager_destroy(&line_mesh_shader_->vbo_manager, backend_context_);
    }
    if(NULL != line_mesh_shader_->vao) {
        renderer_backend_vao_destroy(backend_context_, &line_mesh_shader_->vao);
    }
}

shader_result_t line_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, size_t vertex_count_, const line_vertex_t* vertices_, vbo_range_t* out_buffer_range_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;
    shader_result_t ret_cleanup = SHADER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_allocation_t tmp_alloc_handle = { 0 };
    size_t write_size = 0;
    bool vbo_written = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(line_mesh_shader_is_initialized(line_mesh_shader_), ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 2), ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_range_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "out_buffer_range_")

    if((SIZE_MAX / sizeof(line_vertex_t) < vertex_count_)) {
        ret = SHADER_OVERFLOW;
        ERROR_MESSAGE("line_mesh_shader_vbo_write(%s) - line_mesh_shader_vbo_write failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    write_size = sizeof(line_vertex_t) * vertex_count_;

    ret_buff_mgr = vbo_manager_write(line_mesh_shader_->vbo_manager, backend_context_, write_size, (const void*)vertices_, &tmp_alloc_handle);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("line_mesh_shader_vbo_write(%s) - vbo write failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_written = true;

    if(0 != (tmp_alloc_handle.offset % sizeof(line_vertex_t))) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_shader_vbo_write(%s) - vbo write failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    out_buffer_range_->allocation_info = tmp_alloc_handle;
    out_buffer_range_->draw_range.first_vertex_count = tmp_alloc_handle.offset / sizeof(line_vertex_t);
    out_buffer_range_->draw_range.vertex_count = vertex_count_;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret && vbo_written) {
        // NOTE: vbo_manager_freeが失敗した場合はvbo_managerにデータ不整合が発生しているため、
        // line_mesh_shader_vbo_write失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
        ret_buff_mgr = vbo_manager_free(line_mesh_shader_->vbo_manager, &tmp_alloc_handle);
        if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
            ret_cleanup = shader_rslt_convert_buffer_manager(ret_buff_mgr);
            ret = SHADER_DATA_CORRUPTED;
            ERROR_MESSAGE("line_mesh_shader_vbo_write(%s, %s) - vbo free failed.", shader_rslt_to_str(ret), shader_rslt_to_str(ret_cleanup));
        }
    }
    return ret;
}

shader_result_t line_mesh_shader_vbo_free(line_mesh_shader_t* line_mesh_shader_, const vbo_range_t* buffer_range_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    IF_ARG_FALSE_GOTO_CLEANUP(line_mesh_shader_is_initialized(line_mesh_shader_), ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_free", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_range_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vbo_free", "buffer_range_")

    if(!vbo_range_is_valid(buffer_range_)) {
        ret = SHADER_BAD_OPERATION;
        ERROR_MESSAGE("line_mesh_shader_vbo_free(%s) - Provided buffer_range_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_buff_mgr = vbo_manager_free(line_mesh_shader_->vbo_manager, &buffer_range_->allocation_info);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("line_mesh_shader_vbo_free(%s) - vbo free failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vao_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_vao_bind", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->vao, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_vao_bind", "vao")

    ret_renderer_backend = renderer_backend_vao_bind(backend_context_, line_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_vao_bind(%s) - Failed to bind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_use(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_use", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_use", "line_mesh_shader_->shader")

    ret_renderer_backend = renderer_backend_shader_use(backend_context_, line_mesh_shader_->shader);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_use(%s) - Failed to switch program for line_mesh_shader.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_model_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "model_matrix_")

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_view_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "view_matrix_")

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_projection_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "projection_matrix_")

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t line_mesh_shader_color_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const uint8_t color_[4]) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "line_mesh_shader_color_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(color_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "color_")

    ret_renderer_backend = renderer_backend_shader_vec4u8_uniform_set(backend_context_, line_mesh_shader_->color_location, color_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("line_mesh_shader_color_set(%s) - Failed to set color.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

static bool line_mesh_shader_is_initialized(const line_mesh_shader_t* line_mesh_shader_) {
    if(NULL == line_mesh_shader_) {
        return false;
    }
    if(NULL == line_mesh_shader_->shader) {
        return false;
    }
    if(NULL == line_mesh_shader_->vao) {
        return false;
    }
    if(NULL == line_mesh_shader_->vbo_manager) {
        return false;
    }
    return true;
}
