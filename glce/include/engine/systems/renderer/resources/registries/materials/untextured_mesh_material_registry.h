// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_material_registry.h
 * @author chocolate-pie24
 * @brief Untextured Mesh Materialの登録、検索、利用権管理、およびResource解放を提供する。
 *
 * @details
 * Untextured Mesh MaterialをResource NameおよびMaterial IDによって管理するRegistry。
 * Reference Countを使用してResourceの共有と生存期間を管理する。
 *
 * @section untextured_mesh_material_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは登録済みResource Nameを所有する。
 * - Material本体はRegistryのEntry内に値として保持する。
 * - 新規登録時、callerが提供したMaterialの値をコピーする。
 * - Registryはcaller側のMaterial Instanceへの所有権や参照を保持しない。
 * - 利用権の取得はMaterialのコピーや所有権移転を伴わない。
 *
 * @par Reference Counting
 * - 登録済みResourceごとにReference Countを保持する。
 * - Reference Countは、Resourceに対する利用権の数を表す。
 * - register()はResourceを新規登録し、最初の利用権を付与する。
 *   新規登録時のReference Countは1とする。
 * - acquire()は登録済みResourceの利用権を追加取得し、
 *   Reference Countを1増加させる。
 * - exists()は登録状態だけを確認し、利用権を取得しない。
 * - 利用権を取得したcallerはrelease()によって返却する責務を持つ。
 * - Reference Countが1以上の間、Resourceの登録状態を維持する。
 * - 最後の利用権が返却された場合、Resource Nameを解放し、
 *   対応するEntryをEmpty Stateへ戻す。
 * - Registryの終了処理では、残存Reference Countに関係なく、
 *   登録済みResourceを解放する。
 *
 * @par Storage / Lifetime
 * - Registry本体およびEntry配列のstorageは、
 *   callerから提供されたSubsystem Allocatorによって管理される。
 * - Registryはこれらのbacking storageを個別に解放しない。
 * - callerはRegistryの利用期間中、Allocatorのlifetimeを保証する。
 *
 * @par Resource Identity
 * - Resource NameはRegistry内で一意とする。
 * - 同一Resource Nameの重複登録は許可しない。
 * - Material IDはRegistry内でのみ意味を持つ。
 * - Registry間でMaterial IDのidentityは共有しない。
 * - 同一Resourceに対する複数の利用権は同じMaterial IDを共有する。
 * - 最後の利用権の返却により解放されたMaterial IDは、
 *   後の新規登録で再利用される場合がある。
 * - Material IDだけでは利用権の保持を証明できない。
 *
 * @par Borrowed View
 * - Resource NameおよびMaterialはconst borrowとして提供する。
 * - Borrowの取得では所有権も利用権も移転しない。
 * - callerは借用したResourceを変更・解放してはならない。
 * - 利用権を保持するcallerは、その利用権が有効な間、
 *   対応するResourceへのborrowを利用できる。
 * - callerは利用権の返却後、その利用権に基づくborrowの
 *   利用を終了しなければならない。
 * - 最後の利用権の返却、またはRegistryの終了処理により、
 *   対応するすべてのborrowは無効となる。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_MATERIALS_UNTEXTURED_MESH_MATERIAL_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_MATERIALS_UNTEXTURED_MESH_MATERIAL_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

typedef struct untextured_mesh_material_registry untextured_mesh_material_registry_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct untextured_mesh_material untextured_mesh_material_t;

/**
 * @brief Untextured Mesh Material Registryを生成する。
 *
 * @par Public API Contract
 * - 最大登録数は1以上UINT16_MAX以下とする。
 * - 出力先はNULLで初期化されている必要がある。
 * - 成功時、指定容量のEntry配列を持つRegistryを生成する。
 * - 生成直後のすべてのEntryはEmpty Stateとし、
 *   Reference Countを0に初期化する。
 * - Registry本体とEntry配列は指定されたSubsystem Allocatorから
 *   確保する。
 * - 回復可能な構築失敗では、今回の構築に伴うAllocatorの
 *   allocation stateをrollbackし、出力先を変更しない。
 * - DATA_CORRUPTED発生時は通常のrollbackを行わない。
 *
 * @param[in] max_material_count_ 最大登録可能Material数。
 * @param[in,out] allocator_ Registryのstorageを提供するSubsystem Allocator。
 * @param[out] out_registry_ 生成したRegistryを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 生成成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数または最大登録数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 出力先がNULLでない、
 *         または下位Allocatorのoperationが成立しない。
 * @retval RESOURCE_REGISTRY_OVERFLOW 配列サイズ計算等でoverflowが発生。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 下位Allocatorの確保制限を超過。
 * @retval RESOURCE_REGISTRY_NO_MEMORY メモリ確保に失敗。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部整合性検証またはrollbackに失敗。
 *
 * @pre allocator_は利用可能なSubsystem Allocatorを指していること。
 * @post 成功時、*out_registry_は通常操作に使用できるRegistryを指す。
 */
resource_registry_result_t untextured_mesh_material_registry_create(size_t max_material_count_, subsystem_allocator_t* allocator_, untextured_mesh_material_registry_t** out_registry_);

/**
 * @brief Untextured Mesh Material Registryの終了処理を行う。
 *
 * @par Public API Contract
 * - 全Entryを走査し、登録済みResourceを解放する。
 * - 各ResourceのReference Countに関係なく、
 *   Resource Nameを一度ずつ破棄する。
 * - 終了処理に先立って、すべての利用権をrelease()によって
 *   返却する必要はない。
 * - Material本体は値として保持されているため、
 *   個別のResource解放処理を必要としない。
 * - 終了処理の完了後、Registryの最大登録数を0に設定する。
 * - Registry本体およびEntry配列のbacking storageは解放しない。
 * - NULL引数またはRegistryの整合性異常を検出した場合は、
 *   終了処理を行わない。
 * - 終了処理の失敗はResultとして返さない。
 * - 正常終了後のRegistryに対する通常操作、
 *   および再度のdeinitializeは許可しない。
 *
 * @param[in,out] registry_ 終了処理対象のRegistry。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 *
 * @post 正常終了時、登録済みResourceはすべて解放され、
 *       Registryのlogical lifetimeが終了する。
 * @post 正常終了後、取得済みのMaterial ID、利用権、
 *       およびborrowはすべて無効となる。
 */
void untextured_mesh_material_registry_deinitialize(untextured_mesh_material_registry_t* registry_);

/**
 * @brief Resource Nameに対応するMaterialの登録状態を確認する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定されたResource NameをRegistryから検索する。
 * - 登録済みの場合はout_exists_へtrueを出力する。
 * - 未登録の場合はout_exists_へfalseを出力する。
 * - 未登録は正常な検索結果として扱い、SUCCESSを返す。
 * - 本APIは利用権を取得せず、Reference Countを変更しない。
 * - 検索結果によってMaterialの生存期間は保証されない。
 * - 失敗時はout_exists_の値を変更しない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] resource_name_ 検索するResource Name。
 * @param[out] out_exists_ 登録状態を受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 検索成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはResource Nameが不正。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの内部整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre out_exists_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、out_exists_へ登録状態が出力される。
 * @post Registryの登録状態およびReference Countは変更されない。
 */
resource_registry_result_t untextured_mesh_material_registry_exists(const untextured_mesh_material_registry_t* registry_, const char* resource_name_, bool* out_exists_);

/**
 * @brief Material IDに対応するUntextured Mesh Materialを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistry内部に保持されたMaterialへのconst borrowを返す。
 * - Materialのコピーは生成せず、所有権を移転しない。
 * - 本APIは利用権を新たに付与せず、Reference Countを変更しない。
 * - callerは取得したMaterialを変更してはならない。
 * - 利用権を保持するcallerは、その利用権が有効な間、
 *   取得したborrowを利用できる。
 * - callerは自身の利用権を返却した後、その利用権に基づく
 *   borrowの利用を終了しなければならない。
 * - 最後の利用権の返却、またはRegistryの終了処理によって、
 *   対応するすべてのborrowは無効となる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] material_id_ 取得するMaterialのID。
 *
 * @return 成功時はUntextured Mesh Materialへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const untextured_mesh_material_t* untextured_mesh_material_registry_material_get(const untextured_mesh_material_registry_t* registry_, uint16_t material_id_);

/**
 * @brief 登録済みUntextured Mesh Materialの利用権を取得する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定されたResource Nameが登録済みである必要がある。
 * - 未登録の場合はBAD_OPERATIONを返す。
 * - 登録済みResourceのReference Countを1増加させる。
 * - Reference CountがUINT16_MAXに達している場合は
 *   LIMIT_EXCEEDEDを返し、利用権を取得しない。
 * - 成功時、対応するMaterial IDをout_material_id_へ出力する。
 * - 成功時、callerは対象Resourceの利用権を1つ取得する。
 * - callerは取得した利用権をrelease()によって返却する責務を持つ。
 * - Resourceの新規登録、コピー、所有権移転は行わない。
 *
 * @par Failure Contract
 * - 失敗時、Reference Countおよびout_material_id_の値を変更しない。
 * - DATA_CORRUPTEDの場合、Registryの内部整合性は保証されない。
 *
 * @param[in,out] registry_ 利用権を取得するRegistry。
 * @param[in] resource_name_ 利用権を取得するResource Name。
 * @param[out] out_material_id_ 対応するMaterial IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 利用権の取得成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはResource Nameが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定されたResource Nameが未登録。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED Reference Countが上限に達している。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの内部整合性検証、
 *         またはMaterial IDの内部表現に異常がある。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre out_material_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、対象ResourceのReference Countが1増加する。
 * @post 成功時、out_material_id_には対象ResourceのMaterial IDが格納される。
 * @post Resource自体の所有権および登録状態は変更されない。
 */
resource_registry_result_t untextured_mesh_material_registry_acquire(untextured_mesh_material_registry_t* registry_, const char* resource_name_, uint16_t* out_material_id_);

/**
 * @brief Untextured Mesh Materialを指定名でRegistryへ新規登録する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - Materialはuntextured_mesh_material_is_valid()の
 *   条件を満たす必要がある。
 * - 同じResource Nameの重複登録は許可しない。
 * - 重複登録の場合はBAD_OPERATIONを返す。
 * - 登録可能なEntryが存在しない場合はLIMIT_EXCEEDEDを返す。
 * - 成功時にはResource Nameを複製し、Registryが所有する。
 * - Materialは値としてEntry内にコピーする。
 * - caller側のMaterial Instanceへの所有権や参照は保持しない。
 * - 新規登録成功時、対象EntryのReference Countを1に設定する。
 * - 新規登録成功によってcallerは最初の利用権を取得する。
 * - callerは取得した利用権をrelease()によって返却する責務を持つ。
 * - 登録したMaterial IDをout_material_id_へ出力する。
 * - 解放済みEntryのIDが後の登録で再利用される場合がある。
 *
 * @par Failure Contract
 * - 回復可能な失敗ではRegistryの登録状態を変更せず、
 *   out_material_id_の値も変更しない。
 * - 回復可能な失敗では、生成済みの一時Resource Nameを破棄する。
 * - DATA_CORRUPTEDの場合は、Registryの状態復元を保証しない。
 *
 * @param[in,out] registry_ 登録先のRegistry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in] material_ 登録するUntextured Mesh Material。
 * @param[out] out_material_id_ 登録したMaterial IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはMaterialが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION Resource Nameが登録済み。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 登録可能なEntryが存在しない、
 *         または下位処理の制限を超過。
 * @retval RESOURCE_REGISTRY_NO_MEMORY Resource Nameの生成に必要な
 *         メモリを確保できない。
 * @retval RESOURCE_REGISTRY_OVERFLOW Resource Name生成処理でoverflowが発生。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部状態の整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre material_は読み取り可能なUntextured Mesh Materialを指していること。
 * @pre out_material_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、指定名と出力IDによって登録したMaterialを参照できる。
 * @post 成功時、RegistryはResource NameとMaterialの値を保持する。
 * @post 成功時、対象EntryのReference Countは1となり、
 *       callerは対象Resourceの利用権を1つ保持する。
 */
resource_registry_result_t untextured_mesh_material_registry_register(untextured_mesh_material_registry_t* registry_, const char* resource_name_, const untextured_mesh_material_t* material_, uint16_t* out_material_id_);

/**
 * @brief 指定IDのUntextured Mesh Materialの利用権を返却する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内である必要がある。
 * - 未登録のIDを指定した場合はBAD_OPERATIONを返す。
 * - 成功時には対象EntryのReference Countを1減少させる。
 * - Reference Countが2以上の場合、減算のみを行い、
 *   Resourceの登録状態を維持する。
 * - Reference Countが1の場合、最後の利用権の返却として、
 *   Resource Nameを破棄する。
 * - 最後の利用権の返却に成功した場合、
 *   対象EntryをEmpty Stateへ戻す。
 * - Material本体は値として保持されているため、
 *   個別のResource解放処理を必要としない。
 * - Empty Stateへ戻ったEntryのIDは、後の登録で再利用される場合がある。
 * - 最後の利用権の返却によって、対応するMaterial IDおよび
 *   取得済みのすべてのborrowは無効となる。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 *
 * @par Failure Contract
 * - Commit前の引数不正または登録状態の不一致では、
 *   Registryの登録状態およびReference Countを変更しない。
 * - DATA_CORRUPTEDの場合は、Registryの内部整合性を保証しない。
 *
 * @param[in,out] registry_ 利用権を返却するRegistry。
 * @param[in] material_id_ 利用権を返却するMaterial ID。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 利用権の返却成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはIDの範囲が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定IDが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの内部整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre callerは対象Materialに対する有効な利用権を保持していること。
 * @pre callerは返却する利用権に基づくborrowの利用を終了していること。
 *
 * @post 成功時、対象Resourceの利用権が1つ返却される。
 * @post 返却前のReference Countが2以上の場合、
 *       登録状態を維持したままReference Countが1減少する。
 * @post 返却前のReference Countが1の場合、
 *       対象EntryはEmpty Stateとなり、Resource Nameは解放済みとなる。
 */
resource_registry_result_t untextured_mesh_material_registry_release(untextured_mesh_material_registry_t* registry_, uint16_t material_id_);

/**
 * @brief Untextured Mesh Material Registryのcanonical validityを判定する。
 *
 * @par Validation Scope
 * 以下の条件を検証する。
 * - RegistryのポインタがNULLではない。
 * - 最大登録数が1以上UINT16_MAX以下である。
 * - Entry配列のポインタがNULLではない。
 * - すべてのEntryがEmpty StateまたはRegistered Stateの
 *   いずれかとして整合している。
 *
 * Empty State:
 * - Resource NameのポインタがNULLである。
 * - Reference Countが0である。
 * - Materialの格納値は検証対象としない。
 *
 * Registered State:
 * - Resource NameのポインタがNULLではない。
 * - Reference Countが1以上である。
 * - Resource NameがGeneral Allocator管理下の
 *   current live allocationである。
 * - Resource NameがvalidなChoco Stringであり、空文字列ではない。
 * - Materialがcanonical validityを満たす。
 *
 * Reference CountとResourceの登録状態に矛盾があるEntry、
 * またはEmpty / Registeredのどちらにも該当しないEntryは
 * invalidとする。
 *
 * Resource NameのEntry間一意性、Registry本体およびEntry配列の
 * backing storageのallocation-level validity、
 * およびAllocatorの内部状態は検証対象に含まない。
 *
 * 本APIはRegistryの状態を変更しない。
 *
 * @param[in] registry_ 検証対象のRegistry。
 *
 * @retval true 全てのcanonical invariantを満たす。
 * @retval false いずれかのcanonical invariantを満たさない。
 *
 * @pre NULL以外のregistry_を指定する場合、Registry、Entry配列、
 *      およびEntryが参照するResourceのstorageは、
 *      検証処理に必要な範囲で読み取り可能であること。
 */
bool untextured_mesh_material_registry_is_valid(const untextured_mesh_material_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
