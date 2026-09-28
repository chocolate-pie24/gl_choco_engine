// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_VTABLES_PLATFORM_BACKEND_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_VTABLES_PLATFORM_BACKEND_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

/**< プラットフォーム内部状態管理構造体前方宣言(実体は各ソースファイルで定義) */
typedef struct platform_backend platform_backend_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct platform_system_config platform_system_config_t;
typedef struct platform_event_view platform_event_view_t;

typedef platform_system_result_t (*pfn_platform_backend_create)(const platform_system_config_t* config_, subsystem_allocator_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_backend_);

typedef void (*pfn_platform_backend_deinitialize)(platform_backend_t* backend_);

typedef platform_system_result_t (*pfn_platform_backend_update)(platform_backend_t* backend_, const platform_event_view_t** out_event_view_);

/**
 * @brief 描画サーフェイスのフロント/バックバッファをスワップする
 *
 * @param[in,out] backend_ 処理対象プラットフォーム内部状態管理オブジェクト
 *
 * @retval PLATFORM_SYSTEM_INVALID_ARGUMENT backend_がNULL
 * @retval 上記以外 プラットフォーム実装依存
 */
typedef platform_system_result_t (*pfn_platform_backend_swap_buffers)(platform_backend_t* backend_);

typedef bool (*pfn_platform_backend_is_valid)(const platform_backend_t* backend_);

/**
 * @brief プラットフォーム処理共通化のための仮想関数テーブル(実装はsrc/platform_system/以下のソースファイルに格納)
 */
typedef struct platform_backend_vtable {
    pfn_platform_backend_create         platform_backend_create;
    pfn_platform_backend_deinitialize   platform_backend_deinitialize;
    pfn_platform_backend_update         platform_backend_update;
    pfn_platform_backend_swap_buffers   platform_backend_swap_buffers;              /**< 関数ポインタ @ref pfn_platform_backend_swap_buffers 参照 */
    pfn_platform_backend_is_valid       platform_backend_is_valid;
} platform_backend_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
