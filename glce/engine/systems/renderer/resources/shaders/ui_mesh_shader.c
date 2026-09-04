// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file ui_mesh_shader.c
 * @author chocolate-pie24
 * @brief UIシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIの実装
 *
 * @date 2026-03-11
 *
 */
#include "engine/systems/renderer/resources/shaders/ui_mesh_shader.h"

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

// TODO: テスト(ui_mesh_shaderは今後も拡張されるため、テストはまだ行わない)
// TODO: DYNAMIC / STATICでそれぞれVBOを作る

/**
 * @brief UIシェーダーリソース構造体
 * @note 本構造体はshader programだけでなく、UI描画用のVAO/VBOとバッファ書き込み状態も保持する
 *
 */
struct ui_mesh_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */

    renderer_backend_context_t* backend_context;
    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */
    renderer_backend_vao_t* vao;         /**< UIシェーダー用VAO */

    vbo_manager_t* vbo_manager;
};

static void destroy_unchecked(ui_mesh_shader_t** ui_mesh_shader_);
static shader_result_t program_initialize(ui_mesh_shader_t* ui_mesh_shader_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_);
static shader_result_t vbo_initialize(ui_mesh_shader_t* ui_mesh_shader_, const ui_mesh_shader_config_t* config_);
static shader_result_t vao_initialize(ui_mesh_shader_t* ui_mesh_shader_);

// validation
static bool ui_mesh_shader_is_initialized(const ui_mesh_shader_t* ui_mesh_shader_);

shader_result_t ui_mesh_shader_create(renderer_backend_context_t* backend_context_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, const ui_mesh_shader_config_t* config_, ui_mesh_shader_t** out_ui_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    ui_mesh_shader_t* tmp_ui_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_create", "out_ui_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ui_mesh_shader_, ret, SHADER_BAD_OPERATION, shader_rslt_to_str(SHADER_BAD_OPERATION), "ui_mesh_shader_create", "*out_ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_create", "vertex_shader_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(fragment_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_create", "fragment_shader_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_create", "config_")
    if('\0' == vertex_shader_fullpath_[0]) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - Provided vertex_shader_fullpath_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == fragment_shader_fullpath_[0]) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - Provided fragment_shader_fullpath_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    if(!ui_mesh_shader_config_is_valid(config_)) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - Provided config_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // ui shader構造体インスタンス生成
    ret_memory_system = memory_system_allocate(sizeof(ui_mesh_shader_t), MEMORY_TAG_RENDERER, (void**)&tmp_ui_mesh_shader);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = shader_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - Failed to allocate memory for tmp_ui_mesh_shader.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_ui_mesh_shader, 0, sizeof(ui_mesh_shader_t));

    tmp_ui_mesh_shader->backend_context = backend_context_;

    ret = program_initialize(tmp_ui_mesh_shader, vertex_shader_fullpath_, fragment_shader_fullpath_);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - program_initialize failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = vbo_initialize(tmp_ui_mesh_shader, config_);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - vbo_initialize failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = vao_initialize(tmp_ui_mesh_shader);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_shader_create(%s) - vao_initialize failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    *out_ui_mesh_shader_ = tmp_ui_mesh_shader;
    tmp_ui_mesh_shader = NULL;

    ret = SHADER_SUCCESS;

cleanup:
    if(NULL != tmp_ui_mesh_shader) {
        destroy_unchecked(&tmp_ui_mesh_shader);
    }
    return ret;
}

void ui_mesh_shader_destroy(ui_mesh_shader_t** ui_mesh_shader_) {
    if(NULL == ui_mesh_shader_) {
        return;
    }
    if(NULL == *ui_mesh_shader_) {
        return;
    }

    // TODO: validatorに変更する
    if(NULL == (*ui_mesh_shader_)->backend_context) {
        ERROR_MESSAGE("ui_mesh_shader_destroy - Provided ui_mesh_shader_ is corrupted.");
        return;
    }
    if(NULL == (*ui_mesh_shader_)->shader) {
        ERROR_MESSAGE("ui_mesh_shader_destroy - Provided ui_mesh_shader_ is corrupted.");
        return;
    }
    if(NULL == (*ui_mesh_shader_)->vao) {
        ERROR_MESSAGE("ui_mesh_shader_destroy - Provided ui_mesh_shader_ is corrupted.");
        return;
    }
    if(NULL == (*ui_mesh_shader_)->vbo_manager) {
        ERROR_MESSAGE("ui_mesh_shader_destroy - Provided ui_mesh_shader_ is corrupted.");
        return;
    }

    destroy_unchecked(ui_mesh_shader_);
}

shader_result_t ui_mesh_shader_vbo_write(ui_mesh_shader_t* ui_mesh_shader_, size_t vertex_count_, const ui_vertex_t* vertices_, vbo_range_t* out_buffer_range_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    shader_result_t ret_cleanup = SHADER_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_allocation_t tmp_alloc_handle = { 0 };
    size_t write_size = 0;
    bool vbo_written = false;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vbo_write", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vbo_write", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_range_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vbo_write", "out_buffer_range_")
    if(0 == vertex_count_ || 6 != vertex_count_) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_mesh_shader_vbo_write(%s) - Provided vertex_count_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_vbo_write(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    write_size = 6 * sizeof(ui_vertex_t);
    ret_buff_mgr = vbo_manager_write(ui_mesh_shader_->vbo_manager, write_size, (const void*)vertices_, &tmp_alloc_handle);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("ui_mesh_shader_vbo_write(%s) - vbo write failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_written = true;

    if(0 != (tmp_alloc_handle.offset % sizeof(ui_vertex_t))) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_vbo_write(%s) - vbo write failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    out_buffer_range_->allocation_info = tmp_alloc_handle;
    out_buffer_range_->draw_range.first_vertex_count = tmp_alloc_handle.offset / sizeof(ui_vertex_t);
    out_buffer_range_->draw_range.vertex_count = vertex_count_;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret && vbo_written) {
        // NOTE: vbo_manager_freeが失敗した場合はvbo_managerにデータ不整合が発生しているため、
        // ui_mesh_shader_vbo_write失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
        ret_buff_mgr = vbo_manager_free(ui_mesh_shader_->vbo_manager, &tmp_alloc_handle);
        if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
            ret_cleanup = shader_rslt_convert_buffer_manager(ret_buff_mgr);
            ret = SHADER_DATA_CORRUPTED;
            ERROR_MESSAGE("ui_mesh_shader_vbo_write(%s, %s) - vbo free failed.", shader_rslt_to_str(ret), shader_rslt_to_str(ret_cleanup));
        }
    }
    return ret;
}

shader_result_t ui_mesh_shader_vbo_free(ui_mesh_shader_t* ui_mesh_shader_, const vbo_range_t* buffer_range_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vbo_free", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_range_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vbo_free", "buffer_range_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_vbo_free(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    if(!vbo_range_is_valid(buffer_range_)) {
        ret = SHADER_BAD_OPERATION;
        ERROR_MESSAGE("ui_mesh_shader_vbo_free(%s) - Provided buffer_range_ is not valid.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_buff_mgr = vbo_manager_free(ui_mesh_shader_->vbo_manager, &buffer_range_->allocation_info);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("ui_mesh_shader_vbo_free(%s) - ui_mesh_shader_vbo_free failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t ui_mesh_shader_vao_bind(const ui_mesh_shader_t* ui_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_vao_bind", "ui_mesh_shader_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_vao_bind(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vao_bind(ui_mesh_shader_->backend_context, ui_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_vao_bind(%s) - Failed to bind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t ui_mesh_shader_use(const ui_mesh_shader_t* ui_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_use", "ui_mesh_shader_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_use(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_use(ui_mesh_shader_->backend_context, ui_mesh_shader_->shader);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_use(%s) - Failed to switch program for ui_mesh_shader.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t ui_mesh_shader_model_matrix_set(const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_model_matrix_set", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_model_matrix_set", "model_matrix_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_model_matrix_set(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(ui_mesh_shader_->backend_context, ui_mesh_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t ui_mesh_shader_view_matrix_set(const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_view_matrix_set", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_view_matrix_set", "view_matrix_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_view_matrix_set(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(ui_mesh_shader_->backend_context, ui_mesh_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t ui_mesh_shader_projection_matrix_set(const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_projection_matrix_set", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "ui_mesh_shader_projection_matrix_set", "projection_matrix_")
    if(!ui_mesh_shader_is_initialized(ui_mesh_shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_shader_projection_matrix_set(%s) - Provided ui_mesh_shader_ is corrupted.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(ui_mesh_shader_->backend_context, ui_mesh_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

static void destroy_unchecked(ui_mesh_shader_t** ui_mesh_shader_) {
    if(NULL == ui_mesh_shader_) {
        return;
    }
    if(NULL == *ui_mesh_shader_) {
        return;
    }
    if(NULL == (*ui_mesh_shader_)->backend_context) {
        ERROR_MESSAGE("ui_mesh_shader_destroy - Provided backend_context_ is not valid.");
        return;
    }
    if(NULL != (*ui_mesh_shader_)->vbo_manager) {
        vbo_manager_destroy(&(*ui_mesh_shader_)->vbo_manager);
    }
    if(NULL != (*ui_mesh_shader_)->vao) {
        renderer_backend_vao_destroy((*ui_mesh_shader_)->backend_context, &(*ui_mesh_shader_)->vao);
    }
    if(NULL != (*ui_mesh_shader_)->shader) {
        renderer_backend_shader_destroy((*ui_mesh_shader_)->backend_context, &(*ui_mesh_shader_)->shader);
    }
    memory_system_free(*ui_mesh_shader_, sizeof(ui_mesh_shader_t), MEMORY_TAG_RENDERER);
    *ui_mesh_shader_ = NULL;
}

static shader_result_t program_initialize(ui_mesh_shader_t* ui_mesh_shader_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp_shader = NULL;
    int32_t tmp_model_matrix_location = 0;
    int32_t tmp_view_matrix_location = 0;
    int32_t tmp_projection_matrix_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "program_initialize", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "program_initialize", "vertex_shader_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(fragment_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "program_initialize", "fragment_shader_fullpath_")

    // シェーダープログラムビルド
    ret = shader_program_builder_create_from_files(ui_mesh_shader_->backend_context, vertex_shader_fullpath_, fragment_shader_fullpath_, &tmp_shader);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_shader_program_initialize(%s) - Failed to build shader program.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret_renderer_backend = renderer_backend_shader_uniform_location_get(ui_mesh_shader_->backend_context, tmp_shader, "g_model_matrix", &tmp_model_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_program_initialize(%s) - Failed to get model matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(ui_mesh_shader_->backend_context, tmp_shader, "g_view_matrix", &tmp_view_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_program_initialize(%s) - Failed to get view matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(ui_mesh_shader_->backend_context, tmp_shader, "g_projection_matrix", &tmp_projection_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("ui_mesh_shader_program_initialize(%s) - Failed to get projection matrix location.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ui_mesh_shader_->model_matrix_location = tmp_model_matrix_location;
    ui_mesh_shader_->view_matrix_location = tmp_view_matrix_location;
    ui_mesh_shader_->projection_matrix_location = tmp_projection_matrix_location;
    ui_mesh_shader_->shader = tmp_shader;
    tmp_shader = NULL;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret && NULL != tmp_shader) {
        if(NULL != ui_mesh_shader_ && NULL != ui_mesh_shader_->backend_context) {
            renderer_backend_shader_destroy(ui_mesh_shader_->backend_context, &tmp_shader);
        }
    }
    return ret;
}

static shader_result_t vbo_initialize(ui_mesh_shader_t* ui_mesh_shader_, const ui_mesh_shader_config_t* config_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;
    vbo_manager_config_t vbo_config = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "vbo_initialize", "ui_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "vbo_initialize", "config_")

    vbo_config.base_align = alignof(float);
    vbo_config.buffer_usage = config_->buffer_usage;
    vbo_config.max_allocation_count = config_->max_allocation_count;
    vbo_config.vbo_size = config_->vbo_size;

    ret_buff_mgr = vbo_manager_create(ui_mesh_shader_->backend_context, &vbo_config, &tmp_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("vbo_initialize(%s) - buffer manager create failed.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ui_mesh_shader_->vbo_manager = tmp_vbo_manager;
    tmp_vbo_manager = NULL;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret) {
        if(NULL != tmp_vbo_manager) {
            vbo_manager_destroy(&tmp_vbo_manager);
        }
    }
    return ret;
}

static shader_result_t vao_initialize(ui_mesh_shader_t* ui_mesh_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    shader_result_t ret_cleanup = SHADER_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    bool vao_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(ui_mesh_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "vao_initialize", "ui_mesh_shader_")

    ret_renderer_backend = renderer_backend_vao_create(ui_mesh_shader_->backend_context, &ui_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to create ui mesh vao.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret_renderer_backend = renderer_backend_vao_bind(ui_mesh_shader_->backend_context, ui_mesh_shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to bind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret_buff_mgr = vbo_manager_bind(ui_mesh_shader_->vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to bind vertex buffer(ui).", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vao_attribute_set(ui_mesh_shader_->backend_context, 0, 2, RENDERER_TYPE_FLOAT, false, sizeof(ui_vertex_t), offsetof(ui_vertex_t, position));  // 頂点座標(layout = 0)
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to set vertex array attribute(vertex).", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vao_attribute_set(ui_mesh_shader_->backend_context, 1, 2, RENDERER_TYPE_FLOAT, false, sizeof(ui_vertex_t), offsetof(ui_vertex_t, tex_coord));    // テクスチャuv座標(layout = 1)
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to set vertex array attribute(texture).", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_buff_mgr = vbo_manager_unbind(ui_mesh_shader_->vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = shader_rslt_convert_buffer_manager(ret_buff_mgr);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to unbind vertex buffer(ui).", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    ret_renderer_backend = renderer_backend_vao_unbind(ui_mesh_shader_->backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vao_initialize(%s) - Failed to unbind vertex array.", shader_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret) {
        if(vbo_bound) {
            ret_buff_mgr = vbo_manager_unbind(ui_mesh_shader_->vbo_manager);
            if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
                ret_cleanup = shader_rslt_convert_buffer_manager(ret_buff_mgr);
                ERROR_MESSAGE("vao_initialize failed. %s", shader_rslt_to_str(ret_cleanup));
                ret = SHADER_DATA_CORRUPTED;
            }
        }
        if(vao_bound) {
            ret_renderer_backend = renderer_backend_vao_unbind(ui_mesh_shader_->backend_context);
            if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
                ret_cleanup = shader_rslt_convert_renderer_backend(ret_renderer_backend);
                ERROR_MESSAGE("vao_initialize failed. %s", shader_rslt_to_str(ret_cleanup));
                ret = SHADER_DATA_CORRUPTED;
            }
        }
        if(vao_created) {
            renderer_backend_vao_destroy(ui_mesh_shader_->backend_context, &ui_mesh_shader_->vao);
        }
    }
    return ret;
}

static bool ui_mesh_shader_is_initialized(const ui_mesh_shader_t* ui_mesh_shader_) {
    if(NULL == ui_mesh_shader_) {
        return false;
    }
    if(NULL == ui_mesh_shader_->shader) {
        return false;
    }
    if(NULL == ui_mesh_shader_->vao) {
        return false;
    }
    if(NULL == ui_mesh_shader_->vbo_manager) {
        return false;
    }
    if(NULL == ui_mesh_shader_->backend_context) {
        return false;
    }
    return true;
}
