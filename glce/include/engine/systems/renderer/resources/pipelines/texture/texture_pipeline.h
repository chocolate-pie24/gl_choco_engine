// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file texture_pipeline.h
 * @author chocolate-pie24
 * @brief TextureのCPU / GPU Resource生成、Registry登録、
 *        および利用権返却を提供する。
 *
 * @details
 * BMPファイルまたは指定RGB色からPixel Dataを取得・生成し、
 * CPU Texture ResourceとGPU Texture Resourceを構築して、
 * Texture Registryへ登録するPipeline APIを提供する。
 *
 * 本PipelineはResourceの生成・登録手順を調整するものであり、
 * 登録済みResourceを恒久的に所有しない。
 *
 * @section texture_pipeline_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - BMPファイルの読み込みとPixel Dataの正規化は
 *   BMP Loaderへ委譲する。
 * - CPU Texture Resourceの構築とPixel Dataの所有は
 *   Texture CPU Resource Moduleへ委譲する。
 * - GPU Textureの生成とPixel Dataの転送は
 *   Texture GPU Resource Moduleへ委譲する。
 * - Resource Nameの複製、登録状態およびReference Countの管理は
 *   Texture Registryへ委譲する。
 * - 新規登録成功時、CPU / GPU Texture Resourceの
 *   所有権はTexture Registryへ移転する。
 * - 本PipelineはTexture Registryをborrowし、所有しない。
 *
 * @par Pixel Data Ownership
 * - BMP Loaderが生成するPixel DataはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで確保される。
 * - Solid ColorのPixel Dataも同じAllocatorとMemory Tagで確保する。
 * - 生成されたPixel Dataは本Operationが一時的に所有する。
 * - CPU Texture Resourceのcreate()成功時、
 *   Pixel Dataの所有権はCPU Texture Resourceへ移転する。
 * - GPU Texture Resourceのcreate()はCPU Texture Resourceから
 *   borrowしたPixel Dataを使用する。
 * - GPU Texture Resourceは入力Pixel DataのCPU側Ownershipを
 *   取得しない。
 *
 * @par Texture Construction
 * - BMP ImportではBMP Loaderが検証・正規化した
 *   Pixel DataとTexture Metadataを使用する。
 * - Solid Color Importでは32 x 32 Pixel、RGB 3 Channelの
 *   Textureを生成する。
 * - Solid Colorの全Pixelには指定されたRGB値を設定する。
 * - GPU TextureのFilteringにはNEARESTを使用する。
 * - GPU TextureのS / T方向のWrappingには
 *   CLAMP_TO_EDGEを使用する。
 * - Texture Unit Indexはcallerが指定する。
 * - Texture Unit Indexの有効性とGPU操作の成立条件は
 *   Texture GPU ResourceおよびRenderer Backendへ委譲する。
 *
 * @par External Trust Boundary
 * - BMPファイルの構文、対応形式、Pixel Dataの正規化および
 *   Texture Metadataの妥当性検証はBMP Loaderが担当する。
 * - Loader成功後のPixel DataはGLCE内部で受理された
 *   trusted representationとして扱う。
 * - 本Pipelineは外部BMPデータのsemantic validationを重複しない。
 * - Solid ColorのPixel Dataは入力RGB値から内部生成するため、
 *   外部ファイルの解析を必要としない。
 *
 * @par Registration / Reference Counting
 * - CPU / GPU Texture Resourceは対としてRegistryへ登録する。
 * - Resource NameはRegistryが複製して所有する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 新規登録成功時、Reference Countは1となる。
 * - callerは登録成功時に最初の利用権を取得する。
 * - 追加の利用権取得はTexture Registryのacquire()が担当する。
 * - release()はTexture Registryのrelease()へ処理を委譲する。
 * - 最後の利用権を返却した場合、RegistryがResource Nameと
 *   CPU / GPU Texture Resourceを破棄する。
 *
 * @par Backend Context Lifetime
 * - GPU Texture Resourceの生成時、
 *   Renderer Backend Contextをborrowする。
 * - GPU Texture ResourceはBackend Contextへのポインタを保持し、
 *   後続のBind / Unbind / Destroyで使用する。
 * - Texture RegistryはCPU / GPU Resourceを所有するが、
 *   Backend Context自体は所有しない。
 * - callerはGPU Texture Resourceのlifetime全体にわたり、
 *   対応するBackend Contextの有効性を保証する。
 * - この条件はRegistryのrelease()およびdeinitialize()に
 *   よるResource破棄が完了するまで適用される。
 *
 * @par Failure / Cleanup
 * - Registry登録前の回復可能な失敗では、
 *   本Operationが所有する一時Pixel Dataおよび
 *   CPU / GPU Texture Resourceを解放する。
 * - CPU Texture ResourceへのPixel DataのOwnership Transfer後は、
 *   Pixel DataをPipelineから個別に解放しない。
 * - Registry登録成功後は、CPU / GPU Texture Resourceの
 *   破棄責務をRegistryへ移転する。
 * - DATA_CORRUPTEDの場合は通常のcleanupを行わず、
 *   Resourceの解放やOwnership状態の復元を保証しない。
 * - Import失敗時はTexture IDの出力先を変更しない。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_TEXTURE_TEXTURE_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_TEXTURE_TEXTURE_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct texture_registry texture_registry_t;

/**
 * @brief BMPファイルからCPU / GPU Texture Resourceを生成し、
 *        Texture Registryへ登録する。
 *
 * @par Public API Contract
 * - 指定BMPファイルを読み込み、Pixel DataとTexture Metadataを取得する。
 * - BMPの読み込み、形式検証、Pixel Dataの正規化は
 *   BMP Loaderへ委譲する。
 * - Loaderが返したPixel DataをCPU Texture Resourceへmoveする。
 * - CPU Texture ResourceからPixel Dataをconst borrowとして取得する。
 * - 借用したPixel Dataを使用してGPU Texture Resourceを生成する。
 * - GPU Textureには指定されたTexture Unit Indexを使用する。
 * - MinificationとMagnificationのFilterはNEARESTとする。
 * - S / T方向のTexture WrappingはCLAMP_TO_EDGEとする。
 * - CPU / GPU Texture Resourceを指定Resource Nameで
 *   Texture Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には登録したTexture IDを出力する。
 *
 * @par Ownership
 * - BMP Loaderが生成したPixel DataのOwnershipは、
 *   Loader成功時に本Operationへ移転する。
 * - texture_cpu_resource_create()成功時、
 *   Pixel DataのOwnershipはCPU Texture Resourceへ移転する。
 * - CPU Texture Resourceから取得したPixel Dataはborrowであり、
 *   個別に解放しない。
 * - texture_gpu_resource_create()はCPU側Pixel Dataをborrowし、
 *   GPU Textureを独立して生成する。
 * - Registry登録成功時、CPU / GPU Texture Resourceの
 *   所有権はTexture Registryへ移転する。
 * - 登録成功時、callerは対象Textureの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Backend Context Lifetime
 * - backend_context_はGPU Texture Resource生成時にborrowする。
 * - GPU Texture Resourceはbackend_context_へのポインタを保持する。
 * - callerは登録したGPU Texture Resourceが破棄されるまで、
 *   対応するBackend Contextを有効に保つ必要がある。
 * - 本PipelineとTexture RegistryはBackend Contextを所有しない。
 *
 * @par Failure Contract
 * - BMP Loader失敗時はLoaderのResultをPipelineのResultへ変換する。
 * - Registry登録前の回復可能な失敗では、
 *   本Operationが所有する一時Pixel Dataおよび
 *   CPU / GPU Texture Resourceを解放する。
 * - Pixel DataのOwnership Transfer後は、
 *   CPU Texture Resourceのdestroy()によってPixel Dataを解放する。
 * - Registry登録が回復可能なエラーで失敗した場合、
 *   CPU / GPU Resourceの所有権はPipeline側に残る。
 * - DATA_CORRUPTEDの場合は通常のcleanupを実行せず、
 *   Resourceの解放完了およびOwnership状態の復元を保証しない。
 * - 失敗時にはout_texture_id_の値を変更しない。
 *
 * @param[in] backend_context_
 *            GPU Textureの生成に使用するRenderer Backend Context。
 * @param[in,out] registry_
 *                登録先のTexture Registry。
 * @param[in] texture_unit_index_
 *            GPU Textureで使用するTexture Unit Index。
 * @param[in] resource_name_
 *            登録するResource Name。
 * @param[in] texture_fullpath_
 *            読み込むBMPファイルのFull Path。
 * @param[out] out_texture_id_
 *             登録したTexture IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         Textureの登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         NULL引数、Resource Name、File Path、
 *         または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         Resource Nameが登録済み、
 *         または下位Operationの実行条件が成立しない。
 * @retval RESOURCE_PIPELINE_FILE_OPEN_ERROR
 *         BMPファイルを開けない。
 * @retval RESOURCE_PIPELINE_FILE_READ_ERROR
 *         BMPファイルの読み込みに失敗。
 * @retval RESOURCE_PIPELINE_UNSUPPORTED_FILE
 *         BMPファイルが対応形式またはsemantic requirementsを満たさない。
 * @retval RESOURCE_PIPELINE_NO_MEMORY
 *         Pixel DataまたはResourceのメモリ確保に失敗。
 * @retval RESOURCE_PIPELINE_OVERFLOW
 *         下位処理でサイズ計算等のoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED
 *         Registry容量、GPU制限、または下位Resourceの制限を超過。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR
 *         File I/OまたはGPU操作等で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         内部ResourceまたはRegistryの整合性異常。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR
 *         下位処理で未分類のエラーが発生。
 *
 * @pre backend_context_とregistry_は利用可能であること。
 * @pre resource_name_とtexture_fullpath_は
 *      読み取り可能なNUL終端文字列であること。
 * @pre texture_unit_index_は対象Backendが受理できる値であること。
 * @pre out_texture_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、Resource Nameに対応するCPU / GPU Texture Resourceが
 *       Texture Registryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_texture_id_に登録IDが格納される。
 * @post 成功時、一時Pixel DataはCPU Texture Resourceの
 *       Ownershipに移転済みとなる。
 */
resource_pipeline_result_t texture_pipeline_import_from_bmp(const renderer_backend_context_t* backend_context_, texture_registry_t* registry_, int32_t texture_unit_index_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_);

/**
 * @brief 指定RGB色から単色Textureを生成し、
 *        CPU / GPU Texture Resourceとして登録する。
 *
 * @par Public API Contract
 * - 指定されたRGB値から32 x 32 Pixelの単色Textureを生成する。
 * - Pixel FormatはRGB、Channel Countは3とする。
 * - 全Pixelに指定されたred_、green_、blue_を設定する。
 * - Pixel DataはGeneral Allocatorから確保する。
 * - 生成したPixel DataをCPU Texture Resourceへmoveする。
 * - CPU Texture ResourceからPixel Dataをconst borrowとして取得する。
 * - 借用したPixel Dataを使用してGPU Texture Resourceを生成する。
 * - GPU Textureには指定されたTexture Unit Indexを使用する。
 * - MinificationとMagnificationのFilterはNEARESTとする。
 * - S / T方向のTexture WrappingはCLAMP_TO_EDGEとする。
 * - CPU / GPU Texture Resourceを指定Resource Nameで
 *   Texture Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には登録したTexture IDを出力する。
 *
 * @par Ownership
 * - solid_color_texture_generate()が生成したPixel Dataは
 *   本Operationが一時的に所有する。
 * - texture_cpu_resource_create()成功時、
 *   Pixel DataのOwnershipはCPU Texture Resourceへ移転する。
 * - CPU Texture Resourceから取得したPixel Dataはborrowであり、
 *   個別に解放しない。
 * - GPU Texture ResourceはGPU側Resourceを独立して所有する。
 * - Registry登録成功時、CPU / GPU Texture Resourceの
 *   所有権はTexture Registryへ移転する。
 * - 登録成功時、callerは対象Textureの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Backend Context Lifetime
 * - backend_context_はGPU Texture Resource生成時にborrowする。
 * - GPU Texture Resourceはbackend_context_へのポインタを保持する。
 * - callerは登録したGPU Texture Resourceが破棄されるまで、
 *   対応するBackend Contextを有効に保つ必要がある。
 * - 本PipelineとTexture RegistryはBackend Contextを所有しない。
 *
 * @par Failure Contract
 * - Pixel Data生成失敗時は対応するPipeline Resultを返す。
 * - Registry登録前の回復可能な失敗では、
 *   本Operationが所有する一時Pixel Dataおよび
 *   CPU / GPU Texture Resourceを解放する。
 * - Pixel DataのOwnership Transfer後は、
 *   CPU Texture Resourceのdestroy()によってPixel Dataを解放する。
 * - Registry登録が回復可能なエラーで失敗した場合、
 *   CPU / GPU Resourceの所有権はPipeline側に残る。
 * - DATA_CORRUPTEDの場合は通常のcleanupを実行せず、
 *   Resourceの解放完了およびOwnership状態の復元を保証しない。
 * - 失敗時にはout_texture_id_の値を変更しない。
 *
 * @param[in] backend_context_
 *            GPU Textureの生成に使用するRenderer Backend Context。
 * @param[in,out] registry_
 *                登録先のTexture Registry。
 * @param[in] texture_unit_index_
 *            GPU Textureで使用するTexture Unit Index。
 * @param[in] resource_name_
 *            登録するResource Name。
 * @param[in] red_
 *            各Pixelに設定するRed成分。
 * @param[in] green_
 *            各Pixelに設定するGreen成分。
 * @param[in] blue_
 *            各Pixelに設定するBlue成分。
 * @param[out] out_texture_id_
 *             登録したTexture IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         単色Textureの登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         引数または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         Resource Nameが登録済み、
 *         または下位Operationの実行条件が成立しない。
 * @retval RESOURCE_PIPELINE_NO_MEMORY
 *         Pixel DataまたはResourceのメモリ確保に失敗。
 * @retval RESOURCE_PIPELINE_OVERFLOW
 *         下位処理でサイズ計算等のoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED
 *         Registry容量、GPU制限、または下位Resourceの制限を超過。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR
 *         GPU操作等で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         内部ResourceまたはRegistryの整合性異常。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR
 *         下位処理で未分類のエラーが発生。
 *
 * @pre backend_context_とregistry_は利用可能であること。
 * @pre resource_name_は読み取り可能なNUL終端文字列であること。
 * @pre texture_unit_index_は対象Backendが受理できる値であること。
 * @pre out_texture_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、32 x 32 Pixelの単色RGB Textureが
 *       Texture Registryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_texture_id_に登録IDが格納される。
 * @post 成功時、一時Pixel DataはCPU Texture Resourceの
 *       Ownershipに移転済みとなる。
 */
resource_pipeline_result_t texture_pipeline_import_from_solid_color(const renderer_backend_context_t* backend_context_, texture_registry_t* registry_, int32_t texture_unit_index_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_);

/**
 * @brief Texture Resourceの利用権を1つ返却する。
 *
 * @par Public API Contract
 * - 指定Texture IDの利用権をTexture Registryへ返却する。
 * - Texture IDの範囲と登録状態の検証は
 *   Texture Registryのrelease()へ委譲する。
 * - Reference Countが2以上の場合、利用権を1つ減少させ、
 *   CPU / GPU Texture Resourceは維持する。
 * - Reference Countが1の場合、最後の利用権の返却として
 *   Resource NameとCPU / GPU Texture Resourceを破棄する。
 * - 最後の利用権を返却したEntryはEmpty Stateへ戻る。
 * - Resourceの破棄はTexture Registryが担当する。
 * - 本PipelineはReference CountやRegistryのEntryを直接操作しない。
 *
 * @par Backend Context Lifetime
 * - GPU Texture Resourceは生成時に借用した
 *   Backend Contextへのポインタを保持する。
 * - 最後の利用権を返却する際、
 *   GPU Texture Resourceの破棄に有効なBackend Contextが必要となる。
 * - 本APIはBackend Contextを引数として受け取らず、
 *   GPU Texture Resourceが保持する参照を使用する。
 *
 * @par Failure Contract
 * - Texture Registryのrelease()が失敗した場合、
 *   そのResultをPipelineのResult Codeへ変換して返す。
 * - Commit前の失敗では対象Entryの登録状態を変更しない。
 * - DATA_CORRUPTEDの場合、Resourceの解放完了や
 *   内部状態の復元は保証されない。
 *
 * @param[in,out] registry_
 *                利用権を返却するTexture Registry。
 * @param[in] texture_id_
 *            利用権を返却するTexture ID。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         利用権の返却成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         RegistryがNULL、またはTexture IDが範囲外。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         指定Texture IDが未登録。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         Registryまたは対象Entryの整合性異常。
 *
 * @pre registry_は利用可能なTexture Registryを指していること。
 * @pre callerは対象Textureに対する有効な利用権を保持していること。
 * @pre callerは返却する利用権に基づくborrowの利用を終了していること。
 * @pre 最後の利用権を返却する場合、
 *      GPU Texture Resourceが参照するBackend Contextは有効であること。
 *
 * @post 成功時、対象Textureの利用権が1つ返却される。
 * @post 返却前のReference Countが2以上の場合、
 *       Resourceの登録状態は維持される。
 * @post 返却前のReference Countが1の場合、
 *       対象EntryはEmpty Stateとなり、所有Resourceは破棄される。
 */
resource_pipeline_result_t texture_pipeline_release(texture_registry_t* registry_, uint16_t texture_id_);

#ifdef __cplusplus
}
#endif
#endif
