#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

typedef struct vbo_manager_config {
    size_t vbo_size;
    size_t max_node_count;
    size_t base_align;
} vbo_manager_config_t;

typedef struct vbo_manager vbo_manager_t;

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_);

void vbo_manager_destroy(renderer_backend_context_t* backend_context_, vbo_manager_t** vbo_manager_);

#ifdef __cplusplus
}
#endif
#endif
