// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file texture_gpu_resource.h
 * @author chocolate-pie24
 * @brief GPU Texture Resourceの生成、破棄、Bindingおよび
 *        Validity検証を提供する。
 *
 * @details
 * Renderer Backendを使用してGPU Textureを生成し、
 * Texture Handleと対応するBackend Contextへの参照を
 * 1つのGPU Texture Resourceとして管理する。
 *
 * @section texture_gpu_resource_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - 本ModuleはGPU Texture Resource本体と、
 *   対応するRenderer Backend Texture Handleを所有する。
 * - Resource本体はGeneral Allocatorから確保し、
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで管理する。
 * - GPU Textureの生成・破棄はRenderer Backendへ委譲する。
 * - destroy()は所有するBackend TextureとResource本体を解放する。
 * - CPU Texture Resource、Pixel Data、およびTexture Registryは
 *   本Moduleの所有対象ではない。
 *
 * @par Backend Context Borrow / Lifetime
 * - create()はRenderer Backend Contextをborrowする。
 * - GPU Texture Resourceは生成時のBackend Contextへの
 *   Borrowed Pointerを内部に保持する。
 * - Backend ContextのOwnershipは取得しない。
 * - bind()、unbind()、destroy()、is_valid()は、
 *   Resourceが保持するBackend Contextを使用する。
 * - callerはGPU Texture Resourceが存在する間、
 *   対応するBackend Contextを有効に保つ必要がある。
 * - GPU Texture Resourceは対応するBackend Contextより先に
 *   破棄する必要がある。
 *
 * @par Texture Construction
 * - Texture Unit Index、Min / Mag Filter、S / T方向の
 *   Wrap Configをcreate()の引数として受け取る。
 * - GPU Textureの生成と設定値の解釈はRenderer Backendへ委譲する。
 * - GPU Texture生成後、TextureをBindする。
 * - 入力Texture MetadataのWidth、Height、Channel Countと
 *   Pixel Dataを使用してGPUへPixelを転送する。
 * - Pixel転送完了後、TextureをUnbindする。
 * - 本ModuleはTextureの具体的なFilteringやWrappingの値を
 *   固定せず、callerの指定値をBackendへ渡す。
 *
 * @par Input Data / Ownership
 * - texture_resource_info_tは生成時に参照する。
 * - Texture Metadataのsemantic validityは
 *   texture_resource_info_is_valid()によって確認する。
 * - Pixel DataはGPU転送中だけborrowする。
 * - GPU Texture Resourceは入力MetadataとPixel Dataの
 *   Pointerを内部に保持しない。
 * - MetadataとPixel DataのOwnershipは移転しない。
 * - Pixel DataはMetadataに対応する読み取り可能な領域を
 *   参照している必要がある。
 *
 * @par Binding
 * - bind()は保存されたBackend ContextとTexture Handleを使用し、
 *   Renderer BackendへTextureのBindを委譲する。
 * - unbind()も同じBackend ContextとTexture Handleを使用する。
 * - Binding状態はGPU Texture Resource内に別途保持しない。
 * - Bindingの成立条件と実行結果はRenderer Backendが管理する。
 *
 * @par Validation
 * - is_valid()はResource PointerのNULL、
 *   Backend Context PointerのNULL、
 *   Backend Texture PointerのNULLを確認する。
 * - さらにrenderer_backend_texture_is_valid()へ
 *   Backend TextureのValidity検証を委譲する。
 * - Backend Contextの実際のLifetimeや、
 *   Root PointerのAllocation Validityを独立して確認しない。
 * - DEBUG / TEST Buildではcreate()のPostconditionと、
 *   destroy()、bind()、unbind()の入力検証にis_valid()を使用する。
 * - RELEASE Buildではこれらの自動Validationを実行しない。
 * - is_valid()自体はBuild Modeによらず呼び出し可能である。
 *
 * @par Failure / Cleanup
 * - create()の成功時にのみResourceの所有ポインタを出力する。
 * - create()の失敗時には、生成途中のBackend Texture、
 *   Binding StateおよびResource本体のCleanupを試行する。
 * - TextureがBind済みの場合、CleanupでUnbindを試行する。
 * - Cleanup中のUnbind失敗はDATA_CORRUPTEDとして分類する。
 * - Cleanupに失敗した場合、GPU側の状態復元や
 *   Resourceの完全な解放は保証しない。
 * - destroy()はvoidであり、BackendやAllocatorの
 *   解放結果をcallerへ返さない。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_TEXTURE_GPU_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_TEXTURE_GPU_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct texture_resource_info texture_resource_info_t;

texture_result_t texture_gpu_resource_create(
    const renderer_backend_context_t* backend_context_,
    int32_t texture_unit_index_,
    texture_format_t format_,
    texture_min_filter_config_t min_filter_config_,
    texture_mag_filter_config_t mag_filter_config_,
    texture_wrap_config_t wrap_config_s_axis_,
    texture_wrap_config_t wrap_config_t_axis_,
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
