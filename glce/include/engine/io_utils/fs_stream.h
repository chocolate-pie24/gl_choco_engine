// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_IO_UTILS_FS_STREAM_H
#define GLCE_ENGINE_IO_UTILS_FS_STREAM_H

/**
 * @file fs_stream.h
 * @author chocolate-pie24
 * @brief filesystem上のfile streamを扱うopaque objectとread operationを提供する
 *
 * @details
 * FS Stream moduleは、open済みfile streamを表す`fs_stream_t`と、
 * byte単位およびtext単位のread operation、
 * file streamのlifetime管理に関する機能を提供する。
 *
 * `fs_stream_t`は内部表現を公開しないopaque objectであり、
 * create成功後はopen済みかつ利用可能なfile sessionとして扱う。
 *
 * @section fs_stream_boundary_contract Module Boundary Contract
 *
 * - `fs_stream_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - FS Stream moduleは`fs_stream_t` object自身と、
 *   そのfile sessionを構成するinternal resourceのownershipおよびlifetimeを管理する。
 * - callerはFS Stream moduleが所有するinternal resourceを直接変更または解放しない。
 * - module APIへ渡す`fs_stream_t*`は、FS Stream moduleによって生成され、
 *   lifetime中にあるobjectを参照するものとする。
 *
 * - `fs_stream_create()`成功時に公開される`fs_stream_t`は、
 *   指定されたfileを指定open modeでopen済みの利用可能なsessionを表す。
 * - publicなclosed stateやpartial initialization stateは持たない。
 * - create失敗時には`fs_stream_t`のownershipをcallerへ移転しない。
 *
 * - full pathは空文字列ではなく、'/'から始まるabsolute pathでなければならない。
 * - open modeには有効な`fs_open_mode_t`を指定する。
 *
 * - read operationを行う場合、streamはread可能なopen modeで作成されていなければならない。
 * - byte readでは0より大きいread sizeを指定する。
 * - byte read用bufferは、指定したread size以上を書き込み可能なstorageを
 *   callerが用意し、そのownershipを保持する。
 *
 * - text read APIへ渡す`choco_string_t`はcallerがownershipを保持し、
 *   FS Stream moduleはそのobjectのownershipを取得しない。
 * - text read APIは読み取った内容をcaller-providedな`choco_string_t`へ格納する。
 *
 * - EOFはfile read operationの正常な終端状態として`FS_STREAM_EOF`で通知する。
 * - partial readが発生した場合、read可能なdataが1byte以上存在すれば読み取ったbyte数とともに成功として扱う。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/core/file_io/fs_types.h"

typedef enum {
    FS_STREAM_SUCCESS = 0,       /**< 実行結果コード: 成功 */
    FS_STREAM_INVALID_ARGUMENT,  /**< 実行結果コード: 無効な引数 */
    FS_STREAM_BAD_OPERATION,     /**< 実行結果コード: API誤用 */
    FS_STREAM_DATA_CORRUPTED,    /**< 実行結果コード: データ破損or未初期化 */
    FS_STREAM_NO_MEMORY,         /**< 実行結果コード: メモリ不足 */
    FS_STREAM_LIMIT_EXCEEDED,    /**< 実行結果コード: システム使用可能範囲上限超過 */
    FS_STREAM_OVERFLOW,          /**< 実行結果コード: 計算過程でオーバーフロー発生 */
    FS_STREAM_FILE_OPEN_ERROR,   /**< 実行結果コード: ファイルオープンエラー */
    FS_STREAM_RUNTIME_ERROR,     /**< 実行結果コード: 実行時エラー */
    FS_STREAM_UNDEFINED_ERROR,   /**< 実行結果コード: 想定していないエラーが発生 */
    FS_STREAM_EOF,               /**< 実行結果コード: ファイルを読み込んだ結果がEOF */
} fs_stream_result_t;

typedef struct fs_stream fs_stream_t;
typedef struct choco_string choco_string_t;

fs_stream_result_t fs_stream_create(fs_stream_t** out_stream_, const char* fullpath_, fs_open_mode_t mode_);

void fs_stream_destroy(fs_stream_t** stream_, bool* out_close_succeeded_);

fs_stream_result_t fs_stream_byte_read(fs_stream_t* stream_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_);

fs_stream_result_t fs_stream_text_file_read(fs_stream_t* stream_, choco_string_t* out_string_);

fs_stream_result_t fs_stream_text_file_line_read(fs_stream_t* stream_, choco_string_t* out_string_);

bool fs_stream_is_valid(const fs_stream_t* stream_);

#ifdef __cplusplus
}
#endif
#endif
