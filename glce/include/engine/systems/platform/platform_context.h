// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup platform
 *
 * @file platform_context.h
 * @author chocolate-pie24
 * @brief プラットフォームシステムのStrategy Contextモジュールを提供する
 *
 * @details
 * ウィンドウ制御、マウス、キーボード処理を全プラットフォーム(X Window System, win32, glfw...)で共通化するために、
 * Strategyパターンを使用する。このレイヤーではStrategyパターンのContextに相当する構造体インスタンスを提供する
 *
 * @date 2025-10-14
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONTEXT_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/core/memory/linear_allocator.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/systems/platform/core/platform_types.h"

/**
 * @brief プラットフォームコンテキスト構造体前方宣言
 *
 */
typedef struct platform_context platform_context_t;
typedef struct platform_event_view platform_event_view_t;
typedef struct platform_config platform_config_t;

platform_result_t platform_initialize(platform_type_t platform_type_, const platform_config_t* config_, linear_alloc_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_context_t** out_platform_context_);

void platform_deinitialize(platform_context_t* platform_context_);

platform_result_t platform_pump_messages(
    platform_context_t* platform_context_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_));

platform_result_t platform_swap_buffers(platform_context_t* platform_context_);

bool platform_is_valid(const platform_context_t* platform_context_);

#ifdef __cplusplus
}
#endif
#endif
