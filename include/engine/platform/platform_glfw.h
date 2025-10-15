/** @addtogroup platform_glfw
 * @{
 *
 * @file platform_glfw.h
 * @author chocolate-pie24
 * @brief GLFWを使用する際の仮想関数テーブルを取得する(Strategyパターンのconcreteオブジェクトに相当)
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
#ifndef GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#define GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/interfaces/platform_interface.h"

#include "engine/core/platform/platform_utils.h"

/**
 * @brief GLFWプラットフォームを使用する際の仮想関数テーブルを取得する
 *
 * @return const platform_vtable_t* 仮想関数テーブル
 */
const platform_vtable_t* platform_glfw_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
