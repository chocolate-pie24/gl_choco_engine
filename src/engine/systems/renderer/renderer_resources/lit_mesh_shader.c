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

    renderer_backend_vao_t* lit_mesh_vao;
    renderer_backend_vbo_t* lit_mesh_vbo;

    size_t vertex_buffer_size;              /**< バーテックスバッファのサイズ */
    size_t current_buffer_offset;           /**< 現在バーテックスバッファに転送されているサイズ(=次転送する際のオフセット) */
};

renderer_result_t lit_mesh_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, lit_mesh_shader_t** out_lit_mesh_shader_) {
    return RENDERER_SUCCESS;
}

void lit_mesh_shader_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t** lit_mesh_shader_) {

}

renderer_result_t lit_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_) {
    return RENDERER_SUCCESS;
}

void lit_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {

}

renderer_result_t lit_mesh_shader_vertex_buffer_vertex_write(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, size_t size_, const void* write_data_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_use(const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    return RENDERER_SUCCESS;
}

renderer_result_t lit_mesh_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_) {
    return RENDERER_SUCCESS;
}
