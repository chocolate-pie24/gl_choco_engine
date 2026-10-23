// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/pipelines/materials/untextured_mesh_material_pipeline.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"
#include "engine/systems/renderer/resources/loaders/untextured_mesh_material_config_loader.h"
#include "engine/systems/renderer/resources/materials/material_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/materials/untextured_mesh_material_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

/*
 * Module Internal Contract
 *
 * Module State:
 * - 本Moduleは永続的な内部状態を保持しない。
 * - Material Registryはcallerからborrowする。
 * - Configurationの読み込み、Materialの構築、
 *   Registry登録および利用権返却を調整する。
 *
 * Configuration / Trust Boundary:
 * - 外部Configuration Fileの読み込みと解析は
 *   Untextured Mesh Material Config Loaderへ委譲する。
 * - File Syntax、Required Field、Optional Field、
 *   Numeric RepresentationおよびConfigurationの
 *   Semantic ValidityはLoaderが検証する。
 * - Loader成功時、検証済みConfigurationをlocal valueとして取得する。
 * - 本Moduleは外部ファイルの解析と
 *   ConfigurationのCanonical Validationを重複しない。
 *
 * Material Construction:
 * - untextured_mesh_material_config_tから
 *   untextured_mesh_material_tを構築する。
 * - ambient、diffuse、specular、shininessを
 *   対応するFieldへ値としてコピーする。
 * - 構築したMaterialはlocal valueとして保持する。
 * - Registry登録前にuntextured_mesh_material_is_valid()で
 *   MaterialのCanonical Validityを検証する。
 * - この検証は全Build Modeで実行する。
 * - 検証失敗は外部ファイル由来の無効なMaterialとして
 *   RESOURCE_PIPELINE_UNSUPPORTED_FILEに分類する。
 *
 * Registration / Commit:
 * - 検証済みMaterialとResource NameをRegistryへ渡す。
 * - Resource Nameの検証、重複登録の判定、
 *   登録可能なEntryの選択はRegistryへ委譲する。
 * - RegistryはMaterialの値をEntryへコピーする。
 * - RegistryはResource Nameを複製して所有する。
 * - 新規登録時のReference Countは1となる。
 * - Registry登録成功が本OperationのCommit境界となる。
 * - Material IDはRegistry登録成功後にcallerへ出力する。
 * - 本ModuleはRegistryのEntryやReference Countを直接変更しない。
 *
 * Ownership / Lifetime:
 * - ConfigurationとMaterialはlocal valueであり、
 *   個別のAllocationやDestroyを必要としない。
 * - Materialの登録にOwnership Transferは伴わない。
 * - Registryはlocal MaterialへのPointerを保持せず、
 *   独立した値コピーを保持する。
 * - Resource NameのStorageとReference CountはRegistryが管理する。
 * - Registry自体のOwnershipは取得しない。
 *
 * Failure:
 * - Loader失敗時はResult Codeを変換して返す。
 * - MaterialのCanonical Validation失敗時は
 *   Registryへの登録を実行しない。
 * - Registry登録失敗時はResult Codeを変換して返す。
 * - Commit前の失敗ではRegistryの登録状態を変更しない。
 * - Registry登録時のDATA_CORRUPTEDでは、
 *   Registryの状態復元を保証しない。
 * - 本Moduleが独自に解放するTemporary Resourceは存在せず、
 *   CleanupではResult Codeを返す。
 * - Import失敗時はMaterial IDの出力先を変更しない。
 *
 * Validation Responsibility:
 * - 公開APIのPointer NULLは本Moduleで直接検査する。
 * - Configuration FileのValidationはLoaderへ委譲する。
 * - 構築したMaterial ValueのCanonical Validationは
 *   Material DomainのValidatorへ委譲する。
 * - Resource Name、Registryの内部整合性、
 *   Entry StateおよびReference CountのValidationは
 *   Material Registryへ委譲する。
 * - RegistryのCanonical Validationは本Moduleでは実行しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - material_registry_、resource_name_、resource_fullpath_、
 *   out_material_id_のNULLを全Build Modeで検査する。
 * - File Pathの空文字列条件と外部Configurationの検証は
 *   Untextured Mesh Material Config Loaderへ委譲する。
 * - Resource Nameの空文字列条件と重複登録の判定は
 *   Material Registryのregister()へ委譲する。
 * - Registryの内部整合性検証はRegistry側のContractへ委譲する。
 *
 * Configuration Loading:
 * - untextured_mesh_material_config_loader_load()を呼び出し、
 *   Configurationをlocal valueとして取得する。
 * - Loader成功後はConfigurationのSemantic Validityを信頼する。
 * - 本APIではFile SyntaxやFieldの解析を繰り返さない。
 * - Loader失敗時はResult Codeを変換して返す。
 *
 * Material Construction:
 * - Configurationのambient、diffuse、specular、
 *   shininessをMaterialの対応Fieldへコピーする。
 * - Materialはlocal valueとして構築する。
 *
 * Result Validation:
 * - untextured_mesh_material_is_valid()によって
 *   構築済みMaterialのCanonical Validityを確認する。
 * - このValidationは全Build Modeで実行する。
 * - Validation失敗時はRESOURCE_PIPELINE_UNSUPPORTED_FILEを返す。
 * - Materialが無効な場合、Registry登録は実行しない。
 *
 * Commit:
 * - 検証済みMaterialを
 *   untextured_mesh_material_registry_register()へ渡す。
 * - RegistryはMaterialをEntryへ値としてコピーする。
 * - Resource Nameの複製、Entry Stateの変更、
 *   Reference Countの初期化はRegistryが担当する。
 * - Registry登録成功後、取得したMaterial IDを
 *   out_material_id_へ出力する。
 *
 * Failure:
 * - Loader、Material Validation、Registry登録の各失敗では
 *   対応するResult Codeを返す。
 * - Registry登録前の失敗ではRegistryを変更しない。
 * - Registry登録の回復可能な失敗では、
 *   Registry側のFailure Contractに従い状態を維持する。
 * - DATA_CORRUPTEDではRegistryの状態復元を保証しない。
 * - 本Operationには独自のResource CleanupやRollbackがない。
 * - 失敗時はout_material_id_を変更しない。
 *
 * Postconditions:
 * - 登録成功時、RegistryはResource NameとMaterialの
 *   独立した値コピーを保持する。
 * - Reference Countは1となる。
 * - PipelineではRegistryのPostconditions Validationを
 *   重複して実行しない。
 */
resource_pipeline_result_t untextured_mesh_material_pipeline_import_from_file(untextured_mesh_material_registry_t* material_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    loader_result_t ret_loader = LOADER_INVALID_ARGUMENT;

    untextured_mesh_material_config_t material_config = { 0 };
    untextured_mesh_material_t material = { 0 };

    uint16_t tmp_material_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(material_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_material_pipeline_import_from_file", "material_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_material_pipeline_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_material_pipeline_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_material_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_material_pipeline_import_from_file", "out_material_id_")

    ret_loader = untextured_mesh_material_config_loader_load(resource_fullpath_, &material_config);
    if(LOADER_SUCCESS != ret_loader) {
        ret = resource_pipeline_result_convert_loader(ret_loader);
        ERROR_MESSAGE("untextured_mesh_material_pipeline_import_from_file(%s) - untextured_mesh_material_config_loader_load failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    material.ambient = material_config.ambient;
    material.diffuse = material_config.diffuse;
    material.shininess = material_config.shininess;
    material.specular = material_config.specular;
    if(!untextured_mesh_material_is_valid(&material)) {
        ret = RESOURCE_PIPELINE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("untextured_mesh_material_pipeline_import_from_file(%s) - Invalid material.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = untextured_mesh_material_registry_register(material_registry_, resource_name_, &material, &tmp_material_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_material_pipeline_import_from_file(%s) - untextured_mesh_material_registry_register failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    *out_material_id_ = tmp_material_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - material_registry_のNULLを全Build Modeで検査する。
 * - Material IDの範囲、EntryのRegistered State、
 *   Reference CountおよびRegistryの内部整合性検証は
 *   untextured_mesh_material_registry_release()へ委譲する。
 * - 本ModuleではRegistryのCanonical Validationを重複しない。
 *
 * Commit:
 * - Material Registryのrelease()を呼び出し、
 *   対象Materialの利用権を1つ返却する。
 * - Reference Countが2以上の場合、
 *   RegistryはReference Countを1減少させてEntryを維持する。
 * - Reference Countが1の場合、
 *   RegistryはResource Nameを破棄してEntryをEmpty Stateへ戻す。
 * - MaterialはValue Typeであり、個別のDestroyを必要としない。
 * - 本ModuleはEntryやReference Countを直接操作しない。
 *
 * Result:
 * - Registryのrelease()が返したResultを
 *   Resource PipelineのResult Codeへ変換する。
 * - Registryのrelease()成功時は
 *   RESOURCE_PIPELINE_SUCCESSを返す。
 *
 * Failure:
 * - RegistryのPrecondition Validationに失敗した場合、
 *   利用権返却処理は実行されない。
 * - DATA_CORRUPTEDの場合、
 *   Registryの内部整合性は保証されない。
 * - 本Module自身はResourceを所有していないため、
 *   独立したRollback処理を持たない。
 *
 * Postconditions:
 * - 成功時、Registry側でReference Countの減少、
 *   またはEntryのEmpty Stateへの遷移が成立する。
 * - 本ModuleではCommit後のCanonical Validationを
 *   重複して実行しない。
 */
resource_pipeline_result_t untextured_mesh_material_pipeline_release(untextured_mesh_material_registry_t* material_registry_, uint16_t material_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(material_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_material_pipeline_release", "material_registry_")

    ret_resource_registry = untextured_mesh_material_registry_release(material_registry_, material_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_material_pipeline_release(%s) - untextured_mesh_material_registry_release failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
