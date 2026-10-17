// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_CONFIG_RENDER_MODEL_CONFIG_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_CONFIG_RENDER_MODEL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;

typedef struct choco_string choco_string_t;

typedef struct render_model_coordinate_config {
    uint16_t coordinate_id;
    uint16_t parent_coordinate_id;

    vec3f_t translate;
    vec3f_t rotation;
} render_model_coordinate_config_t;

typedef struct line_mesh_config {
    choco_string_t* mesh_name;
    choco_string_t* geometry_resource_name;
    vec4u8_t line_color;

    uint16_t coordinate_id;
} line_mesh_config_t;

typedef struct point_mesh_config {
    choco_string_t* mesh_name;
    choco_string_t* geometry_resource_name;

    uint16_t coordinate_id;
} point_mesh_config_t;

typedef struct ui_mesh_config {
    choco_string_t* mesh_name;
    choco_string_t* geometry_resource_name;
    choco_string_t* texture_resource_name;

    uint16_t coordinate_id;
} ui_mesh_config_t;

typedef struct untextured_mesh_config {
    choco_string_t* mesh_name;
    choco_string_t* geometry_resource_name;
    choco_string_t* material_resource_name;

    uint16_t coordinate_id;
} untextured_mesh_config_t;

typedef struct render_model_config {
    size_t coordinate_count;
    render_model_coordinate_config_t* coordinates;

    size_t line_mesh_count;
    size_t point_mesh_count;
    size_t ui_mesh_count;
    size_t untextured_mesh_count;

    line_mesh_config_t* line_mesh_configs;
    point_mesh_config_t* point_mesh_configs;
    ui_mesh_config_t* ui_mesh_configs;
    untextured_mesh_config_t* untextured_mesh_configs;
} render_model_config_t;

render_model_result_t render_model_config_coordinate_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, render_model_coordinate_config_t* out_config_);

// TODO: 以下はuntexture_meshとrender_modelの接続が完了したら作成する
// render_model_result_t render_model_config_line_mesh_initialize(const char* mesh_name_, const char* geometry_resource_name_, vec4u8_t line_color_, uint16_t coordinate_id_);
// render_model_result_t render_model_config_point_mesh_initialize(const char* mesh_name_, const char* geometry_resource_name_, uint16_t coordinate_id_);
// render_model_result_t render_model_config_ui_mesh_initialize(const char* mesh_name_, const char* geometry_resource_name_, const char* texture_resource_name_, uint16_t coordinate_id_);
render_model_result_t render_model_config_untextured_mesh_initialize(const char* mesh_name_, const char* geometry_resource_name_, const char* material_resource_name_, uint16_t coordinate_id_, untextured_mesh_config_t* out_config_);

// NOTE: deinitialize後はconfig_はinvalidな状態となる
void render_model_config_coordinate_deinitialize(render_model_coordinate_config_t* config_);

// NOTE: deinitialize後はconfig_はinvalidな状態となる
void render_model_config_untextured_mesh_deinitialize(untextured_mesh_config_t* config_);

// NOTE: deinitialize後はconfig_はinvalidな状態となる
void render_model_config_render_model_deinitialize(render_model_config_t* config_);


// TODO: 以下はrender_modelの構造が確定したのちに作成する
bool render_model_coordinate_config_is_valid(const render_model_coordinate_config_t* config_);
bool line_mesh_config_is_valid(const line_mesh_config_t* config_);
bool point_mesh_config_is_valid(const point_mesh_config_t* config_);
bool ui_mesh_config_is_valid(const ui_mesh_config_t* config_);
bool untextured_mesh_config_is_valid(const untextured_mesh_config_t* config_);
bool render_model_config_is_valid(const render_model_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
