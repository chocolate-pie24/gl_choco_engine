/** @ingroup resource
 *
 * @file ui_geom_config_loader.h
 * @author chocolate-pie24
 * @brief UI描画用ジオメトリコンフィグレーションファイルのローダーAPI定義
 *
 * @details コンフィグレーションファイルフォーマット
 * - ファイル格納場所: assets/configs/
 * - ファイル拡張子: .ui_geom
 *
 * @code{.unparsed}
 * コンフィグレーションファイルには矩形領域の幅と高さの設定が必須で、以下のように記載する
 *
 * icon_width = 1
 * icon_height = 1
 * @endcode
 *
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_CONFIG_LOADERS_UI_GEOM_CONFIG_LOADER_H
#define GLCE_ENGINE_RESOURCE_CONFIG_LOADERS_UI_GEOM_CONFIG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/resource/core/resource_types.h"

/**
 * @brief UI描画用ジオメトリ設定値格納構造体
 *
 */
typedef struct ui_geom_config {
    uint16_t icon_width;    /**< 矩形領域形状指定(幅) */
    uint16_t icon_height;   /**< 矩形領域形状指定(高さ) */
} ui_geom_config_t;

/**
 * @brief UI描画用ジオメトリコンフィグレーションファイルをロードし、設定値を取得する
 *
 * @note 処理に失敗した場合、out_config_の状態は変更されない
 *
 * @param[in] name_ 設定値格納ファイル名(パス, 拡張子は含まない)
 * @param[out] out_config_ 設定値格納先ui_geom_config_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - out_config_ == NULL
 * @retval RESOURCE_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステムのシステム使用可能範囲上限を超過
 * - 一行に含まれる文字列が長すぎる
 * @retval RESOURCE_NO_MEMORY メモリ割り当て失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - フルパス文字列が長すぎる
 * - コンフィグレーションファイルに含まれる行数が多すぎる
 * - 読み込んだ設定値が規定値を超過
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - 処理過程において内部データ不整合が発生
 * - コンフィグレーションファイルに規定とは異なる設定値が含まれる
 * @retval RESOURCE_FILE_OPEN_ERROR コンフィグレーションファイルオープンエラー
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_FILE_READ_ERROR 読み込み中に実行時エラーが発生
 * @retval RESOURCE_UNDEFINED_ERROR 上記以外の不明なエラーが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t ui_geom_config_loader_load(const char* name_, ui_geom_config_t* out_config_);

#ifdef __cplusplus
}
#endif
#endif
