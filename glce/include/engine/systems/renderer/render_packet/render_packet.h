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

typedef struct render_packet render_packet_t;

typedef struct mat4x4f mat4x4f_t;
typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;
typedef struct frame_allocator frame_allocator_t;

render_packet_result_t render_packet_create(size_t max_memory_pool_size_, render_packet_t** out_render_packet_);

void render_packet_destroy(render_packet_t** render_packet_);

render_packet_result_t render_packet_initialize(size_t max_line_mesh_count_, size_t max_point_mesh_count_, size_t max_ui_mesh_count_, size_t max_untextured_mesh_count_, render_packet_t* render_packet_);

void render_packet_deinitialize(render_packet_t* render_packet_);

void render_packet_read_state_reset(render_packet_t* render_packet_);

render_packet_result_t render_packet_line_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_);

render_packet_result_t render_packet_point_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_);

render_packet_result_t render_packet_ui_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_);

render_packet_result_t render_packet_untextured_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_);

render_packet_result_t render_packet_line_mesh_push(line_mesh_t* mesh_, mat4x4f_t* model_matrix_, render_packet_t* render_packet_);

render_packet_result_t render_packet_point_mesh_push(point_mesh_t* mesh_, mat4x4f_t* model_matrix_, render_packet_t* render_packet_);

render_packet_result_t render_packet_ui_mesh_push(ui_mesh_t* mesh_, mat4x4f_t* model_matrix_, render_packet_t* render_packet_);

render_packet_result_t render_packet_untextured_mesh_push(untextured_mesh_t* mesh_, mat4x4f_t* model_matrix_, render_packet_t* render_packet_);

bool render_packet_line_mesh_read_next(render_packet_t* render_packet_, line_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_);

bool render_packet_point_mesh_read_next(render_packet_t* render_packet_, point_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_);

bool render_packet_ui_mesh_read_next(render_packet_t* render_packet_, ui_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_);

bool render_packet_untextured_mesh_read_next(render_packet_t* render_packet_, untextured_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_);

bool render_packet_is_valid(const render_packet_t* render_packet_);

#ifdef __cplusplus
}
#endif
#endif
