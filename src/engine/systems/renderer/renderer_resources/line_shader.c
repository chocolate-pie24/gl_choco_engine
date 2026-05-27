#include <stdint.h>

#include "engine/systems/renderer/renderer_resources/line_shader.h"

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

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// TODO: テスト(line_shaderは今後も拡張されるため、テストはまだ行わない)

struct line_shader {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */
    int32_t color_location;                 /**< 色情報のユニフォーム変数Location */

    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */

    renderer_backend_vao_t* line_vao;       /**< 線分描画シェーダー用VAO */
    renderer_backend_vbo_t* line_vbo;       /**< 線分描画シェーダー用VBO */

    size_t vertex_buffer_size;              /**< バーテックスバッファのサイズ */
    size_t current_buffer_offset;           /**< 現在バーテックスバッファ転送されているサイズ(=次転送する際のオフセット) */
};

renderer_result_t line_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, line_shader_t** out_line_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;

    line_shader_t* tmp_line_shader = NULL;

    fs_utils_t* frag_fs_utils = NULL;
    fs_utils_t* vert_fs_utils = NULL;
    choco_string_t* vert_shader_source = NULL;
    choco_string_t* frag_shader_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_create", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_create", "out_line_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_create", "*out_line_shader_")

    // シェーダーソース格納用choco_string生成
    ret_string = choco_string_default_create(&vert_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to create string for vert_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_string = choco_string_default_create(&frag_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to create string for frag_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソース読み込み用fs_utils生成
    ret_fs_utils = fs_utils_create(file_path_, name_, ".frag", FILESYSTEM_MODE_READ, &frag_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to create fs_utils for fragment_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_create(file_path_, name_, ".vert", FILESYSTEM_MODE_READ, &vert_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to create fs_utils for vertex_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムロード
    ret_fs_utils = fs_utils_text_file_read(frag_fs_utils, frag_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to read shader source(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_fs_utils = fs_utils_text_file_read(vert_fs_utils, vert_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("line_shader_create(%s) - Failed to read shader source(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // line shader構造体インスタンス生成
    ret = renderer_mem_allocate(sizeof(line_shader_t), (void**)&tmp_line_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to allocate memory for tmp_line_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_line_shader->shader = NULL;
    tmp_line_shader->line_vao = NULL;
    tmp_line_shader->line_vbo = NULL;
    tmp_line_shader->model_matrix_location = 0;
    tmp_line_shader->view_matrix_location = 0;
    tmp_line_shader->projection_matrix_location = 0;
    tmp_line_shader->color_location = 0;
    tmp_line_shader->current_buffer_offset = 0;
    tmp_line_shader->vertex_buffer_size = 0;

    // シェーダーモジュール生成
    ret = renderer_backend_shader_create(backend_context_, &tmp_line_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to create shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーコンパイル / リンク
    ret = renderer_backend_shader_compile(SHADER_TYPE_VERTEX, choco_string_c_str(vert_shader_source), backend_context_, tmp_line_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to compile shader object(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_compile(SHADER_TYPE_FRAGMENT, choco_string_c_str(frag_shader_source), backend_context_, tmp_line_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to compile shader object(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_link(backend_context_, tmp_line_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // uniform location
    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_line_shader->shader, "g_model_matrix", &tmp_line_shader->model_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to get model matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_line_shader->shader, "g_view_matrix", &tmp_line_shader->view_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to get view matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_line_shader->shader, "g_projection_matrix", &tmp_line_shader->projection_matrix_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to get projection matrix location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_uniform_location_get(backend_context_, tmp_line_shader->shader, "g_line_color", &tmp_line_shader->color_location);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_create(%s) - Failed to get color location.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    choco_string_destroy(&vert_shader_source);
    choco_string_destroy(&frag_shader_source);
    fs_utils_destroy(&vert_fs_utils);
    fs_utils_destroy(&frag_fs_utils);

    *out_line_shader_ = tmp_line_shader;
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
        if(NULL != tmp_line_shader) {
            line_shader_destroy(backend_context_, &tmp_line_shader);
        }
    }
    return ret;
}

void line_shader_destroy(renderer_backend_context_t* backend_context_, line_shader_t** line_shader_) {
    if(NULL == line_shader_) {
        return;
    }
    if(NULL == *line_shader_) {
        return;
    }
    if(NULL == backend_context_) {
        return;
    }
    line_shader_vertex_buffer_destroy(backend_context_, *line_shader_);
    if(NULL != (*line_shader_)->shader) {
        renderer_backend_shader_destroy(backend_context_, &(*line_shader_)->shader);
    }
    renderer_mem_free(*line_shader_, sizeof(line_shader_t));
    *line_shader_ = NULL;
}

renderer_result_t line_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    bool vao_created = false;
    bool vbo_created = false;
    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_create", "line_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_buffer_create", "line_vao")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(line_shader_->line_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_buffer_create", "line_vbo")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == line_shader_->current_buffer_offset, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_buffer_create", "current_buffer_offset")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != buffer_size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_create", "buffer_size_")

    ret = renderer_backend_vertex_array_create(backend_context_, &line_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to create line vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_created = true;

    ret = renderer_backend_vertex_buffer_create(backend_context_, &line_shader_->line_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to create line vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_created = true;

    ret = renderer_backend_vertex_array_bind(backend_context_, line_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret = renderer_backend_vertex_buffer_bind(backend_context_, line_shader_->line_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to bind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret = renderer_backend_vertex_array_attribute_set(backend_context_, line_shader_->line_vao, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3, 0);  // 頂点座標(layout = 0)
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to set vertex array attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_buffer_vertex_load(backend_context_, line_shader_->line_vbo, buffer_size_, 0, buffer_usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to create vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_vertex_array_unbind(backend_context_, line_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = renderer_backend_vertex_buffer_unbind(backend_context_, line_shader_->line_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_create(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    line_shader_->vertex_buffer_size = buffer_size_;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(vbo_created) {
            if(vbo_bound) {
                renderer_backend_vertex_buffer_unbind(backend_context_, line_shader_->line_vbo);
            }
            renderer_backend_vertex_buffer_destroy(backend_context_, &line_shader_->line_vbo);
        }
        if(vao_created) {
            if(vao_bound) {
                renderer_backend_vertex_array_unbind(backend_context_, line_shader_->line_vao);
            }
            renderer_backend_vertex_array_destroy(backend_context_, &line_shader_->line_vao);
        }
        if(NULL != line_shader_) {
            line_shader_->current_buffer_offset = 0;
            line_shader_->vertex_buffer_size = 0;
        }
    }

    return ret;
}

void line_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_) {
    if(NULL == backend_context_) {
        return;
    }
    if(NULL == line_shader_) {
        return;
    }
    if(NULL != line_shader_->line_vbo) {
        renderer_backend_vertex_buffer_destroy(backend_context_, &line_shader_->line_vbo);
    }
    if(NULL != line_shader_->line_vao) {
        renderer_backend_vertex_array_destroy(backend_context_, &line_shader_->line_vao);
    }
    line_shader_->current_buffer_offset = 0;
    line_shader_->vertex_buffer_size = 0;
}

renderer_result_t line_shader_vertex_buffer_write(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, size_t size_, void* write_data_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_write", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_write", "line_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_->line_vbo, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_buffer_write", "line_vbo")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_write", "write_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_buffer_write", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP(line_shader_->current_buffer_offset <= (SIZE_MAX - size_), ret, RENDERER_LIMIT_EXCEEDED, renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED), "line_shader_vertex_buffer_write", "size_")
    IF_ARG_FALSE_GOTO_CLEANUP((line_shader_->current_buffer_offset + size_) <= line_shader_->vertex_buffer_size, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_buffer_write", "size_")

    // NOTE: VBOはこの中でbindされる
    ret = renderer_backend_vertex_buffer_vertex_subload(backend_context_, line_shader_->line_vbo, line_shader_->current_buffer_offset, size_, write_data_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_write(%s) - Failed to write vertex data.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    line_shader_->current_buffer_offset += size_;

    ret = renderer_backend_vertex_buffer_unbind(backend_context_, line_shader_->line_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_buffer_write(%s) - Failed to unbind vertex buffer.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && NULL != line_shader_) {
            if(NULL != line_shader_->line_vbo) {
                renderer_backend_vertex_buffer_unbind(backend_context_, line_shader_->line_vbo);
            }
        }
    }
    return ret;
}

renderer_result_t line_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_array_bind", "line_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_array_bind", "line_vao")

    ret = renderer_backend_vertex_array_bind(backend_context_, line_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_array_bind(%s) - Failed to bind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != backend_context_ && NULL != line_shader_ && NULL != line_shader_->line_vao) {
            renderer_backend_vertex_array_unbind(backend_context_, line_shader_->line_vao);
        }
    }
    return ret;
}

renderer_result_t line_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_array_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_vertex_array_unbind", "line_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_->line_vao, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "line_shader_vertex_array_unbind", "line_vao")

    ret = renderer_backend_vertex_array_unbind(backend_context_, line_shader_->line_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_vertex_array_unbind(%s) - Failed to unbind vertex array.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

renderer_result_t line_shader_use(const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_use", "line_shader_")

    ret = renderer_backend_shader_use(backend_context_, line_shader_->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_use(%s) - Failed to switch program for line_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_model_matrix_set", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_model_matrix_set", "line_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_shader_->shader, line_shader_->model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_model_matrix_set(%s) - Failed to set model matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_view_matrix_set", "view_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_view_matrix_set", "line_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_shader_->shader, line_shader_->view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_view_matrix_set(%s) - Failed to set view matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, line_shader_t* line_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_projection_matrix_set", "projection_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_projection_matrix_set", "line_shader_")

    ret = renderer_backend_shader_mat4f_uniform_set(backend_context_, line_shader_->shader, line_shader_->projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_projection_matrix_set(%s) - Failed to set projection matrix.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t line_shader_color_set(const uint8_t color_[4], line_shader_t* line_shader_, renderer_backend_context_t* backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(color_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_color_set", "color_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_color_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(line_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "line_shader_color_set", "line_shader_")

    ret = renderer_backend_shader_vec4u8_uniform_set(backend_context_, line_shader_->shader, line_shader_->color_location, color_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("line_shader_color_set(%s) - Failed to set color.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
