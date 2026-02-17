#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< renderer_backend_context内部情報管理構造体前方宣言 */

/**
 * @brief レンダラーバックエンドのメモリを確保し、初期化を行う
 *
 * @param allocator_ メモリ確保用リニアアロケータ
 * @param target_api_ 使用するグラフィックスAPI
 * @param out_renderer_backend_context_ レンダラーバックエンド内部情報管理構造体インスタンスへのダブルポインタ
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - allocator_ == NULL
 * - out_renderer_backend_context_ == NULL
 * - *out_renderer_backend_context_ != NULL
 * - target_api_が既定値外
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_RUNTIME_ERROR 以下のいずれか
 * - シェーダー用vtable取得失敗
 * - VAO用vtable取得失敗
 * - VBO用vtable取得失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_);

/**
 * @brief レンダラーバックエンドの終了処理を行う
 *
 * @note レンダラーバックエンドはサブシステムであり、リニアアロケータでメモリを確保する。
 * リニアアロケータは個別のメモリ開放は不可のため、このAPIではメモリの解放は行わない。
 *
 * @note 使用API別の具体的な処理内容
 * - OpenGL3.3: 何もしない
 *
 * @param renderer_backend_context_ 終了処理対象レンダラーバックエンド構造体インスタンスへのポインタ
 */
void renderer_backend_destroy(renderer_backend_context_t* renderer_backend_context_);

#ifdef __cplusplus
}
#endif
#endif
