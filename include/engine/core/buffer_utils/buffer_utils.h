/** @ingroup buffer_utils
 *
 * @file buffer_utils.h
 * @author chocolate-pie24
 * @brief データが密にパックされたバッファへのデータの書き込みと、バッファからのデータの取り出しAPIの定義
 *
 * @todo float型、double型のデータ取り出しAPIは必要に応じて追加する
 * @todo バッファへのデータ書き込みAPIは必要に応じて追加する
 *
 * @version 0.1
 * @date 2026-05-14
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CORE_BUFFER_UTILS_BUFFER_UTILS_H
#define GLCE_ENGINE_CORE_BUFFER_UTILS_BUFFER_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief バッファからint16_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
int16_t buffer_utils_le_int16_t_get(const char* buff_);

/**
 * @brief バッファからint32_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
int32_t buffer_utils_le_int32_t_get(const char* buff_);

/**
 * @brief バッファからint64_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
int64_t buffer_utils_le_int64_t_get(const char* buff_);

/**
 * @brief バッファからuint16_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
uint16_t buffer_utils_le_uint16_t_get(const char* buff_);

/**
 * @brief バッファからuint32_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
uint32_t buffer_utils_le_uint32_t_get(const char* buff_);

/**
 * @brief バッファからuint64_tデータを取り出す
 *
 * @warning リトルエンディアンCPU専用
 * @note buff_ == NULLの場合はassertによって停止する(TEST_BUILD / DEBUG_BUILD時のみ)
 *
 * @param buff_ 取り出し元のバッファ
 *
 * @return 取り出したデータ
 */
uint64_t buffer_utils_le_uint64_t_get(const char* buff_);

#ifdef __cplusplus
}
#endif
#endif
