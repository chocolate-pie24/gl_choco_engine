// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup platform
 *
 * @file platform_vtable.h
 * @author chocolate-pie24
 * @brief プラットフォームシステムのInterface構造体を提供する
 *
 * @details
 * ウィンドウ制御、マウス、キーボード処理を全プラットフォーム(x11, win32, glfw...)で共通化するために、
 * Strategyパターンを使用する。このレイヤーではStrategyパターンのinterfaceに相当する構造体インスタンスを提供する
 *
 * @date 2025-10-14
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_VTABLES_PLATFORM_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_VTABLES_PLATFORM_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/systems/platform/core/platform_types.h"

/**< プラットフォーム内部状態管理構造体前方宣言(実体は各ソースファイルで定義) */
typedef struct platform_backend platform_backend_t;
typedef struct platform_config platform_config_t;
typedef struct linear_alloc linear_alloc_t;

typedef platform_result_t (*pfn_platform_backend_initialize)(const platform_config_t* config_, linear_alloc_t* linear_alloc_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_platform_backend_);

typedef void (*pfn_platform_backend_deinitialize)(platform_backend_t* platform_backend_);

/**
 * @brief ウィンドウ、キーボード、マウスイベントを吸い上げ、各イベントをコールバック内で処理する
 *
 * @note
 * - ウィンドウクローズイベントについては、イベントキューが満杯時に破棄される可能性を考慮し、戻り値で返す
 *
 * @param[in,out] platform_backend_ 処理対象プラットフォーム内部状態管理オブジェクト
 * @param[in] window_event_callback ウィンドウイベント発生時用コールバック関数
 * @param[in] keyboard_event_callback キーボードイベント発生時用コールバック関数
 * @param[in] mouse_event_callback マウスイベント発生時用コールバック関数
 *
 * @retval PLATFORM_INVALID_ARGUMENT 以下のいずれか
 * - platform_backend_ == NULL または プラットフォーム内部状態管理オブジェクトが未初期化
 * - window_event_callback == NULL
 * - keyboard_event_callback == NULL
 * - mouse_event_callback == NULL
 * @retval PLATFORM_SUCCESS          イベントの吸い上げおよびイベントコールバックの処理に成功し、正常終了
 * @retval PLATFORM_WINDOW_CLOSE     ウィンドウクローズイベントが発生
 * @retval その他                     プラットフォーム実装依存
 */
typedef platform_result_t (*pfn_platform_backend_pump_messages)(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_));

/**
 * @brief 描画サーフェイスのフロント/バックバッファをスワップする
 *
 * @param[in,out] platform_backend_ 処理対象プラットフォーム内部状態管理オブジェクト
 *
 * @retval PLATFORM_INVALID_ARGUMENT platform_backend_がNULL
 * @retval その他                     プラットフォーム実装依存
 */
typedef platform_result_t (*pfn_platform_backend_swap_buffers)(platform_backend_t* platform_backend_);

typedef bool (*pfn_platform_backend_is_valid)(const platform_backend_t* platform_backend_);

/**
 * @brief プラットフォーム処理共通化のための仮想関数テーブル(実装はsrc/platform/以下のソースファイルに格納)
 */
typedef struct platform_vtable {
    pfn_platform_backend_initialize     platform_backend_initialize;                      /**< 関数ポインタ @ref pfn_platform_backend_init 参照 */
    pfn_platform_backend_deinitialize   platform_backend_deinitialize;                   /**< 関数ポインタ @ref pfn_platform_backend_destroy 参照 */
    pfn_platform_backend_pump_messages  platform_backend_pump_messages;             /**< 関数ポインタ @ref pfn_platform_backend_pump_messages 参照 */
    pfn_platform_backend_swap_buffers   platform_backend_swap_buffers;              /**< 関数ポインタ @ref pfn_platform_backend_swap_buffers 参照 */
    pfn_platform_backend_is_valid       platform_backend_is_valid;
} platform_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
