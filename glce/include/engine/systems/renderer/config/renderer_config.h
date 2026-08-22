#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_CONFIG_RENDERER_CONFIG_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_CONFIG_RENDERER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

typedef struct renderer_config {
    size_t line_mesh_shader_vbo_size;
    size_t line_mesh_shader_max_allocation_count;
    buffer_usage_t line_mesh_shader_buffer_usage;

    size_t lit_mesh_shader_vbo_size;
    size_t lit_mesh_shader_max_allocation_count;
    buffer_usage_t lit_mesh_shader_buffer_usage;

    size_t point_mesh_shader_vbo_size;
    size_t point_mesh_shader_max_allocation_count;
    buffer_usage_t point_mesh_shader_buffer_usage;

    size_t ui_mesh_shader_vbo_size;
    size_t ui_mesh_shader_max_allocation_count;
    buffer_usage_t ui_mesh_shader_buffer_usage;
} renderer_config_t;

bool renderer_config_is_valid(const renderer_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
