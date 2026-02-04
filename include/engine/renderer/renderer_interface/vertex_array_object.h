#ifndef GLCE_ENGINE_RENDERER_RENDERER_INTERFACE_VERTEX_ARRAY_OBJECT_H
#define GLCE_ENGINE_RENDERER_RENDERER_INTERFACE_VERTEX_ARRAY_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_core/renderer_types.h"

typedef struct renderer_backend_vao renderer_backend_vao_t;

typedef renderer_result_t (*pfn_vertex_array_create)(renderer_backend_vao_t** vertex_array_);

typedef void (*pfn_vertex_array_destroy)(renderer_backend_vao_t** vertex_array_);

typedef renderer_result_t (*pfn_vertex_array_bind)(const renderer_backend_vao_t* vertex_array_);

typedef renderer_result_t (*pfn_vertex_array_unbind)(void);

typedef renderer_result_t (*pfn_vertex_array_attribute_set)(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

typedef struct renderer_vao_vtable {
    pfn_vertex_array_create vertex_array_create;
    pfn_vertex_array_destroy vertex_array_destroy;
    pfn_vertex_array_bind vertex_array_bind;
    pfn_vertex_array_unbind vertex_array_unbind;
    pfn_vertex_array_attribute_set vertex_array_attribute_set;
} renderer_vao_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
