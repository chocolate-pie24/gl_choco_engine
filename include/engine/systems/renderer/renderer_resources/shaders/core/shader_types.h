#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_CORE_SHADER_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_CORE_SHADER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

typedef struct draw_range {
    size_t first_vertex_count;
    size_t vertex_count;
} draw_range_t;

typedef struct vertex_buffer_range {
    draw_range_t draw_range;
    vertex_allocation_t allocation_info;
} vertex_buffer_range_t;

#ifdef __cplusplus
}
#endif
#endif
