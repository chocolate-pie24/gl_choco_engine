#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_POINT_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_POINT_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

typedef struct point_shader point_shader_t;   /**< ポイント描画シェーダーリソース構造体前方宣言 */

renderer_result_t point_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, point_shader_t** out_point_shader_);

void point_shader_destroy(renderer_backend_context_t* backend_context_, point_shader_t** point_shader_);

renderer_result_t point_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_, buffer_usage_t point_buffer_usage_, buffer_usage_t color_buffer_usage_, size_t point_buffer_size_, size_t color_buffer_size_);

void point_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_);

renderer_result_t point_shader_vertex_buffer_point_write(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_, size_t size_, const void* write_data_);

renderer_result_t point_shader_vertex_buffer_color_write(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_, size_t size_, const void* write_data_);

renderer_result_t point_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_);

renderer_result_t point_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, point_shader_t* point_shader_);

renderer_result_t point_shader_use(const point_shader_t* point_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t point_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const point_shader_t* point_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t point_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const point_shader_t* point_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t point_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, const point_shader_t* point_shader_, renderer_backend_context_t* backend_context_);

#ifdef __cplusplus
}
#endif
#endif
