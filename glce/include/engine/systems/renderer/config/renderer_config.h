#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_CONFIG_RENDERER_CONFIG_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_CONFIG_RENDERER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/core/renderer_types.h"

typedef struct line_mesh_shader_config {
    size_t vbo_size;
    size_t max_allocation_count;
    buffer_usage_t buffer_usage;
} line_mesh_shader_config_t;

typedef struct lit_mesh_shader_config {
    size_t vbo_size;
    size_t max_allocation_count;
    buffer_usage_t buffer_usage;
} lit_mesh_shader_config_t;

typedef struct point_mesh_shader_config {
    size_t vbo_size;
    size_t max_allocation_count;
    buffer_usage_t buffer_usage;
} point_mesh_shader_config_t;

typedef struct ui_mesh_shader_config {
    size_t vbo_size;
    size_t max_allocation_count;
    buffer_usage_t buffer_usage;
} ui_mesh_shader_config_t;

typedef struct renderer_config {
    line_mesh_shader_config_t line_mesh_shader_config;
    lit_mesh_shader_config_t lit_mesh_shader_config;
    point_mesh_shader_config_t point_mesh_shader_config;
    ui_mesh_shader_config_t ui_mesh_shader_config;
} renderer_config_t;

bool line_mesh_shader_config_is_valid(const line_mesh_shader_config_t* config_);

bool lit_mesh_shader_config_is_valid(const lit_mesh_shader_config_t* config_);

bool point_mesh_shader_config_is_valid(const point_mesh_shader_config_t* config_);

bool ui_mesh_shader_config_is_valid(const ui_mesh_shader_config_t* config_);

bool renderer_config_is_valid(const renderer_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
