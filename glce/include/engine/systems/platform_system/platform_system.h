// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_PLATFORM_SYSTEM_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_PLATFORM_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

typedef struct platform_system platform_system_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct platform_system_config platform_system_config_t;

platform_system_result_t platform_system_create(const platform_system_config_t* config_, subsystem_allocator_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_system_t** out_platform_system_);

void platform_system_deinitialize(platform_system_t* platform_system_);

/**
 * @brief Platform由来のEventを更新し、Event Viewを提供する
 *
 * @details
 * Event Viewおよび参照先のEvent StorageはPlatform Systemが所有する。
 * CallerにはOwnershipを移転せず、Borrowとして提供する。
 *
 * Event ViewはPlatform SystemのLifetime中に限り参照できる。
 * 次回のplatform_system_update()によって内容が更新されるため、
 * 以前のEvent内容が保持されることは保証しない。
 *
 * CallerはEvent Viewと参照先Storageを変更・解放してはならない。
 * 過去のEventを保持する場合は、Caller自身でCopyする。
 *
 * @param[in] platform_system_ Platform System
 * @param[out] out_event_view_ BorrowされたEvent ViewへのPointer
 *
 * @retval PLATFORM_SYSTEM_SUCCESS 更新成功
 * @retval PLATFORM_SYSTEM_INVALID_ARGUMENT 引数または内部Preconditionの不正
 * @retval PLATFORM_SYSTEM_LIMIT_EXCEEDED Event Storageの容量上限超過
 */
platform_system_result_t platform_system_update(platform_system_t* platform_system_, const platform_event_view_t** out_event_view_);

platform_system_result_t platform_system_swap_buffers(platform_system_t* platform_system_);

bool platform_system_is_valid(const platform_system_t* platform_system_);

#ifdef __cplusplus
}
#endif
#endif
