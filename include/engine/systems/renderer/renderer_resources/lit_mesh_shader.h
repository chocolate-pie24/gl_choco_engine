#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LIT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LIT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

// 光源・法線・材質色などを使って、陰影付きでmeshを描画するshader
typedef struct lit_mesh_shader lit_mesh_shader_t;

renderer_result_t lit_mesh_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, lit_mesh_shader_t** out_lit_mesh_shader_);

void lit_mesh_shader_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t** lit_mesh_shader_);

renderer_result_t lit_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_);

void lit_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

renderer_result_t lit_mesh_shader_vertex_buffer_vertex_write(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, size_t size_, const void* write_data_);

renderer_result_t lit_mesh_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

renderer_result_t lit_mesh_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

renderer_result_t lit_mesh_shader_use(const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t lit_mesh_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t lit_mesh_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_);

renderer_result_t lit_mesh_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, const lit_mesh_shader_t* lit_mesh_shader_, renderer_backend_context_t* backend_context_);

#ifdef __cplusplus
}
#endif
#endif
