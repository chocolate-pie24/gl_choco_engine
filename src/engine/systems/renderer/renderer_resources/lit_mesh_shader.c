/** @ingroup renderer
 *
 * @file lit_mesh_shader.c
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダーであるlit_meshシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIの実装
 *
 * @note ライティング等がまだ未実装なので、当面は単色描画となる
 *
 * @version 0.1
 * @date 2026-06-03
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/renderer_resources/lit_mesh_shader.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vbo.h"

#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_memory.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// TODO: テスト(lit_mesh_shaderは今後も拡張されるため、テストはまだ行わない)
// TODO: DYNAMIC / STATICでそれぞれVBOを作る
// TODO: vbo_config_t

struct lit_mesh_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */
    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */

    renderer_backend_vao_t* lit_mesh_vao;   /** シェーダーVAO */
    renderer_backend_vbo_t* lit_mesh_vbo;   /**< 頂点情報VBO(point_normal_vertex_t) */

    size_t vertex_buffer_size;              /**< バーテックスバッファのサイズ */
    size_t current_buffer_offset;           /**< 現在バーテックスバッファに転送されているサイズ(=次転送する際のオフセット) */
};

renderer_result_t lit_mesh_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, lit_mesh_shader_t** out_lit_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;

    lit_mesh_shader_t* tmp_lit_mesh_shader = NULL;

    fs_utils_t* frag_fs_utils = NULL;
    fs_utils_t* vert_fs_utils = NULL;
    choco_string_t* vert_shader_source = NULL;
    choco_string_t* frag_shader_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_create", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_create", "out_lit_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_create", "*out_lit_mesh_shader_")

    // シェーダーソース格納用choco_string生成
    ret_string = choco_string_default_create(&vert_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to create string for vert_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_string = choco_string_default_create(&frag_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to create string for frag_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソース読み込み用fs_utils生成
    ret_fs_utils = fs_utils_create(file_path_, name_, ".frag", FILESYSTEM_MODE_READ, &frag_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to create fs_utils for fragment_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_create(file_path_, name_, ".vert", FILESYSTEM_MODE_READ, &vert_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to create fs_utils for vertex_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムロード
    ret_fs_utils = fs_utils_text_file_read(frag_fs_utils, frag_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to read shader source(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_fs_utils = fs_utils_text_file_read(vert_fs_utils, vert_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to read shader source(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // lit mesh shader構造体インスタンス生成
    ret = renderer_mem_allocate(sizeof(lit_mesh_shader_t), (void**)&tmp_lit_mesh_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to allocate memory for tmp_lit_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_lit_mesh_shader->shader = NULL;
    tmp_lit_mesh_shader->lit_mesh_vao = NULL;
    tmp_lit_mesh_shader->lit_mesh_vbo = NULL;
    tmp_lit_mesh_shader->model_matrix_location = 0;
    tmp_lit_mesh_shader->view_matrix_location = 0;
    tmp_lit_mesh_shader->projection_matrix_location = 0;
    tmp_lit_mesh_shader->current_buffer_offset = 0;
    tmp_lit_mesh_shader->vertex_buffer_size = 0;

    // シェーダーモジュール生成
    ret = renderer_backend_shader_create(backend_context_, &tmp_lit_mesh_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to create shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーコンパイル / リンク
    ret = renderer_backend_shader_compile(SHADER_TYPE_VERTEX, choco_string_c_str(vert_shader_source), backend_context_, tmp_lit_mesh_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to compile shader object(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_compile(SHADER_TYPE_FRAGMENT, choco_string_c_str(frag_shader_source), backend_context_, tmp_lit_mesh_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to compile shader object(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_link(backend_context_, tmp_lit_mesh_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_lit_mesh_shader->shader, "g_model_matrix", &tmp_lit_mesh_shader->model_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to get model matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_lit_mesh_shader->shader, "g_view_matrix", &tmp_lit_mesh_shader->view_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to get view matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_lit_mesh_shader->shader, "g_projection_matrix", &tmp_lit_mesh_shader->projection_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_create(%s) - Failed to get projection matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    choco_string_destroy(&vert_shader_source);
    choco_string_destroy(&frag_shader_source);
    fs_utils_destroy(&vert_fs_utils);
    fs_utils_destroy(&frag_fs_utils);

    *out_lit_mesh_shader_ = tmp_lit_mesh_shader;
    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != vert_fs_utils) {
            fs_utils_destroy(&vert_fs_utils);
        }
        if(NULL != frag_fs_utils) {
            fs_utils_destroy(&frag_fs_utils);
        }
        if(NULL != frag_shader_source) {
            choco_string_destroy(&frag_shader_source);
        }
        if(NULL != vert_shader_source) {
            choco_string_destroy(&vert_shader_source);
        }
        if(NULL != tmp_lit_mesh_shader) {
            lit_mesh_shader_destroy(backend_context_, &tmp_lit_mesh_shader);
        }
    }
    return ret;
}

void lit_mesh_shader_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t** lit_mesh_shader_) {
    if(NULL == lit_mesh_shader_) {
        WARN_MESSAGE("lit_mesh_shader_destroy - Provided lit_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == *lit_mesh_shader_) {
        WARN_MESSAGE("lit_mesh_shader_destroy - Provided *lit_mesh_shader_ is not valid.");
        return;
    }
    if(NULL == backend_context_) {
        WARN_MESSAGE("lit_mesh_shader_destroy - Provided backend_context_ is not valid.");
        return;
    }
    lit_mesh_shader_vertex_buffer_destroy(backend_context_, *lit_mesh_shader_);
    if(NULL != (*lit_mesh_shader_)->shader) {
        renderer_backend_shader_destroy(backend_context_, &(*lit_mesh_shader_)->shader);
    }
    renderer_mem_free(*lit_mesh_shader_, sizeof(lit_mesh_shader_t));
    *lit_mesh_shader_ = NULL;
}

renderer_result_t lit_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    bool vao_created = false;
    bool vbo_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_create", "lit_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(lit_mesh_shader_->lit_mesh_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_buffer_create", "lit_mesh_vao")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(lit_mesh_shader_->lit_mesh_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_buffer_create", "lit_mesh_vbo")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == lit_mesh_shader_->current_buffer_offset, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_buffer_create", "current_buffer_offset")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != buffer_size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_create", "buffer_size_")

    ret = renderer_backend_vertex_array_create(backend_context_, &lit_mesh_shader_->lit_mesh_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to create lit mesh vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret = renderer_backend_vertex_buffer_create(backend_context_, &lit_mesh_shader_->lit_mesh_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to create lit mesh vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_created = true;

    ret = renderer_backend_vertex_array_bind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret = renderer_backend_vertex_buffer_bind(backend_context_, lit_mesh_shader_->lit_mesh_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to bind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    // attribute
    // position: vec3f_t 12byte
    // normal: vec4i8_t 4byte(x, y, z, padding)
    ret = renderer_backend_vertex_array_attribute_set(backend_context_, lit_mesh_shader_->lit_mesh_vao, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(point_normal_vertex_t), 0);  // 座標情報(layout = 0)
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to set vertex array attribute(position).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_array_attribute_set(backend_context_, lit_mesh_shader_->lit_mesh_vao, 1, 3, RENDERER_TYPE_BYTE, true, sizeof(point_normal_vertex_t), sizeof(float) * 3);    // 法線情報(layout = 1)
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to set vertex array attribute(normal).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_vertex_load(backend_context_, lit_mesh_shader_->lit_mesh_vbo, buffer_size_, 0, buffer_usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to create vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_array_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = renderer_backend_vertex_buffer_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_create(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    lit_mesh_shader_->vertex_buffer_size = buffer_size_;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(vbo_created) {
            if(vbo_bound) {
                renderer_backend_vertex_buffer_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vbo);
            }
            renderer_backend_vertex_buffer_destroy(backend_context_, &lit_mesh_shader_->lit_mesh_vbo);
        }
        if(vao_created) {
            if(vao_bound) {
                renderer_backend_vertex_array_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
            }
            renderer_backend_vertex_array_destroy(backend_context_, &lit_mesh_shader_->lit_mesh_vao);
        }
        if(NULL != lit_mesh_shader_) {
            lit_mesh_shader_->current_buffer_offset = 0;
            lit_mesh_shader_->vertex_buffer_size = 0;
        }
    }

    return ret;
}

void lit_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {
    if(NULL == backend_context_) {
        WARN_MESSAGE("lit_mesh_shader_vertex_buffer_destroy - Provided backend_context_ is not valid.");
        return;
    }
    if(NULL == lit_mesh_shader_) {
        WARN_MESSAGE("lit_mesh_shader_vertex_buffer_destroy - Provided lit_mesh_shader_ is not valid.");
        return;
    }
    if(NULL != lit_mesh_shader_->lit_mesh_vbo) {
        renderer_backend_vertex_buffer_destroy(backend_context_, &lit_mesh_shader_->lit_mesh_vbo);
    }
    if(NULL != lit_mesh_shader_->lit_mesh_vao) {
        renderer_backend_vertex_array_destroy(backend_context_, &lit_mesh_shader_->lit_mesh_vao);
    }
    lit_mesh_shader_->current_buffer_offset = 0;
    lit_mesh_shader_->vertex_buffer_size = 0;
}

renderer_result_t lit_mesh_shader_vertex_buffer_vertex_write(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, size_t size_, const void* write_data_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_write", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_write", "lit_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_->lit_mesh_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_buffer_write", "lit_mesh_vbo")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_write", "write_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_buffer_write", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(lit_mesh_shader_->current_buffer_offset <= (SIZE_MAX - size_), ret, RENDERER_LIMIT_EXCEEDED, renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED), "lit_mesh_shader_vertex_buffer_write", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP((lit_mesh_shader_->current_buffer_offset + size_) <= lit_mesh_shader_->vertex_buffer_size, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_buffer_write", "size_")

    // NOTE: VBOはこの中でbindされる
    ret = renderer_backend_vertex_buffer_vertex_subload(backend_context_, lit_mesh_shader_->lit_mesh_vbo, lit_mesh_shader_->current_buffer_offset, size_, write_data_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_write(%s) - Failed to write vertex data.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    lit_mesh_shader_->current_buffer_offset += size_;

    ret = renderer_backend_vertex_buffer_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_buffer_write(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && NULL != lit_mesh_shader_) {
            if(NULL != lit_mesh_shader_->lit_mesh_vbo) {
                renderer_backend_vertex_buffer_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vbo);
            }
        }
    }
    return ret;
}

renderer_result_t lit_mesh_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_array_bind", "lit_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_->lit_mesh_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_array_bind", "lit_mesh_vao")

    ret = renderer_backend_vertex_array_bind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_array_bind(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && NULL != lit_mesh_shader_ && NULL != lit_mesh_shader_->lit_mesh_vao) {
            renderer_backend_vertex_array_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
        }
    }
    return ret;
}

renderer_result_t lit_mesh_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_array_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_vertex_array_unbind", "lit_mesh_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_->lit_mesh_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "lit_mesh_shader_vertex_array_unbind", "lit_mesh_vao")

    ret = renderer_backend_vertex_array_unbind(backend_context_, lit_mesh_shader_->lit_mesh_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_vertex_array_unbind(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

renderer_result_t lit_mesh_shader_use(const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_use", "lit_mesh_shader_")

    ret = renderer_backend_shader_use(backend_context_, lit_mesh_shader_->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_use(%s) - Failed to switch program for lit_mesh_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t lit_mesh_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_model_matrix_set", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_model_matrix_set", "lit_mesh_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, lit_mesh_shader_->shader, lit_mesh_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t lit_mesh_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_view_matrix_set", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_view_matrix_set", "lit_mesh_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, lit_mesh_shader_->shader, lit_mesh_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t lit_mesh_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_projection_matrix_set", "projection_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "lit_mesh_shader_projection_matrix_set", "lit_mesh_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, lit_mesh_shader_->shader, lit_mesh_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
