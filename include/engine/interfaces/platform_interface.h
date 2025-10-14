/** @addtogroup interfaces
 * @{
 *
 * @file platform_interface.h
 * @author chocolate-pie24
 * @brief 全プラットフォーム共通の仮想関数テーブルを提供する
 *
 * @details
 * ウィンドウ制御、マウス、キーボード処理を全プラットフォーム(x11, win32, glfw...)で共通化するために、
 * strategyパターンを使用する。このレイヤーではstrategyパターンのinterfaceに相当するオブジェクトを提供する
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H
#define GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/core/platform/platform_utils.h"

#include "engine/core/event/window_event.h"
#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"

/**< プラットフォーム内部状態管理オブジェクト前方宣言(実体は各ソースファイルで定義) */
typedef struct platform_state platform_state_t;

/**
 * @brief プラットフォーム処理共通化のための仮想関数テーブル(実装はsrc/platform/以下のソースファイルに格納)
 *
 */
typedef struct platform_vtable {
    void (*platform_state_preinit)(size_t* memory_requirement_, size_t* alignment_requirement_);
    platform_result_t (*platform_state_init)(platform_state_t* platform_state_);
    void (*platform_state_destroy)(platform_state_t* platform_state_);
    platform_result_t (*platform_window_create)(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_);
    platform_result_t (*platform_pump_messages)(
        platform_state_t* platform_state_,
        void (*window_event_callback)(const window_event_t* event_),
        void (*keyboard_event_callback)(const keyboard_event_t* event_),
        void (*mouse_event_callback)(const mouse_event_t* event_));
} platform_vtable_t;

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
