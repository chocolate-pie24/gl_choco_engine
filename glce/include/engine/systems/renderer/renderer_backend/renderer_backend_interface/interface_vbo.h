/**
 * @ingroup renderer
 *
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VBO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef renderer_backend_result_t (*pfn_vertex_buffer_create)(renderer_backend_vbo_t** vertex_buffer_); /**< renderer_vbo_vtableが保持するvertex_buffer_createの前方宣言 */
typedef void (*pfn_vertex_buffer_destroy)(renderer_backend_vbo_t** vertex_buffer_); /**< renderer_vbo_vtableが保持するvertex_buffer_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_vertex_buffer_bind)(const renderer_backend_vbo_t* vertex_buffer_);   /**< renderer_vbo_vtableが保持するvertex_buffer_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vertex_buffer_unbind)(void);    /**< renderer_vbo_vtableが保持するvertex_buffer_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vertex_buffer_vertex_load)(size_t load_size_, const void* load_data_, buffer_usage_t usage_); /**< renderer_vbo_vtableが保持するvertex_buffer_vertex_loadの前方宣言 */
typedef renderer_backend_result_t (*pfn_vertex_buffer_vertex_subload)(size_t offset_, size_t size_, const void* load_data_);  /**< renderer_vbo_vtableが保持するvertex_buffer_vertex_subloadの前方宣言 */

/**
 * @brief VBO機能仮想関数テーブル
 *
 */
typedef struct renderer_vbo_vtable {
    /**
     * @brief VBO構造体インスタンスのメモリを確保し、VBOハンドルを生成する
     *
     * @param[out] vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - vertex_buffer_がNULL
     * - *vertex_buffer_が非NULL
     * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_BACKEND_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
     * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
     * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vertex_buffer_create vertex_buffer_create;

    /**
     * @brief renderer_backend_vbo_t構造体インスタンスのメモリを解放し、VBOも削除する
     *
     * @param[in,out] vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
     */
    pfn_vertex_buffer_destroy vertex_buffer_destroy;

    /**
     * @brief VBOをbindする
     *
     * @param[in] vertex_buffer_ bind対象vbo
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT vertex_buffer_ == NULL
     * @retval RENDERER_BACKEND_BAD_OPERATION 未初期化のvertex_buffer_が渡された
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vertex_buffer_bind vertex_buffer_bind;

    /**
     * @brief VBOをunbindする
     *
     * @note 特定のVBOを指定してunbindするAPIではなく、現在のGL_ARRAY_BUFFER bindingを解除するAPIである
     *
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vertex_buffer_unbind vertex_buffer_unbind;

    /**
     * @brief GPU側頂点情報格納領域を生成し、頂点情報を転送する
     *
     * @warning 本APIを呼び出す前に対象のVBOをbindしておくこと
     * @note load_data_ == NULLの場合は頂点情報格納領域の生成のみを行い、頂点情報の転送は行わない
     *
     * @param[in] load_size_ 頂点情報格納領域サイズ(byte)
     * @param[in] load_data_ 転送頂点情報配列へのポインタ
     * @param[in] usage_ バッファ使用方法種別
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT load_size_ == 0
     * @retval RENDERER_BACKEND_RUNTIME_ERROR 規定値外のusage_
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vertex_buffer_vertex_load vertex_buffer_vertex_load;

    /**
     * @brief 生成済みのGPU側頂点情報格納領域に対し、転送位置を指定して頂点情報を転送する
     *
     * @warning 本APIを呼び出す前に対象のVBOをbindしておくこと
     *
     * @param[in] offset_ 頂点情報格納領域の先頭から転送開始位置までのオフセット(byte)
     * @param[in] size_ 頂点情報転送サイズ(byte)
     * @param[in] load_data_ 転送する頂点情報配列へのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - load_data_ == NULL
     * - size_ == 0
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vertex_buffer_vertex_subload vertex_buffer_vertex_subload;
} renderer_vbo_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
