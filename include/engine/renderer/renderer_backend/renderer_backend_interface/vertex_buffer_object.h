#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_VERTEX_BUFFER_OBJECT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_VERTEX_BUFFER_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

typedef renderer_result_t (*pfn_vertex_buffer_create)(renderer_backend_vbo_t** vertex_buffer_);

typedef void (*pfn_vertex_buffer_destroy)(renderer_backend_vbo_t** vertex_buffer_);

typedef renderer_result_t (*pfn_vertex_buffer_bind)(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_);

typedef renderer_result_t (*pfn_vertex_buffer_unbind)(const renderer_backend_vbo_t* vertex_buffer_);

typedef renderer_result_t (*pfn_vertex_buffer_vertex_load)(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

typedef struct renderer_vbo_vtable {
    pfn_vertex_buffer_create vertex_buffer_create;
    pfn_vertex_buffer_destroy vertex_buffer_destroy;
    pfn_vertex_buffer_bind vertex_buffer_bind;
    pfn_vertex_buffer_unbind vertex_buffer_unbind;
    pfn_vertex_buffer_vertex_load vertex_buffer_vertex_load;
} renderer_vbo_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
