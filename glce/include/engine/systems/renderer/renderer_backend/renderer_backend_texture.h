// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_texture.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するテクスチャ操作機能の窓口を上位層に提供する
 *
 * @date 2026-05-18
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TEXTURE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief Renderer Backend Textureを生成する
 *
 * @details
 * Backend Contextが保持するTexture vtableを使用して、
 * Concrete側のTexture生成処理を呼び出す。
 *
 * 現在のGL33 Concreteでは、General Allocatorで管理Objectを確保し、
 * OpenGL Texture Handleの生成およびFilter / Wrap設定を要求する。
 *
 * GL33 ConcreteはOpenGL APIの実行結果を検証しないため、
 * SUCCESSはGPU側Texture Resourceの正常性まで保証しない。
 *
 * Texture管理Objectの所有権は成功時にCallerへ渡す。
 * 破棄にはrenderer_backend_texture_destroy()を使用する。
 *
 * @param[in] backend_context_
 * Texture vtableを保持するRenderer Backend Context
 *
 * @param[in] texture_unit_index_
 * Texture Unitの0始まりのIndex。0以上であること。
 *
 * @param[in] min_filter_config_ 縮小Filter設定
 * @param[in] mag_filter_config_ 拡大Filter設定
 * @param[in] wrap_config_s_axis_ S軸のWrap設定
 * @param[in] wrap_config_t_axis_ T軸のWrap設定
 *
 * @param[out] out_texture_handle_
 * 生成したTexture管理Objectの格納先。
 * 呼び出し前に*out_texture_handle_はNULLであること。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * Concrete側の生成処理が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * Backend Context、Output Pointer、Texture Unit Index、
 * Filter / Wrap設定、または下位Allocatorへの入力が不正
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Texture vtableがNULL、または下位Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * 管理ObjectのStorageを確保できなかった
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術Overflowが検出された
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorの内部データ破損が検出された
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義または変換対象外の結果を受け取った
 */
renderer_backend_result_t renderer_backend_texture_create(const renderer_backend_context_t* backend_context_, int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** out_texture_handle_);

/**
 * @brief Renderer Backend Textureを破棄する
 *
 * @details
 * Backend Contextが保持するTexture vtableを使用し、
 * Concrete側の破棄処理を呼び出す。
 *
 * 現在のGL33 Concreteでは、OpenGL Texture Handleの削除を
 * 要求した後、General Allocatorで管理Objectを解放する。
 *
 * backend_context_またはTexture vtableがNULLの場合、
 * Warningを出力して破棄処理を行わず終了する。
 *
 * texture_handle_または*texture_handle_がNULLの場合も、
 * Warningを出力して終了する。
 *
 * Concrete側の破棄処理が実行された場合、
 * *texture_handle_はNULLへ設定される。
 *
 * OpenGL APIおよびGeneral Allocatorの解放結果は
 * Callerへ通知しない。
 *
 * @param[in] backend_context_
 * Texture破棄用vtableを保持するRenderer Backend Context
 *
 * @param[in,out] texture_handle_
 * 破棄対象のTexture管理Objectを保持するPointerの格納先
 */
void renderer_backend_texture_destroy(const renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_);

/**
 * @brief テクスチャをactiveにし、bindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->texture_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * Concrete側でTexture Handleが0、
 * またはTexture Unit Indexが負数であることを検出した
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_texture_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

/**
 * @brief テクスチャをunbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->texture_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * Concrete側でTexture Handleが0、
 * またはTexture Unit Indexが負数であることを検出した
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_texture_unbind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

/**
 * @brief 現在bind中の2D TextureへPixel Dataを転送する
 *
 * @details
 * Texture vtableを介してConcrete側のPixel Uploadを呼び出す。
 *
 * 現在のGL33 Concreteでは、GL_TEXTURE_2Dに対して
 * glTexImage2D()を呼び出す。
 *
 * channel_count_は3(RGB)または4(RGBA)のみ許可する。
 * Pixel Dataの型にはGL_UNSIGNED_BYTEを使用する。
 *
 * 現在のGL33 Concreteは転送前にGL_UNPACK_ALIGNMENTを1へ、
 * 成功経路の終了時に4へ設定する。
 *
 * OpenGL APIの実行結果やTextureのbind状態は検証しない。
 *
 * @warning
 * 本APIを呼び出す前に対象Textureを
 * renderer_backend_texture_bind()等によってbindしておくこと。
 *
 * @pre
 * pixels_は転送に必要なPixel Dataを読み取り可能な
 * Storageを指していること。
 *
 * @param[in] backend_context_
 * Texture vtableを保持するRenderer Backend Context
 *
 * @param[in] width_ Pixel Dataの幅。0より大きいこと。
 * @param[in] height_ Pixel Dataの高さ。0より大きいこと。
 * @param[in] channel_count_ Channel数。3または4。
 * @param[in] pixels_ 転送元Pixel Data
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * Concrete側の転送処理が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_またはpixels_がNULL、
 * width_またはheight_が0以下、
 * またはchannel_count_が3と4以外
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Texture vtableが設定されていない
 */
renderer_backend_result_t renderer_backend_texture_pixel_upload(const renderer_backend_context_t* backend_context_, int32_t width_, int32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

/**
 * @brief Renderer Backend Textureの構造的妥当性を検証する
 *
 * @details
 * 次の条件を検証する。
 *
 * - texture_handle_がNULLではない
 * - Renderer Backend Contextがvalidである
 * - Concrete側のTexture Validatorがtrueを返す
 *
 * 現在のGL33 Concreteでは、次の条件を確認する。
 *
 * - Texture Handleが0ではない
 * - Texture Unit Indexが0以上である
 * - Min Filter設定が有効である
 * - Mag Filter設定が有効である
 * - S / T軸のWrap設定が有効である
 *
 * Renderer Backend ContextのValidationでは、
 * 各Backend vtableが選択中のConcreteと一致することも確認する。
 *
 * 本関数はglIsTexture()等によるOpenGL側のResource存在確認、
 * Texture UnitのHardware上限確認、および
 * 管理Object自身のAllocation Liveness確認を行わない。
 *
 * @pre
 * NULLではないtexture_handle_は、
 * 読み取り可能な有効期間中の管理Objectを指すこと。
 *
 * @param[in] backend_context_
 * 検証対象のRenderer Backend Context
 *
 * @param[in] texture_handle_
 * 検証対象のTexture管理Object
 *
 * @retval true
 * 検査対象の構造的妥当性が成立している
 *
 * @retval false
 * 検査対象の構造的妥当性が成立していない
 *
 * @note
 * 本関数は対象Objectを変更しない。
 */
bool renderer_backend_texture_is_valid(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

#ifdef __cplusplus
}
#endif
#endif
