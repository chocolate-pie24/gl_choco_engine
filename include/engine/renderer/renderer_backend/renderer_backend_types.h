/**
 * @ingroup renderer_backend
 * @file renderer_backend_types.h
 * @author chocolate-pie24
 * @brief renderer_backend内で共通して使用するデータ構造を提供する
 *
 * @version 0.1
 * @date 2026-02-06
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TYPES_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_backend_shader renderer_backend_shader_t;   /**< シェーダーモジュール内部状態管理構造体前方宣言 */

typedef struct renderer_backend_vao renderer_backend_vao_t;         /**< VAOモジュール内部状態管理構造体前方宣言 */

typedef struct renderer_backend_vbo renderer_backend_vbo_t;         /**< VBOモジュール内部状態管理構造体前方宣言 */

#ifdef __cplusplus
}
#endif
#endif
