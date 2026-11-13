// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_STL_VIEWER_STL_VIEWER_H
#define GLCE_APPLICATION_DEMOS_STL_VIEWER_STL_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/demos/core/scene_types.h"

typedef struct stl_viewer stl_viewer_t;

typedef struct renderer_frontend renderer_frontend_t;
typedef struct engine_event_view engine_event_view_t;

scene_result_t stl_viewer_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, stl_viewer_t** out_viewer_);

void stl_viewer_destroy(renderer_frontend_t* renderer_frontend_, stl_viewer_t** viewer_);

scene_result_t stl_viewer_update(
    stl_viewer_t* stl_viewer_,
    const engine_event_view_t* event_view_,
    float delta_time_,
    bool window_resized_,
    int framebuffer_width_,
    int framebuffer_height_);

scene_result_t stl_viewer_draw(stl_viewer_t* stl_viewer_, renderer_frontend_t* renderer_frontend_);

bool stl_viewer_is_valid(const stl_viewer_t* stl_viewer_);

#ifdef __cplusplus
}
#endif
#endif
