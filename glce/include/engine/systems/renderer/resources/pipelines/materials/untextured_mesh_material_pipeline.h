// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_material_pipeline.h
 * @author chocolate-pie24
 * @brief Untextured Mesh MaterialのConfiguration Fileからの生成、
 *        Registry登録および利用権返却を提供する。
 *
 * @details
 * Untextured Mesh Material Configuration Fileを読み込み、
 * Material Valueを構築してGeometryとは独立した
 * Material Registryへ登録するPipeline APIを提供する。
 *
 * 本PipelineはResourceの生成・登録手順を調整するものであり、
 * 登録済みMaterialの状態やReference Countを直接管理しない。
 *
 * @section untextured_mesh_material_pipeline_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - Configuration Fileの読み込みと解析は
 *   Untextured Mesh Material Config Loaderへ委譲する。
 * - Materialのsemantic validityは
 *   Material Domainのcanonical validatorによって確認する。
 * - Resource Nameの複製、Materialの登録状態、
 *   およびReference Countの管理はMaterial Registryへ委譲する。
 * - 本PipelineはMaterial Registryをborrowし、所有しない。
 * - ConfigurationとMaterialはValue Typeとして扱い、
 *   本Pipeline内のlocal storageに保持する。
 * - Registryは登録時にMaterialの値をコピーする。
 * - Registryは本Pipeline内のMaterial Instanceへの
 *   ポインタやOwnershipを保持しない。
 *
 * @par Configuration / Trust Boundary
 * - 外部Configuration Fileのsyntax、supported key、
 *   required fieldおよびnumeric representationの検証は
 *   Untextured Mesh Material Config Loaderが担当する。
 * - Loaderは省略可能なFieldのDefault Valueを補完する。
 * - Loader成功時、ambient、diffuse、specular、
 *   shininessのすべてのFieldが確定する。
 * - Loader成功後のConfigurationは、
 *   GLCE内部で受理されたtrusted representationとして扱う。
 * - 本PipelineはConfiguration Fileの解析や
 *   file-level validationを重複して実行しない。
 *
 * @par Material Construction / Validation
 * - Loaderが返したConfigurationから
 *   untextured_mesh_material_tを構築する。
 * - ambient、diffuse、specular、shininessを
 *   対応するFieldへ値としてコピーする。
 * - Registry登録前にuntextured_mesh_material_is_valid()で
 *   Materialのcanonical validityを確認する。
 * - 反射係数はfiniteかつ0.0f以上1.0f以下、
 *   shininessはfiniteかつ0.0f以上とする。
 * - 構築したMaterialがinvalidな場合は、
 *   RESOURCE_PIPELINE_UNSUPPORTED_FILEを返す。
 * - Materialの検証はすべてのBuild Modeで実行する。
 *
 * @par Registration / Reference Counting
 * - Materialは指定されたResource NameでRegistryへ新規登録する。
 * - Resource NameはRegistryが複製して所有する。
 * - Material本体はRegistryのEntry内へ値としてコピーする。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 新規登録成功時、Reference Countは1となる。
 * - callerは登録成功時に最初の利用権を取得する。
 * - 追加の利用権取得はMaterial Registryのacquire()が担当する。
 * - release()はMaterial Registryのrelease()へ処理を委譲する。
 * - 最後の利用権を返却した場合、RegistryはResource Nameを解放し、
 *   対応するEntryをEmpty Stateへ戻す。
 * - MaterialはValue Typeであるため、
 *   個別のResource破棄処理を必要としない。
 *
 * @par Resource Identity / Lifetime
 * - Material IDは対象Registry内でのみ意味を持つ。
 * - 同一Materialに対する複数の利用権は同じIDを共有する。
 * - 最後の利用権の返却によって解放されたIDは、
 *   後の新規登録で再利用される場合がある。
 * - Material Registryのlifetimeはcallerが保証する。
 * - 本PipelineはRegistry内部のMaterialへの
 *   Borrowed Pointerを保持しない。
 *
 * @par Failure Contract
 * - Loaderで失敗した場合、MaterialをRegistryへ登録しない。
 * - Materialのcanonical validationに失敗した場合も、
 *   Registryへの登録を行わない。
 * - Registryへの登録が回復可能なエラーで失敗した場合、
 *   Registryの登録状態とReference Countは変更されない。
 * - ConfigurationとMaterialはlocal valueであり、
 *   Pipelineによる個別のResource解放処理は不要とする。
 * - DATA_CORRUPTED発生時は、下位Moduleの内部状態の
 *   整合性や復元を保証しない。
 * - Import失敗時はMaterial IDの出力先を変更しない。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_MATERIALS_UNTEXTURED_MESH_MATERIAL_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_MATERIALS_UNTEXTURED_MESH_MATERIAL_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct untextured_mesh_material_registry untextured_mesh_material_registry_t;

/**
 * @brief Configuration FileからUntextured Mesh Materialを生成し、
 *        Material Registryへ登録する。
 *
 * @par Public API Contract
 * - 指定されたConfiguration Fileを読み込み、
 *   Untextured Mesh Materialの設定値を取得する。
 * - Fileの読み込み、解析、semantic validationは
 *   untextured_mesh_material_config_loader_load()へ委譲する。
 * - Loaderが返したConfigurationからMaterialを構築する。
 * - ambient、diffuse、specular、shininessを
 *   ConfigurationからMaterialへ値としてコピーする。
 * - 構築したMaterialのcanonical validityを
 *   untextured_mesh_material_is_valid()で確認する。
 * - Materialがinvalidな場合はUNSUPPORTED_FILEとして扱う。
 * - 検証済みMaterialを指定Resource NameでRegistryへ新規登録する。
 * - Resource Nameの妥当性と重複登録はRegistryが判定する。
 * - 成功時には登録したMaterial IDを出力する。
 *
 * @par Configuration Validation
 * - Configuration Fileのsyntax、supported key、
 *   required fieldおよびnumeric rangeはLoaderが検証する。
 * - Optional FieldのDefault ValueはLoaderが補完する。
 * - PipelineではFileの解析処理を繰り返さない。
 * - Material構築後のcanonical validationは、
 *   DEBUG / TEST / RELEASEのすべてで実行する。
 * - Materialの検証条件はMaterial DomainのContractに従う。
 *
 * @par Ownership
 * - 入力File PathとResource Nameのownershipはcallerが保持する。
 * - Loaderから取得するConfigurationはValue Typeであり、
 *   本Operationのlocal storageへ出力される。
 * - 構築したMaterialもValue Typeとしてlocal storageに保持する。
 * - Registryへの登録時にはMaterialの値をコピーする。
 * - Material InstanceのOwnership Transferは発生しない。
 * - RegistryはResource Nameを複製して所有する。
 * - 登録成功時、callerは対象Materialの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Failure Contract
 * - Loaderの失敗時はResultをPipelineのResult Codeへ変換して返す。
 * - Loader失敗時にはRegistryへの登録を行わない。
 * - Materialのcanonical validationに失敗した場合は
 *   RESOURCE_PIPELINE_UNSUPPORTED_FILEを返す。
 * - Registry登録失敗時はResult Codeを変換して返す。
 * - Registry登録の回復可能な失敗では、
 *   Registryの登録状態およびReference Countを変更しない。
 * - ConfigurationとMaterialはlocal valueであり、
 *   Pipelineによる個別のcleanupを必要としない。
 * - DATA_CORRUPTEDの場合、下位Moduleの内部状態の
 *   整合性や復元を保証しない。
 * - 失敗時にはout_material_id_の値を変更しない。
 *
 * @param[in,out] material_registry_
 *                登録先のUntextured Mesh Material Registry。
 * @param[in] resource_name_
 *            登録するResource Name。
 * @param[in] resource_fullpath_
 *            読み込むMaterial Configuration FileのFull Path。
 * @param[out] out_material_id_
 *             登録したMaterial IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         Materialの登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         NULL引数、Resource Name、File Path、
 *         または下位APIの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         Resource Nameが登録済み、
 *         または下位Operationの実行条件が成立しない。
 * @retval RESOURCE_PIPELINE_FILE_OPEN_ERROR
 *         Configuration Fileを開けない。
 * @retval RESOURCE_PIPELINE_FILE_READ_ERROR
 *         Configuration Fileの読み込みに失敗。
 * @retval RESOURCE_PIPELINE_UNSUPPORTED_FILE
 *         Configuration Fileの構文・semantic条件が不正、
 *         または構築したMaterialがcanonical validityを満たさない。
 * @retval RESOURCE_PIPELINE_NO_MEMORY
 *         LoaderまたはRegistryの一時Resource確保に失敗。
 * @retval RESOURCE_PIPELINE_OVERFLOW
 *         下位処理でサイズ計算等のoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED
 *         Registryの登録可能数または下位Resourceの制限を超過。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR
 *         File I/O等の下位処理で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         下位Moduleの内部整合性検証に失敗。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR
 *         下位処理で未分類のエラーが発生。
 *
 * @pre material_registry_は利用可能なRegistryを指していること。
 * @pre resource_name_は空文字列ではない
 *      読み取り可能なNUL終端文字列であること。
 * @pre resource_fullpath_は空文字列ではない
 *      読み取り可能なNUL終端のFile Pathであること。
 * @pre out_material_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、指定Resource NameでMaterialがRegistryへ登録される。
 * @post 成功時、RegistryはMaterialの独立した値コピーを保持する。
 * @post 成功時、Reference Countは1となり、
 *       out_material_id_に登録IDが格納される。
 * @post 入力文字列のownershipは移転しない。
 */
resource_pipeline_result_t untextured_mesh_material_pipeline_import_from_file(untextured_mesh_material_registry_t* material_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

/**
 * @brief Untextured Mesh Materialの利用権を1つ返却する。
 *
 * @par Public API Contract
 * - 指定Material IDの利用権をMaterial Registryへ返却する。
 * - Material IDの範囲と登録状態の検証は
 *   Material Registryのrelease()へ委譲する。
 * - Reference Countが2以上の場合、利用権を1つ減少させ、
 *   Materialの登録状態を維持する。
 * - Reference Countが1の場合、最後の利用権の返却として
 *   Resource Nameを破棄し、EntryをEmpty Stateへ戻す。
 * - Material本体はValue TypeとしてEntry内に保持されるため、
 *   個別のResource解放処理を必要としない。
 * - 最後の利用権の返却によってMaterial IDと
 *   対応するBorrowed Viewは無効となる。
 * - 本PipelineはReference CountやEntryを直接操作しない。
 * - Registry本体およびEntry配列のbacking storageは解放しない。
 *
 * @par Failure Contract
 * - Material Registryのrelease()が失敗した場合、
 *   そのResultをPipelineのResult Codeへ変換して返す。
 * - Commit前の引数不正または登録状態の不一致では、
 *   Registryの登録状態とReference Countを変更しない。
 * - DATA_CORRUPTEDの場合、Registryの内部整合性は保証されない。
 *
 * @param[in,out] material_registry_
 *                利用権を返却するMaterial Registry。
 * @param[in] material_id_
 *            利用権を返却するMaterial ID。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         利用権の返却成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         RegistryがNULL、またはMaterial IDが範囲外。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         指定Material IDが未登録。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         Registryの内部整合性検証に失敗。
 *
 * @pre material_registry_は利用可能なRegistryを指していること。
 * @pre callerは対象Materialの有効な利用権を保持していること。
 * @pre callerは返却する利用権に基づくborrowの利用を終了していること。
 *
 * @post 成功時、対象Materialの利用権が1つ返却される。
 * @post 返却前のReference Countが2以上の場合、
 *       Resourceの登録状態を維持したままReference Countが1減少する。
 * @post 返却前のReference Countが1の場合、
 *       対象EntryはEmpty Stateとなり、Resource Nameは解放済みとなる。
 */
resource_pipeline_result_t untextured_mesh_material_pipeline_release(untextured_mesh_material_registry_t* material_registry_, uint16_t material_id_);

#ifdef __cplusplus
}
#endif
#endif
