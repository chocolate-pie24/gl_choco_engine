// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_LINEAR_VBO_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_LINEAR_VBO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"

typedef struct renderer_backend_vbo renderer_backend_vbo_t;

typedef struct linear_vbo_manager_config {
    size_t vbo_size;                /**< GPU側VBOおよび論理memory poolの総サイズ(byte) */
    size_t base_align;              /**< 全allocationへ適用する固定base alignment(byte) */
    buffer_usage_t buffer_usage;    /**< Renderer Backendへ指定するbuffer usage */
} linear_vbo_manager_config_t;

typedef struct linear_vbo_manager linear_vbo_manager_t;

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

buffer_manager_result_t linear_vbo_manager_create(renderer_backend_context_t* backend_context_, const linear_vbo_manager_config_t* config_, linear_vbo_manager_t** out_vbo_manager_);

void linear_vbo_manager_destroy(linear_vbo_manager_t** vbo_manager_);

// NOTE: エラーチェックが不要の場合はout_allocated_はNULLで良い
buffer_manager_result_t linear_vbo_manager_write(linear_vbo_manager_t* vbo_manager_, size_t size_, const void* write_data_, size_t* out_offset_);

buffer_manager_result_t linear_vbo_manager_reset(linear_vbo_manager_t* vbo_manager_);

buffer_manager_result_t linear_vbo_manager_bind(linear_vbo_manager_t* vbo_manager_);

buffer_manager_result_t linear_vbo_manager_unbind(const linear_vbo_manager_t* vbo_manager_);

const renderer_backend_vbo_t* linear_vbo_manager_vbo_get(const linear_vbo_manager_t* vbo_manager_);

bool linear_vbo_manager_config_is_valid(const linear_vbo_manager_config_t* config_);

bool linear_vbo_manager_is_valid(const linear_vbo_manager_t* vbo_manager_);

#ifdef __cplusplus
}
#endif
#endif
