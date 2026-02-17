/**
 * @ingroup renderer_backend_interface
 * @file interface_vbo.h
 * @author chocolate-pie24
 * @brief VBOモジュールが提供する機能をグラフィックスAPIによって差し替え可能な仮想関数テーブルを提供する
 *
 * @note renderer_backendはStrategyパターンによるグラフィックスAPI抽象化を行っている。
 * vertex_buffer_objectはVBOモジュールについてのStrategy Interfaceに相当する。
 *
 * @note 本モジュールのリソース管理責務はモジュールが負う。ユーザー側でのメモリ確保、解放は行わないこと。
 *
 * @version 0.1
 * @date 2026-02-06
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VBO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief VBO構造体インスタンスのメモリを確保し、初期化(VBOの生成)する
 *
 * @param[in,out] vertex_buffer_ VBO構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_buffer_ == NULL
 * - *vertex_buffer_ != NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 * @retval 上記以外 グラフィックスAPI実装依存
 */
typedef renderer_result_t (*pfn_vertex_buffer_create)(renderer_backend_vbo_t** vertex_buffer_);

/**
 * @brief VBOを無効化し、VBO構造体インスタンスのメモリを解放する
 *
 * @note 2重destroyを許可する(*vertex_buffer_ == NULLで何もしない)
 *
 * @param[in,out] vertex_buffer_ 無効化、メモリ開放対象VBO構造体インスタンスへのダブルポインタ
 */
typedef void (*pfn_vertex_buffer_destroy)(renderer_backend_vbo_t** vertex_buffer_);

/**
 * @brief VBOのbindを行う
 *
 * @note 既にbind済みのVBOの場合は何もしない
 *
 * @todo renderer_frontend作成後、外部非公開とする
 *
 * @param[in] vertex_buffer_ bind対象VBO構造体インスタンスへのポインタ
 * @param[in,out] out_vbo_id_ bindされたVBO ID格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_buffer_ == NULL
 * - out_vbo_id_ == NULL
 * @retval RENDERER_SUCCESS bindに成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_buffer_bind)(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_);

/**
 * @brief VBOのunbindを行う
 *
 * @todo renderer_frontend作成後、外部非公開とする
 *
 * @param[in] vertex_buffer_ unbind対象VBO構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_buffer_ == NULL
 * @retval RENDERER_SUCCESS unbindに成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_buffer_unbind)(const renderer_backend_vbo_t* vertex_buffer_);

/**
 * @brief VBOが管理する頂点バッファに頂点情報を転送する
 *
 * @param vertex_buffer_ VBO構造体インスタンスへのポインタ
 * @param load_size_ 転送サイズ(byte)
 * @param load_data_ 転送データの先頭アドレス
 * @param usage_ バッファデータの取り扱い @ref buffer_usage_t
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_buffer_ == NULL
 * - load_data_ == NULL
 * - load_size_ == 0
 * @retval RENDERER_RUNTIME_ERROR usage_の値が規定範囲外
 * @retval RENDERER_SUCCESS データの転送に成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_buffer_vertex_load)(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

typedef struct renderer_vbo_vtable {
    pfn_vertex_buffer_create vertex_buffer_create;              /**< 関数ポインタ @ref pfn_vertex_buffer_create 参照 */
    pfn_vertex_buffer_destroy vertex_buffer_destroy;            /**< 関数ポインタ @ref pfn_vertex_buffer_destroy 参照 */
    pfn_vertex_buffer_bind vertex_buffer_bind;                  /**< 関数ポインタ @ref pfn_vertex_buffer_bind 参照 */
    pfn_vertex_buffer_unbind vertex_buffer_unbind;              /**< 関数ポインタ @ref pfn_vertex_buffer_unbind 参照 */
    pfn_vertex_buffer_vertex_load vertex_buffer_vertex_load;    /**< 関数ポインタ @ref pfn_vertex_buffer_vertex_load 参照 */
} renderer_vbo_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
