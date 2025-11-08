/** @ingroup platform_glfw
 *
 * @file platform_glfw.h
 * @author chocolate-pie24
 * @brief GLFW APIで実装されたプラットフォームシステムAPIを提供する
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
#ifndef GLCE_ENGINE_PLATFORM_CONCRETES_PLATFORM_GLFW_H
#define GLCE_ENGINE_PLATFORM_CONCRETES_PLATFORM_GLFW_H
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

#ifdef TEST_BUILD
/**
 * @brief テスト専用API(外部から強制的に引数で指定したret_を返すようにする)
 *
 * @param ret_ 返り値設定値
 */
void platform_glfw_result_controller_set(platform_result_t ret_);

/**
 * @brief テスト専用API(テスト設定値をリセットする)
 *
 */
void platform_glfw_result_controller_reset(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
