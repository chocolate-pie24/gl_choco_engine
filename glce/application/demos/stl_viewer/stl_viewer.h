// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_STL_VIEWER_STL_VIEWER_H
#define GLCE_APPLICATION_DEMOS_STL_VIEWER_STL_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/demos/stl_viewer/core/stl_viewer_types.h"

typedef struct stl_viewer stl_viewer_t;

typedef struct render_packet render_packet_t;
typedef struct renderer_frontend renderer_frontend_t;
typedef struct untextured_mesh_config untextured_mesh_config_t;

stl_viewer_result_t stl_viewer_create(const char* asset_root_path_, const untextured_mesh_config_t* config_, renderer_frontend_t* renderer_frontend_, stl_viewer_t** out_viewer_);

void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, stl_viewer_t** viewer_);

stl_viewer_result_t stl_viewer_render_packet_build(const stl_viewer_t* viewer_, render_packet_t* packet_);

bool stl_viewer_is_valid(const stl_viewer_t* viewer_);

#ifdef __cplusplus
}
#endif
#endif
