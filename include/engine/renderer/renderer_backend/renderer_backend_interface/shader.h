#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

typedef void (*pfn_renderer_shader_preinit)(size_t* memory_requirement_, size_t* alignment_requirement_);

typedef renderer_result_t (*pfn_renderer_shader_init)(renderer_backend_shader_t* shader_handle_);

typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_compile)(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_use)(renderer_backend_shader_t* shader_handle_);

typedef struct renderer_shader_vtable {
    pfn_renderer_shader_preinit renderer_shader_preinit;
    pfn_renderer_shader_init renderer_shader_init;
    pfn_renderer_shader_destroy renderer_shader_destroy;
    pfn_renderer_shader_compile renderer_shader_compile;
    pfn_renderer_shader_link renderer_shader_link;
    pfn_renderer_shader_use renderer_shader_use;
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
