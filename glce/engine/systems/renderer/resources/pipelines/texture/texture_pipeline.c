// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/pipelines/texture/texture_pipeline.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"
#include "engine/systems/renderer/resources/loaders/bmp_loader.h"
#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/textures/texture_cpu_resource.h"
#include "engine/systems/renderer/resources/textures/texture_gpu_resource.h"
#include "engine/systems/renderer/resources/registries/texture/texture_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

/*
 * Module Internal Contract
 *
 * Module State:
 * - 本Moduleは永続的な内部状態を保持しない。
 * - Renderer Backend ContextとTexture Registryはcallerからborrowする。
 * - CPU / GPU Texture Resourceの生成、Registry登録、
 *   および利用権返却を一連のOperationとして調整する。
 *
 * Pixel Data Construction:
 * - BMP ImportではBMP Loaderを使用してPixel Dataと
 *   Texture Resource Metadataを取得する。
 * - Solid Color Importでは32 x 32 Pixel、RGB 3 Channelの
 *   Pixel Dataを内部生成する。
 * - Pixel DataはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで確保される。
 * - BMP Loaderによる外部ファイルのsemantic validationと
 *   Pixel Dataの正規化はLoader Moduleへ委譲する。
 * - Solid Color Importでは全Pixelに指定RGB値を設定する。
 *
 * Temporary Ownership:
 * - Pixel Data生成成功後、CPU Texture Resourceの
 *   生成成功までは本OperationがPixel Dataを所有する。
 * - texture_cpu_resource_create()成功時、
 *   Pixel DataのOwnershipはCPU Texture Resourceへ移転し、
 *   tmp_pixelsはNULLとなる。
 * - CPU Texture Resourceから取得するtmp_pixels2は
 *   Pixel Dataへのconst borrowであり、個別に解放しない。
 * - texture_gpu_resource_create()は借用したPixel Dataから
 *   GPU Textureを生成する。
 * - GPU Texture ResourceはCPU側Pixel DataのOwnershipを
 *   取得しない。
 * - Registry登録までは本OperationがCPU / GPU Texture Resourceを
 *   一時的に所有する。
 *
 * GPU Texture:
 * - Texture Min / Mag FilterにはNEARESTを指定する。
 * - Texture S / T WrapにはCLAMP_TO_EDGEを指定する。
 * - Texture Unit Indexはcallerの指定値を下位Moduleへ渡す。
 * - GPU Textureの生成、転送、内部状態のvalidationは
 *   Texture GPU Resource Moduleへ委譲する。
 * - GPU Texture Resourceは生成時に借用した
 *   Renderer Backend Contextへのポインタを保持する。
 * - CPU / GPU Resource登録後も、対応するBackend Contextの
 *   lifetimeがGPU Texture Resourceより短くなってはならない。
 *
 * Registration / Commit:
 * - CPU / GPU Texture Resourceを対としてTexture Registryへ渡す。
 * - Resource Nameの複製、重複登録判定、登録Slotの選択、
 *   Reference Countの初期化はTexture Registryへ委譲する。
 * - Registryのregister()成功が、
 *   CPU / GPU Texture ResourceのOwnership Transfer完了点となる。
 * - 登録成功後、cpu_resourceとgpu_resourceはNULLとなる。
 * - Texture IDはRegistry登録成功後にcallerへ出力する。
 * - 登録成功後のResource LifetimeはTexture Registryが管理する。
 *
 * Failure / Cleanup:
 * - DATA_CORRUPTED以外の失敗時は、
 *   本Operationが所有するResourceをcleanupで解放する。
 * - tmp_pixelsが残っている場合はGeneral Allocatorへ返却する。
 * - GPU Texture Resourceをdestroyした後、
 *   CPU Texture Resourceをdestroyする。
 * - Pixel DataのOwnership Transfer後は、
 *   CPU Texture Resourceのdestroy()がPixel Dataを解放する。
 * - 下位ModuleのResource解放処理の成立条件は、
 *   各ModuleのContractへ委譲する。
 * - DATA_CORRUPTED発生時は通常のcleanupを行わず、
 *   Resourceの解放やOwnership状態の復元を保証しない。
 * - 失敗時はTexture IDの出力先を変更しない。
 *
 * Validation Responsibility:
 * - 本Moduleは公開APIで必要なPointerのNULLを直接検査する。
 * - BMPファイルのsemantic validationはBMP Loaderへ委譲する。
 * - Texture Resource Metadataのsemantic validityは
 *   Texture DomainのValidatorを使用する下位Moduleへ委譲する。
 * - CPU Pixel StorageのOwnershipとValidityは
 *   Texture CPU Resource Moduleへ委譲する。
 * - GPU Textureの生成条件とBackend操作のvalidationは
 *   Texture GPU Resource Moduleへ委譲する。
 * - Resource Name、登録状態、Reference Countのvalidationは
 *   Texture Registryへ委譲する。
 * - 本Moduleでは下位Resource全体のcanonical validationを
 *   重複して実行しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

static resource_pipeline_result_t bmp_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_);
static resource_pipeline_result_t solid_color_texture_generate(uint8_t red_, uint8_t green_, uint8_t blue_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_);

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - backend_context_、registry_、resource_name_、
 *   texture_fullpath_、out_texture_id_のNULLを
 *   全Build Modeで検査する。
 * - File PathとBMPファイル内容のvalidationは
 *   bmp_load()およびBMP Loaderへ委譲する。
 * - Resource Nameの空文字列条件と重複登録の判定は、
 *   Texture Registryのregister()へ委譲する。
 * - Texture Unit Indexの有効性は下位GPU Resourceと
 *   Renderer BackendのContractに委譲する。
 * - 本APIではTexture RegistryやBackend Contextの
 *   canonical validationを直接実行しない。
 *
 * Pixel Data Loading:
 * - bmp_load()を使用してPixel Dataと
 *   texture_resource_info_tを取得する。
 * - bmp_load()成功時、tmp_pixelsのOwnershipを取得する。
 * - BMP Loaderが成立させた外部データのsemantic validityを信頼し、
 *   本APIで同じvalidationを繰り返さない。
 *
 * CPU Resource Construction:
 * - texture_cpu_resource_create()へresource_infoと
 *   tmp_pixelsの所有ポインタを渡す。
 * - 成功時、Pixel DataのOwnershipはCPU Resourceへ移転する。
 * - Ownership Transfer後のtmp_pixelsはNULLとなる。
 * - texture_cpu_resource_pixels_get()を使用して、
 *   CPU ResourceのPixel Dataをconst borrowとして取得する。
 * - 借用したPixel Dataは個別に解放しない。
 *
 * GPU Resource Construction:
 * - texture_gpu_resource_create()を使用して
 *   GPU Texture Resourceを生成する。
 * - GPU Textureの生成には借用したPixel Dataを使用する。
 * - FilterにはNEAREST、WrapにはCLAMP_TO_EDGEを指定する。
 * - Backend ContextとTexture Unit Indexを下位Moduleへ渡す。
 * - GPU Texture ResourceはBackend Contextをborrowし、
 *   その参照を内部に保持する。
 *
 * Commit:
 * - texture_registry_register()へResource Nameと
 *   CPU / GPU Resourceの所有ポインタを渡す。
 * - 登録成功によって両ResourceのOwnershipはRegistryへ移転する。
 * - Registry側がReference Countを1に初期化する。
 * - 登録成功後、tmp_texture_idをout_texture_id_へ出力する。
 *
 * Failure / Cleanup:
 * - 下位APIの失敗は対応するPipeline Result Codeへ変換する。
 * - 回復可能な失敗では、
 *   tmp_pixelsが残っていればGeneral Allocatorへ返却する。
 * - 生成済みGPU Texture Resourceをdestroyする。
 * - 生成済みCPU Texture Resourceをdestroyする。
 * - CPU ResourceへのOwnership Transfer後は、
 *   Pixel Dataを個別に解放しない。
 * - Registry登録前の回復可能な失敗では、
 *   CPU / GPU ResourceのOwnershipは本Operationに残る。
 * - DATA_CORRUPTEDの場合はcleanupを中断してreturnする。
 * - この場合、Resourceの解放完了やOwnership状態の復元を
 *   保証しない。
 * - 失敗時はout_texture_id_を変更しない。
 *
 * Postconditions:
 * - 成功時、Texture RegistryがCPU / GPU Resourceの
 *   Registered Stateを保持する。
 * - 成功時、cpu_resourceとgpu_resourceはNULLとなる。
 * - 成功時、Pixel Dataは登録済みCPU Resourceによって所有される。
 * - Pipeline側でRegistryのpostcondition validationを
 *   重複して実行しない。
 */
resource_pipeline_result_t texture_pipeline_import_from_bmp(const renderer_backend_context_t* backend_context_, texture_registry_t* registry_, int32_t texture_unit_index_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    texture_result_t ret_texture = TEXTURE_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    texture_cpu_resource_t* cpu_resource = NULL;
    texture_gpu_resource_t* gpu_resource = NULL;

    texture_resource_info_t resource_info = { 0 };
    uint8_t* tmp_pixels = NULL; // CPUリソースにmoveされるピクセルデータ
    const uint8_t* tmp_pixels2 = NULL;  // CPUリソースから借用するピクセルデータ
    uint16_t tmp_texture_id = 0;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "texture_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_bmp", "out_texture_id_")

    ret = bmp_load(texture_fullpath_, &resource_info, &tmp_pixels);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - bmp_load failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // CPU側リソース生成
    ret_texture = texture_cpu_resource_create(&resource_info, &tmp_pixels, &cpu_resource);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_cpu_resource_create failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }
    // texture_cpu_resource_createによってtmp_pixelsはNULLになっているためCPUリソースから借用
    ret_texture = texture_cpu_resource_pixels_get(cpu_resource, &tmp_pixels2);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_cpu_resource_pixels_get failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // GPU側リソース生成
    ret_texture = texture_gpu_resource_create(backend_context_, texture_unit_index_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &resource_info, tmp_pixels2, &gpu_resource);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_gpu_resource_create failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // Registry登録(gpu_resource, cpu_resourceはregistry側にmoveされる)
    ret_resource_registry = texture_registry_register(registry_, resource_name_, &gpu_resource, &cpu_resource, &tmp_texture_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("texture_pipeline_import_from_bmp(%s) - texture_registry_register failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = tmp_texture_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            general_allocator_free((void**)&tmp_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
        texture_gpu_resource_destroy(&gpu_resource);
        texture_cpu_resource_destroy(&cpu_resource);
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - backend_context_、registry_、resource_name_、
 *   out_texture_id_のNULLを全Build Modeで検査する。
 * - RGB値はuint8_tで受け取るため、
 *   個別の数値範囲検査を行わない。
 * - Resource Nameの空文字列条件と重複登録の判定は、
 *   Texture Registryのregister()へ委譲する。
 * - Texture Unit Indexの有効性は下位GPU Resourceと
 *   Renderer BackendのContractに委譲する。
 * - 本APIではTexture RegistryやBackend Contextの
 *   canonical validationを直接実行しない。
 *
 * Pixel Data Generation:
 * - solid_color_texture_generate()を使用して
 *   32 x 32 Pixel、RGB 3 ChannelのPixel Dataを生成する。
 * - 全Pixelに指定RGB値を設定する。
 * - 生成成功時、tmp_pixelsのOwnershipを取得する。
 * - Resource Metadataは同Helperによって設定される。
 *
 * CPU Resource Construction:
 * - texture_cpu_resource_create()へresource_infoと
 *   tmp_pixelsの所有ポインタを渡す。
 * - 成功時、Pixel DataのOwnershipはCPU Resourceへ移転する。
 * - Ownership Transfer後のtmp_pixelsはNULLとなる。
 * - texture_cpu_resource_pixels_get()を使用して、
 *   CPU ResourceのPixel Dataをconst borrowとして取得する。
 * - 借用したPixel Dataは個別に解放しない。
 *
 * GPU Resource Construction:
 * - texture_gpu_resource_create()を使用して
 *   GPU Texture Resourceを生成する。
 * - GPU Textureの生成には借用したPixel Dataを使用する。
 * - FilterにはNEAREST、WrapにはCLAMP_TO_EDGEを指定する。
 * - Backend ContextとTexture Unit Indexを下位Moduleへ渡す。
 * - GPU Texture ResourceはBackend Contextをborrowし、
 *   その参照を内部に保持する。
 *
 * Commit:
 * - texture_registry_register()へResource Nameと
 *   CPU / GPU Resourceの所有ポインタを渡す。
 * - 登録成功によって両ResourceのOwnershipはRegistryへ移転する。
 * - Registry側がReference Countを1に初期化する。
 * - 登録成功後、tmp_texture_idをout_texture_id_へ出力する。
 *
 * Failure / Cleanup:
 * - 下位APIの失敗は対応するPipeline Result Codeへ変換する。
 * - 回復可能な失敗では、
 *   tmp_pixelsが残っていればGeneral Allocatorへ返却する。
 * - 生成済みGPU Texture Resourceをdestroyする。
 * - 生成済みCPU Texture Resourceをdestroyする。
 * - CPU ResourceへのOwnership Transfer後は、
 *   Pixel Dataを個別に解放しない。
 * - Registry登録前の回復可能な失敗では、
 *   CPU / GPU ResourceのOwnershipは本Operationに残る。
 * - DATA_CORRUPTEDの場合はcleanupを中断してreturnする。
 * - この場合、Resourceの解放完了やOwnership状態の復元を
 *   保証しない。
 * - 失敗時はout_texture_id_を変更しない。
 *
 * Postconditions:
 * - 成功時、Texture RegistryがCPU / GPU Resourceの
 *   Registered Stateを保持する。
 * - 成功時、cpu_resourceとgpu_resourceはNULLとなる。
 * - 成功時、Pixel Dataは登録済みCPU Resourceによって所有される。
 * - Pipeline側でRegistryのpostcondition validationを
 *   重複して実行しない。
 */
resource_pipeline_result_t texture_pipeline_import_from_solid_color(const renderer_backend_context_t* backend_context_, texture_registry_t* registry_, int32_t texture_unit_index_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    texture_result_t ret_texture = TEXTURE_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    texture_cpu_resource_t* cpu_resource = NULL;
    texture_gpu_resource_t* gpu_resource = NULL;

    texture_resource_info_t resource_info = { 0 };
    uint8_t* tmp_pixels = NULL; // CPUリソースにmoveされるピクセルデータ
    const uint8_t* tmp_pixels2 = NULL;  // CPUリソースから借用するピクセルデータ
    uint16_t tmp_texture_id = 0;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_import_from_solid_color", "out_texture_id_")

    ret = solid_color_texture_generate(red_, green_, blue_, &resource_info, &tmp_pixels);
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - solid_color_texture_generate failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // CPU側リソース生成
    ret_texture = texture_cpu_resource_create(&resource_info, &tmp_pixels, &cpu_resource);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_cpu_resource_create failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }
    // texture_cpu_resource_createによってtmp_pixelsはNULLになっているためCPUリソースから借用
    ret_texture = texture_cpu_resource_pixels_get(cpu_resource, &tmp_pixels2);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_cpu_resource_pixels_get failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // GPU側リソース生成
    ret_texture = texture_gpu_resource_create(backend_context_, texture_unit_index_, TEXTURE_MIN_FILTER_CONFIG_NEAREST, TEXTURE_MAG_FILTER_CONFIG_NEAREST, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &resource_info, tmp_pixels2, &gpu_resource);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = resource_pipeline_result_convert_texture(ret_texture);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_gpu_resource_create failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    // Registry登録(gpu_resource, cpu_resourceはregistry側にmoveされる)
    ret_resource_registry = texture_registry_register(registry_, resource_name_, &gpu_resource, &cpu_resource, &tmp_texture_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("texture_pipeline_import_from_solid_color(%s) - texture_registry_register failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = tmp_texture_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(RESOURCE_PIPELINE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            general_allocator_free((void**)&tmp_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
        texture_gpu_resource_destroy(&gpu_resource);
        texture_cpu_resource_destroy(&cpu_resource);
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - registry_のNULLを全Build Modeで検査する。
 * - Texture IDの範囲、EntryのRegistered State、
 *   Reference CountおよびRegistryの内部整合性は、
 *   texture_registry_release()へ検証を委譲する。
 * - Pipeline自身はRegistryのcanonical validationを
 *   重複して実行しない。
 *
 * Commit:
 * - texture_registry_release()を呼び出し、
 *   指定Textureの利用権を1つ返却する。
 * - Reference Countが2以上の場合、
 *   RegistryはCountを1減少させResourceを維持する。
 * - Reference Countが1の場合、
 *   RegistryはResource NameとCPU / GPU Texture Resourceを破棄し、
 *   EntryをEmpty Stateへ戻す。
 * - 本PipelineはReference CountやEntryを直接変更しない。
 *
 * Backend Context Lifetime:
 * - GPU Texture Resourceは生成時のBackend Contextをborrowしている。
 * - GPU Texture Resourceの破棄が必要な場合、
 *   そのBackend Contextは有効である必要がある。
 * - 本APIはBackend Contextを直接受け取らない。
 *
 * Result:
 * - Registryのrelease()が返したResultを
 *   Resource PipelineのResult Codeへ変換する。
 * - Registryのrelease()成功時は
 *   RESOURCE_PIPELINE_SUCCESSを返す。
 *
 * Failure:
 * - 本Pipeline自身はResourceを所有していないため、
 *   独立したrollback処理を持たない。
 * - RegistryのPrecondition Validationで失敗した場合は、
 *   利用権返却処理を実行しない。
 * - DATA_CORRUPTEDの場合、下位Resourceの解放完了や
 *   内部状態の復元を保証しない。
 *
 * Postconditions:
 * - 利用権返却後のEntry StateはTexture Registryが管理する。
 * - Pipeline側では追加のcanonical validationを実行しない。
 */
resource_pipeline_result_t texture_pipeline_release(texture_registry_t* registry_, uint16_t texture_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "texture_pipeline_release", "registry_")

    ret_resource_registry = texture_registry_release(registry_, texture_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("texture_pipeline_release(%s) - texture_registry_release failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

/*
 * Private Helper Contract / Validation Policy
 *
 * Responsibility:
 * - BMP Loaderを呼び出してPixel Dataと
 *   Texture Resource Metadataを取得する。
 * - LoaderのResultをResource PipelineのResult Codeへ変換する。
 * - 外部BMPの解析やPixel Dataの正規化を独自に実行しない。
 *
 * Preconditions:
 * - fullpath_、out_pixels_、out_resource_info_のNULLを
 *   全Build Modeで検査する。
 * - *out_pixels_はNULLであることを要求する。
 * - *out_pixels_がNULLでない場合はBAD_OPERATIONとする。
 * - File PathとBMP内容のsemantic validityは
 *   bmp_loader_load()へ委譲する。
 * - 本HelperではLoaderのValidationを重複しない。
 *
 * Resource Construction:
 * - bmp_loader_load()へFile Pathとlocal outputを渡す。
 * - Loader成功時、tmp_resource_infoとtmp_pixelsには
 *   検証・正規化済みのTexture情報が格納される。
 * - Pixel DataのstorageはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで確保される。
 *
 * Commit:
 * - Loader成功後、Texture Metadataをout_resource_info_へ出力する。
 * - tmp_pixelsをout_pixels_へ渡してOwnershipを移転する。
 * - callerは成功後のPixel Dataの解放責務を引き受ける。
 *
 * Failure:
 * - Loaderの失敗時はResult Codeを変換して返す。
 * - Loader失敗時の一時Resource管理はLoaderのContractに委譲する。
 * - 本HelperはPixel Dataの独立したAllocationや
 *   Rollback処理を行わない。
 * - 失敗時はout_resource_info_と* out_pixels_を変更しない。
 *
 * Postconditions:
 * - 成功時、callerはPixel Dataと対応するResource Metadataを取得する。
 * - 成功時、Pixel DataのOwnershipはcallerへ移転する。
 */
static resource_pipeline_result_t bmp_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    loader_result_t ret_loader = LOADER_INVALID_ARGUMENT;

    uint8_t* tmp_pixels = NULL;
    texture_resource_info_t tmp_resource_info = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_PIPELINE_BAD_OPERATION, resource_pipeline_result_to_str(RESOURCE_PIPELINE_BAD_OPERATION), "bmp_load", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_info_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "bmp_load", "out_resource_info_")

    ret_loader = bmp_loader_load(fullpath_, &tmp_resource_info, &tmp_pixels);
    if(LOADER_SUCCESS != ret_loader) {
        ret = resource_pipeline_result_convert_loader(ret_loader);
        ERROR_MESSAGE("bmp_load(%s) - Failed to load BMP file(%s).", resource_pipeline_result_to_str(ret), fullpath_);
        goto cleanup;
    }

    *out_resource_info_ = tmp_resource_info;
    *out_pixels_ = tmp_pixels;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

/*
 * Private Helper Contract / Validation Policy
 *
 * Responsibility:
 * - 指定RGB値で構成された32 x 32 PixelのTexture Dataを生成する。
 * - Pixel Dataと対応するtexture_resource_info_tをcallerへ出力する。
 *
 * Preconditions:
 * - out_pixels_とout_resource_info_のNULLを
 *   全Build Modeで検査する。
 * - *out_pixels_はNULLであることを要求する。
 * - *out_pixels_がNULLでない場合はINVALID_ARGUMENTとする。
 * - red_、green_、blue_はuint8_tであり、
 *   追加の数値範囲検査を行わない。
 *
 * Pixel Representation:
 * - Widthは32、Heightは32、Channel Countは3とする。
 * - Pixel Data Sizeは32 * 32 * 3 Byteとする。
 * - PixelはRGB順の連続した3 Byteで表現する。
 * - Row PaddingやAlpha Channelは含まない。
 * - 全Pixelに同じRGB値を設定する。
 *
 * Allocation:
 * - Pixel DataはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで確保する。
 * - 生成中はtmp_pixelsがAllocationを一時的に所有する。
 * - 固定サイズのPixel Bufferに対して書き込みを行う。
 *
 * Commit:
 * - すべてのPixelを書き込んだ後、
 *   out_resource_info_へWidth、Height、Channel Count、
 *   Pixel Data Sizeを設定する。
 * - 完成したPixel Dataの所有ポインタをout_pixels_へ移転する。
 * - Ownership Transfer後、tmp_pixelsをNULLにする。
 *
 * Failure / Cleanup:
 * - Allocation失敗時はGeneral AllocatorのResultを
 *   Resource PipelineのResult Codeへ変換する。
 * - tmp_pixelsが残っている場合はGeneral Allocatorへ返却する。
 * - Pixel DataのOwnership Transfer前に失敗した場合、
 *   callerはPixel DataのOwnershipを取得しない。
 * - 失敗時はout_resource_info_と*out_pixels_を変更しない。
 *
 * Postconditions:
 * - 成功時、out_resource_info_は32 x 32 RGB Textureの
 *   Resource Metadataを保持する。
 * - 成功時、*out_pixels_は3072 ByteのPixel Dataを指す。
 * - 成功時、Pixel DataのOwnershipはcallerへ移転する。
 */
static resource_pipeline_result_t solid_color_texture_generate(uint8_t red_, uint8_t green_, uint8_t blue_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    const int32_t tmp_width = 32;
    const int32_t tmp_height = 32;
    const uint8_t tmp_channel_count = 3;
    const size_t pixel_size = (size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count;
    uint8_t* tmp_pixels = NULL;
    size_t pixel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_info_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "solid_color_texture_generate", "out_resource_info_")

    ret_general_allocator = general_allocator_allocate(pixel_size, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_pipeline_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("solid_color_texture_generate(%s) - general_allocator_allocate failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    pixel_count = (size_t)tmp_width * (size_t)tmp_height;
    for(size_t i = 0, ii = 0; i != pixel_count; ++i, ii += 3) {
        tmp_pixels[ii] = red_;
        tmp_pixels[ii + 1] = green_;
        tmp_pixels[ii + 2] = blue_;
    }

    out_resource_info_->channel_count = tmp_channel_count;
    out_resource_info_->height = tmp_height;
    out_resource_info_->pixel_data_size = pixel_size;
    out_resource_info_->width = tmp_width;
    *out_pixels_ = tmp_pixels;

    tmp_pixels = NULL;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(NULL != tmp_pixels) {
        general_allocator_free((void**)&tmp_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        tmp_pixels = NULL;
    }
    return ret;
}
