/** @addtogroup base
 * @{
 *
 * @file choco_macros.h
 * @author chocolate-pie24
 * @brief システム全体で使用する共通マクロ定義
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef GLCE_ENGINE_BASE_CHOCO_MACROS_H
#define GLCE_ENGINE_BASE_CHOCO_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef __clang__
  /**
   * @brief テスト関数をカバレッジ計測対象外とするためのマクロ定義(clang使用時のみ)
   *
   */
  #define NO_COVERAGE __attribute__((no_profile_instrument_function))
#else
  #define NO_COVERAGE
#endif

/**
 * @brief KiB定義(=1024)
 *
 */
#define KIB ((size_t)(1ULL << 10))

/**
 * @brief MiB定義(=1024 * 1024)
 *
 */
#define MIB ((size_t)(1ULL << 20))

/**
 * @brief GiB定義(=1024 * 1024 * 1024)
 *
 */
#define GIB ((size_t)(1ULL << 30))

/**
 * @brief 2の冪乗かをチェックする
 *
 * @retval true  2の冪乗
 * @retval false 2の冪乗ではない
 */
#define IS_POWER_OF_TWO(val_) ( ((size_t)(val_) != 0u) && ( (((size_t)(val_) & ((size_t)(val_) - 1u)) == 0u)))

#ifdef __cplusplus
}
#endif
#endif

/*@}*/
