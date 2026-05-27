#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LINE_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LINE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

typedef struct line_shader line_shader_t;   /**< 線分描画シェーダーリソース構造体前方宣言 */

renderer_result_t line_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, line_shader_t** out_line_shader_);

void line_shader_destroy(renderer_backend_context_t* backend_context_, line_shader_t** line_shader_);

renderer_result_t line_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_);

void line_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

renderer_result_t line_shader_vertex_buffer_write(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, size_t size_, void* write_data_);

renderer_result_t line_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

renderer_result_t line_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

renderer_result_t line_shader_use(const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t line_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t line_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t line_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t line_shader_color_set(const uint8_t color_[4], line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

#ifdef __cplusplus
}
#endif
#endif
