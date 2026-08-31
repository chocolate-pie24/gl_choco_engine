// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file filesystem.h
 * @author chocolate-pie24
 * @brief ファイルシステムモジュールAPIの提供
 *
 * @details ファイルシステムモジュールは、ファイルI/Oについて最も基本的なAPIを提供する。
 * そのため、1行単位の読み込みや、ファイル全体の読み込みといった処理は提供しない。
 * これらの処理には可変長文字列バッファのリソース管理が必要で、choco_stringモジュールを使用したい。
 * choco_stringモジュールを使用するとなると、containersレイヤーよりも上層にfilesystemを位置づける必要がある。
 * 一方で、ファイルI/Oについての基本的な処理はcoreレイヤーに置きたい。このため、高度な処理と基本的な処理を分け、基本的な処理はcore/filesystemに置くことにする。
 * なお、高度な処理は、io_utils/fs_streamに格納する。
 *
 * @date 2025-12-23
 *
 */
#ifndef GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H
#define GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/file_io/fs_types.h"

/**
 * @brief ファイルシステムモジュール内部状態管理構造体前方宣言(内部データ構造は外部非公開)
 *
 */
typedef struct filesystem filesystem_t;

/**
 * @brief ファイルシステムモジュールの実行結果コード定義
 *
 */
typedef enum {
    FILESYSTEM_SUCCESS = 0,         /**< 実行結果コード: 成功 */
    FILESYSTEM_INVALID_ARGUMENT,    /**< 実行結果コード: 無効な引数 */
    FILESYSTEM_RUNTIME_ERROR,       /**< 実行結果コード: 実行時エラー */
    FILESYSTEM_NO_MEMORY,           /**< 実行結果コード: メモリ不足 */
    FILESYSTEM_FILE_OPEN_ERROR,     /**< 実行結果コード: ファイルオープン失敗 */
    FILESYSTEM_UNDEFINED_ERROR,     /**< 実行結果コード: 未定義エラー */
    FILESYSTEM_LIMIT_EXCEEDED,      /**< 実行結果コード: システムリソースが使用可能範囲を超過 */
    FILESYSTEM_BAD_OPERATION,       /**< 実行結果コード: API誤用 */
    FILESYSTEM_DATA_CORRUPTED,      /**< 実行結果コード: 内部データ破損 */
    FILESYSTEM_EOF,                 /**< 実行結果コード: ファイル読み取りEOF */
} filesystem_result_t;

filesystem_result_t filesystem_create(filesystem_t** filesystem_, const char* fullpath_, fs_open_mode_t mode_);

void filesystem_destroy(filesystem_t** filesystem_, bool* out_close_succeeded_);

filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* result_n_, char* buffer_);

bool filesystem_is_valid(const filesystem_t* filesystem_);

#ifdef __cplusplus
}
#endif
#endif
