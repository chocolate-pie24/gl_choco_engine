/** @ingroup renderer
 *
 * @file point_mesh_shader.c
 * @author chocolate-pie24
 * @brief ポイント描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIの実装
 *
 * @version 0.1
 * @date 2026-05-29
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/renderer_resources/shaders/point_mesh_shader.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_memory.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_program_builder.h"

// TODO: テスト(point_mesh_shaderは今後も拡張されるため、テストはまだ行わない)
// TODO: DYNAMIC / STATICでそれぞれVBOを作る
// TODO: vbo_config_t

/**
 * @brief ポイント描画用シェーダーリソース構造体
 * @note 本構造体はshader programだけでなく、ポイント描画用のVAO/VBOとバッファ書き込み状態も保持する
 * @note colorのvboは別にする。vertexには色情報は持たせたくない。テクスチャマテリアルはvertexでは親構造体に持たせるため、それに倣って色は親に持たせる。
 * @todo TODO: FreeListを使用したバッファ管理
 *
 *
 */
struct point_mesh_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */

    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */

    renderer_backend_vao_t* point_vao;      /**< ポイント描画シェーダーVAO */
    renderer_backend_vbo_t* point_vbo;      /**< ポイント描画シェーダー頂点情報VBO */
    renderer_backend_vbo_t* color_vbo;      /**< ポイント描画シェーダー色情報VBO */

    size_t point_vertex_buffer_size;        /**< 頂点情報バーテックスバッファサイズ */
    size_t point_current_buffer_offset;     /**< 現在頂点情報バーテックスバッファに転送されているサイズ(=次転送する際のオフセット) */

    size_t color_vertex_buffer_size;        /**< 色情報バーテックスバッファサイズ */
    size_t color_current_buffer_offset;     /**< 現在色情報バーテックスバッファに転送されているサイズ(=次転送する際のオフセット) */

    size_t current_vertex_count;            /**< 現在バーテックスバッファに転送されている頂点数 */
};

renderer_result_t point_mesh_shader_create(point_mesh_shader_t** out_point_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    point_mesh_shader_t* tmp_point_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_create", "out_point_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_create", "*out_point_mesh_shader_")

    // point shader構造体インスタンス生成
    ret = renderer_mem_allocate(sizeof(point_mesh_shader_t), (void**)&tmp_point_mesh_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_create(%s) - Failed to allocate memory for tmp_point_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_point_mesh_shader->shader = NULL;
    tmp_point_mesh_shader->point_vao = NULL;
    tmp_point_mesh_shader->point_vbo = NULL;
    tmp_point_mesh_shader->color_vbo = NULL;
    tmp_point_mesh_shader->model_matrix_location = 0;
    tmp_point_mesh_shader->view_matrix_location = 0;
    tmp_point_mesh_shader->projection_matrix_location = 0;
    tmp_point_mesh_shader->point_current_buffer_offset = 0;
    tmp_point_mesh_shader->point_vertex_buffer_size = 0;
    tmp_point_mesh_shader->color_current_buffer_offset = 0;
    tmp_point_mesh_shader->color_vertex_buffer_size = 0;
    tmp_point_mesh_shader->current_vertex_count = 0;

    *out_point_mesh_shader_ = tmp_point_mesh_shader;

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

void point_mesh_shader_destroy(renderer_backend_context_t* backend_context_, point_mesh_shader_t** point_mesh_shader_) {
    if(NULL == point_mesh_shader_) {
        WARN_MESSAGE("point_mesh_shader_destroy - Provided point_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == *point_mesh_shader_) {
        WARN_MESSAGE("point_mesh_shader_destroy - Provided *point_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == backend_context_) {
        WARN_MESSAGE("point_mesh_shader_destroy - Provided backend_context_ is not valid.");
        return;
    }
    point_mesh_shader_vertex_buffer_destroy(backend_context_, *point_mesh_shader_);
    if(NULL != (*point_mesh_shader_)->shader) {
        renderer_backend_shader_destroy(backend_context_, &(*point_mesh_shader_)->shader);
    }
    renderer_mem_free(*point_mesh_shader_, sizeof(point_mesh_shader_t));
    *point_mesh_shader_ = NULL;
}

renderer_result_t point_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, const char* file_path_, const char* name_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp_shader = NULL;
    int32_t tmp_model_matrix_location = 0;
    int32_t tmp_view_matrix_location = 0;
    int32_t tmp_projection_matrix_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_program_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_program_initialize", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_program_initialize", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_program_initialize", "name_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(point_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_program_initialize", "point_mesh_shader_->shader")

    // シェーダープログラムビルド
    ret = shader_program_builder_create_from_files(backend_context_, file_path_, name_, &tmp_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_program_initialize(%s) - Failed to build shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_model_matrix", &tmp_model_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_program_initialize(%s) - Failed to get model matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_view_matrix", &tmp_view_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_program_initialize(%s) - Failed to get view matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_shader, "g_projection_matrix", &tmp_projection_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_program_initialize(%s) - Failed to get projection matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    point_mesh_shader_->model_matrix_location = tmp_model_matrix_location;
    point_mesh_shader_->view_matrix_location = tmp_view_matrix_location;
    point_mesh_shader_->projection_matrix_location = tmp_projection_matrix_location;
    point_mesh_shader_->shader = tmp_shader;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret && NULL != tmp_shader) {
        renderer_backend_shader_destroy(backend_context_, &tmp_shader);
    }
    return ret;
}

renderer_result_t point_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, buffer_usage_t point_buffer_usage_, buffer_usage_t color_buffer_usage_, size_t point_buffer_size_, size_t color_buffer_size_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    bool vao_created = false;
    bool point_vbo_created = false;
    bool color_vbo_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_create", "point_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(point_mesh_shader_->point_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "point_vao")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(point_mesh_shader_->point_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "point_vbo")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(point_mesh_shader_->color_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "color_vbo")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == point_mesh_shader_->point_current_buffer_offset, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "point_current_buffer_offset")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == point_mesh_shader_->color_current_buffer_offset, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "color_current_buffer_offset")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == point_mesh_shader_->current_vertex_count, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_create", "current_vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != point_buffer_size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_create", "point_buffer_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != color_buffer_size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_create", "color_buffer_size_")

    ret = renderer_backend_vertex_array_create(backend_context_, &point_mesh_shader_->point_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to create point vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret = renderer_backend_vertex_buffer_create(backend_context_, &point_mesh_shader_->point_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to create point vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    point_vbo_created = true;

    ret = renderer_backend_vertex_buffer_create(backend_context_, &point_mesh_shader_->color_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to create color vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    color_vbo_created = true;

    ret = renderer_backend_vertex_array_bind(backend_context_, point_mesh_shader_->point_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    // point VBO
    ret = renderer_backend_vertex_buffer_bind(backend_context_, point_mesh_shader_->point_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to bind vertex buffer(point).", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_array_attribute_set(backend_context_, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3, 0);  // 頂点座標(layout = 0)
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to set vertex array attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_vertex_load(backend_context_, point_buffer_size_, 0, point_buffer_usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to create vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // color VBO
    ret = renderer_backend_vertex_buffer_bind(backend_context_, point_mesh_shader_->color_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to bind vertex buffer(color).", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_array_attribute_set(backend_context_, 1, 4, RENDERER_TYPE_UNSIGNED_BYTE, true, sizeof(uint8_t) * 4, 0);  // 色情報(layout = 1), OpenGLで正規化
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to set vertex array attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_vertex_load(backend_context_, color_buffer_size_, 0, color_buffer_usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to create vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_array_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_create(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    point_mesh_shader_->point_vertex_buffer_size = point_buffer_size_;
    point_mesh_shader_->color_vertex_buffer_size = color_buffer_size_;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(vbo_bound) {
            renderer_backend_vertex_buffer_unbind(backend_context_);
        }
        if(point_vbo_created) {
            renderer_backend_vertex_buffer_destroy(backend_context_, &point_mesh_shader_->point_vbo);
        }
        if(color_vbo_created) {
            renderer_backend_vertex_buffer_destroy(backend_context_, &point_mesh_shader_->color_vbo);
        }
        if(vao_created) {
            if(vao_bound) {
                renderer_backend_vertex_array_unbind(backend_context_);
            }
            renderer_backend_vertex_array_destroy(backend_context_, &point_mesh_shader_->point_vao);
        }
        if(NULL != point_mesh_shader_) {
            point_mesh_shader_->point_current_buffer_offset = 0;
            point_mesh_shader_->point_vertex_buffer_size = 0;

            point_mesh_shader_->color_current_buffer_offset = 0;
            point_mesh_shader_->color_vertex_buffer_size = 0;
        }
    }

    return ret;
}

void point_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_) {
    if(NULL == backend_context_) {
        WARN_MESSAGE("point_mesh_shader_vertex_buffer_destroy - Provided backend_context_ is not valid.");
        return;
    }
    if(NULL == point_mesh_shader_) {
        WARN_MESSAGE("point_mesh_shader_vertex_buffer_destroy - Provided point_mesh_shader_ is not valid.");
        return;
    }
    if(NULL != point_mesh_shader_->point_vbo) {
        renderer_backend_vertex_buffer_destroy(backend_context_, &point_mesh_shader_->point_vbo);
    }
    if(NULL != point_mesh_shader_->color_vbo) {
        renderer_backend_vertex_buffer_destroy(backend_context_, &point_mesh_shader_->color_vbo);
    }
    if(NULL != point_mesh_shader_->point_vao) {
        renderer_backend_vertex_array_destroy(backend_context_, &point_mesh_shader_->point_vao);
    }
    point_mesh_shader_->point_current_buffer_offset = 0;
    point_mesh_shader_->point_vertex_buffer_size = 0;

    point_mesh_shader_->color_current_buffer_offset = 0;
    point_mesh_shader_->color_vertex_buffer_size = 0;

    point_mesh_shader_->current_vertex_count = 0;
}

renderer_result_t point_mesh_shader_vertex_buffer_point_append(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const point_vertex_t* write_data_, size_t* out_vertex_offset_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    size_t vertex_count = 0;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->point_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_point_append", "point_vbo")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "write_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(point_mesh_shader_->point_current_buffer_offset <= (SIZE_MAX - size_), ret, RENDERER_OVERFLOW, renderer_rslt_to_str(RENDERER_OVERFLOW), "point_mesh_shader_vertex_buffer_point_append", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP((point_mesh_shader_->point_current_buffer_offset + size_) <= point_mesh_shader_->point_vertex_buffer_size, ret, RENDERER_LIMIT_EXCEEDED, renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED), "point_mesh_shader_vertex_buffer_point_append", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_offset_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "out_vertex_offset_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (size_ % sizeof(point_vertex_t)), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_point_append", "size_")

    ret = renderer_backend_vertex_buffer_bind(backend_context_, point_mesh_shader_->point_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_point_append(%s) - Failed to bind point vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_buffer_vertex_subload(backend_context_, point_mesh_shader_->point_current_buffer_offset, size_, write_data_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_point_append(%s) - Failed to write vertex data.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_point_append(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // NOTE: vertex_countは必ずsize_よりも小さいため、point_mesh_shader_->current_vertex_countのオーバーフローチェックは不要
    vertex_count = size_ / sizeof(point_vertex_t);
    *out_vertex_offset_ = point_mesh_shader_->current_vertex_count;
    point_mesh_shader_->point_current_buffer_offset += size_;
    point_mesh_shader_->current_vertex_count += vertex_count;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && vbo_bound) {
            renderer_backend_vertex_buffer_unbind(backend_context_);
        }
    }
    return ret;
}

renderer_result_t point_mesh_shader_vertex_buffer_color_append(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const vec4u8_t* write_data_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_color_append", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_color_append", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->color_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_color_append", "color_vbo")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_color_append", "write_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_color_append", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(point_mesh_shader_->color_current_buffer_offset <= (SIZE_MAX - size_), ret, RENDERER_LIMIT_EXCEEDED, renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED), "point_mesh_shader_vertex_buffer_color_append", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP((point_mesh_shader_->color_current_buffer_offset + size_) <= point_mesh_shader_->color_vertex_buffer_size, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_buffer_color_append", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (size_ % sizeof(vec4u8_t)), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_buffer_color_append", "size_")

    ret = renderer_backend_vertex_buffer_bind(backend_context_, point_mesh_shader_->color_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_color_append(%s) - Failed to bind color vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_buffer_vertex_subload(backend_context_, point_mesh_shader_->color_current_buffer_offset, size_, write_data_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_color_append(%s) - Failed to write color data.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_buffer_color_append(%s) - Failed to unbind color buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    point_mesh_shader_->color_current_buffer_offset += size_;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && vbo_bound) {
            renderer_backend_vertex_buffer_unbind(backend_context_);
        }
    }
    return ret;
}

renderer_result_t point_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_vertex_array_bind", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->point_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_vertex_array_bind", "point_vao")

    ret = renderer_backend_vertex_array_bind(backend_context_, point_mesh_shader_->point_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_vertex_array_bind(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

renderer_result_t point_mesh_shader_use(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_use", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_use", "point_mesh_shader_->shader")

    ret = renderer_backend_shader_use(backend_context_, point_mesh_shader_->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_use(%s) - Failed to switch program for point_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t point_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_model_matrix_set", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_model_matrix_set", "point_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_model_matrix_set", "model_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, point_mesh_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t point_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_view_matrix_set", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_view_matrix_set", "point_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_view_matrix_set", "view_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, point_mesh_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t point_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_projection_matrix_set", "point_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(point_mesh_shader_->shader, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "point_mesh_shader_projection_matrix_set", "point_mesh_shader_->shader")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "point_mesh_shader_projection_matrix_set", "projection_matrix_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, point_mesh_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
