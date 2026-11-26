// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_TEXTURE_GPU_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_TEXTURE_GPU_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct texture_gpu_config texture_gpu_config_t;

texture_result_t texture_gpu_resource_create(
    const renderer_backend_context_t* backend_context_,
    int32_t texture_unit_index_,
    const texture_gpu_config_t* config_,
    int32_t width_,
    int32_t height_,
    const uint8_t* pixels_,
    texture_gpu_resource_t** out_texture_resource_);

/**
 * @brief GPU Texture Resourceを破棄する。
 *
 * @par Public API Contract
 * - Resourceが所有するBackend TextureをRenderer Backendで破棄する。
 * - Backend Textureの破棄にはResourceが保持する
 *   Backend Contextを使用する。
 * - GPU Texture Resource本体をGeneral Allocatorへ返却する。
 * - texture_resource_または*texture_resource_がNULLの場合は
 *   何も行わない。
 * - DEBUG / TEST Buildでは破棄前にis_valid()を実行し、
 *   無効なResourceの場合は破棄を中断する。
 * - RELEASE Buildではis_valid()による事前検証を行わない。
 * - 本APIはvoidであり、下位の解放結果をcallerへ返さない。
 *
 * @par Ownership
 * - Backend TextureとGPU Texture Resource本体を解放する。
 * - Backend Context自体は破棄しない。
 * - CPU Texture ResourceやPixel DataのOwnershipには関与しない。
 *
 * @par Failure Contract
 * - DEBUG / TEST BuildでValidity検証に失敗した場合、
 *   Resourceの破棄を実行しない。
 * - Backend Textureの破棄結果とGeneral Allocatorの
 *   解放結果は本APIの戻り値では通知されない。
 * - 下位解放処理に失敗した場合の完全なResource解放は
 *   保証しない。
 *
 * @param[in,out] texture_resource_
 *                破棄するGPU Texture Resourceの所有ポインタ。
 *
 * @pre 有効なResourceを破棄する場合、
 *      保存されたBackend ContextとBackend Textureが
 *      利用可能であること。
 * @pre 有効なResourceを破棄する場合、
 *      callerが対象Resourceの所有権を保持していること。
 *
 * @post 正常終了時、対象Resourceは破棄され、
 *       *texture_resource_はNULLとなる。
 * @post Backend ContextのOwnershipは変更されない。
 */
void texture_gpu_resource_destroy(texture_gpu_resource_t** texture_resource_);

/**
 * @brief GPU Texture Resourceが所有するTextureをBindする。
 *
 * @par Public API Contract
 * - Resource内部に保持されたBackend Contextと
 *   Backend Textureを使用する。
 * - TextureのBind処理をrenderer_backend_texture_bind()へ委譲する。
 * - 対象Texture UnitのActive化とTexture Bindingは
 *   Renderer Backendが担当する。
 * - DEBUG / TEST Buildではis_valid()によって
 *   ResourceのValidityを確認する。
 * - 本APIはGPU Texture ResourceのOwnershipを変更しない。
 * - Binding状態をResource内部に保存しない。
 *
 * @par Validation
 * - texture_resource_のNULLを全Build Modeで検査する。
 * - DEBUG / TEST BuildではResourceのCanonical Validityを確認する。
 * - RELEASE Buildではis_valid()による事前検証を行わない。
 * - BackendのOperation条件はrenderer_backend_texture_bind()へ
 *   検証を委譲する。
 *
 * @param[in] texture_resource_
 *            Bind対象のGPU Texture Resource。
 *
 * @retval TEXTURE_SUCCESS
 *         TextureのBind成功。
 * @retval TEXTURE_INVALID_ARGUMENT
 *         ResourceがNULL、またはBackendの入力条件が不正。
 * @retval TEXTURE_BAD_OPERATION
 *         BackendのOperationが成立しない。
 * @retval TEXTURE_DATA_CORRUPTED
 *         ResourceのValidity検証、またはBackendの整合性検証に失敗。
 * @retval TEXTURE_RUNTIME_ERROR
 *         Backendで実行時エラーが発生。
 * @retval TEXTURE_UNDEFINED_ERROR
 *         Backendで未分類のエラーが発生。
 *
 * @pre texture_resource_は有効なGPU Texture Resourceを指すこと。
 * @pre Resourceが保持するBackend Contextは利用可能であること。
 *
 * @post 成功時、対象TextureがRenderer BackendにBindされる。
 * @post ResourceのOwnershipは変更されない。
 */
texture_result_t texture_gpu_resource_bind(const texture_gpu_resource_t* texture_resource_);

/**
 * @brief GPU Texture Resourceが所有するTextureをUnbindする。
 *
 * @par Public API Contract
 * - Resource内部に保持されたBackend Contextと
 *   Backend Textureを使用する。
 * - TextureのUnbind処理をrenderer_backend_texture_unbind()へ委譲する。
 * - Texture Unitに対応するUnbind操作はRenderer Backendが担当する。
 * - DEBUG / TEST Buildではis_valid()によって
 *   ResourceのValidityを確認する。
 * - 本APIはGPU Texture ResourceのOwnershipを変更しない。
 * - Binding状態をResource内部に保存しない。
 *
 * @par Validation
 * - texture_resource_のNULLを全Build Modeで検査する。
 * - DEBUG / TEST BuildではResourceのCanonical Validityを確認する。
 * - RELEASE Buildではis_valid()による事前検証を行わない。
 * - BackendのOperation条件はrenderer_backend_texture_unbind()へ
 *   検証を委譲する。
 *
 * @param[in] texture_resource_
 *            Unbind対象のGPU Texture Resource。
 *
 * @retval TEXTURE_SUCCESS
 *         TextureのUnbind成功。
 * @retval TEXTURE_INVALID_ARGUMENT
 *         ResourceがNULL、またはBackendの入力条件が不正。
 * @retval TEXTURE_BAD_OPERATION
 *         BackendのOperationが成立しない。
 * @retval TEXTURE_DATA_CORRUPTED
 *         ResourceのValidity検証、またはBackendの整合性検証に失敗。
 * @retval TEXTURE_RUNTIME_ERROR
 *         Backendで実行時エラーが発生。
 * @retval TEXTURE_UNDEFINED_ERROR
 *         Backendで未分類のエラーが発生。
 *
 * @pre texture_resource_は有効なGPU Texture Resourceを指すこと。
 * @pre Resourceが保持するBackend Contextは利用可能であること。
 *
 * @post 成功時、Renderer Backendによる対象Textureの
 *       Unbind操作が完了する。
 * @post ResourceのOwnershipは変更されない。
 */
texture_result_t texture_gpu_resource_unbind(const texture_gpu_resource_t* texture_resource_);

/**
 * @brief GPU Texture ResourceのCanonical Validityを検証する。
 *
 * @par Validation Scope
 * - texture_resource_がNULLでないことを確認する。
 * - 内部Backend Context PointerがNULLでないことを確認する。
 * - 内部Backend Texture PointerがNULLでないことを確認する。
 * - renderer_backend_texture_is_valid()を呼び出し、
 *   Backend TextureのValidityを検証する。
 * - すべての条件を満たす場合にtrueを返す。
 *
 * @par Validation Boundary
 * - Backend Texture固有のValidity条件は
 *   Renderer BackendのValidatorへ委譲する。
 * - GPU Texture Resource本体のAllocation Validityを
 *   General Allocatorで独立して検証することはない。
 * - Borrowed Backend ContextのNULLは検査するが、
 *   参照先のAllocation ValidityやLifetimeは保証しない。
 * - 入力Pixel Data、Texture Metadata、Binding Stateは
 *   Resource内に保持していないため、検証対象ではない。
 *
 * @par Execution
 * - 本API自体はBuild Modeによらず実行できる。
 * - DEBUG / TEST Buildではcreate()のPostcondition Validationと、
 *   destroy()、bind()、unbind()の入力検証から使用する。
 * - RELEASE Buildでは、これらのAPIからの
 *   自動的なValidator呼び出しを行わない。
 *
 * @param[in] texture_resource_
 *            検証対象のGPU Texture Resource。
 *
 * @retval true
 *         Resourceが保持するPointerとBackend Textureの
 *         Validity条件を満たしている。
 * @retval false
 *         NULL条件またはBackend TextureのValidity検証に失敗。
 *
 * @pre NULL以外のtexture_resource_は読み取り可能な
 *      GPU Texture Resourceを指していること。
 * @pre 内部Backend ContextがNULLでない場合、
 *      参照先のlifetimeが成立していること。
 */
bool texture_gpu_resource_is_valid(const texture_gpu_resource_t* texture_resource_);

#ifdef __cplusplus
}
#endif
#endif
