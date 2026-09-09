// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_backend_shader renderer_backend_shader_t;   /**< シェーダーハンドル構造体(シェーダープログラム／シェーダーオブジェクトのハンドルを保持する構造体)の前方宣言 */
typedef struct renderer_backend_vao renderer_backend_vao_t;         /**< VAOモジュール内部状態管理構造体前方宣言 */
typedef struct renderer_backend_vbo renderer_backend_vbo_t;         /**< VBOモジュール内部状態管理構造体前方宣言 */
typedef struct renderer_backend_texture renderer_backend_texture_t; /**< テクスチャハンドル構造体の前方宣言 */

typedef enum {
    RENDERER_BACKEND_SUCCESS = 0,
    RENDERER_BACKEND_INVALID_ARGUMENT,
    RENDERER_BACKEND_RUNTIME_ERROR,
    RENDERER_BACKEND_NO_MEMORY,
    RENDERER_BACKEND_LIMIT_EXCEEDED,
    RENDERER_BACKEND_BAD_OPERATION,
    RENDERER_BACKEND_DATA_CORRUPTED,
    RENDERER_BACKEND_OVERFLOW,
    RENDERER_BACKEND_SHADER_COMPILE_ERROR,
    RENDERER_BACKEND_SHADER_LINK_ERROR,
    RENDERER_BACKEND_UNDEFINED_ERROR,
} renderer_backend_result_t;

/**
 * @brief 上位層でグラフィックスAPI固有の変数型を使用しないで済むよう、グラフィックスAPI固有型を定義
 *
 */
typedef enum {
    RENDERER_TYPE_FLOAT,            /**< データ型: GLfloat */
    RENDERER_TYPE_UNSIGNED_BYTE,    /**< データ型: GL_UNSIGNED_BYTE */
    RENDERER_TYPE_BYTE,             /**< データ型: GL_BYTE */
} renderer_type_t;

typedef enum {
    SHADER_STAGE_VERTEX,
    SHADER_STAGE_FRAGMENT,
} shader_stage_t;

#ifdef __cplusplus
}
#endif
#endif
