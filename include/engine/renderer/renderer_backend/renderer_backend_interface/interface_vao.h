/**
 * @ingroup renderer_backend_interface
 * @file interface_vao.h
 * @author chocolate-pie24
 * @brief VAOモジュールが提供する機能をグラフィックスAPIによって差し替え可能な仮想関数テーブルを提供する
 *
 * @note renderer_backendはStrategyパターンによるグラフィックスAPI抽象化を行っている。
 * vertex_array_objectはVAOモジュールについてのStrategy Interfaceに相当する。
 *
 * @note 本モジュールのリソース管理責務はモジュールが負う。ユーザー側でのメモリ確保、解放は行わないこと。
 *
 * @version 0.1
 * @date 2026-02-06
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VAO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief VAO構造体インスタンスのメモリを確保し、初期化(VAOの生成)する
 *
 * @param[in,out] vertex_array_ VAO構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_ == NULL
 * - *vertex_array_ != NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 * @retval 上記以外 グラフィックスAPI実装依存
 */
typedef renderer_result_t (*pfn_vertex_array_create)(renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAOを無効化し、VAO構造体インスタンスのメモリを解放する
 *
 * @note 2重destroyを許可する(*vertex_array_ == NULLで何もしない)
 *
 * @param[in,out] vertex_array_ 無効化、メモリ開放対象VAO構造体インスタンスへのダブルポインタ
 */
typedef void (*pfn_vertex_array_destroy)(renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAOのbindを行う
 *
 * @note 既にbind済みのVAOの場合は何もしない
 *
 * @todo renderer_frontend作成後、外部非公開とする
 *
 * @param[in] vertex_array_ bind対象VAO構造体インスタンスへのポインタ
 * @param[in,out] out_vao_id_ bindされたVAO ID格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_ == NULL
 * - out_vao_id_ == NULL
 * @retval RENDERER_SUCCESS bindに成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_array_bind)(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_);

/**
 * @brief VAOのunbindを行う
 *
 * @todo renderer_frontend作成後、外部非公開とする
 *
 * @param[in] vertex_array_ unbind対象VAO構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_ == NULL
 * @retval RENDERER_SUCCESS unbindに成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_array_unbind)(const renderer_backend_vao_t* vertex_array_);

/**
 * @brief VAOで管理する頂点情報のレイアウト情報を設定する
 *
 * @note 本API内でbind処理を行う(既にbind済であればbindしない)ため、実行前のbindは不要
 *
 * @param[in] vertex_array_ 設定対象VAO構造体インスタンスへのポインタ
 * @param layout_ シェーダープログラム内のどのバッファ変数の設定値かを指定
 * @param size_ 頂点情報に含まれるデータの数([x, y, z]の3次元座標のみであれば3)
 * @param type_ バッファに格納されているデータの型 @ref renderer_type_t
 * @param normalized_ 与えられた頂点データを正規化するかどうかを指定
 * @param stride_ 頂点情報1つあたりのサイズを指定(GLfloat型の[x, y, z]であれば、sizeof(GLfloat) x 3を指定)
 * @param offset_ 「この頂点属性の先頭が、現在GL_ARRAY_BUFFERにバインドされているバッファの先頭から何バイト目にあるか」を指定
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_ == NULL
 * @retval RENDERER_RUNTIME_ERROR type_の値が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 * @retval 上記以外 グラフィックスAPIごとの実装依存
 */
typedef renderer_result_t (*pfn_vertex_array_attribute_set)(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

typedef struct renderer_vao_vtable {
    pfn_vertex_array_create vertex_array_create;                /**< 関数ポインタ @ref pfn_vertex_array_create 参照 */
    pfn_vertex_array_destroy vertex_array_destroy;              /**< 関数ポインタ @ref pfn_vertex_array_destroy 参照 */
    pfn_vertex_array_bind vertex_array_bind;                    /**< 関数ポインタ @ref pfn_vertex_array_bind */
    pfn_vertex_array_unbind vertex_array_unbind;                /**< 関数ポインタ @ref pfn_vertex_array_unbind 参照 */
    pfn_vertex_array_attribute_set vertex_array_attribute_set;  /**< 関数ポインタ @ref pfn_vertex_array_attribute_set 参照 */
} renderer_vao_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
