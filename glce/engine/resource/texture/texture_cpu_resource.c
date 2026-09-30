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
 * - widthは0より大きい。
 * - heightは0より大きい。
 * - channel_countはRGBを表す3、またはRGBAを表す4である。
 * - width * height * channel_countはsize_tで表現可能である。
 * - pixel_data_sizeはwidth * height * channel_countと一致する。
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
 * - pixel elementの内容そのものについては、
 *   Loader等のupstream trust boundaryでtrusted internal representationとして
 *   受理済みであることを前提とする。
 * - Texture CPU Resource module固有のcanonical stateは、
 *   image metadata、pixel data size relation、およびpixel storageのownership relationである。
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
 * - private shallow validatorは、texture_cpu_resource_t自身が保持するscalar fieldと、
 *   scalar field間のlocal structural relationを検証する。
 * - shallow validatorはwidthおよびheightが0より大きいこと、
 *   channel_countが3または4であることを確認する。
 * - width * height * channel_countの計算がsize_tで表現可能であることを確認し、
 *   算出したexpected pixel data sizeとpixel_data_sizeが一致することを確認する。
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
 *   texture_cpu_resource_tをowned pointerとして保持するownerが存在する場合、
 *   そのstorage validityはowner側のownership closureとして扱う。
 *
 * - 現在のcanonical validatorはpixelsがGeneral Allocator上の
 *   current allocationであることまでを確認する。
 * - pixel storageがGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたことは、
 *   現在のGeneral Allocator APIではpointerから再確認できないため、
 *   module boundary contractとして信頼する。
 * - pixelsのactual allocation sizeとpixel_data_sizeの整合性も現在検証しない。
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
    size_t pixel_data_size;
    uint16_t width;         /**< テクスチャ幅 */
    uint16_t height;        /**< テクスチャ高さ(左上原点の画像を基準にする) */
    uint8_t channel_count;  /**< チャンネルカウント(RGB or RGBAのみサポート) */
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
//   public output contractおよびownership move operationに必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_texture_resource_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   BAD_OPERATIONとして扱う。
//
// - width_およびheight_は0より大きいことを要求する。
// - channel_count_はRGBを表す3、またはRGBAを表す4であることを要求する。
// - width_ * height_ * channel_count_の計算がsize_tの表現可能範囲内であることを検証する。
// - 算出したexpected pixel data sizeとpixel_data_size_が一致することを要求する。
//
// - *pixels_が指すpixel storageは、Loader等のupstream trust boundaryで
//   trusted internal representationとして受理済みであることを前提とする。
// - pixel elementの内容そのものについて、Texture CPU Resource moduleでは
//   semantic validationを重複して行わない。
//
// - move対象のpixel storageはGeneral Allocatorから
//   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたlive allocationであることを
//   Module Boundary Contractとして要求する。
// - 現在のGeneral Allocator APIではmemory tagをpointerから完全に再確認できないため、
//   createのPreconditionsではgeneral_allocator_ptr_is_allocated()による
//   allocation provenanceの部分的な再検証も行わず、caller contractとして信頼する。
//
// - texture_cpu_resource_t storageのallocationはGeneral Allocatorへ委譲する。
// - General Allocatorから返されたresultはResource resultへ変換して伝播する。
// - 下位moduleからDATA_CORRUPTEDを受け取った場合は、
//   fail-stop ruleに従ってsuspectなownership graphを辿る通常cleanupを行わない。
//
// - Prepare中にtmp_cpu_resource->pixelsへ*pixels_を格納しても、
//   この時点ではpixel storageのownershipはcallerに残る。
// - pixel storageのownership transferはOutput commit時にのみ成立する。
// - create成功時は完成済みtexture_cpu_resource_tを*out_texture_resource_へ公開した後、
//   *pixels_をNULLへ変更し、pixel storageのownershipをTexture CPU Resource moduleへ移転する。
// - create失敗時は*pixels_を変更せず、pixel storageのownershipはcallerに残る。
// - create失敗時の通常cleanupでは、Texture CPU Resource moduleがまだownershipを
//   commitしていないpixel storageをfreeしない。
//
// - DEBUG_BUILD / TEST_BUILDではtemporary resourceのconstruction完了後、
//   callerへ公開する前のStable boundaryでtexture_cpu_resource_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - このPostcondition validationはupstream pixel dataをtrust boundaryとして
//   再認証するためのものではなく、construction後のtexture_cpu_resource_tが
//   Module Internal Contractを満たすことをdiagnosticとして確認するために行う。
// - canonical Postcondition validationに成功した場合のみ、
//   texture_cpu_resource_tおよびpixel storageのownership transitionをcommitする。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t texture_cpu_resource_create(uint16_t width_, uint16_t height_, uint8_t channel_count_, size_t pixel_data_size_, uint8_t** pixels_, texture_cpu_resource_t** out_texture_resource_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    size_t expected_pixel_data_size = 0;
    const size_t width_size_t = (size_t)width_;
    const size_t height_size_t = (size_t)height_;
    const size_t channel_count_size_t = (size_t)channel_count_;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "out_texture_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "texture_cpu_resource_create", "*out_texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(*pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*pixels_")
    if(0 == width_ || 0 == height_ || (3 != channel_count_ && 4 != channel_count_)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided width_, height_ or channel_count_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / width_size_t) < height_size_t) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / channel_count_size_t) < (width_size_t * height_size_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    expected_pixel_data_size = width_size_t * height_size_t * channel_count_size_t;
    if(expected_pixel_data_size != pixel_data_size_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided pixel_data_size_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(texture_cpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    tmp_cpu_resource->channel_count = channel_count_;
    tmp_cpu_resource->height = height_;
    tmp_cpu_resource->width = width_;
    tmp_cpu_resource->pixels = *pixels_;
    tmp_cpu_resource->pixel_data_size = pixel_data_size_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(tmp_cpu_resource)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Postcondition validation failed for 'tmp_cpu_resource'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
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

// texture_cpu_resource_pixel_size_get Validation Policy
//
// - texture_resource_、out_width_、out_height_およびout_channel_count_の
//   pointer existenceはpublic API contractとして
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - 本operationがconsumeするのはtexture_resource_自身が保持する
//   width、height、channel_countおよびそれらに関係するlocal structural stateであり、
//   owned pixel storageのallocation validityやpixel dataの内容には依存しない。
//
// - DEBUG_BUILD / TEST_BUILDではoutput公開前にprivate shallow validatorを実行し、
//   width、height、channel_count、pixel_data_size間の
//   local structural invariantをPreconditionとして検証する。
// - shallow validationに失敗した場合はRESOURCE_DATA_CORRUPTEDを返し、
//   metadataをcallerへ公開しない。
// - 本operationではowned pixel storageをdereferenceせず、
//   ownership closureをconsumeしないためcanonical validationは行わない。
// - RELEASE_BUILDではautomatic shallow validationを行わず、
//   Module Internal Contractが成立していることを前提としてmetadataを公開する。
//
// - 本operationはtexture_cpu_resource_tおよびowned pixel storageを変更せず、
//   ownership relationも変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t texture_cpu_resource_pixel_size_get(const texture_cpu_resource_t* texture_resource_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_channel_count_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(texture_resource_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_size_get(%s) - Precondition validation failed for 'texture_resource_'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_width_ = texture_resource_->width;
    *out_height_ = texture_resource_->height;
    *out_channel_count_ = texture_resource_->channel_count;

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
//   width、height、channel_count、pixel_data_size間の
//   local structural invariantを検証する。
// - shallow validation成功後、pixels != NULLであることを確認する。
// - owned pixel storageのownership closureとして、
//   general_allocator_ptr_is_allocated()によってpixelsがGeneral Allocator上の
//   current allocationであることを確認する。
//
// - pixel storageがGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたことは、
//   現在のGeneral Allocator APIではpointerから再確認できないため、
//   Module Boundary Contractとして信頼する。
// - pixelsのactual allocation sizeとpixel_data_sizeのrelationも現在検証しない。
// - 将来的にallocation provenanceおよびownership transferを
//   より厳密に検証する必要が生じた場合は、
//   allocation metadataを保持するownership descriptor等の仕組みを検討する。
//
// - pixel elementの内容そのものはupstream trust boundaryで
//   trusted internal representationとして受理済みであることを前提とし、
//   canonical validatorではpixel dataを全走査しない。
//
// - texture_resource_自身のallocation validityは本validatorでは検証しない。
//   texture_resource_をowned pointerとして保持するownerが存在する場合、
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
    size_t expected_pixel_data_size = 0;
    size_t tmp_height = 0;
    size_t tmp_width = 0;
    size_t tmp_channel_count = 0;

    if(NULL == texture_resource_) {
        return false;
    }
    if(0 == texture_resource_->height || 0 == texture_resource_->width || (3 != texture_resource_->channel_count && 4 != texture_resource_->channel_count)) {
        return false;
    }

    tmp_height = texture_resource_->height;
    tmp_width = texture_resource_->width;
    tmp_channel_count = texture_resource_->channel_count;
    if((SIZE_MAX / tmp_height) < tmp_width) {
        return false;
    }
    if((SIZE_MAX / tmp_channel_count) < (tmp_width * tmp_height)) {
        return false;
    }
    expected_pixel_data_size = tmp_width * tmp_height * tmp_channel_count;
    if(expected_pixel_data_size != texture_resource_->pixel_data_size) {
        return false;
    }
    return true;
}
