/** @ingroup gl33
 *
 * @file vertex_array_object.h
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VAOを使用するためのラッパーAPIを提供する
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_VERTEX_ARRAY_OBJECT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_VERTEX_ARRAY_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "engine/renderer/renderer_base/renderer_types.h"

/**
 * @brief VAO構造体前方宣言
 *
 * @note 内部データを隠蔽することで、この構造体インスタンスを生成する際にはメモリの動的確保が必須となる。
 * ただ、この構造体のインスタンスは最終的にシェーダー構造体で持つことになる。
 * このため、更新頻度は多くないため、内部データを隠蔽する
 */
typedef struct vertex_array_object vertex_array_object_t;

/**
 * @brief VAO構造体インスタンスのメモリを確保し初期化する
 *
 * @param vertex_array_ vertex_array_object_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * vertex_array_object_t* vao = NULL;
 * renderer_result_t ret = vertex_array_create(&vao);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_がNULL
 * - *vertex_array_が非NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t vertex_array_create(vertex_array_object_t** vertex_array_);

/**
 * @brief vertex_array_object_t構造体インスタンスのメモリを解放し、OpenGLContext内のVAOも削除する
 *
 * @param vertex_array_ vertex_array_object_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * vertex_array_object_t* vao = NULL;
 * renderer_result_t ret = vertex_array_create(&vao);
 * // エラー処理
 * vertex_array_destroy(&vao);
 * @endcode
 */
void vertex_array_destroy(vertex_array_object_t** vertex_array_);

/**
 * @brief glBindVertexArray APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param vertex_array_ Bind対象vao
 *
 * 使用例:
 * @code{.c}
 * vertex_array_object_t* vao = NULL;
 * renderer_result_t ret = vertex_array_create(&vao);
 * // エラー処理
 * ret = vertex_array_bind(vao);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_がNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t vertex_array_bind(const vertex_array_object_t* vertex_array_);

/**
 * @brief VAOアンバインド処理
 *
 * 使用例:
 * @code{.c}
 * vertex_array_object_t* vao = NULL;
 * renderer_result_t ret = vertex_array_create(&vao);
 * // エラー処理
 * ret = vertex_array_bind(vao);
 * // エラー処理
 * vertex_array_unbind();
 * @endcode
 *
 * @retval RENDERER_SUCCESS 現状では内部で呼び出すglBindVertexArrayに対して個別にglGetErrorを行わないため、常に成功
 */
renderer_result_t vertex_array_unbind(void);

/**
 * @brief glVertexAttribPointerのラッパーAPIであり、layout設定の後に設定の有効化(glEnableVertexAttribArray)を行う
 *
 * @param layout_ シェーダープログラム内のどのバッファ変数の設定値かを指定
 * @param size_ 頂点情報に含まれるデータの数([x, y, z]の3次元座標のみであれば3)
 * @param type_ バッファに格納されているデータの型 @ref renderer_type_t
 * @param normalized_ 与えられた頂点データを正規化するかどうかを指定
 * @param stride_ 頂点情報1つあたりのサイズを指定(GLfloat型の[x, y, z]であれば、sizeof(GLfloat) x 3を指定)
 * @param offset_ 「この頂点属性の先頭が、現在GL_ARRAY_BUFFERにバインドされているバッファの先頭から何バイト目にあるか」を指定
 *
 * 使用例:
 * @code{.c}
 * static const GLfloat vertex_buffer_data[] = {
 * -1.0f, -1.0f, 0.0f,
 * 1.0f, -1.0f, 0.0f,
 * 0.0f,  1.0f, 0.0f,
 * };
 *
 * glVertexAttribPointer(
 * 0,
 * 3,
 * GL_FLOAT,
 * GL_FALSE,
 * sizeof(GLfloat) * 3,
 * (void*)0
 * );
 * @endcode
 *
 * @retval RENDERER_RUNTIME_ERROR type_の値が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t vertex_array_attribute_set(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

#ifdef __cplusplus
}
#endif
#endif
