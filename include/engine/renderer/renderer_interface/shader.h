#ifndef GLCE_ENGINE_RENDERER_RENDERER_INTERFACE_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_INTERFACE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

typedef struct renderer_backend_shader renderer_backend_shader_t;

typedef renderer_result_t (*pfn_renderer_shader_create)(renderer_backend_shader_t** shader_handle_);

typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t** shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_compile)(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_use)(renderer_backend_shader_t* shader_handle_);

typedef struct renderer_shader_vtable {
    pfn_renderer_shader_create renderer_shader_create;
    pfn_renderer_shader_destroy renderer_shader_destroy;
    pfn_renderer_shader_compile renderer_shader_compile;
    pfn_renderer_shader_link renderer_shader_link;
    pfn_renderer_shader_use renderer_shader_use;
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
