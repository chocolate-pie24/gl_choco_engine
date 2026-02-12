/** @ingroup renderer_base
 *
 * @file renderer_types.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるデータ型を提供する
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
#ifndef GLCE_ENGINE_RENDERER_BASE_RENDERER_TYPES_H
#define GLCE_ENGINE_RENDERER_BASE_RENDERER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GraphicsAPI種別リスト
 *
 */
typedef enum {
    GRAPHICS_API_GL33,  /**< OpenGL 3.3 */
} target_graphics_api_t;

/**
 * @brief レンダラーレイヤー実行結果コード定義
 *
 */
typedef enum {
    RENDERER_SUCCESS = 0,           /**< 処理成功 */
    RENDERER_INVALID_ARGUMENT,      /**< 無効な引数 */
    RENDERER_RUNTIME_ERROR,         /**< 実行時エラー */
    RENDERER_NO_MEMORY,             /**< メモリ確保失敗 */
    RENDERER_SHADER_COMPILE_ERROR,  /**< シェーダーコンパイルエラー */
    RENDERER_SHADER_LINK_ERROR,     /**< シェーダーリンクエラー */
    RENDERER_LIMIT_EXCEEDED,        /**< システム使用可能範囲上限超過 */
    RENDERER_BAD_OPERATION,         /**< API誤用 */
    RENDERER_DATA_CORRUPTED,        /**< メモリ破損,未初期化 */
    RENDERER_UNDEFINED_ERROR,       /**< 不明なエラー */
} renderer_result_t;

/**
 * @brief 上位層で頂点情報データ種別の識別にグラフィックスAPI固有の型を使用しないで済むよう、頂点情報のデータ種別を定義
 *
 */
typedef enum {
    BUFFER_USAGE_STATIC,    /**< データ種別: 頻繁に書き換わることのないデータ */
    BUFFER_USAGE_DYNAMIC,   /**< データ種別: 頻繁に書き換わるデータ */
} buffer_usage_t;

/**
 * @brief 上位層でグラフィックスAPI固有の変数型を使用しないで済むよう、グラフィックスAPI固有型を定義
 *
 */
typedef enum {
    RENDERER_TYPE_FLOAT,    /**< データ型: GLfloat */
} renderer_type_t;

/**
 * @brief 上位層でシェーダー種別の識別にグラフィックスAPI固有の型を使用しないで済むよう、シェーダー種別リストを定義
 *
 */
typedef enum {
    SHADER_TYPE_VERTEX,     /**< バーテックスシェーダー */
    SHADER_TYPE_FRAGMENT,   /**< フラグメントシェーダー */
} shader_type_t;

#ifdef __cplusplus
}
#endif
#endif
