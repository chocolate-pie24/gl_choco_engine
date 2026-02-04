#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct renderer_backend_context renderer_backend_context_t;

renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_);

void renderer_backend_destroy(renderer_backend_context_t* renderer_backend_context_);

renderer_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_vertex_array_bind(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_vertex_array_unbind(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_vertex_array_attribute_set(renderer_backend_context_t* backend_context_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

renderer_result_t renderer_backend_vertex_buffer_bind(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_vertex_buffer_unbind(renderer_backend_context_t* backend_context_);

renderer_result_t renderer_backend_vertex_buffer_vertex_load(renderer_backend_context_t* backend_context_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

#ifdef __cplusplus
}
#endif
#endif
