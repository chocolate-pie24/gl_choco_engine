/** @ingroup renderer
 *
 * @file line_mesh_shader.c
 * @author chocolate-pie24
 * @brief 線分描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIの実装
 *
 * @version 0.1
 * @date 2026-05-28
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/renderer_resources/shaders/line_mesh_shader.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_memory.h"
#include "engine/systems/renderer/renderer_core/allocators/range_free_list.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_program_builder.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

// TODO: テスト(line_mesh_shaderは今後も拡張されるため、テストはまだ行わない)
// TODO: DYNAMIC / STATICでそれぞれVBOを作る

/**
 * @brief 線分描画用シェーダーリソース構造体
 * @note 本構造体はshader programだけでなく、line描画用のVAO/VBOとバッファ書き込み状態も保持する
 * @todo TODO: FreeListを使用したバッファ管理
 *
 */
struct line_mesh_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */
    int32_t color_location;                 /**< 色情報のユニフォーム変数Location */

    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */

    renderer_backend_vao_t* line_vao;       /**< 線分描画シェーダー用VAO */

    vbo_manager_t* vbo_manager;
    vbo_manager_config_t vbo_config;
};

// validation
static bool vbo_config_is_valid(const vbo_manager_config_t* config_);
static bool line_mesh_shader_is_initialized(const line_mesh_shader_t* line_mesh_shader_);

renderer_result_t line_mesh_shader_create(line_mesh_shader_t** out_line_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    line_mesh_shader_t* tmp_line_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_create", "out_line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_create", "*out_line_mesh_shader_")

    // line shader構造体インスタンス生成
    ret = renderer_mem_allocate(sizeof(line_mesh_shader_t), (void**)&tmp_line_mesh_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_create(%s) - Failed to allocate memory for tmp_line_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_line_mesh_shader, 0, sizeof(line_mesh_shader_t));

    *out_line_mesh_shader_ = tmp_line_mesh_shader;
    ret = RENDERER_SUCCESS;

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
    renderer_mem_free(*line_mesh_shader_, sizeof(line_mesh_shader_t));
    *line_mesh_shader_ = NULL;
}

renderer_result_t line_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const char* file_path_, const char* name_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp_shader = NULL;
    int32_t tmp_model_matrix_location = 0;
    int32_t tmp_view_matrix_location = 0;
    int32_t tmp_projection_matrix_location = 0;
    int32_t tmp_color_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_program_initialize", "name_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_program_initialize", "line_mesh_shader_->shader")

    // シェーダープログラムビルド
    ret = shader_program_builder_create_from_files(backend_context_, file_path_, name_, &tmp_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to build shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_model_matrix", &tmp_model_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get model matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_view_matrix", &tmp_view_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get view matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_projection_matrix", &tmp_projection_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get projection matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_line_color", &tmp_color_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_program_initialize(%s) - Failed to get color location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    line_mesh_shader_->model_matrix_location = tmp_model_matrix_location;
    line_mesh_shader_->view_matrix_location = tmp_view_matrix_location;
    line_mesh_shader_->projection_matrix_location = tmp_projection_matrix_location;
    line_mesh_shader_->color_location = tmp_color_location;
    line_mesh_shader_->shader = tmp_shader;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret && NULL != tmp_shader) {
        renderer_backend_shader_destroy(backend_context_, &tmp_shader);
    }
    return ret;
}

renderer_result_t line_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const vbo_manager_config_t* vbo_config_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_create", "line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vertex_buffer_create", "line_vao")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->vbo_manager, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vertex_buffer_create", "line_mesh_shader_->vbo_manager")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_config_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_create", "vbo_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(vbo_config_is_valid(vbo_config_), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_create", "vbo_config_")

    ret_buff_mgr = vbo_manager_create(backend_context_, vbo_config_, &tmp_vbo_manager);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = RENDERER_RUNTIME_ERROR;   // TODO: vbo_manager_create仕様が安定したら適切な実行結果コードに変換する
        ERROR_MESSAGE("line_mesh_shader_vbo_initialize(%s) - buffer manager create failed.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    line_mesh_shader_->vbo_config = *vbo_config_;
    line_mesh_shader_->vbo_manager = tmp_vbo_manager;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != tmp_vbo_manager) {
            vbo_manager_destroy(&tmp_vbo_manager, backend_context_);
        }
    }

    return ret;
}

renderer_result_t line_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    bool vao_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vao_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vao_initialize", "line_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_mesh_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vao_initialize", "line_vao")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->vbo_manager, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vao_initialize", "line_mesh_shader_->vbo_manager")

    ret = renderer_backend_vertex_array_create(backend_context_, &line_mesh_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to create line vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret = renderer_backend_vertex_array_bind(backend_context_, line_mesh_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret_buff_mgr = vbo_manager_bind(line_mesh_shader_->vbo_manager, backend_context_);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        // TODO: buffer_manager仕様確定後、実行結果コード変換を適切にする
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to bind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_array_attribute_set(backend_context_, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3, 0);  // 頂点座標(layout = 0)
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to set vertex array attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret_buff_mgr = vbo_manager_unbind(backend_context_);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        // TODO: buffer_manager仕様確定後、実行結果コード変換を適切にする
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    ret = renderer_backend_vertex_array_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_vao_initialize(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(vbo_bound) {
            vbo_manager_unbind(backend_context_);
        }
        if(vao_bound) {
            renderer_backend_vertex_array_unbind(backend_context_);
        }
        if(vao_created) {
            renderer_backend_vertex_array_destroy(backend_context_, &line_mesh_shader_->line_vao);
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
    if(NULL != line_mesh_shader_->line_vao) {
        renderer_backend_vertex_array_destroy(backend_context_, &line_mesh_shader_->line_vao);
    }
}

renderer_result_t line_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, size_t size_, const line_vertex_t* write_data_, vertex_buffer_range_t* out_buffer_range_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    vertex_allocation_t tmp_alloc_handle = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(line_mesh_shader_is_initialized(line_mesh_shader_), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "write_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (size_ % (sizeof(line_vertex_t) * 2)), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_range_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "out_buffer_range_")

    ret_buff_mgr = vbo_manager_write(line_mesh_shader_->vbo_manager, backend_context_, size_, (const void*)write_data_, &tmp_alloc_handle);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = RENDERER_RUNTIME_ERROR;   // TODO: buffer_managerの仕様が安定したら適切な実行結果コードに変換する
        ERROR_MESSAGE("line_mesh_shader_vbo_write(%s) - vbo write failed.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    out_buffer_range_->allocation_size = tmp_alloc_handle.allocated_size;
    out_buffer_range_->draw_range.first_vertex_count = tmp_alloc_handle.byte_offset / sizeof(line_vertex_t);
    out_buffer_range_->draw_range.vertex_count = size_ / sizeof(line_vertex_t);

    ret = RENDERER_SUCCESS;

cleanup:
    // TODO: range_free_listの2-phase allocation完成後、ロールバックを追加する
    return ret;
}

renderer_result_t line_mesh_shader_vbo_free(line_mesh_shader_t* line_mesh_shader_, const vertex_buffer_range_t* buffer_range_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    buffer_manager_result_t ret_buff_mgr = BUFFER_MANAGER_INVALID_ARGUMENT;

    vertex_allocation_t alloc_info = { 0 };

    IF_ARG_FALSE_GOTO_CLEANUP(line_mesh_shader_is_initialized(line_mesh_shader_), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_write", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_range_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vbo_free", "buffer_range_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != buffer_range_->allocation_size, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vbo_free", "buffer_range_->allocation_size")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != buffer_range_->draw_range.vertex_count, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vbo_free", "buffer_range_->draw_range.vertex_count")

    if((SIZE_MAX / sizeof(line_vertex_t)) < buffer_range_->draw_range.first_vertex_count) {
        ret = RENDERER_OVERFLOW;
        ERROR_MESSAGE("line_mesh_shader_vbo_free(%s) - line_mesh_shader_vbo_free failed.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    alloc_info.allocated_size = buffer_range_->allocation_size;
    alloc_info.byte_offset = buffer_range_->draw_range.first_vertex_count * sizeof(line_vertex_t);

    ret_buff_mgr = vbo_manager_free(line_mesh_shader_->vbo_manager, &alloc_info);
    if(BUFFER_MANAGER_SUCCESS != ret_buff_mgr) {
        ret = RENDERER_RUNTIME_ERROR;   // TODO: buffer_managerの仕様が安定したら適切な実行結果コードに変換する
        ERROR_MESSAGE("line_mesh_shader_vbo_free(%s) - vbo free failed.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

// renderer_result_t line_mesh_shader_vertex_buffer_append(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, size_t size_, const line_vertex_t* write_data_, size_t* out_vertex_offset_) {
//     renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
//     size_t vertex_count = 0;
//     bool vbo_bound = false;

//     IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "backend_context_")
//     IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "line_mesh_shader_")
//     IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->line_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vertex_buffer_append", "line_vbo")
//     IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "write_data_")
//     IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "size_")
//     IF_ARG_FALSE_GOTO_CLEANUP(line_mesh_shader_->current_buffer_offset <= (SIZE_MAX - size_), ret, RENDERER_OVERFLOW, renderer_rslt_to_str(RENDERER_OVERFLOW), "line_mesh_shader_vertex_buffer_append", "size_")
//     IF_ARG_FALSE_GOTO_CLEANUP((line_mesh_shader_->current_buffer_offset + size_) <= line_mesh_shader_->vertex_buffer_size, ret, RENDERER_LIMIT_EXCEEDED, renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED), "line_mesh_shader_vertex_buffer_append", "size_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_vertex_offset_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "out_vertex_offset_")
//     IF_ARG_FALSE_GOTO_CLEANUP(0 == (size_ % (sizeof(line_vertex_t) * 2)), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vertex_buffer_append", "size_")

//     ret = renderer_backend_vertex_buffer_bind(backend_context_, line_mesh_shader_->line_vbo);
//     if(RENDERER_SUCCESS != ret) {
//         ERROR_MESSAGE("line_mesh_shader_vertex_buffer_append(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
//         goto cleanup;
//     }
//     vbo_bound = true;

//     ret = renderer_backend_vertex_buffer_vertex_subload(backend_context_, line_mesh_shader_->current_buffer_offset, size_, write_data_);
//     if(RENDERER_SUCCESS != ret) {
//         ERROR_MESSAGE("line_mesh_shader_vertex_buffer_append(%s) - Failed to write vertex data.", renderer_rslt_to_str(ret));
//         goto cleanup;
//     }

//     ret = renderer_backend_vertex_buffer_unbind(backend_context_);
//     if(RENDERER_SUCCESS != ret) {
//         ERROR_MESSAGE("line_mesh_shader_vertex_buffer_append(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
//         goto cleanup;
//     }

//     // NOTE: vertex_countは必ずsize_よりも小さいため、line_mesh_shader_->current_vertex_countのオーバーフローチェックは不要
//     vertex_count = size_ / sizeof(line_vertex_t);

//     *out_vertex_offset_ = line_mesh_shader_->current_vertex_count;
//     line_mesh_shader_->current_buffer_offset += size_;
//     line_mesh_shader_->current_vertex_count += vertex_count;

//     ret = RENDERER_SUCCESS;

// cleanup:
//     if(RENDERER_SUCCESS != ret) {
//         if(NULL != backend_context_ && vbo_bound) {
//             renderer_backend_vertex_buffer_unbind(backend_context_);
//         }
//     }
//     return ret;
// }

renderer_result_t line_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vao_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_vao_bind", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_vao_bind", "line_vao")

    ret = renderer_backend_vertex_array_bind(backend_context_, line_mesh_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_vao_bind(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

renderer_result_t line_mesh_shader_use(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_use", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_use", "line_mesh_shader_->shader")

    ret = renderer_backend_shader_use(backend_context_, line_mesh_shader_->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_use(%s) - Failed to switch program for line_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_model_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_model_matrix_set", "model_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_view_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_view_matrix_set", "view_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_projection_matrix_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_projection_matrix_set", "projection_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_mesh_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_mesh_shader_color_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const uint8_t color_[4]) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "line_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_mesh_shader_color_set", "line_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(color_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_mesh_shader_color_set", "color_")

    ret = renderer_backend_shader_vec4u8_uniform_set(backend_context_, line_mesh_shader_->color_location, color_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_shader_color_set(%s) - Failed to set color.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

static bool vbo_config_is_valid(const vbo_manager_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_node_count) {
        return false;
    }
    if(BUFFER_USAGE_DYNAMIC != config_->buffer_usage && BUFFER_USAGE_STATIC != config_->buffer_usage) {
        return false;
    }
    if(alignof(float) != config_->base_align) {
        return false;
    }
    return true;
}

static bool line_mesh_shader_is_initialized(const line_mesh_shader_t* line_mesh_shader_) {
    if(NULL == line_mesh_shader_) {
        return false;
    }
    if(NULL == line_mesh_shader_->shader) {
        return false;
    }
    if(NULL == line_mesh_shader_->line_vao) {
        return false;
    }
    if(NULL == line_mesh_shader_->vbo_manager) {
        return false;
    }
    return true;
}
