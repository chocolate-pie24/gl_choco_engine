/** @ingroup renderer_core
 *
 * @file renderer_memory.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤーでのメモリ確保、メモリ解放処理のテストのため、memory_system内のallocate, free関数のラッパーAPIを提供する
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
#ifndef GLCE_ENGINE_RENDERER_CORE_RENDERER_MEMORY_H
#define GLCE_ENGINE_RENDERER_CORE_RENDERER_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_base/renderer_types.h"

/**
 * @brief memory_system_allocateのラッパーAPI
 * @note レンダラーレイヤー専用APIのため、メモリータグはMEMORY_TAG_RENDERER固定
 *
 * @param size_ メモリ確保サイズ(byte)
 * @param out_ptr_ 確保したメモリの格納先
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - メモリシステム未初期化
 * - out_ptr == NULL
 * - *out_ptr != NULL
 * - mem_tag_ >= MEMORY_TAG_MAX
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS size_ == 0または割り当てに成功し正常終了
 */
renderer_result_t render_mem_allocate(size_t size_, void** out_ptr_);

/**
 * @brief memory_system_freeのラッパーAPI
 * @note レンダラーレイヤー専用APIのため、メモリータグはMEMORY_TAG_RENDERER固定
 *
 * @param ptr_ 解放対象メモリアドレス
 * @param size_ 解放サイズ
 */
void render_mem_free(void* ptr_, size_t size_);

#ifdef TEST_BUILD
void render_mem_test_param_set(renderer_result_t err_code_);
void render_mem_test_param_reset(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
