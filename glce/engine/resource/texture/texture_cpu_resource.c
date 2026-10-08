// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file texture_cpu_resource.c
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソースを操作するモジュールAPIの実装
 *
 * @date 2026-05-14
 *
 */
#include "engine/resource/texture/texture_cpu_resource.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - texture_cpu_resource_tが保持するresource_infoは、
 *   texture_resource_info_is_valid()が定義する
 *   GLCE内部texture resource metadataとしてのsemantic validityを満たす。
 * - pixelsはNULLではない。
 * - pixelsはGeneral Allocator上のlive allocationを参照する。
 *
 * Representation / Ownership:
 * - texture_cpu_resource_t object自身はGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得したallocationとして保持する。
 * - pixelsが指すstorageはcreate成功時にcallerからownershipをmoveされたresourceであり、
 *   Texture CPU Resource moduleが単独でownershipを保持する。
 * - moveされたpixel storageはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたallocationであることを
 *   module boundary contractとして前提とする。
 * - texture_cpu_resource_t object自身とpixelsが指すpixel storageは、
 *   それぞれ独立したallocationとして保持する。
 * - Texture CPU Resource moduleはtexture_cpu_resource_t objectのlifetimeとともに
 *   owned pixel storageのlifetimeを管理する。
 *
 * - resource_infoを構成する各fieldおよびfield間relationのsemantic ownershipは
 *   Resource Coreのtexture_resource_info_tに属する。
 * - Texture CPU Resource moduleはそれらのsemanticを独自に再定義せず、
 *   texture_resource_info_is_valid()へvalidationを委譲する。
 *
 * - pixel elementの内容そのものについては、
 *   Loader等のupstream trust boundaryでtrusted internal representationとして
 *   受理済みであることを前提とする。
 * - Texture CPU Resource module固有のcanonical stateは、
 *   validなresource_infoとowned pixel storageの存在、および
 *   pixel storageとのownership relationによって構成される。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractで定義したcanonical stateを基準として行う。
 *
 * - private shallow validatorは、texture_cpu_resource_t自身が保持する
 *   resource_infoのsemantic validityを検証する。
 * - resource_infoを構成するwidth、height、channel_count、pixel_data_sizeおよび
 *   それらのfield間relationのvalidationは、
 *   semantic ownerであるtexture_resource_info_is_valid()へ委譲する。
 * - Texture CPU Resource moduleではresource_info固有のsemantic conditionを
 *   重複して実装しない。
 * - shallow validationではpixelsのpointer existence、
 *   pixelsが指すallocationのvalidity、およびpixel dataの内容を検証しない。
 *
 * - canonical validatorはshallow validation成功後、
 *   pixels != NULLであることを確認する。
 * - owned pixel storageを利用する前提となるownership closureとして、
 *   general_allocator_ptr_is_allocated()によってpixelsがGeneral Allocator上の
 *   current allocationであることを確認する。
 *
 * - canonical validatorはpixel elementの内容を走査しない。
 * - pixel dataの内容そのものはLoader等のupstream trust boundaryで
 *   trusted internal representationとして受理済みであることを前提とし、
 *   Texture CPU Resource moduleではそのsemanticを再認証しない。
 *
 * - canonical validatorは、引数として渡されたtexture_cpu_resource_t自身の
 *   allocation validityを検証しない。
 * - texture_cpu_resource_tをowned pointerとして保持するownerが存在する場合、
 *   そのstorage validityはowner側のownership closureとして扱う。
 *
 * - 現在のcanonical validatorはpixelsがGeneral Allocator上の
 *   current allocationであることまでを確認する。
 * - pixel storageがGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたことは、
 *   現在のGeneral Allocator APIではpointerから再確認できないため、
 *   module boundary contractとして信頼する。
 * - pixelsのactual allocation sizeとresource_info.pixel_data_sizeの整合性も
 *   現在検証しない。
 * - 将来的にownership transferおよびallocation provenanceを
 *   より明示的かつ検証可能にする必要が生じた場合は、
 *   pointer、allocation size、memory tag等のmetadataを保持する
 *   ownership descriptor等の仕組みを検討する。
 *
 * - Texture CPU Resource moduleはexternal dataをtrusted internal representationへ
 *   昇格させるtrust boundaryとして扱わない。
 * - createへ渡されるpixel storageおよびpixel dataは、
 *   upstreamで必要なvalidationが完了したtrusted representationとして扱う。
 *
 * - explicit canonical validatorであるtexture_cpu_resource_is_valid()の
 *   validation semanticsはBUILD_MODEによって変更しない。
 * - public API内部でcanonicalまたはshallow validatorをautomaticに実行するかどうかは、
 *   各operationがconsumeするstate、relation、memory safetyおよび
 *   lifecycle transitionに基づいてAPIごとに決定する。
 *
 * - canonical validationによってDATA_CORRUPTEDが確定した場合、
 *   suspectなownership graphを辿るcleanupまたはresource releaseは行わない。
 *
 * - validatorは対象stateを変更せず、resource allocation、repair、log出力等の
 *   observable side effectを発生させない。
 * - validation failure時はfalseを返す。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief テクスチャCPU側リソース構造体
 *
 */
struct texture_cpu_resource {
    texture_resource_info_t resource_info;
    uint8_t* pixels;        /**< テクスチャピクセルデータ */
};

// ============================================================
// Private Function Declarations
// ============================================================
// Validators
static bool is_valid_shallow(const texture_cpu_resource_t* texture_resource_);

// ============================================================
// Public API
// ============================================================

// texture_cpu_resource_create Validation Policy
//
// - create前には有効なtexture_cpu_resource_tが存在しないため、
//   PreconditionsではTexture CPU Resource validatorを使用しない。
//
// - out_texture_resource_、pixels_および*pixels_のpointer existenceは、
//   public API contractおよびownership moveに必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_texture_resource_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   BAD_OPERATIONとして扱う。
//
// - resource_info_はvalidであることを要求する。
// - resource_info_のsemantic validityはsemantic ownerである
//   texture_resource_info_is_valid()へ委譲する。
// - Texture CPU Resource moduleではresource_info_を構成する各fieldおよび
//   field間relationのvalidationを重複して実装しない。
// - resource_info_がvalidでない場合はRESOURCE_INVALID_ARGUMENTとして扱う。
//
// - *pixels_が指すpixel storageはModule Boundary Contractを満たす
//   trusted internal representationであることをcaller contractとして要求する。
// - pixel elementの内容そのものについてsemantic validationを行わない。
// - pixel storageのallocation provenanceはcaller contractとして信頼し、
//   createのPreconditionsでは再検証しない。
//
// - Prepareではtexture_cpu_resource_t storageをGeneral Allocatorから確保し、
//   resource_info_および*pixels_を使用してcandidate representationを構築する。
// - candidate construction中はpixel storageのownershipをcallerに残す。
//
// - DEBUG_BUILD / TEST_BUILDではcandidate construction完了後、
//   public stateへcommitする前にtexture_cpu_resource_is_valid()を実行し、
//   canonical Commit eligibility validationを行う。
// - Commit eligibility validationはcandidate representationが
//   Module Internal Contractを満たすことをdiagnosticとして確認するものであり、
//   upstream dataのsemanticを再認証するものではない。
// - RELEASE_BUILDではautomatic Commit eligibility validationを行わない。
//
// - Commit成功時にtexture_cpu_resource_tをcallerへ公開し、
//   pixel storageのownershipをTexture CPU Resource moduleへmoveする。
// - create失敗時にはpixel storageのownershipを取得せず、callerに残す。
//
// - 下位moduleからDATA_CORRUPTEDを受け取った場合、または
//   Commit eligibility validationによってDATA_CORRUPTEDが確定した場合は、
//   fail-stop ruleに従いsuspectなownership graphを辿る通常cleanupを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t texture_cpu_resource_create(const texture_resource_info_t* resource_info_, uint8_t** pixels_, texture_cpu_resource_t** out_texture_resource_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "out_texture_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "texture_cpu_resource_create", "*out_texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(*pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*pixels_")
    if(!texture_resource_info_is_valid(resource_info_)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided resource_info is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(texture_cpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    tmp_cpu_resource->resource_info = *resource_info_;
    tmp_cpu_resource->pixels = *pixels_;

    // Commit eligibility.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(tmp_cpu_resource)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Commit eligibility validation failed for 'tmp_cpu_resource'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    *out_texture_resource_ = tmp_cpu_resource;
    tmp_cpu_resource = NULL;
    *pixels_ = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_cpu_resource) {
            general_allocator_free((void**)&tmp_cpu_resource, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
    }

    return ret;
}

// texture_cpu_resource_destroy Validation Policy
//
// - texture_resource_ == NULLまたは*texture_resource_ == NULLはno-opとして扱う。
//
// - DEBUG_BUILD / TEST_BUILDではresource release開始前に
//   texture_cpu_resource_is_valid()を実行し、
//   canonical Precondition validationを行う。
// - canonical validationに失敗した場合はsuspectなowned pixel storageを辿らず、
//   pixel storageおよびtexture_cpu_resource_t storageのfreeを行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてdestroyを実行する。
//
// - owned pixel storageはModule Boundary Contractに従い、
//   General AllocatorからGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得された
//   allocationであることを前提としてreleaseする。
// - owned pixel storageおよびtexture_cpu_resource_t自身のstorage releaseは
//   General Allocatorへ委譲する。
// - destroyによってobject lifetimeが終了するため、
//   Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_resource_) {
    if(NULL == texture_resource_) {
        return;
    }
    if(NULL == *texture_resource_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(*texture_resource_)) {
        ERROR_MESSAGE("texture_cpu_resource_destroy(%s) - Precondition validation failed for '*texture_resource_'.", resource_result_to_str(RESOURCE_DATA_CORRUPTED));
        return;
    }
#endif

    general_allocator_free((void**)&(*texture_resource_)->pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
    general_allocator_free((void**)texture_resource_, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
}

// texture_cpu_resource_pixels_get Validation Policy
//
// - texture_resource_およびout_pixels_のpointer existenceは
//   public API contractとしてRELEASE_BUILDを含む全BUILDで検証する。
// - *out_pixels_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   BAD_OPERATIONとして扱う。
//
// - 本operationはowned pixel storageへのborrowed pointerをcallerへ公開する。
// - returned pointerの利用可能性はowned pixel storageのlivenessおよび
//   texture_cpu_resource_tとのownership relationに依存するため、
//   本operationはownership closureをconsumeする。
//
// - DEBUG_BUILD / TEST_BUILDではoutput公開前に
//   texture_cpu_resource_is_valid()を実行し、
//   texture_resource_にcanonical Precondition validationを行う。
// - canonical validationに失敗した場合はRESOURCE_DATA_CORRUPTEDを返し、
//   borrowed pixel viewをcallerへ公開しない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてborrowed viewを公開する。
//
// - returned pixel pointerはborrowed viewであり、
//   pixel storageのownershipはTexture CPU Resource moduleに残る。
// - 本operationはtexture_cpu_resource_tおよびowned pixel storageを変更せず、
//   ownership relationも変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t texture_cpu_resource_pixels_get(const texture_cpu_resource_t* texture_resource_, const uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixels_get", "texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixels_get", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "texture_cpu_resource_pixels_get", "*out_pixels_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(texture_resource_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixels_get(%s) - Precondition validation failed for 'texture_resource_'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_pixels_ = texture_resource_->pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// texture_cpu_resource_resource_info_get Validation Policy
//
// - texture_resource_およびout_resource_info_のpointer existenceは
//   public API contractとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - 本operationがconsumeするのはtexture_resource_自身が保持する
//   resource_infoのlocal semantic stateであり、
//   owned pixel storageのallocation validityやpixel dataの内容には依存しない。
//
// - DEBUG_BUILD / TEST_BUILDではoutput公開前にprivate shallow validatorを実行する。
// - shallow validatorはembedded resource_infoのsemantic validityを
//   texture_resource_info_is_valid()へ委譲して確認する。
// - shallow validationに失敗した場合はRESOURCE_DATA_CORRUPTEDを返し、
//   resource_infoをcallerへ公開しない。
// - 本operationではowned pixel storageをdereferenceせず、
//   ownership closureをconsumeしないためcanonical validationは行わない。
// - RELEASE_BUILDではautomatic shallow validationを行わず、
//   Module Internal Contractが成立していることを前提としてresource_infoを公開する。
//
// - outputにはtexture_resource_が保持するresource_infoのvalue copyを返す。
// - 本operationはtexture_cpu_resource_tおよびowned pixel storageを変更せず、
//   ownership relationも変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t texture_cpu_resource_resource_info_get(const texture_cpu_resource_t* texture_resource_, texture_resource_info_t* out_resource_info_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_resource_info_get", "texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_info_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_resource_info_get", "out_resource_info_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(texture_resource_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_resource_info_get(%s) - Precondition validation failed for 'texture_resource_'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_resource_info_ = texture_resource_->resource_info;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// texture_cpu_resource_is_valid Validation Policy
//
// - 本APIはtexture_cpu_resource_tのpublic canonical validatorである。
// - texture_resource_ == NULLの場合はfalseを返す。
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
//
// - 最初にprivate shallow validatorを実行し、
//   embedded resource_infoのsemantic validityを確認する。
// - resource_infoを構成する各fieldおよびfield間relationのvalidationは
//   texture_resource_info_is_valid()へ委譲し、
//   Texture CPU Resource moduleでは重複して実装しない。
//
// - shallow validation成功後、pixels != NULLであることを確認する。
// - owned pixel storageのownership closureとして、
//   general_allocator_ptr_is_allocated()によってpixelsがGeneral Allocator上の
//   current allocationであることを確認する。
//
// - pixel storageがGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたことは、
//   現在のGeneral Allocator APIではpointerから再確認できないため、
//   Module Boundary Contractとして信頼する。
// - pixelsのactual allocation sizeとresource_info.pixel_data_sizeのrelationも
//   現在検証しない。
// - 将来的にallocation provenanceおよびownership transferを
//   より厳密に検証する必要が生じた場合は、
//   allocation metadataを保持するownership descriptor等の仕組みを検討する。
//
// - pixel elementの内容そのものはupstream trust boundaryで
//   trusted internal representationとして受理済みであることを前提とし、
//   canonical validatorではpixel dataを全走査しない。
//
// - texture_resource_自身のallocation validityは本validatorでは検証しない。
// - texture_resource_をowned pointerとして保持するownerが存在する場合、
//   そのstorage validityはowner側のownership closureとして扱う。
//
// - validatorは対象stateを変更せず、resource allocation、repair、log出力等の
//   observable side effectを発生させない。
// - validation failure時はfalseを返す。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_resource_) {
    if(NULL == texture_resource_) {
        return false;
    }
    if(!is_valid_shallow(texture_resource_)) {
        return false;
    }
    if(NULL == texture_resource_->pixels) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*) texture_resource_->pixels)) {
        return false;
    }
    return true;
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const texture_cpu_resource_t* texture_resource_) {
    if(NULL == texture_resource_) {
        return false;
    }
    if(!texture_resource_info_is_valid(&texture_resource_->resource_info)) {
        return false;
    }
    return true;
}
