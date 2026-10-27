// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

typedef struct render_model_config render_model_config_t;
typedef struct render_model render_model_t;

typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;

render_model_result_t render_model_create(size_t untextured_mesh_count_, untextured_mesh_t** untextured_meshes_, uint16_t** untextured_mesh_coordinate_ids_, render_model_t** out_model_);

void render_model_destroy(render_model_t** render_model_);

render_model_result_t render_model_untextured_mesh_mesh_get(const render_model_t* render_model_, uint16_t untextured_mesh_index_, untextured_mesh_t* out_mesh_);

render_model_result_t render_model_untextured_mesh_coordinate_id_get(const render_model_t* render_model_, uint16_t untextured_mesh_index_, uint16_t* out_coordinate_id_);

render_model_result_t render_model_untextured_mesh_count_get(const render_model_t* render_model_, size_t* out_count_);

#ifdef __cplusplus
}
#endif
#endif
