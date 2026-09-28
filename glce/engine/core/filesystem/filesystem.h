// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file filesystem.h
 * @author chocolate-pie24
 * @brief 基本的なfile I/Oを提供するFilesystem module
 *
 * @details
 * Filesystem moduleは、fileのopen / closeおよびbyte単位のreadなど、
 * file I/Oの基本的なoperationを提供する。
 *
 * 1行単位の読み込みやfile全体の読み込みなど、
 * 可変長文字列bufferのresource管理を必要とする高水準なfile I/Oは本moduleでは扱わない。
 * これらの処理はio_utils/fs_streamが担当する。
 *
 * filesystem_tはopaque typeとし、1つのopened file resourceとそのopen modeを表す。
 *
 * @section filesystem_boundary_contract Module Boundary Contract
 *
 * Lifecycle / Ownership:
 * - filesystem_create()はcomplete constructorである。
 *   成功時には対象fileのopenを含む初期化をすべて完了し、即座に利用可能なfilesystem_tをcallerへ公開する。
 * - create途中のpartial stateはcallerへ公開しない。
 * - filesystem_t自身のstorageはGeneral Allocatorから取得し、
 *   Filesystem moduleがそのlifetimeを管理する。
 * - filesystem_tはcreate時にopenしたfile resourceをそのlifetime中所有する。
 * - filesystem_tのlifetime終了にはfilesystem_destroy()を使用する。
 * - filesystem_create()を使用する前からfilesystem_destroy()が完了するまで、
 *   General Allocatorは利用可能でなければならない。
 *
 * Validation:
 * - filesystem_is_valid()はfilesystem_tのcanonical validatorである。
 * - canonical validityはFilesystem moduleが保証するfilesystem_t自身の
 *   lifecycleおよびstructural invariantを対象とする。
 *
 * @date 2025-12-23
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
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

filesystem_result_t filesystem_create(filesystem_t** out_filesystem_, const char* fullpath_, fs_open_mode_t mode_);

void filesystem_destroy(filesystem_t** filesystem_, bool* out_close_succeeded_);

filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_);

bool filesystem_is_valid(const filesystem_t* filesystem_);

#ifdef __cplusplus
}
#endif
#endif
