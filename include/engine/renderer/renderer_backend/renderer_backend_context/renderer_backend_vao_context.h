#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VAO_CONTEXT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VAO_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;

renderer_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

renderer_result_t renderer_backend_vertex_array_bind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_);

renderer_result_t renderer_backend_vertex_array_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_);

renderer_result_t renderer_backend_vertex_array_attribute_set(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

#ifdef __cplusplus
}
#endif
#endif
