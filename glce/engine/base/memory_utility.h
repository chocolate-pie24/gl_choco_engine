// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_BASE_MEMORY_UTILITY_H
#define GLCE_ENGINE_BASE_MEMORY_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

// ============================================================
// alignment utility
// ============================================================
/**
 * @brief value_のサイズを指定されたバイト境界(alignment_)の倍数に切り上げout_aligned_size_に格納する
 *
 * @pre out_aligned_size_ != NULL
 * @pre alignment_が2の冪乗であること
 *
 * @param[in] value_ 指定されたバイト境界に切り上げる対象のサイズ
 * @param[in] alignment_ 指定バイト境界
 * @param[out] out_aligned_size_ 計算結果格納先
 *
 * @return true 処理に成功し正常終了
 * @return false 以下のいずれか
 * - alignment_が2の冪乗ではない
 * - out_aligned_size_がNULL
 * - 計算過程でオーバーフローが発生
 */
bool memory_utility_align_up(size_t value_, size_t alignment_, size_t* out_aligned_size_);

/**
 * @brief value_が指定されたバイト境界(alignment_)に配置されているかを判定する
 *
 * @pre out_is_aligned_ != NULL
 * @pre alignment_が2の冪乗であること
 *
 * @param[in] value_ 判定対象値
 * @param[in] alignment_ 判定対象バイト境界
 * @param[out] out_is_aligned_ 判定結果格納先(true: alignment_境界にアラインされている, false: alignment_境界にアラインされていない)
 *
 * @return true 処理に成功し正常終了
 * @return false 以下のいずれか
 * - out_is_aligned_ == NULL
 * - alignment_が2の冪乗ではない
 */
bool memory_utility_is_aligned(size_t value_, size_t alignment_, bool* out_is_aligned_);

/**
 * @brief value_のサイズを指定されたバイト境界(alignment_)の倍数に切り上げた時に必要となるpadding量を計算し、out_padding_size_に格納する
 *
 * @pre out_padding_size_ != NULL
 * @pre alignment_が2の冪乗であること
 *
 * @param[in] value_ 計算対象値
 * @param[in] alignment_ 計算対象バイト境界
 * @param[out] out_padding_size_ 計算結果格納先
 *
 * @return true 処理に成功し正常終了
 * @return false 以下のいずれか
 * - out_padding_size_ == NULL
 * - alignment_が2の冪乗ではない
 * - 計算過程でオーバーフローが発生
 */
bool memory_utility_padding_calc(size_t value_, size_t alignment_, size_t* out_padding_size_);

#ifdef __cplusplus
}
#endif
#endif
