// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file ui_mesh_geometry_registry.h
 * @author chocolate-pie24
 * @brief UI Mesh Geometryの登録、検索、利用権管理、
 *        およびResource解放を提供する。
 *
 * @details
 * UI Mesh GeometryのCPU Resourceと対応するVBO Allocationを、
 * Resource NameおよびGeometry IDによって管理するRegistry。
 * Reference Countを使用してResourceの共有と生存期間を管理する。
 *
 * Geometryの生成、GPUへの頂点データの転送は担当しない。
 *
 * @section ui_mesh_geometry_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは登録済みCPU GeometryとResource Nameを所有する。
 * - Registryは登録済みGeometryに対応するVBO Allocationの
 *   解放責務を持つ。
 * - VBO Manager本体はRegistryが所有しない。
 * - 新規登録成功時、CPU GeometryとVBO Allocationの解放責務は
 *   callerからRegistryへ移転する。
 * - 利用権の取得はResource自体の所有権移転を伴わない。
 *
 * @par Reference Counting
 * - 登録済みResourceごとにReference Countを保持する。
 * - Reference CountはResourceに対する利用権の数を表す。
 * - register()はResourceを新規登録し、最初の利用権を付与する。
 *   新規登録時のReference Countは1とする。
 * - acquire()は登録済みResourceの利用権を追加取得し、
 *   Reference Countを1増加させる。
 * - exists()は登録状態だけを確認し、利用権を取得しない。
 * - 利用権を取得したcallerはrelease()によって返却する責務を持つ。
 * - Reference Countが1以上の間、Resourceの登録状態を維持する。
 * - 最後の利用権が返却された場合、Resourceを解放し、
 *   対応するEntryをEmpty Stateへ戻す。
 * - Registryの終了処理では、残存Reference Countに関係なく、
 *   登録済みResourceを解放する。
 *
 * @par Storage / Lifetime
 * - Registry本体およびEntry配列のstorageは、
 *   callerから提供されたSubsystem Allocatorによって管理される。
 * - Registryはこれらのbacking storageを個別に解放しない。
 * - callerはRegistryの利用期間中、Allocatorのlifetimeを保証する。
 * - VBO Allocationの解放には、対象Allocationを管理する
 *   VBO Managerを使用する。
 * - callerはVBO Allocationの解放が完了するまで、
 *   対応するVBO Managerのlifetimeを保証する。
 *
 * @par Resource Identity
 * - Resource NameはRegistry内で一意とする。
 * - 同一Resource Nameの重複登録は許可しない。
 * - Geometry IDはRegistry内でのみ意味を持つ。
 * - Registry間でGeometry IDのidentityは共有しない。
 * - 同一Resourceに対する複数の利用権は同じGeometry IDを共有する。
 * - 最後の利用権の返却により解放されたGeometry IDは、
 *   後の新規登録で再利用される場合がある。
 * - Geometry IDだけでは利用権の保持を証明できない。
 *
 * @par Borrowed View
 * - CPU GeometryおよびDraw Rangeはconst borrowとして提供する。
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t;   /**< UI描画用ジオメトリレジストリのopaque型 */

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct ui_mesh_geometry ui_mesh_geometry_t;                     /**< UI描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;
typedef struct draw_range draw_range_t;
typedef struct vbo_manager vbo_manager_t;

/**
 * @brief UI Mesh Geometry Registryを生成する。
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
 * @param[in] max_geometry_count_ 最大登録可能Geometry数。
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
resource_registry_result_t ui_mesh_geometry_registry_create(size_t max_geometry_count_, subsystem_allocator_t* allocator_, ui_mesh_geometry_registry_t** out_registry_);

/**
 * @brief UI Mesh Geometry Registryの終了処理を行う。
 *
 * @par Public API Contract
 * - 全Entryを走査し、登録済みResourceを解放する。
 * - 各ResourceのReference Countに関係なく、
 *   VBO Allocation、CPU Geometry、Resource Nameを一度ずつ解放する。
 * - 終了処理に先立って、すべての利用権をrelease()によって
 *   返却する必要はない。
 * - 各EntryではVBO Allocationを先に解放し、
 *   その後CPU GeometryとResource Nameを破棄する。
 * - 全Entryの終了処理が成功した場合、
 *   Registryの最大登録数を0に設定する。
 * - Registry本体およびEntry配列のbacking storageは解放しない。
 * - VBO Manager自体は破棄しない。
 * - NULL引数、Registryの整合性異常、またはEntryの終了処理失敗を
 *   検出した場合は、その時点で終了処理を打ち切る。
 * - 途中で終了処理に失敗した場合、それ以前に解放したEntryの
 *   状態は復元せず、残りのEntryの解放完了は保証しない。
 * - 終了処理の失敗はResultとして返さない。
 * - 正常終了後のRegistryに対する通常操作、
 *   および再度のdeinitializeは許可しない。
 *
 * @param[in,out] registry_ 終了処理対象のRegistry。
 * @param[in,out] vbo_manager_ VBO Allocationの解放に使用するVBO Manager。
 *
 * @pre registry_は利用可能なRegistryを指していること。
 * @pre vbo_manager_は登録済みVBO Allocationを管理する、
 *      利用可能なVBO Managerを指していること。
 * @pre callerは対象Registryから取得したborrowの利用を終了していること。
 *
 * @post 正常終了時、登録済みResourceはすべて解放され、
 *       Registryのlogical lifetimeが終了する。
 * @post 正常終了後、取得済みのGeometry ID、利用権、
 *       およびborrowはすべて無効となる。
 */
void ui_mesh_geometry_registry_deinitialize(ui_mesh_geometry_registry_t* registry_, vbo_manager_t* vbo_manager_);

/**
 * @brief Resource Nameに対応するGeometryの登録状態を確認する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定されたResource NameをRegistryから検索する。
 * - 登録済みの場合はout_exists_へtrueを出力する。
 * - 未登録の場合はout_exists_へfalseを出力する。
 * - 未登録は正常な検索結果として扱い、SUCCESSを返す。
 * - 本APIは利用権を取得せず、Reference Countを変更しない。
 * - 検索結果によってGeometryの生存期間は保証されない。
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
resource_registry_result_t ui_mesh_geometry_registry_exists(const ui_mesh_geometry_registry_t* registry_, const char* resource_name_, bool* out_exists_);

/**
 * @brief Geometry IDに対応するUI Mesh Geometryを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistryが所有するCPU Geometryへのconst borrowを返す。
 * - Geometryのコピーは生成せず、所有権を移転しない。
 * - 本APIは利用権を新たに付与せず、Reference Countを変更しない。
 * - callerは取得したGeometryを変更・解放してはならない。
 * - 利用権を保持するcallerは、その利用権が有効な間、
 *   取得したborrowを利用できる。
 * - callerは自身の利用権を返却した後、その利用権に基づく
 *   borrowの利用を終了しなければならない。
 * - 最後の利用権の返却、またはRegistryの終了処理によって、
 *   対応するすべてのborrowは無効となる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] geometry_id_ 取得するGeometryのID。
 *
 * @return 成功時はUI Mesh Geometryへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const ui_mesh_geometry_t* ui_mesh_geometry_registry_geometry_get(const ui_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

/**
 * @brief Geometry IDに対応するDraw Rangeを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistryが保持するDraw Rangeへのconst borrowを返す。
 * - Draw Rangeのコピーは生成せず、所有権を移転しない。
 * - 本APIは利用権を新たに付与せず、Reference Countを変更しない。
 * - callerは取得したDraw Rangeを変更してはならない。
 * - 利用権を保持するcallerは、その利用権が有効な間、
 *   取得したborrowを利用できる。
 * - callerは自身の利用権を返却した後、その利用権に基づく
 *   borrowの利用を終了しなければならない。
 * - 最後の利用権の返却、またはRegistryの終了処理によって、
 *   対応するすべてのborrowは無効となる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] geometry_id_ Draw Rangeを取得するGeometry ID。
 *
 * @return 成功時はDraw Rangeへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const draw_range_t* ui_mesh_geometry_registry_draw_range_get(const ui_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

/**
 * @brief 登録済みUI Mesh Geometryの利用権を取得する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定されたResource Nameが登録済みである必要がある。
 * - 未登録の場合はBAD_OPERATIONを返す。
 * - 登録済みResourceのReference Countを1増加させる。
 * - Reference CountがUINT16_MAXに達している場合は
 *   LIMIT_EXCEEDEDを返し、利用権を取得しない。
 * - 成功時、対応するGeometry IDをout_geometry_id_へ出力する。
 * - 成功時、callerは対象Resourceの利用権を1つ取得する。
 * - callerは取得した利用権をrelease()によって返却する責務を持つ。
 * - Resourceの新規登録、コピー、所有権移転は行わない。
 *
 * @par Failure Contract
 * - 失敗時、Reference Countおよびout_geometry_id_の値を変更しない。
 * - DATA_CORRUPTEDの場合、Registryの内部整合性は保証されない。
 *
 * @param[in,out] registry_ 利用権を取得するRegistry。
 * @param[in] resource_name_ 利用権を取得するResource Name。
 * @param[out] out_geometry_id_ 対応するGeometry IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 利用権の取得成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはResource Nameが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定されたResource Nameが未登録。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED Reference Countが上限に達している。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの内部整合性検証、
 *         またはGeometry IDの内部表現に異常がある。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、対象ResourceのReference Countが1増加する。
 * @post 成功時、out_geometry_id_には対象ResourceのGeometry IDが格納される。
 * @post Resource自体の所有権および登録状態は変更されない。
 */
resource_registry_result_t ui_mesh_geometry_registry_acquire(ui_mesh_geometry_registry_t* registry_, const char* resource_name_, uint16_t* out_geometry_id_);

/**
 * @brief UI Mesh Geometryを指定名でRegistryへ新規登録する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - CPU Geometryはui_mesh_geometry_is_valid()の
 *   条件を満たす必要がある。
 * - VBO Rangeはvbo_range_is_valid()の条件を満たす必要がある。
 * - 同じResource Nameの重複登録は許可しない。
 * - 重複登録の場合はBAD_OPERATIONを返す。
 * - 登録可能なEntryが存在しない場合はLIMIT_EXCEEDEDを返す。
 * - 成功時にはResource Nameを複製し、CPU Geometryと
 *   VBO Allocationの解放責務をRegistryへ移転する。
 * - 新規登録成功時、対象EntryのReference Countを1に設定する。
 * - 新規登録成功によってcallerは最初の利用権を取得する。
 * - callerは取得した利用権をrelease()によって返却する責務を持つ。
 * - 成功時、*geometry_をNULLにし、vbo_range_をゼロ初期化する。
 * - 登録したGeometry IDをout_geometry_id_へ出力する。
 * - 解放済みEntryのIDが後の登録で再利用される場合がある。
 *
 * @par Failure Contract
 * - 回復可能な失敗ではRegistryの登録状態を変更せず、
 *   CPU GeometryとVBO Allocationの所有権はcallerに残る。
 * - 回復可能な失敗ではgeometry_、vbo_range_、
 *   out_geometry_id_の参照先を変更しない。
 * - DATA_CORRUPTEDの場合は、Registryの状態復元および
 *   所有権移転の完了状態を保証しない。
 *
 * @param[in,out] registry_ 登録先のRegistry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in,out] geometry_ 登録するCPU Geometryの所有ポインタ。
 * @param[in,out] vbo_range_ 登録するVBO AllocationのDescriptor。
 * @param[out] out_geometry_id_ 登録したGeometry IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数、CPU Geometry、
 *         またはVBO Rangeが不正。
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
 * @pre geometry_はcallerが所有する有効なCPU Geometryを指していること。
 * @pre vbo_range_は解放されていないVBO AllocationのDescriptorを指していること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、指定名と出力IDによって登録したGeometryを参照できる。
 * @post 成功時、CPU GeometryとVBO Allocationの解放責務はRegistryが持つ。
 * @post 成功時、対象EntryのReference Countは1となり、
 *       callerは対象Resourceの利用権を1つ保持する。
 */
resource_registry_result_t ui_mesh_geometry_registry_register(ui_mesh_geometry_registry_t* registry_, const char* resource_name_, ui_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_);

/**
 * @brief 指定IDのUI Mesh Geometryの利用権を返却する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内である必要がある。
 * - 未登録のIDを指定した場合はBAD_OPERATIONを返す。
 * - 成功時には対象EntryのReference Countを1減少させる。
 * - Reference Countが2以上の場合、減算のみを行い、
 *   Resourceの登録状態および所有権を維持する。
 * - Reference Countが1の場合、最後の利用権の返却として、
 *   VBO Allocation、CPU GeometryおよびResource Nameを解放する。
 * - 最後の利用権の返却に成功した場合、
 *   対象EntryをEmpty Stateへ戻す。
 * - Empty Stateへ戻ったEntryのIDは、後の登録で再利用される場合がある。
 * - 最後の利用権の返却によって、対応するGeometry IDおよび
 *   取得済みのすべてのborrowは無効となる。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 *
 * @par Failure Contract
 * - Commit前の引数不正または登録状態の不一致では、
 *   Registryの登録状態およびReference Countを変更しない。
 * - DATA_CORRUPTEDの場合は、対象Resourceの解放完了および
 *   Registryの状態復元を保証しない。
 *
 * @param[in,out] registry_ 利用権を返却するRegistry。
 * @param[in,out] vbo_manager_ 最後の利用権の返却時に
 *            VBO Allocationの解放へ使用するVBO Manager。
 * @param[in] geometry_id_ 利用権を返却するGeometry ID。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 利用権の返却成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはIDの範囲が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定IDが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの内部整合性検証、
 *         またはResource解放に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre vbo_manager_は対象VBO Allocationを管理する、
 *      利用可能なVBO Managerを指していること。
 * @pre callerは対象Geometryに対する有効な利用権を保持していること。
 * @pre callerは返却する利用権に基づくborrowの利用を終了していること。
 *
 * @post 成功時、対象Resourceの利用権が1つ返却される。
 * @post 返却前のReference Countが2以上の場合、
 *       登録状態を維持したままReference Countが1減少する。
 * @post 返却前のReference Countが1の場合、
 *       対象EntryはEmpty Stateとなり、所有していたResourceは解放済みとなる。
 */
resource_registry_result_t ui_mesh_geometry_registry_release(ui_mesh_geometry_registry_t* registry_, vbo_manager_t* vbo_manager_, uint16_t geometry_id_);

/**
 * @brief UI Mesh Geometry Registryのcanonical validityを判定する。
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
 * - Reference Countが0である。
 * - Resource NameとCPU GeometryのポインタがNULLである。
 * - VBO Allocation Descriptorの各フィールドがゼロ状態である。
 *
 * Registered State:
 * - Reference Countが1以上である。
 * - Resource NameとCPU GeometryのポインタがNULLではない。
 * - Resource NameがGeneral Allocator管理下の
 *   current live allocationである。
 * - Resource NameがvalidなChoco Stringであり、空文字列ではない。
 * - CPU GeometryがGeneral Allocator管理下の
 *   current live allocationである。
 * - CPU Geometryがcanonical validityを満たす。
 * - VBO Rangeがcanonical validityを満たす。
 *
 * Reference CountとResourceの登録状態に矛盾があるEntry、
 * またはEmpty / Registeredのどちらにも該当しないEntryは
 * invalidとする。
 *
 * Resource Nameの一意性、CPU GeometryとVBO Rangeの頂点数の一致、
 * CPU GeometryのデータサイズとVBO Allocation Sizeの関係、
 * VBO Allocationの実在性、Registry本体とEntry配列の
 * backing storageのallocation-level validity、
 * およびAllocatorの内部状態は検証対象に含まない。
 *
 * 本APIはRegistryの状態を変更しない。
 *
 * @param[in] registry_ 検証対象のRegistry。
 *
 * @retval true すべてのcanonical invariantを満たす。
 * @retval false いずれかのcanonical invariantを満たさない。
 *
 * @pre NULL以外のregistry_を指定する場合、Registry、Entry配列、
 *      およびEntryが参照するResourceのstorageは、
 *      検証処理に必要な範囲で読み取り可能であること。
 */
bool ui_mesh_geometry_registry_is_valid(const ui_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
