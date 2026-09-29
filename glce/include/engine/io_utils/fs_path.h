// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file fs_path.h
 * @author chocolate-pie24
 * @brief filesystem pathを所有するopaque objectと、その基本操作を提供する
 *
 * @details
 * FS Path moduleは、filesystem上のpathを保持する`fs_path_t`と、
 * pathの生成、参照およびlifetime管理に関する機能を提供する。
 *
 * `fs_path_t`は内部表現を公開しないopaque objectであり、
 * callerは`fs_path_t*`を通してmodule APIを利用する。
 *
 * @section fs_path_boundary_contract Module Boundary Contract
 *
 * - `fs_path_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - FS Path moduleは、`fs_path_t` object自身のstorageおよび
 *   objectが所有するfull path storageのallocation / releaseを管理する。
 * - callerは`fs_path_t` object自身のstorageまたはobjectが所有する
 *   full path storageを直接解放しない。
 * - module APIへ渡す`fs_path_t*`は、FS Path moduleによって生成され、
 *   lifetime中にあるobjectを参照するものとする。
 *
 * - `fs_path_t`が表すfull pathは空文字列ではない。
 * - FS Path moduleがpath componentを連結してfull pathを構築する場合、
 *   separatorにはplatformによらず'/'を使用する。
 *
 * - path生成のsourceとして受け取るC stringはborrowとして扱い、
 *   FS Path moduleはそのstorageのownershipを取得しない。
 * - sourceとして受け取るC stringへのpointerはobject内部へ保持せず、
 *   必要な文字列dataはFS Path moduleが所有するstorageへ保持する。
 * - sourceとして受け取るC stringは、終端NULを持つ有効なC stringとして扱う。
 *
 * - full path参照APIから返されるC stringへのpointerはborrowed pointerであり、
 *   callerはそのstorageを変更または解放しない。
 * - full path参照APIから返されるpointerのlifetimeは、
 *   ownerである`fs_path_t` objectのlifetimeに従う。
 *
 * - Windows platformは現時点では未対応である。
 *
 */
#ifndef GLCE_ENGINE_IO_UTILS_FS_PATH_H
#define GLCE_ENGINE_IO_UTILS_FS_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct fs_path fs_path_t;

typedef enum {
    FS_PATH_SUCCESS = 0,       /**< 実行結果コード: 成功 */
    FS_PATH_INVALID_ARGUMENT,  /**< 実行結果コード: 無効な引数 */
    FS_PATH_BAD_OPERATION,     /**< 実行結果コード: API誤用 */
    FS_PATH_DATA_CORRUPTED,    /**< 実行結果コード: データ破損or未初期化 */
    FS_PATH_NO_MEMORY,         /**< 実行結果コード: メモリ不足 */
    FS_PATH_LIMIT_EXCEEDED,    /**< 実行結果コード: システム使用可能範囲上限超過 */
    FS_PATH_OVERFLOW,          /**< 実行結果コード: 計算過程でオーバーフロー発生 */
    FS_PATH_RUNTIME_ERROR,     /**< 実行結果コード: 実行時エラー */
    FS_PATH_UNDEFINED_ERROR,   /**< 実行結果コード: 想定していないエラーが発生 */
} fs_path_result_t;

fs_path_result_t fs_path_create(fs_path_t** out_path_, const char* base_path_, const char* path_, const char* name_, const char* extension_);

fs_path_result_t fs_path_create_from_executable_directory(fs_path_t** out_path_);

void fs_path_destroy(fs_path_t** path_);

const char* fs_path_fullpath_get(const fs_path_t* path_);

bool fs_path_is_valid(const fs_path_t* path_);

#ifdef __cplusplus
}
#endif
#endif
