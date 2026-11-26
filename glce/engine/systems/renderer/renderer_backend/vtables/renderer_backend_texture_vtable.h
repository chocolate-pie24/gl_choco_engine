// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_texture_vtable.h
 * @author chocolate-pie24
 * @brief テクスチャ操作関数をまとめたvtableを定義する
 *
 * @date 2026-05-18
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_TEXTURE_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_TEXTURE_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct texture_gpu_config texture_gpu_config_t;

typedef renderer_backend_result_t (*pfn_renderer_texture_create)(int32_t texture_unit_index_, const texture_gpu_config_t* config_, renderer_backend_texture_t** out_texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_createの前方宣言 */
typedef void (*pfn_renderer_texture_destroy)(renderer_backend_texture_t** texture_handle_); /**< renderer_texture_vtableが保持するrenderer_texture_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_bind)(const renderer_backend_texture_t* texture_handle_);   /**< renderer_texture_vtableが保持するrenderer_texture_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_unbind)(const renderer_backend_texture_t* texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_pixel_upload)(const renderer_backend_texture_t* texture_handle_, int32_t width_, int32_t height_, const uint8_t* pixels_);
typedef bool (*pfn_renderer_texture_is_valid)(const renderer_backend_texture_t* texture_handle_);

/**
 * @brief Renderer Backend GPU側テクスチャリソース操作用仮想関数テーブル
 *
 */
typedef struct renderer_texture_vtable {
    pfn_renderer_texture_create renderer_texture_create;

    /**
     * @brief Texture管理Objectと対応するGPU Resourceを破棄する
     *
     * @details
     * Concrete側でTexture Handleの削除を要求し、
     * 管理ObjectのStorageを解放する。
     *
     * 現在のGL33 Concreteでは、
     * texture_handle_または*texture_handle_がNULLの場合は
     * 何も行わない。
     *
     * 解放処理を実行した場合、
     * *texture_handle_をNULLへ設定する。
     *
     * GPU Resourceの削除結果および
     * 管理Objectの解放結果は通知しない。
     *
     * @param[in,out] texture_handle_
     * 破棄対象のTexture管理Objectを保持するPointerの格納先
     */
    pfn_renderer_texture_destroy renderer_texture_destroy;

    /**
     * @brief テクスチャをactiveにし、bindする
     *
     * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_bind renderer_texture_bind;

    /**
     * @brief テクスチャをactiveにし、unbindする
     *
     * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_unbind renderer_texture_unbind;

    pfn_renderer_texture_pixel_upload renderer_texture_pixel_upload;

    /**
     * @brief Concrete側Texture管理Objectの妥当性を検証する
     *
     * @details
     * Concreteが所有するTexture内部表現を検証する。
     *
     * 現在のGL33 Concreteでは、Texture Handle、
     * Texture Unit Index、Min / Mag Filter設定、
     * およびS / T軸のWrap設定を確認する。
     *
     * Renderer Backend Contextの妥当性や、
     * OpenGL側のTexture Resource存在確認は行わない。
     *
     * @param[in] texture_handle_
     * 検証対象のTexture管理Object
     *
     * @retval true
     * Concrete側の構造的妥当性が成立している
     *
     * @retval false
     * Concrete側の構造的妥当性が成立していない
     */
    pfn_renderer_texture_is_valid renderer_texture_is_valid;
} renderer_texture_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
