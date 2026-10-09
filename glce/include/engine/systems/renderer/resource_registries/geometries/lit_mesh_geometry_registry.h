// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file lit_mesh_geometry_registry.h
 * @author chocolate-pie24
 * @brief Lit Mesh Geometryの登録、検索、および登録解除を提供する。
 *
 * @details
 * Lit Mesh GeometryのCPU Resourceと対応するVBO Allocationを、
 * Resource NameおよびGeometry IDによって管理するRegistry。
 * Geometryの生成、GPUへの頂点データの転送、描画処理は担当しない。
 *
 * @section lit_mesh_geometry_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは登録済みLit Mesh GeometryのCPU Resourceを所有する。
 * - Registryは登録名の文字列Resourceを所有する。
 * - Registryは登録済みGeometryに対応するVBO Allocationの解放責務を持つ。
 * - VBO本体はLit Mesh Shader側が所有し、Registryは所有しない。
 * - 登録成功時、CPU GeometryおよびVBO Allocationの解放責務は
 *   callerからRegistryへ移転する。
 *
 * @par Storage / Lifetime
 * - Registry本体およびEntry配列のstorageは、
 *   callerから提供されたSubsystem Allocatorによって管理される。
 * - Registryはこれらのbacking storageを個別に解放しない。
 * - callerはRegistryの利用期間中、Allocatorのlifetimeを保証する。
 * - VBO Allocationの解放には、対象Allocationを管理する
 *   Lit Mesh Shaderを使用する。
 * - callerは登録済みVBO Allocationの解放が完了するまで、
 *   対応するLit Mesh Shaderのlifetimeを保証する。
 *
 * @par Resource Identity
 * - Geometry IDはRegistry内でのみ意味を持つ。
 * - Registry間でGeometry IDのidentityは共有しない。
 * - 同一Registry内でResource Nameの重複登録は許可しない。
 * - 登録解除されたGeometry IDは、後の登録で再利用される場合がある。
 *
 * @par Borrowed View
 * - 登録済みCPU GeometryおよびDraw Rangeへの参照はconst borrowとして提供する。
 * - callerは取得した参照先を変更・解放してはならない。
 * - 対象Geometryの登録解除、またはRegistryのlifetime終了によって
 *   対応するborrowは無効になる。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_LIT_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_LIT_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct lit_mesh_geometry_registry lit_mesh_geometry_registry_t; /**< 単色ライティング描画用ジオメトリレジストリのopaque型 */

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct lit_mesh_geometry lit_mesh_geometry_t;                   /**< 単色ライティング描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;
typedef struct draw_range draw_range_t;
typedef struct lit_mesh_shader lit_mesh_shader_t;

/**
 * @brief Lit Mesh Geometry Registryを生成する。
 *
 * @par Public API Contract
 * - 最大登録数は1以上UINT16_MAX以下とする。
 * - 出力先はNULLで初期化されている必要がある。
 * - 成功時には、指定容量の未登録Entryを持つRegistryを生成する。
 * - 回復可能な構築失敗では、今回の構築に伴うAllocatorの
 *   allocation stateを復元し、出力先はNULLのまま維持する。
 * - DATA_CORRUPTED発生時は通常のrollbackを行わない。
 *
 * @param[in] max_geometry_count_ 最大登録可能Geometry数。
 * @param[in,out] allocator_ Registryのstorageを提供するSubsystem Allocator。
 * @param[out] out_registry_ 生成したRegistryを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 生成成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 出力先がNULLでない、または下位Allocatorのoperationが成立しない。
 * @retval RESOURCE_REGISTRY_OVERFLOW 配列サイズ計算等でoverflowが発生。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 下位Allocatorの確保制限を超過。
 * @retval RESOURCE_REGISTRY_NO_MEMORY メモリ確保に失敗。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部状態の整合性検証またはrollbackに失敗。
 *
 * @pre allocator_は利用可能なSubsystem Allocatorを指していること。
 * @post 成功時、*out_registry_は通常操作に使用できるRegistryを指す。
 */
resource_registry_result_t lit_mesh_geometry_registry_create(size_t max_geometry_count_, subsystem_allocator_t* allocator_, lit_mesh_geometry_registry_t** out_registry_);

/**
 * @brief Registryの利用可能状態を終了する。
 *
 * @par Public API Contract
 * - 登録済みEntryが所有するCPU GeometryとResource Nameを破棄する。
 * - 登録済みVBO Allocationを、対応するLit Mesh Shaderを使用して解放する。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 * - 正常終了後、Registryは利用可能状態ではなくなり、
 *   再初期化せずに通常操作へ使用することはできない。
 * - 同じRegistryに対する再度のdeinitializeは許可しない。
 *
 * @par Failure Contract
 * - 引数不正、内部状態の異常、またはResource解放失敗を検出した場合は
 *   処理を中断する。
 * - 処理中断時は、すべての登録済みResourceの解放完了を保証しない。
 *
 * @param[in,out] registry_ 終了対象のRegistry。
 * @param[in,out] shader_ VBO Allocationの解放に使用するLit Mesh Shader。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre shader_は登録済みVBO Allocationを管理する、利用可能な
 *      Lit Mesh Shaderを指していること。
 * @pre shader_のlifetimeは本APIの実行完了まで維持されること。
 *
 * @post 正常終了後、registry_のlogical lifetimeは終了する。
 */
void lit_mesh_geometry_registry_deinitialize(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_);

/**
 * @brief 指定名のLit Mesh Geometryが登録されているか判定する。
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
 * @retval true 指定名のGeometryが登録されている。
 * @retval false 未登録、または検索を実行できなかった。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 */
bool lit_mesh_geometry_registry_exists(const lit_mesh_geometry_registry_t* registry_, const char* name_);

/**
 * @brief Geometry IDに対応するLit Mesh Geometryを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistryが所有するCPU Geometryへのconst borrowを返す。
 * - Geometryのコピーは生成せず、callerへ所有権を移転しない。
 * - 戻り値はcallerが解放・変更してはならない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によって
 *   戻り値のborrowは無効になる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] geometry_id_ 取得するGeometryのID。
 *
 * @return 成功時はLit Mesh Geometryへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const lit_mesh_geometry_t* lit_mesh_geometry_registry_geometry_get(const lit_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

/**
 * @brief Resource Nameに対応するGeometry IDを取得する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - 指定名のGeometryが登録されている場合、そのIDを出力する。
 * - 指定名が未登録の場合はBAD_OPERATIONを返す。
 * - 失敗時、出力先の値は変更しない。
 * - Registryの登録状態は変更しない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 * @param[out] out_geometry_id_ 取得したGeometry IDの出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 取得成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定名のGeometryが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 *
 * @post 成功時、出力したIDは指定名の登録済みGeometryを識別する。
 */
resource_registry_result_t lit_mesh_geometry_registry_id_get(const lit_mesh_geometry_registry_t* registry_, const char* name_, uint16_t* out_geometry_id_);

/**
 * @brief Geometry IDに対応するDraw Rangeを取得する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内であり、登録済みである必要がある。
 * - 成功時にはRegistryが保持するDraw Rangeへのconst borrowを返す。
 * - Draw Rangeのコピーは生成せず、callerへ所有権を移転しない。
 * - 戻り値はcallerが変更してはならない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によって
 *   戻り値のborrowは無効になる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] geometry_id_ Draw Rangeを取得するGeometry ID。
 *
 * @return 成功時はDraw Rangeへのconstポインタ。
 * @return 失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const draw_range_t* lit_mesh_geometry_registry_draw_range_get(const lit_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

/**
 * @brief Lit Mesh Geometryを指定名でRegistryへ登録する。
 *
 * @par Public API Contract
 * - Resource Nameは空文字列ではないNUL終端文字列とする。
 * - CPU Geometryは`lit_mesh_geometry_is_valid()`の条件を満たす必要がある。
 * - VBO Rangeは`vbo_range_is_valid()`の条件を満たす必要がある。
 * - 同じResource Nameの重複登録は許可しない。
 * - 登録可能なEntryが存在しない場合はLIMIT_EXCEEDEDを返す。
 * - 成功時にはResource Nameを複製し、CPU Geometryと
 *   VBO Allocationの解放責務をRegistryへ移転する。
 * - 成功時、*geometry_をNULLにし、vbo_range_をゼロ初期化する。
 * - 登録したGeometry IDをout_geometry_id_へ出力する。
 * - 登録解除済みEntryのIDが再利用される場合がある。
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
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数、CPU Geometry、またはVBO Rangeが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION Resource Nameが登録済み。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 登録可能なEntryが存在しない、または下位処理の制限を超過。
 * @retval RESOURCE_REGISTRY_NO_MEMORY Resource Nameの生成に必要なメモリを確保できない。
 * @retval RESOURCE_REGISTRY_OVERFLOW Resource Nameの生成処理でoverflowが発生。
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
 */
resource_registry_result_t lit_mesh_geometry_registry_register(lit_mesh_geometry_registry_t* registry_, const char* resource_name_, lit_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_);

/**
 * @brief 指定IDのLit Mesh Geometryを登録解除する。
 *
 * @par Public API Contract
 * - 指定IDはRegistryの有効範囲内である必要がある。
 * - 未登録のIDを指定した場合はBAD_OPERATIONを返す。
 * - 成功時には対象EntryのVBO Allocationを対応するShaderから解放し、
 *   CPU GeometryとResource Nameを破棄する。
 * - 成功時には対象Entryを未登録状態に戻す。
 * - 登録解除後、そのIDは再登録時に再利用される場合がある。
 * - 登録解除したGeometryのIDおよび取得済みborrowは無効になる。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 *
 * @par Failure Contract
 * - Commit前の引数不正または登録状態の不一致では、
 *   Registryの登録状態を変更しない。
 * - DATA_CORRUPTEDの場合は、対象Resourceの解放完了および
 *   Registryの状態復元を保証しない。
 *
 * @param[in,out] registry_ 登録解除対象のRegistry。
 * @param[in,out] shader_ VBO Allocationの解放に使用するLit Mesh Shader。
 * @param[in] geometry_id_ 登録解除するGeometry ID。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録解除成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはIDの範囲が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定IDが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証またはResource解放に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre shader_は対象VBO Allocationを管理する、利用可能な
 *      Lit Mesh Shaderを指していること。
 * @pre 登録解除するGeometryに対するborrowの利用が終了していること。
 *
 * @post 成功時、指定IDに対応するEntryは未登録状態になる。
 * @post 成功時、対象Entryが所有していたResourceは解放済みとなる。
 */
resource_registry_result_t lit_mesh_geometry_registry_unregister(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_, uint16_t geometry_id_);

/**
 * @brief Lit Mesh Geometry Registryのcanonical validityを判定する。
 *
 * @par Validation Scope
 * 以下の条件を検証する。
 * - RegistryのポインタがNULLではない。
 * - 最大登録数が1以上UINT16_MAX以下である。
 * - Entry配列のポインタがNULLではない。
 * - 各EntryがEmpty StateまたはRegistered Stateのいずれかを満たす。
 *
 * Empty State:
 * - Resource NameとCPU GeometryのポインタがNULLである。
 * - VBO Allocation Descriptorのすべてのフィールドがゼロ状態である。
 *
 * Registered State:
 * - Resource NameがvalidなChoco Stringであり、空文字列ではない。
 * - CPU Geometryがcanonical validityを満たす。
 * - VBO Rangeがcanonical validityを満たす。
 *
 * Resource Nameの一意性、CPU GeometryとVBO Rangeの頂点数の一致、
 * CPU GeometryのデータサイズとVBO Allocation Sizeの関係、
 * backing storageのallocation-level validity、
 * およびAllocatorの内部状態は検証対象に含まない。
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
bool lit_mesh_geometry_registry_is_valid(const lit_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
