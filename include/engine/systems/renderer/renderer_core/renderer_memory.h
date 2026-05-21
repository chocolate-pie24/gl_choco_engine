/** @ingroup renderer_core
 *
 * @file renderer_memory.h
 * @author chocolate-pie24
 * @brief renderer_memoryは、レンダラーレイヤー内でのメモリ確保/解放における実行結果コードと、メモリタグを統一化するため、choco_memoryモジュールのラップAPIを提供する
 *
 * @version 0.1
 * @date 2026-02-12
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_RENDERER_MEMORY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_RENDERER_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/renderer_core/renderer_types.h"

/**
 * @brief memory_system_allocateのラッパーAPI
 * @note レンダラーレイヤー専用APIのため、メモリータグはMEMORY_TAG_RENDERER固定
 *
 * @param size_ メモリ確保サイズ(byte)
 * @param out_ptr_ 確保したメモリの格納先
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った
 * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_SUCCESS 以下のいずれか
 * - size_ == 0(*out_ptr_はNULLのまま)
 * - 割り当てに成功し正常終了
 */
renderer_result_t renderer_mem_allocate(size_t size_, void** out_ptr_);

/**
 * @brief memory_system_freeのラッパーAPI
 * @note レンダラーレイヤー専用APIのため、メモリータグはMEMORY_TAG_RENDERER固定
 *
 * @note 引数にはvoid*型を渡しており、ptr_のメモリ開放後、NULLをセットすることはできない。
 * この仕様は、標準ライブラリのfree()の仕様に合わせた。なので、呼び出し側でメモリの解放後、NULLをセットすること。
 *
 * @param ptr_ 解放対象メモリアドレス
 * @param size_ 解放サイズ
 */
void renderer_mem_free(void* ptr_, size_t size_);

#ifdef __cplusplus
}
#endif
#endif
