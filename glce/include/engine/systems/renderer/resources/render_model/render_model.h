// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

typedef struct model_transform_tree model_transform_tree_t;

typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;

typedef struct render_model_coordinate_config {
    uint16_t coordinate_id;
    uint16_t parent_coordinate_id;

    vec3f_t translate;
    vec3f_t rotation;
} render_model_coordinate_config_t;

typedef struct line_mesh_bind_config {
    const char* mesh_name;
    const char* geometry_resource_name;
    vec4u8_t line_color;

    int16_t coordinate_id;
} line_mesh_bind_config_t;

typedef struct point_mesh_bind_config {
    const char* mesh_name;
    const char* geometry_resource_name;

    int16_t coordinate_id;
} point_mesh_bind_config_t;

typedef struct ui_mesh_bind_config {
    const char* mesh_name;
    const char* geometry_resource_name;
    const char* texture_resource_name;

    int16_t coordinate_id;
} ui_mesh_bind_config_t;

typedef struct untextured_mesh_bind_config {
    const char* mesh_name;
    const char* geometry_resource_name;
    const char* material_resource_name;

    int16_t coordinate_id;
} untextured_mesh_bind_config_t;

typedef struct render_model_config {
    size_t coordinate_count;
    const render_model_coordinate_config_t* coordinates;

    size_t line_mesh_count;
    size_t point_mesh_count;
    size_t ui_mesh_count;
    size_t untextured_mesh_count;

    const line_mesh_bind_config_t* line_mesh_bind_configs;
    const point_mesh_bind_config_t* point_mesh_bind_configs;
    const ui_mesh_bind_config_t* ui_mesh_bind_configs;
    const untextured_mesh_bind_config_t* untextured_mesh_bind_configs;
} render_model_config_t;

typedef struct render_model {
    size_t line_mesh_count;
    size_t point_mesh_count;
    size_t ui_mesh_count;
    size_t untextured_mesh_count;

    line_mesh_t* line_meshes;
    point_mesh_t* point_meshes;
    ui_mesh_t* ui_meshes;
    untextured_mesh_t* untextured_meshes;

    model_transform_tree_t* transform_tree;
} render_model_t;

render_model_result_t render_model_create(const render_model_config_t* config_, const vec3f_t* root_translate_, const vec3f_t* root_rotation_, render_model_t** out_render_model_);

void render_model_destroy(render_model_t** render_model_);

#ifdef __cplusplus
}
#endif
#endif
