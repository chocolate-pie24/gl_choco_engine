// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file mouse_event.h
 * @author chocolate-pie24
 * @brief 全プラットフォーム共通で使用可能なマウスボタン定義と、マウスイベント構造体定義
 *
 * @date 2025-10-14
 *
 */
#ifndef GLCE_ENGINE_CORE_EVENT_MOUSE_EVENT_H
#define GLCE_ENGINE_CORE_EVENT_MOUSE_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    MOUSE_EVENT_BUTTON,
    MOUSE_EVENT_MOVE,
    MOUSE_EVENT_SCROLL,
} mouse_event_code_t;

typedef enum {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
} mouse_button_t;

typedef union mouse_event_args {
    struct {
        mouse_button_t button;
        bool pressed;
        double x;
        double y;
    } button;

    struct {
        double x;
        double y;

        double x_offset;
        double y_offset;
    } move;

    struct {
        double x_offset;
        double y_offset;
    } scroll;
} mouse_event_args_t;

typedef struct mouse_event {
    mouse_event_code_t event_code;
    mouse_event_args_t event_args;
} mouse_event_t;

#ifdef __cplusplus
}
#endif
#endif
