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
#include "engine/systems/renderer/resources/textures/core/texture_format.h"
#include "engine/systems/renderer/resources/textures/core/texture_err_utils.h"

struct texture_gpu_resource {
    renderer_backend_texture_t* backend_texture;
    const renderer_backend_context_t* backend_context;
};

texture_result_t texture_gpu_resource_create(const renderer_backend_context_t* backend_context_, int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, const texture_resource_info_t* resource_info_, const uint8_t* pixels_, texture_gpu_resource_t** out_texture_resource_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_gpu_resource_t* tmp_texture_gpu_resource = NULL;
    renderer_backend_texture_t* tmp_backend_texture = NULL;

    bool texture_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(resource_info_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, TEXTURE_BAD_OPERATION);
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, TEXTURE_INVALID_ARGUMENT);

    ret_general_allocator = general_allocator_allocate(sizeof(texture_gpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_texture_gpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = texture_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - general_allocator_allocate failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_create(backend_context_, texture_unit_index_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, &tmp_backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_create failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_bind(backend_context_, tmp_backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_bind failed.", texture_result_to_str(ret));
        goto cleanup;
    }
    texture_bound = true;

    if(TEXTURE_FORMAT_RGBA8 == resource_info_->format) {
        ret_renderer_backend = renderer_backend_texture_pixel_upload_rgba8(backend_context_, resource_info_->width, resource_info_->height, pixels_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = texture_result_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_pixel_upload_rgba8 failed.", texture_result_to_str(ret));
            goto cleanup;
        }
    } else if(TEXTURE_FORMAT_RGB8 == resource_info_->format) {
        ret_renderer_backend = renderer_backend_texture_pixel_upload_rgb8(backend_context_, resource_info_->width, resource_info_->height, pixels_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = texture_result_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_pixel_upload_rgb8 failed.", texture_result_to_str(ret));
            goto cleanup;
        }
    } else if(TEXTURE_FORMAT_R8UI == resource_info_->format)  {
        ret_renderer_backend = renderer_backend_texture_pixel_upload_r8ui(backend_context_, resource_info_->width, resource_info_->height, pixels_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret = texture_result_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_pixel_upload_r8ui failed.", texture_result_to_str(ret));
            goto cleanup;
        }
    } else {
        ret = TEXTURE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - Provided resource_info_ is not valid.", texture_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, tmp_backend_texture);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = texture_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_unbind failed.", texture_result_to_str(ret));
        goto cleanup;
    }
    texture_bound = false;

    tmp_texture_gpu_resource->backend_context = backend_context_;
    tmp_texture_gpu_resource->backend_texture = tmp_backend_texture;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_gpu_resource_is_valid(tmp_texture_gpu_resource)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_gpu_resource_create(%s) - Postcondition validation failed for 'tmp_texture_gpu_resource'.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_texture_resource_ = tmp_texture_gpu_resource;
    tmp_texture_gpu_resource = NULL;
    tmp_backend_texture = NULL;

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_DATA_CORRUPTED == ret || TEXTURE_SUCCESS == ret) {
        return ret;
    }
    if(texture_bound) {
        ret_renderer_backend = renderer_backend_texture_unbind(backend_context_, tmp_backend_texture);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("texture_gpu_resource_create(%s) - renderer_backend_texture_unbind failed.", texture_result_to_str(TEXTURE_DATA_CORRUPTED));
            ret = TEXTURE_DATA_CORRUPTED;
            return ret;
        }
    }
    if(NULL != backend_context_ && NULL != tmp_backend_texture) {
        renderer_backend_texture_destroy(backend_context_, &tmp_backend_texture);
    }
    if(NULL != tmp_texture_gpu_resource) {
        general_allocator_free((void**)&tmp_texture_gpu_resource, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        tmp_texture_gpu_resource = NULL;
    }
    return ret;
}

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
    general_allocator_free((void**)texture_resource_, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
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

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, TEXTURE_INVALID_ARGUMENT);
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

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, TEXTURE_INVALID_ARGUMENT);
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
