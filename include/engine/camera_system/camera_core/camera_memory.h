/** @ingroup camera_system
 *
 * @file camera_memory.h
 * @author chocolate-pie24
 * @brief カメラシステムレイヤー内でのメモリ確保/解放における実行結果コードと、メモリタグを統一化するため、choco_memoryモジュールのラップAPIを提供する
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_MEMORY_H
#define GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/camera_system/camera_core/camera_types.h"

/**
 * @brief memory_system_allocateのラッパーAPI
 * @note カメラシステムレイヤー専用APIのため、メモリータグはMEMORY_TAG_CAMERA固定
 *
 * @param size_ メモリ確保サイズ(byte)
 * @param out_ptr_ 確保したメモリの格納先
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - メモリシステム未初期化
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * @retval CAMERA_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval CAMERA_NO_MEMORY メモリ割り当て失敗
 * @retval CAMERA_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った
 * @retval CAMERA_SUCCESS 以下のいずれか
 * - size_ == 0(*out_ptr_はNULLのまま)
 * - 割り当てに成功し正常終了
 */
camera_result_t camera_mem_allocate(size_t size_, void** out_ptr_);

/**
 * @brief memory_system_freeのラッパーAPI
 * @note カメラシステムレイヤー専用APIのため、メモリータグはMEMORY_TAG_CAMERA固定
 *
 * @note 引数にはvoid*型を渡しており、ptr_のメモリ開放後、NULLをセットすることはできない。
 * この仕様は、標準ライブラリのfree()の仕様に合わせた。なので、呼び出し側でメモリの解放後、NULLをセットすること。
 *
 * @param ptr_ 解放対象メモリアドレス
 * @param size_ 解放サイズ
 */
void camera_mem_free(void* ptr_, size_t size_);

#ifdef __cplusplus
}
#endif
#endif
