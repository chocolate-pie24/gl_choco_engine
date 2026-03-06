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

#include "engine/platform/platform_interface.h"

/**
 * @brief GLFWプラットフォームを使用する際の仮想関数テーブルを取得する
 *
 * @note vtableの詳細は @ref platform_vtable_t を参照
 *
 * @return const platform_vtable_t* 仮想関数テーブル
 */
const platform_vtable_t* platform_glfw_vtable_get(void);

#ifdef TEST_BUILD
#include "engine/platform/platform_core/platform_types.h"
/**
 * @brief テスト用に、platform_glfwモジュールの実行結果コードを指定値に固定する
 *
 * @note この関数を実行した後は、platform_glfw_result_controller_resetが呼ばれるまで効果が継続する
 *
 * @param result_code_ 出力実行結果コード
 */
void platform_glfw_result_controller_set(platform_result_t ret_);

/**
 * @brief platform_glfw_result_controller_setによる実行結果コードの固定を解除する
 *
 */
void platform_glfw_result_controller_reset(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
