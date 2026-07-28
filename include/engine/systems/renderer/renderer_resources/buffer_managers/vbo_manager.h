#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/renderer_core/renderer_types.h"
#include "engine/systems/renderer/renderer_core/allocators/range_allocator.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

typedef struct vbo_manager_config {
    size_t vbo_size;
    size_t max_allocation_count;
    size_t base_align;
    buffer_usage_t buffer_usage;
} vbo_manager_config_t;

// NOTE: 将来のvbo_managerの拡張(vbo_pool, pageの導入)を見据え、range_allocation_tとは別に構造体を用意する
typedef struct vertex_allocation {
    range_allocation_t range_allocation;
} vertex_allocation_t;

typedef struct vbo_manager vbo_manager_t;                           /**< VBO Managerのopaque型 */
typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_);

void vbo_manager_destroy(vbo_manager_t** vbo_manager_, renderer_backend_context_t* backend_context_);

buffer_manager_result_t vbo_manager_write(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_, size_t size_, const void* write_data_, vertex_allocation_t* out_allocation_handle_);

buffer_manager_result_t vbo_manager_free(vbo_manager_t* vbo_manager_, const vertex_allocation_t* allocation_handle_);

buffer_manager_result_t vbo_manager_bind(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_);

buffer_manager_result_t vbo_manager_unbind(const renderer_backend_context_t* backend_context_);

void vbo_manager_status_print(const vbo_manager_t* vbo_manager_);

void vbo_manager_debug_print(const vbo_manager_t* vbo_manager_);

#ifdef __cplusplus
}
#endif
#endif
