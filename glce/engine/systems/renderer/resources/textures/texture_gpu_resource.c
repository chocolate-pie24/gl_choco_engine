// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/textures/texture_gpu_resource.h"

#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_texture.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_err_utils.h"

/*
 * Module Internal Contract
 *
 * Canonical State:
 * - texture_gpu_resource_tは有効なBackend Textureと、
 *   その操作に使用するBackend Contextへの参照を保持する。
 * - backend_textureはNULLではない。
 * - backend_contextはNULLではない。
 * - backend_textureはrenderer_backend_texture_is_valid()が
 *   定義するBackend TextureのValidity条件を満たす。
 *
 * Representation / Ownership:
 * - texture_gpu_resource_t自身はGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_RENDERERで確保する。
 * - backend_textureはRenderer Backendが生成したResourceであり、
 *   本Moduleが解放責務を保持する。
 * - backend_contextはcallerからborrowしたPointerであり、
 *   本Moduleは所有・破棄しない。
 * - Backend Contextへの参照はResourceのlifetime中保持する。
 * - GPU Texture Resourceは入力Pixel Dataおよび
 *   texture_resource_info_tへのPointerを保持しない。
 * - GPU側のTexture StorageはRenderer Backendが管理する。
 *
 * Construction:
 * - General AllocatorからResource本体を確保する。
 * - General Allocatorは確保したstorageをゼロ初期化する。
 * - Backend Textureの生成後、対象TextureをBindする。
 * - Texture MetadataとPixel Dataを使用してGPUへ転送する。
 * - Pixel転送完了後、対象TextureをUnbindする。
 * - Backend ContextへのBorrowed Pointerは、
 *   通常のTexture生成処理が完了した後に格納する。
 * - DEBUG / TEST Buildでは完成したResourceのValidityを検証する。
 * - すべての処理が成功した後にResourceの所有権をcallerへ渡す。
 *
 * Construction State:
 * - texture_createdはBackend Textureの生成成功を表す。
 * - texture_boundはBackend TextureのBind成功後、
 *   対応するUnbindが成功するまでの状態を表す。
 * - これらはcreate()内部だけで使用するlocal stateである。
 * - TextureのBinding Stateは完成後のResourceには保持しない。
 * - Construction中の一時ResourceのOwnershipは、
 *   create()が成功してcallerへ出力するまで本Operationに属する。
 *
 * Failure / Cleanup:
 * - Constructionの失敗時にはcleanupへ移行する。
 * - texture_boundがtrueの場合、Backend TextureのUnbindを試行する。
 * - Cleanup時のUnbindが失敗した場合は、
 *   ResultをTEXTURE_DATA_CORRUPTEDへ変更する。
 * - texture_createdがtrueの場合、
 *   Backend Textureの破棄を試行する。
 * - Resource本体が確保済みであればGeneral Allocatorへ返却する。
 * - CleanupはUnbind失敗後もTexture破棄とMemory解放を試行する。
 * - Backend Textureのdestroy()とGeneral Allocatorのfree()は
 *   void APIであり、解放結果を本Operationへ返さない。
 * - したがって、Cleanup失敗時の完全なResource解放や
 *   Backend Binding Stateの復元は保証しない。
 * - 失敗時、生成Resourceの所有権をcallerへ移転しない。
 *
 * Destruction:
 * - destroy()は保存されたBackend Contextを使用して
 *   Backend Textureの破棄を要求する。
 * - 続いてResource本体をGeneral Allocatorへ返却する。
 * - destroy()はvoid APIであり、下位の解放結果を返さない。
 * - 通常の破棄経路の最後にcallerの所有PointerをNULLにする。
 * - Backend Context自体は破棄しない。
 *
 * Validation Responsibility:
 * - texture_resource_info_tのsemantic validityは
 *   texture_resource_info_is_valid()へ委譲する。
 * - Texture Unit、Filter、Wrap、およびGPU操作の条件は
 *   Renderer Backendへ委譲する。
 * - 本Moduleのis_valid()は内部Pointerの存在と
 *   Backend TextureのValidityを確認する。
 * - Backend Texture固有のValidationは
 *   renderer_backend_texture_is_valid()へ委譲する。
 * - Root Object自身のAllocation Validityは検証しない。
 * - Borrowed Backend ContextのLifetimeは検証しない。
 * - 入力Pixel DataのAllocation Validityと内容は検証しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

struct texture_gpu_resource {
    renderer_backend_texture_t* backend_texture;
    const renderer_backend_context_t* backend_context;
};

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - backend_context_、resource_info_、out_texture_resource_、
 *   pixels_のNULLを全Build Modeで検査する。
 * - *out_texture_resource_がNULLであることを要求する。
 * - 出力先がNULLでない場合はTEXTURE_BAD_OPERATIONを返す。
 * - texture_resource_info_is_valid()を使用して
 *   Texture Metadataのsemantic validityを検証する。
 * - Metadataが無効な場合はTEXTURE_INVALID_ARGUMENTを返す。
 * - Texture Unit、Filter、Wrapの有効性は
 *   Renderer Backendのcreate()へ委譲する。
 * - Pixel Dataの内容やAllocation Validityは検証しない。
 *
 * Resource Allocation:
 * - General Allocatorからtexture_gpu_resource_tを確保する。
 * - Memory TagにはGENERAL_ALLOCATOR_MEMORY_TAG_RENDERERを使用する。
 * - General Allocatorによるゼロ初期化を前提として、
 *   backend_textureの初期値はNULLとなる。
 * - Allocationの失敗はTexture Result Codeへ変換する。
 *
 * Backend Texture Construction:
 * - renderer_backend_texture_create()でBackend Textureを生成する。
 * - 生成成功時、texture_createdをtrueにする。
 * - renderer_backend_texture_bind()でTextureをBindする。
 * - Bind成功時、texture_boundをtrueにする。
 * - renderer_backend_texture_pixel_upload()でPixel Dataを転送する。
 * - renderer_backend_texture_unbind()でTextureをUnbindする。
 * - Unbind成功時、texture_boundをfalseにする。
 * - 各Backend APIの失敗はTexture Result Codeへ変換する。
 *
 * Commit / Postconditions:
 * - Texture生成・転送・Unbind完了後、
 *   Resource内部のbackend_contextにbackend_context_を格納する。
 * - DEBUG / TEST Buildでは
 *   texture_gpu_resource_is_valid()で完成状態を検証する。
 * - Postcondition Validationの失敗は
 *   TEXTURE_DATA_CORRUPTEDとする。
 * - 全処理の成功後、tmp_texture_gpu_resourceを
 *   *out_texture_resource_へ出力する。
 * - 所有権の移転はこの出力操作で確定する。
 * - RELEASE BuildではPostcondition Validationを実行しない。
 *
 * Failure / Cleanup:
 * - 失敗時は、生成途中のResourceをcallerへ出力しない。
 * - texture_boundがtrueの場合、
 *   renderer_backend_texture_unbind()を再度呼び出す。
 * - CleanupでUnbindが失敗した場合は、
 *   TEXTURE_DATA_CORRUPTEDを設定する。
 * - texture_createdがtrueで、Backend Contextと
 *   Resource PointerがNULLでない場合、
 *   renderer_backend_texture_destroy()を呼び出す。
 * - Resource PointerがNULLでなければ
 *   general_allocator_free()で解放を試行する。
 * - Cleanup中のUnbindに失敗しても、
 *   Backend Textureの破棄とMemory解放は継続する。
 * - TEXTURE_DATA_CORRUPTEDの場合もCleanupを試行する。
 * - destroy()とfree()はvoidであり、
 *   Cleanupの完全成功をResult Codeからは保証できない。
 * - 失敗時は*out_texture_resource_を変更しない。
 *
 * Validation Boundary:
 * - Resource Metadataのsemantic validationは
 *   Texture DomainのValidatorへ委譲する。
 * - GPU Textureの生成条件とOperationの成否は
 *   Renderer Backendへ委譲する。
 * - 完成したResourceのValidityは
 *   本Moduleのis_valid()によって判定する。
 * - 入力Pixel DataのLifetimeと読み取り可能な範囲は
 *   callerのContractとして扱う。
 */
texture_result_t texture_gpu_resource_create(const renderer_backend_context_t* backend_context_, int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, const texture_resource_info_t* resource_info_, const uint8_t* pixels_, texture_gpu_resource_t** out_texture_resource_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_gpu_resource_t* tmp_texture_gpu_resource = NULL;

    bool texture_created = false;
    bool texture_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_info_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_create", "resource_info_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_create", "out_texture_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, TEXTURE_BAD_OPERATION, texture_result_to_str(TEXTURE_BAD_OPERATION), "texture_gpu_resource_create", "*out_texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_create", "pixels_")
    if(!texture_resource_info_is_valid(resource_info_)) {
        ret = TEXTURE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - Provided resource_info_ is not valid.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(texture_gpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_texture_gpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = texture_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - general_allocator_allocate failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_create(backend_context_, texture_unit_index_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, &tmp_texture_gpu_resource->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_create failed.", texture_result_to_str(ret));
        goto cleanup;
    }
    texture_created = true;

    ret_renderer_backend = renderer_backend_texture_bind(backend_context_, tmp_texture_gpu_resource->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_bind failed.", texture_result_to_str(ret));
        goto cleanup;
    }
    texture_bound = true;

    ret_renderer_backend = renderer_backend_texture_pixel_upload(backend_context_, resource_info_->width, resource_info_->height, resource_info_->channel_count, pixels_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_pixel_upload failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, tmp_texture_gpu_resource->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_unbind failed.", texture_result_to_str(ret));
        goto cleanup;
    }
    texture_bound = false;

    tmp_texture_gpu_resource->backend_context = backend_context_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_gpu_resource_is_valid(tmp_texture_gpu_resource)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - Postcondition validation failed for 'tmp_texture_gpu_resource'.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_texture_resource_ = tmp_texture_gpu_resource;

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
        if(texture_bound) {
            ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, tmp_texture_gpu_resource->backend_texture);
            if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
                ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_unbind failed.", texture_result_to_str(TEXTURE_DATA_CORRUPTED));
                ret = TEXTURE_DATA_CORRUPTED;
            }
        }
        if(texture_created && NULL != backend_context_ && NULL != tmp_texture_gpu_resource) {
            renderer_backend_texture_destroy(backend_context_, &tmp_texture_gpu_resource->backend_texture);
        }
        if(NULL != tmp_texture_gpu_resource) {
            general_allocator_free((void**)&tmp_texture_gpu_resource, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
            tmp_texture_gpu_resource = NULL;
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - texture_resource_または*texture_resource_がNULLの場合は
 *   No-opとしてreturnする。
 * - DEBUG / TEST Buildでは、
 *   texture_gpu_resource_is_valid()によって
 *   ResourceのValidityを確認する。
 * - DEBUG / TEST BuildでValidationに失敗した場合は、
 *   Resourceを破棄せずにreturnする。
 * - RELEASE Buildではis_valid()を実行しない。
 * - 保存されたBackend ContextのLifetimeは
 *   callerが保証するContractとして扱う。
 *
 * Resource Destruction:
 * - Resourceが保持するbackend_contextとbackend_textureを使用し、
 *   renderer_backend_texture_destroy()を呼び出す。
 * - Backend Textureの破棄後、General Allocatorを使用して
 *   texture_gpu_resource_t本体を解放する。
 * - Memory TagにはGENERAL_ALLOCATOR_MEMORY_TAG_RENDERERを使用する。
 * - Backend Context自体は破棄しない。
 *
 * Failure:
 * - 本APIはvoidであり、下位の解放結果を返さない。
 * - renderer_backend_texture_destroy()と
 *   general_allocator_free()もvoid APIである。
 * - そのため、Backend Textureの破棄やMemory解放の
 *   完了をResult Codeによって確認しない。
 * - DEBUG / TEST BuildでValidity検証に失敗した場合は、
 *   Resourceを変更せずに処理を中断する。
 *
 * Postconditions:
 * - 通常の破棄経路では、General Allocatorへの解放要求後、
 *   *texture_resource_をNULLに設定する。
 * - 下位解放処理が内部で失敗した場合でも、
 *   本APIは独立した失敗通知を行わない。
 * - Backend ContextのOwnershipとLifetimeは変更しない。
 */
void texture_gpu_resource_destroy(texture_gpu_resource_t** texture_resource_) {
    if(NULL == texture_resource_) {
        return;
    }
    if(NULL == *texture_resource_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_gpu_resource_is_valid(*texture_resource_)) {
        ERROR_MESSAGE("texture_gpu_resource_destroy(%s) - Provided texture_resource_ is corrupted.", texture_result_to_str(TEXTURE_DATA_CORRUPTED));
        return;
    }
#endif
    renderer_backend_texture_destroy((*texture_resource_)->backend_context, &(*texture_resource_)->backend_texture);
    general_allocator_free((void**)texture_resource_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    *texture_resource_ = NULL;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - texture_resource_のNULLを全Build Modeで検査する。
 * - DEBUG / TEST Buildでは、
 *   texture_gpu_resource_is_valid()を実行する。
 * - ResourceのValidity検証に失敗した場合は
 *   TEXTURE_DATA_CORRUPTEDを返す。
 * - RELEASE Buildではis_valid()を実行しない。
 * - Backend ContextのLifetimeはcallerのContractとして扱う。
 *
 * Operation:
 * - Resource内部のbackend_contextとbackend_textureを取得する。
 * - renderer_backend_texture_bind()へ渡して
 *   Backend TextureのBindを実行する。
 * - BackendのResultをTexture Result Codeへ変換する。
 * - BackendのBind成功時にTEXTURE_SUCCESSを返す。
 *
 * State / Ownership:
 * - 本APIはBackendのBinding Stateを変更する。
 * - texture_gpu_resource_tの内部Fieldは変更しない。
 * - ResourceのOwnership Transferは発生しない。
 * - Resource内部にBinding Stateを保存しない。
 *
 * Failure:
 * - ResourceがNULLの場合はTEXTURE_INVALID_ARGUMENTを返す。
 * - DEBUG / TEST BuildでValidity検証に失敗した場合は
 *   BackendのBind操作を実行しない。
 * - BackendのBind失敗時には変換したResult Codeを返す。
 * - 本Moduleでは独立したRollbackを行わない。
 *
 * Postconditions:
 * - 成功時、Renderer BackendによるBindが完了する。
 * - Resource自身のCanonical Stateは変更しない。
 */
texture_result_t texture_gpu_resource_bind(const texture_gpu_resource_t* texture_resource_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_bind", "texture_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_gpu_resource_is_valid(texture_resource_)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_bind(%s) - provided texture_resource_ is corrupted.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_texture_bind(texture_resource_->backend_context, texture_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_bind(%s) - renderer_backend_texture_bind failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - texture_resource_のNULLを全Build Modeで検査する。
 * - DEBUG / TEST Buildでは、
 *   texture_gpu_resource_is_valid()を実行する。
 * - ResourceのValidity検証に失敗した場合は
 *   TEXTURE_DATA_CORRUPTEDを返す。
 * - RELEASE Buildではis_valid()を実行しない。
 * - Backend ContextのLifetimeはcallerのContractとして扱う。
 *
 * Operation:
 * - Resource内部のbackend_contextとbackend_textureを取得する。
 * - renderer_backend_texture_unbind()へ渡して
 *   Backend TextureのUnbindを実行する。
 * - BackendのResultをTexture Result Codeへ変換する。
 * - BackendのUnbind成功時にTEXTURE_SUCCESSを返す。
 *
 * State / Ownership:
 * - 本APIはBackendのBinding Stateを変更する。
 * - texture_gpu_resource_tの内部Fieldは変更しない。
 * - ResourceのOwnership Transferは発生しない。
 * - Resource内部にBinding Stateを保存しない。
 *
 * Failure:
 * - ResourceがNULLの場合はTEXTURE_INVALID_ARGUMENTを返す。
 * - DEBUG / TEST BuildでValidity検証に失敗した場合は
 *   BackendのUnbind操作を実行しない。
 * - BackendのUnbind失敗時には変換したResult Codeを返す。
 * - 本Moduleでは独立したRollbackを行わない。
 *
 * Postconditions:
 * - 成功時、Renderer BackendによるUnbindが完了する。
 * - Resource自身のCanonical Stateは変更しない。
 */
texture_result_t texture_gpu_resource_unbind(const texture_gpu_resource_t* texture_resource_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, TEXTURE_INVALID_ARGUMENT, texture_result_to_str(TEXTURE_INVALID_ARGUMENT), "texture_gpu_resource_unbind", "texture_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_gpu_resource_is_valid(texture_resource_)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_unbind(%s) - provided texture_resource_ is corrupted.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_texture_unbind(texture_resource_->backend_context, texture_resource_->backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_unbind(%s) - renderer_backend_texture_unbind failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

/*
 * Canonical Validator / Validation Policy
 *
 * Validation Scope:
 * - texture_resource_がNULLの場合はfalseを返す。
 * - backend_contextがNULLの場合はfalseを返す。
 * - backend_textureがNULLの場合はfalseを返す。
 * - renderer_backend_texture_is_valid()を呼び出して
 *   Backend TextureのValidityを確認する。
 * - すべての条件を満たした場合にtrueを返す。
 *
 * Validation Responsibility:
 * - GPU Texture Resource固有の内部Pointer Relationを確認する。
 * - Backend Texture固有のValidityは
 *   Renderer BackendのValidatorへ委譲する。
 * - Texture Resource Metadataのsemantic validityは
 *   Texture DomainのValidatorが所有する。
 * - GPU Texture ResourceはTexture Metadataや
 *   Pixel Dataを保持しないため、これらを再検証しない。
 *
 * Allocation / Lifetime:
 * - 引数texture_resource_自身のAllocation Validityは
 *   本Validatorでは検証しない。
 * - backend_contextはBorrowed Pointerであり、
 *   NULL以外であることだけではLifetimeを保証できない。
 * - Backend Contextが有効な期間内に呼び出すことを
 *   callerのContractとして要求する。
 * - Backend TextureのValidity確認は
 *   renderer_backend_texture_is_valid()へ委譲する。
 *
 * Execution:
 * - 本APIはBuild Modeによらず呼び出せる。
 * - DEBUG / TEST Buildではcreate()のPostconditionと、
 *   destroy()、bind()、unbind()のPreconditionから使用される。
 * - RELEASE Buildでは、これらのAPIによる
 *   自動Validationは実行されない。
 *
 * Result:
 * - falseを返す条件を検出した時点で処理を終了する。
 * - Backend TextureのValidity検証が成功した場合のみ
 *   trueを返す。
 * - 本ValidatorはResourceおよびBackendの状態を変更しない。
 */
bool texture_gpu_resource_is_valid(const texture_gpu_resource_t* texture_resource_) {
    if(NULL == texture_resource_) {
        return false;
    }
    if(NULL == texture_resource_->backend_context) {
        return false;
    }
    if(NULL == texture_resource_->backend_texture) {
        return false;
    }
    if(!renderer_backend_texture_is_valid(texture_resource_->backend_context, texture_resource_->backend_texture)) {
        return false;
    }
    return true;
}
