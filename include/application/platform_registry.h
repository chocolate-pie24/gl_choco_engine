/** @addtogroup application
 * @{
 *
 * @file platform_registry.h
 * @author chocolate-pie24
 * @brief プラットフォーム(x11, win32, glfw...)の差異を吸収するため、プラットフォームに応じた仮想関数テーブル取得処理を提供する
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_APP_PLATFORM_REGISTRY_H
#define GLCE_APP_PLATFORM_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/platform/platform_utils.h"
#include "engine/interfaces/platform_interface.h"

/**
 * @brief プラットフォーム(x11, win32, glfw...)の差異を吸収するため、プラットフォームに応じた仮想関数テーブル取得処理
 *
 * 使用例:
 * @code
 * const platform_vtable_t vtable = platform_registry_vtable_get(PLATFORM_USE_GLFW);    // GLFWを使用したテーブルを取得
 * @endcode
 *
 * @param[in] platform_type_ 仮想関数テーブルを取得するプラットフォーム種別
 * @return const platform_vtable_t* 仮想関数テーブル(引数で指定したプラットフォームが見つからない場合はNULL)
 */
const platform_vtable_t* platform_registry_vtable_get(platform_type_t platform_type_);

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
