// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_APPLICATION_SCENES_VTABLES_SCENE_VTABLES_H
#define GLCE_APPLICATION_SCENES_VTABLES_SCENE_VTABLES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/scenes/core/scene_types.h"

typedef struct renderer_frontend renderer_frontend_t;
typedef struct engine_event_view engine_event_view_t;

typedef scene_result_t (*pfn_scene_create)(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, void** out_scene_concrete_);

typedef void (*pfn_scene_destroy)(renderer_frontend_t* renderer_frontend_, void** scene_concrete_);

typedef scene_result_t (*pfn_scene_update)(void* scene_concrete_, const engine_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_);

typedef scene_result_t (*pfn_scene_draw)(void* scene_concrete_, renderer_frontend_t* renderer_frontend_);

typedef bool (*pfn_scene_is_valid)(const void* scene_concrete_);

typedef struct scene_vtable {
    pfn_scene_create    scene_create;
    pfn_scene_destroy   scene_destroy;
    pfn_scene_update    scene_update;
    pfn_scene_draw      scene_draw;
    pfn_scene_is_valid  scene_is_valid;
} scene_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
