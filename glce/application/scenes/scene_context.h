// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_APPLICATION_SCENES_SCENE_CONTEXT_H
#define GLCE_APPLICATION_SCENES_SCENE_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/scenes/core/scene_types.h"

typedef struct scene_context scene_context_t;

typedef struct renderer_frontend renderer_frontend_t;
typedef struct platform_event_view platform_event_view_t;

typedef enum {
    SCENE_NONE,
    SCENE_SAMPLE_ROBOT,
    SCENE_STL_VIEWER,
} scene_list_t;

scene_result_t scene_context_create(scene_context_t** out_scene_);

void scene_context_destroy(renderer_frontend_t* renderer_frontend_, scene_context_t** scene_);

scene_list_t scene_context_current_scene_get(const scene_context_t* scene_);

scene_result_t scene_context_change_scene(scene_context_t* scene_, renderer_frontend_t* renderer_frontend_, const void* scene_config_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, scene_list_t next_scene_);

scene_result_t scene_context_update(scene_context_t* scene_, const platform_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_);

scene_result_t scene_context_draw(scene_context_t* scene_, renderer_frontend_t* renderer_frontend_);

bool scene_context_is_valid(const scene_context_t* scene_);

#ifdef __cplusplus
}
#endif
#endif
