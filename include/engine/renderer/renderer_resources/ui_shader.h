#ifndef GLCE_ENGINE_RENDERER_RESOURCES_UI_SHADER_H
#define GLCE_ENGINE_RENDERER_RESOURCES_UI_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/context.h"

typedef struct ui_shader ui_shader_t;

renderer_result_t ui_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, ui_shader_t** out_ui_shader_);

void ui_shader_destroy(renderer_backend_context_t* backend_context_, ui_shader_t** ui_shader_);

renderer_result_t ui_shader_use(renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

renderer_result_t ui_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

renderer_result_t ui_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

renderer_result_t ui_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

#ifdef __cplusplus
}
#endif
#endif
