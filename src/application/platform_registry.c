/**
 * @file platform_registry.c
 * @author chocolate-pie24
 * @brief プラットフォーム(x11, win32, glfw...)の差異を吸収するため、プラットフォームに応じた仮想関数テーブル取得処理の実装
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>

#include "application/platform_registry.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/platform/platform_glfw.h"

#include "engine/core/platform/platform_utils.h"

const platform_vtable_t* platform_registry_vtable_get(platform_type_t platform_type_) {
    switch (platform_type_) {
    case PLATFORM_USE_GLFW:
        return platform_glfw_vtable_get();
    default:
        return NULL;
    }
}
