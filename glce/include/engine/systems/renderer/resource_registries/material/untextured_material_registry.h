// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_material_registry.h
 * @author chocolate-pie24
 * @brief Untextured Materialの登録、検索、および登録解除を提供する。
 *
 * @details
 * Untextured MaterialをResource NameとMaterial IDによって管理するRegistry。
 * Materialの描画処理、Shaderの設定、Resourceの読み込みは担当しない。
 *
 * @section untextured_material_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは登録名の文字列Resourceを所有する。
 * - Materialは値として保持し、caller側のMaterial Instanceへの
 *   所有権や参照を保持しない。
 *
 * @par Storage / Lifetime
 * - Registry本体およびEntry配列のstorageは、
 *   callerから提供されたSubsystem Allocatorによって管理される。
 * - Registryはこれらのbacking storageを個別に解放しない。
 * - callerはRegistryの利用期間中、Allocatorのlifetimeを保証する。
 *
 * @par Resource Identity
 * - Material IDはRegistry内でのみ意味を持つ。
 * - Registry間でMaterial IDのidentityは共有しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_MATERIAL_UNTEXTURED_MATERIAL_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_MATERIAL_UNTEXTURED_MATERIAL_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct untextured_material_registry untextured_material_registry_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct untextured_material untextured_material_t;

/**
 * @brief Untextured Material Registryを生成する。
 *
 * @par Public API Contract
 * - 最大登録数は1以上UINT16_MAX以下とする。
 * - 出力先はNULLで初期化されている必要がある。
 * - 成功時には、指定容量の未登録Entryを持つRegistryを生成する。
 * - 回復可能な構築失敗では、今回の構築に伴うAllocatorの
 *   allocation stateを復元し、出力先はNULLのまま維持する。
 * - DATA_CORRUPTED発生時は通常のrollbackを行わない。
 *
 * @param[in] max_untextured_material_count_ 最大登録可能Material数。
 * @param[in,out] allocator_ Registryのstorageを提供するSubsystem Allocator。
 * @param[out] out_registry_ 生成したRegistryを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 生成成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 出力先がNULLでない。
 * @retval RESOURCE_REGISTRY_OVERFLOW 配列サイズ計算でoverflowが発生。
 * @retval RESOURCE_REGISTRY_NO_MEMORY メモリ確保に失敗。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部状態の整合性検証またはrollbackに失敗。
 *
 * @pre allocator_は利用可能なSubsystem Allocatorを指していること。
 * @post 成功時、*out_registry_は通常操作に使用できるRegistryを指す。
 */
resource_registry_result_t untextured_material_registry_create(size_t max_untextured_material_count_, subsystem_allocator_t* allocator_, untextured_material_registry_t** out_registry_);

/**
 * @brief Registryの利用可能状態を終了する。
 *
 * @par Public API Contract
 * - 登録済みEntryが所有するResource Nameを破棄する。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 * - 正常終了後、Registryは利用可能状態ではなくなり、
 *   再初期化せずに通常操作へ使用することはできない。
 * - 同じRegistryに対する再度のdeinitializeは許可しない。
 *
 * @param[in,out] registry_ 終了対象のRegistry。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @post 正常終了後、registry_のlogical lifetimeは終了する。
 */
void untextured_material_registry_deinitialize(untextured_material_registry_t* registry_);

/**
 * @brief 指定名のMaterialが登録されているか判定する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - Registryの登録状態は変更しない。
 * - 引数不正や内部状態の異常を検出した場合もfalseを返すため、
 *   falseだけから未登録であると断定することはできない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 *
 * @retval true 指定名のMaterialが登録されている。
 * @retval false 未登録、または検索を実行できなかった。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 */
bool untextured_material_registry_exists(const untextured_material_registry_t* registry_, const char* name_);

/**
 * @brief Material IDに対応するResource Nameを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistryが所有するResource Nameへのconst borrowを返す。
 * - 戻り値はcallerが解放・変更してはならない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によって
 *   戻り値のborrowは無効になる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] untextured_material_id_ Resource Nameを取得するMaterial ID。
 *
 * @return 成功時はResource Nameへのconst charポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const char* untextured_material_registry_name_get(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_);

/**
 * @brief Material IDに対応するUntextured Materialを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistry内部に保持されたMaterialへのconst borrowを返す。
 * - Materialのコピーは生成せず、callerへ所有権を移転しない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によって
 *   戻り値のborrowは無効になる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] untextured_material_id_ 取得するMaterialのID。
 *
 * @return 成功時はUntextured Materialへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const untextured_material_t* untextured_material_registry_material_get(const untextured_material_registry_t* registry_, uint16_t untextured_material_id_);

/**
 * @brief Resource Nameに対応するMaterial IDを取得する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定名のMaterialが登録されている場合、そのIDを出力する。
 * - 指定名が未登録の場合はBAD_OPERATIONを返す。
 * - 失敗時、出力先の値は変更しない。
 * - Registryの登録状態は変更しない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 * @param[out] out_untextured_material_id_ 取得したMaterial IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 取得成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定名のMaterialが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 *
 * @post 成功時、出力したIDは指定名の登録済みMaterialを識別する。
 */
resource_registry_result_t untextured_material_registry_id_get(const untextured_material_registry_t* registry_, const char* name_, uint16_t* out_untextured_material_id_);

/**
 * @brief Untextured Materialを指定名でRegistryへ登録する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - Materialは`untextured_material_is_valid()`の条件を満たす必要がある。
 * - 同じResource Nameの重複登録は許可しない。
 * - 登録可能なEntryが存在しない場合はLIMIT_EXCEEDEDを返す。
 * - 成功時にはMaterialを値として登録し、割り当てたIDを出力する。
 * - 登録解除済みEntryのIDが再利用される場合がある。
 *
 * @par Failure Contract
 * - 回復可能な失敗ではRegistryの登録状態を変更せず、
 *   出力先の値も変更しない。
 * - DATA_CORRUPTEDの場合は、Registryの状態復元を保証しない。
 *
 * @param[in,out] registry_ 登録先のRegistry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in] material_ 登録するUntextured Material。
 * @param[out] out_untextured_material_id_ 登録したMaterial IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはMaterialが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION Resource Nameが登録済み。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 登録可能なEntryが存在しない。
 * @retval RESOURCE_REGISTRY_NO_MEMORY Resource Nameの生成に必要なメモリを確保できない。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部状態の整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre material_は読み取り可能なUntextured Materialを指していること。
 *
 * @post 成功時、指定名と出力IDによって登録したMaterialを参照できる。
 */
resource_registry_result_t untextured_material_registry_register(untextured_material_registry_t* registry_, const char* resource_name_, const untextured_material_t* material_, uint16_t* out_untextured_material_id_);

/**
 * @brief 指定IDのUntextured Materialを登録解除する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内である必要がある。
 * - 未登録のIDを指定した場合はBAD_OPERATIONを返す。
 * - 成功時には対象EntryのResource Nameを破棄し、登録状態を解除する。
 * - 登録解除後、そのIDは再登録時に再利用される場合がある。
 * - 登録解除したMaterialのIDおよび取得済みborrowは無効になる。
 * - Registryのbacking storageは解放しない。
 *
 * @param[in,out] registry_ 登録解除対象のRegistry。
 * @param[in] untextured_material_id_ 登録解除するMaterial ID。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録解除成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはIDの範囲が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定IDが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 *
 * @post 成功時、指定IDに対応するEntryは未登録状態になる。
 */
resource_registry_result_t untextured_material_registry_unregister(untextured_material_registry_t* registry_, uint16_t untextured_material_id_);

/**
 * @brief Untextured Material Registryのcanonical validityを判定する。
 *
 * @par Validation Scope
 * 以下の条件を検証する。
 * - RegistryのポインタがNULLではない。
 * - 最大登録数が1以上UINT16_MAX以下である。
 * - Entry配列のポインタがNULLではない。
 * - 各登録済みEntryのResource NameがvalidなChoco Stringであり、
 *   空文字列ではない。
 * - 各登録済みEntryのMaterialがcanonical validityを満たす。
 *
 * 未登録EntryのMaterial値は検査しない。
 * また、Resource Nameの一意性、backing storageのallocation-level
 * validity、およびAllocatorの内部状態は検証対象に含まない。
 *
 * @param[in] registry_ 検証対象のRegistry。
 *
 * @retval true 全てのcanonical invariantを満たす。
 * @retval false いずれかのcanonical invariantを満たさない。
 *
 * @pre NULL以外のregistry_を指定する場合、検証対象のstorageは
 *      検証処理に必要な範囲で読み取り可能であること。
 */
bool untextured_material_registry_is_valid(const untextured_material_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
