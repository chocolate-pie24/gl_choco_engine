// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup renderer
 *
 * @file renderer_types.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるデータ型を提供する
 *
 * @date 2025-12-19
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_PACKET_RENDER_PACKET_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_PACKET_RENDER_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"

typedef struct mat4x4f mat4x4f_t;
typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;
typedef struct frame_allocator frame_allocator_t;

typedef struct line_mesh_draw_request {
    size_t max_mesh_count;
    size_t mesh_count;
    line_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} line_mesh_render_packet_t;

typedef struct point_mesh_draw_request {
    size_t max_mesh_count;
    size_t mesh_count;
    point_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} point_mesh_render_packet_t;

typedef struct ui_mesh_draw_request {
    size_t max_mesh_count;
    size_t mesh_count;
    ui_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} ui_mesh_render_packet_t;

typedef struct untextured_mesh_draw_request {
    size_t max_mesh_count;
    size_t mesh_count;
    untextured_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} untextured_mesh_render_packet_t;

typedef struct render_packet {
    frame_allocator_t* allocator;

    line_mesh_render_packet_t line_mesh_packet;
    point_mesh_render_packet_t point_mesh_packet;
    ui_mesh_render_packet_t ui_mesh_packet;
    untextured_mesh_render_packet_t untextured_mesh_packet;
} render_packet_t;

render_packet_result_t render_packet_create(size_t max_memory_pool_size_, render_packet_t** out_render_packet_);

void render_packet_destroy(render_packet_t** render_packet_);

render_packet_result_t render_packet_initialize(size_t max_line_mesh_count_, size_t max_point_mesh_count_, size_t max_ui_mesh_count_, size_t max_untextured_mesh_count_, render_packet_t* render_packet_);

void render_packet_deinitialize(render_packet_t* render_packet_);

bool render_packet_is_valid(const render_packet_t* render_packet_);

#ifdef __cplusplus
}
#endif
#endif
