/** @ingroup renderer_core
 *
 * @file renderer_err_utils.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるエラー処理関連APIを提供する
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_CORE_RENDERER_ERR_UTILS_H
#define GLCE_ENGINE_RENDERER_CORE_RENDERER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief レンダラーレイヤー実行結果コードを文字列に変換する
 *
 * @param result_ 実行結果コード
 * @return const char* 文字列化されたコード
 */
const char* renderer_rslt_to_str(renderer_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
