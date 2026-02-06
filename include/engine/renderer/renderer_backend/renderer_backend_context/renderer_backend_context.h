#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct renderer_backend_context renderer_backend_context_t;

renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_);

void renderer_backend_destroy(renderer_backend_context_t* renderer_backend_context_);

#ifdef __cplusplus
}
#endif
#endif
