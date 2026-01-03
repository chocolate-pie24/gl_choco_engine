/** @ingroup gl33
 *
 * @file vertex_buffer_object.h
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VBOを使用するためのラッパーAPIを提供する
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @todo TODO: vertex_buffer_vertex_reload
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_VERTEX_BUFFER_OBJECT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_VERTEX_BUFFER_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_base/renderer_types.h"

/**
 * @brief VBO構造体前方宣言
 *
 * @note 内部データを隠蔽することで、この構造体インスタンスを生成する際にはメモリの動的確保が必須となる。
 * ただ、この構造体のインスタンスは最終的にシェーダー構造体で持つことになる。
 * このため、更新頻度は多くないため、内部データを隠蔽する
 */
typedef struct vertex_buffer_object vertex_buffer_object_t;

/**
 * @brief VBO構造体インスタンスのメモリを確保し初期化する
 *
 * @param vertex_buffer_ vertex_buffer_object_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * vertex_buffer_object_t* vbo = NULL;
 * renderer_result_t ret = vertex_buffer_create(&vbo);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_buffer_がNULL
 * - *vertex_buffer_が非NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t vertex_buffer_create(vertex_buffer_object_t** vertex_buffer_);

/**
 * @brief vertex_buffer_object_t構造体インスタンスのメモリを解放し、OpenGLContext内のVBOも削除する
 *
 * @param vertex_buffer_ vertex_buffer_object_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * vertex_buffer_object_t* vbo = NULL;
 * renderer_result_t ret = vertex_buffer_create(&vbo);
 * // エラー処理
 * vertex_buffer_destroy(&vbo);
 * @endcode
 */
void vertex_buffer_destroy(vertex_buffer_object_t** vertex_buffer_);

/**
 * @brief glBindBuffer APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param vertex_buffer_ Bind対象vbo
 *
 * 使用例:
 * @code{.c}
 * vertex_buffer_object_t* vbo = NULL;
 * renderer_result_t ret = vertex_buffer_create(&vbo);
 * // エラー処理
 * ret = vertex_buffer_bind(vbo);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_buffer_がNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t vertex_buffer_bind(const vertex_buffer_object_t* vertex_buffer_);

/**
 * @brief VBOアンバインド処理
 *
 * 使用例:
 * @code{.c}
 * vertex_buffer_object_t* vbo = NULL;
 * renderer_result_t ret = vertex_buffer_create(&vbo);
 * // エラー処理
 * ret = vertex_buffer_bind(vbo);
 * // エラー処理
 * vertex_buffer_unbind();
 * @endcode
 *
 * @retval RENDERER_SUCCESS 現状では内部で呼び出すglBindBufferに対して個別にglGetErrorを行わないため、常に成功
 */
renderer_result_t vertex_buffer_unbind(void);

/**
 * @brief VBOで管理する頂点情報をGPUへ転送する(glBufferData APIのラッパーAPI)
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 * @note 現在バインド中のVBOに紐づくバッファへ転送されるため、事前に @ref vertex_buffer_bind によってバインドしておくこと
 *
 * 使用例:
 * @code{.c}
 * float data[] = { 1.0f, 2.0f, 3.0f };
 * renderer_result_t ret = vertex_buffer_vertex_load(sizeof(float) * 3, data, BUFFER_USAGE_STATIC);
 * // エラー処理
 * @endcode
 *
 * @param load_size_ 転送サイズ(byte)
 * @param load_data_ 転送データの先頭アドレス
 * @param usage_ バッファデータの取り扱い @ref buffer_usage_t
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - load_data_ == NULL
 * - load_size_ == 0
 * @retval RENDERER_RUNTIME_ERROR usage_の値が規定範囲外
 * @retval RENDERER_SUCCESS データの転送に成功し、正常終了
 */
renderer_result_t vertex_buffer_vertex_load(size_t load_size_, void* load_data_, buffer_usage_t usage_);

// renderer_result_t vertex_buffer_vertex_reload(void);

#ifdef __cplusplus
}
#endif
#endif
