/** @ingroup texture_system
 *
 * @file texture_manager.h
 * @author chocolate-pie24
 * @brief テクスチャリソース(CPU / GPU)管理システムモジュールAPI定義
 *
 * @version 0.1
 * @date 2026-05-18
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEXTURE_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEXTURE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct linear_alloc linear_alloc_t;
typedef struct texture_manager texture_manager_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct renderer_backend_texture renderer_backend_texture_t;

#define INVALID_TEXTURE_ID (-1) /**< 無効なテクスチャ識別子 */

/**
 * @brief テクスチャ管理システムモジュール実行結果コード定義
 *
 */
typedef enum {
    TEXTURE_SYSTEM_SUCCESS = 0,        /**< 処理成功 */
    TEXTURE_SYSTEM_NO_MEMORY,          /**< メモリ不足 */
    TEXTURE_SYSTEM_RUNTIME_ERROR,      /**< 実行時エラー */
    TEXTURE_SYSTEM_INVALID_ARGUMENT,   /**< 引数異常 */
    TEXTURE_SYSTEM_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    TEXTURE_SYSTEM_BAD_OPERATION,      /**< API誤用 */
    TEXTURE_SYSTEM_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    TEXTURE_SYSTEM_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    TEXTURE_SYSTEM_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    TEXTURE_SYSTEM_FILE_CLOSE_ERROR,   /**< ファイルクローズ失敗 */
    TEXTURE_SYSTEM_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    TEXTURE_SYSTEM_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    TEXTURE_SYSTEM_UNDEFINED_ERROR,    /**< 未定義エラー */
} texture_system_result_t;

/**
 * @brief テクスチャ管理システムのメモリを確保し、テクスチャリソース(CPU / GPU)配列の要素を全てNULLで初期化する
 *
 * @note texture_managerモジュールはシステム起動時から終了時まで常駐するため、リニアアロケータによるリソース確保を行う
 *
 * @param[in] max_texture_count_ システムで使用するテクスチャ最大数
 * @param[in,out] allocator_ 使用するリニアアロケータへのポインタ
 * @param[out] out_texture_manager_ 初期化対象構造体インスタンスへのダブルポインタ
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - allocator_ == NULL
 * - out_texture_manager_ == NULL
 * - *out_texture_manager_ != NULL
 * - max_texture_count_ <= 0
 * @retval TEXTURE_SYSTEM_NO_MEMORY メモリ確保失敗
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_);

/**
 * @brief テクスチャリソース(CPU / GPU)管理システムが管理するすべてのリソースを解放し、リソース配列の要素をすべてNULLで初期化する
 *
 * @note texture_manager_自身のメモリは解放しない
 * @note 以下の場合はワーニングメッセージを出力する
 * - backend_context_ == NULL
 * - texture_manager_ == NULL
 * - texture_manager_が未初期化
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 */
void texture_manager_deinitialize(renderer_backend_context_t* backend_context_, texture_manager_t* texture_manager_);

/**
 * @brief テクスチャリソース管理システムにテクスチャリソースを登録し、リソースの読み込み、GPUへのアップロードを行う
 *
 * @note 下記のテクスチャ名称はテスト用のビルトインテクスチャで、事前のファイル準備は不要
 * - test_texture_red
 * - test_texture_green
 * - test_texture_blue
 * @note 処理に失敗した場合、texture_manager_およびout_texture_id_の状態は不変(ただし、GPU側バッファ状態は影響がある場合がある)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] gpu_unit_num_ 使用するテクスチャスロット番号(0以上)
 * @param[in] texture_name_ テクスチャファイル名(拡張子は含まない)
 * @param[in,out] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 * @param[out] out_texture_id_ 登録したテクスチャリソースが保持される管理システムリソース配列のインデックス
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_name_ == NULL
 * - texture_manager_ == NULL
 * - out_texture_id_ == NULL
 * - gpu_unit_num_ < 0
 * @retval TEXTURE_SYSTEM_BAD_OPERATION 以下のいずれか
 * - テクスチャ管理システム(texture_manager_)が未初期化
 * - Renderer Backend未初期化
 * - メモリシステム未初期化
 * - テクスチャサイズ異常
 * - 指定したテクスチャ名称がすでにシステムに登録済み
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 以下のいずれか
 * - テクスチャ管理システムのリソース管理配列データ不整合
 * - テクスチャリソース内部データ破損
 * @retval TEXTURE_SYSTEM_LIMIT_EXCEEDED 以下のいずれか
 * - テクスチャ管理システム使用可能範囲上限超過
 * - メモリシステムの使用可能範囲上限超過
 * @retval TEXTURE_SYSTEM_OVERFLOW 処理過程でオーバーフロー発生
 * @retval TEXTURE_SYSTEM_NO_MEMORY メモリ確保失敗
 * @retval TEXTURE_SYSTEM_UNSUPPORTED_FILE サポート対象外の画像ファイル
 * @retval TEXTURE_SYSTEM_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval TEXTURE_SYSTEM_FILE_CLOSE_ERROR ファイルクローズ失敗
 * @retval TEXTURE_SYSTEM_FILE_READ_ERROR ファイル読み込み失敗
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_register(renderer_backend_context_t* backend_context_, int32_t gpu_unit_num_, const char* texture_name_, texture_manager_t* texture_manager_, int16_t* out_texture_id_);

/**
 * @brief テクスチャリソース管理システムからテクスチャリソースを破棄する
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] texture_id_ 破棄対象テクスチャ識別子(リソース配列のインデックス)
 * @param[in,out] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_manager_ == NULL
 * - texture_id_が不正(0未満またはシステムで管理可能な上限値を超過)
 * @retval TEXTURE_SYSTEM_BAD_OPERATION texture_manager_が未初期化
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 管理システムのデータ不整合
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_unregister(renderer_backend_context_t* backend_context_, int16_t texture_id_, texture_manager_t* texture_manager_);

/**
 * @brief テクスチャリソース管理システムからテクスチャ名称を指定してテクスチャリソースを破棄する
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] name_ 破棄対象テクスチャ名称
 * @param[in,out] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_manager_ == NULL
 * - name_ == NULL
 * @retval TEXTURE_SYSTEM_BAD_OPERATION 以下のいずれか
 * - texture_manager_が未初期化
 * - 指定したテクスチャ名称がシステム内に見つからない
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 管理システムのデータ不整合
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_unregister_by_name(renderer_backend_context_t* backend_context_, const char* name_, texture_manager_t* texture_manager_);

/**
 * @brief テクスチャリソース管理システムからテクスチャ名称に対応するテクスチャ識別子を取得する
 *
 * @param[in] name_ 識別子取得対象テクスチャ名称
 * @param[in] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 * @param[out] out_texture_id_ テクスチャ識別子格納先
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - texture_manager_ == NULL
 * - name_ == NULL
 * - out_texture_id_ == NULL
 * @retval TEXTURE_SYSTEM_BAD_OPERATION 以下のいずれか
 * - texture_manager_が未初期化
 * - 指定したテクスチャ名称のテクスチャがシステム内に見つからない
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 管理システムのデータ不整合
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_texture_id_get(const char* name_, const texture_manager_t* texture_manager_, int16_t* out_texture_id_);

/**
 * @brief テクスチャリソース管理システムからテクスチャ識別子を指定してGPUリソースを取得する
 *
 * @note 取得したGPUリソースの所有権はシステムが持つため、呼び出し側でのリソースの破棄は禁止
 *
 * @param[in] texture_id_ GPUリソースを取得するテクスチャ識別子
 * @param[in] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 * @param[out] out_gpu_resource_ GPUリソース格納先
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - texture_manager_ == NULL
 * - out_gpu_resource_ == NULL
 * - texture_id_が不正(0未満またはシステムで管理可能な上限値を超過)
 * @retval TEXTURE_SYSTEM_BAD_OPERATION 以下のいずれか
 * - texture_manager_が未初期化
 * - 指定したテクスチャ識別子がシステムに未登録
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 管理システムのデータ不整合
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_gpu_resource_get(int16_t texture_id_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_);

/**
 * @brief テクスチャリソース管理システムからテクスチャ名称を指定してGPUリソースを取得する
 *
 * @note 取得したGPUリソースの所有権はシステムが持つため、呼び出し側でのリソースの破棄は禁止
 *
 * @param[in] name_ GPUリソースを取得するテクスチャ名称
 * @param[in] texture_manager_ テクスチャリソース管理システム構造体インスタンスへのポインタ
 * @param[out] out_gpu_resource_ GPUリソース格納先
 *
 * @retval TEXTURE_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - texture_manager_ == NULL
 * - out_gpu_resource_ == NULL
 * - name_ == NULL
 * @retval TEXTURE_SYSTEM_BAD_OPERATION 以下のいずれか
 * - texture_manager_が未初期化
 * - 指定したテクスチャ名称がシステムに未登録
 * @retval TEXTURE_SYSTEM_DATA_CORRUPTED 管理システムのデータ不整合
 * @retval TEXTURE_SYSTEM_SUCCESS 処理に成功し、正常終了
 */
texture_system_result_t texture_manager_gpu_resource_get_by_name(const char* name_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_);

#ifdef __cplusplus
}
#endif
#endif
