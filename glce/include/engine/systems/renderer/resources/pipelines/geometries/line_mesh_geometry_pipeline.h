// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file line_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 * @brief Line Mesh Geometryの生成、GPU頂点データ転送、
 *        Registry登録および利用権返却を提供する。
 *
 * @details
 * Line Meshの入力データからCPU Geometryを生成し、
 * VBO Managerへの頂点データ転送とGeometry Registryへの登録を
 * 一連のOperationとして実行する。
 *
 * 本PipelineはResourceの生成・登録手順を調整するものであり、
 * 登録済みResourceやVBO Manager自体を恒久的に所有しない。
 *
 * @section line_mesh_geometry_pipeline_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - CPU Geometryの生成はLine Mesh Geometry Moduleへ委譲する。
 * - GPU頂点データの転送とVBO Allocationの確保は
 *   VBO Managerへ委譲する。
 * - Resource Nameの複製と登録状態の管理は
 *   Geometry Registryへ委譲する。
 * - VBO Manager本体はcaller側が所有し、本Pipelineはborrowする。
 * - 新規登録成功時、CPU GeometryとVBO Allocationの
 *   解放責務はGeometry Registryへ移転する。
 * - 登録済みResourceのlifetimeは、
 *   Geometry RegistryのReference Countによって管理される。
 *
 * @par Import
 * - 頂点配列またはAABBからCPU Geometryを生成する。
 * - CPU Geometryの頂点データをVBO Managerへ書き込む。
 * - VBO Allocation Offsetから描画開始頂点を算出する。
 * - CPU GeometryとDraw RangeをGeometry Registryへ登録する。
 * - 新規登録成功時、Reference Countは1となり、
 *   callerは最初の利用権を取得する。
 * - 入力された頂点配列やAABBのownershipは取得しない。
 *
 * @par VBO Allocation / Draw Range
 * - Line Meshの各頂点はline_vertex_tによって表現する。
 * - Line Mesh Geometryの頂点数は正の偶数とする。
 * - VBOへの書込サイズはvertex_count * sizeof(line_vertex_t)とする。
 * - VBO Allocation Offsetはsizeof(line_vertex_t)で
 *   割り切れる必要がある。
 * - Draw Rangeの開始頂点は、
 *   Allocation Offset / sizeof(line_vertex_t)として算出する。
 * - callerは、この条件を満たすAllocationを返す
 *   VBO Managerの構成と利用方法を保証する。
 * - VBO Managerのbase_alignには、Line Geometryの
 *   書込サイズと整合する値を指定する必要がある。
 * - OffsetとVertex Strideの不整合を検出した場合は
 *   DATA_CORRUPTEDとして扱う。
 *
 * @par Reference Counting / Release
 * - 本Pipelineは登録済みResourceのReference Countを保持しない。
 * - release()はGeometry Registryのrelease()へ処理を委譲する。
 * - Reference Countが2以上の場合は利用権を1つ返却するだけで、
 *   CPU GeometryとVBO Allocationを維持する。
 * - 最後の利用権を返却した場合、Geometry Registryが
 *   CPU GeometryとVBO Allocationを解放する。
 * - 他のcallerによる利用権の取得はGeometry Registryの
 *   acquire()によって行う。
 *
 * @par Failure / Rollback
 * - Registryへの登録前に発生した回復可能な失敗では、
 *   本Operationで確保したCPU GeometryとVBO Allocationを解放する。
 * - Rollbackに失敗した場合はDATA_CORRUPTEDとして扱う。
 * - DATA_CORRUPTED発生時は、所有関係や内部状態を
 *   安全に判断できない可能性があるため、
 *   通常のcleanupによる解放を行わない。
 * - Import失敗時はGeometry IDを出力しない。
 * - Registryへの登録後は、Resourceの解放を
 *   Registryのlifetime管理へ委譲する。
 *
 * @par Lifetime
 * - callerはPipelineのOperation中、
 *   VBO ManagerとGeometry Registryが利用可能であることを保証する。
 * - Geometry RegistryがVBO Allocationを所有している間、
 *   対応するVBO Managerは利用可能である必要がある。
 * - VBO ManagerのBackend Contextに関するlifetimeは、
 *   VBO Manager自身のModule Boundary Contractに従う。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct vbo_manager vbo_manager_t;
typedef struct line_mesh_geometry_registry line_mesh_geometry_registry_t;   /**< 線分描画用ジオメトリレジストリのopaque型 */
typedef struct line_vertex line_vertex_t;
typedef struct aabb_3d aabb_3d_t;

/**
 * @brief Line Mesh頂点配列をCPU / GPU Resourceとして登録する。
 *
 * @par Public API Contract
 * - 入力頂点数は0より大きい偶数とする。
 * - 頂点配列の2頂点を1線分として扱う。
 * - 入力頂点配列から独立したCPU Geometryを生成する。
 * - CPU Geometryの生成はline_mesh_geometry_create_from_vertices()
 *   へ委譲する。
 * - 頂点データをVBO Managerへ書き込み、
 *   対応するVBO Allocationを確保する。
 * - 書込サイズの計算にはoverflow検査を適用する。
 * - Allocation Offsetがsizeof(line_vertex_t)の
 *   倍数であることを確認する。
 * - Allocation OffsetとVertex StrideからDraw Rangeを構築する。
 * - CPU GeometryとVBO Allocationを指定Resource Nameで
 *   Geometry Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には対応するGeometry IDを出力する。
 *
 * @par Ownership
 * - 入力頂点配列のownershipはcallerが保持する。
 * - 本Operationが生成したCPU GeometryとVBO Allocationは、
 *   Registryへの登録成功時にRegistryの管理下へ移転する。
 * - 登録成功時、callerは対象Geometryの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Failure Contract
 * - Registryへの登録前の回復可能な失敗では、
 *   本Operationが生成したCPU Geometryと
 *   確保済みVBO Allocationを解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDを返す。
 * - DATA_CORRUPTED発生時は通常のcleanupを実行せず、
 *   Resourceの解放・Ownership状態の復元を保証しない。
 * - 失敗時にはout_geometry_id_の値を変更しない。
 *
 * @param[in,out] vbo_manager_ 頂点データの転送とVBO Allocationの
 *                管理に使用するVBO Manager。
 * @param[in,out] geometry_registry_ 登録先のLine Mesh Geometry Registry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in] vertices_ 入力頂点配列。
 * @param[in] vertex_count_ 入力頂点数。
 * @param[out] out_geometry_id_ 登録したGeometry IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS 登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 引数、頂点数、
 *         Resource Name、または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION Resource Nameが登録済み、
 *         VBO書込条件が成立しない、または下位APIのOperationが不成立。
 * @retval RESOURCE_PIPELINE_OVERFLOW 頂点データサイズ等の計算でoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED Registryの登録数、
 *         VBO容量、または下位Resourceの使用制限を超過。
 * @retval RESOURCE_PIPELINE_NO_MEMORY CPU Resource等のメモリ確保に失敗。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR 下位処理で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED 内部状態の整合性異常、
 *         Allocation Offsetの不整合、またはrollback失敗。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR 下位処理で未分類のエラーが発生。
 *
 * @pre vbo_manager_とgeometry_registry_は利用可能であること。
 * @pre resource_name_は読み取り可能なNUL終端文字列であること。
 * @pre vertices_はvertex_count_個の有効な頂点を参照すること。
 * @pre VBO ManagerはLine VertexのStrideと整合する
 *      Allocation Offsetを生成できるように構成されていること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、指定Resource NameでCPU Geometryと
 *       VBO AllocationがRegistryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_geometry_id_に登録IDが格納される。
 * @post 入力頂点配列のownershipは移転しない。
 */
resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(vbo_manager_t* vbo_manager_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_);

/**
 * @brief AABBからLine Mesh Geometryを生成し、CPU / GPU Resourceとして登録する。
 *
 * @par Public API Contract
 * - 入力AABBから、その12辺を表現するLine Mesh Geometryを生成する。
 * - AABBは1個のみを対象とする。
 * - 生成されるCPU Geometryは24頂点、12線分から構成される。
 * - CPU Geometryの生成はline_mesh_geometry_create_from_aabbs()
 *   へ委譲する。
 * - 生成したGeometryから頂点配列をconst borrowとして取得する。
 * - 取得した頂点データをVBO Managerへ転送し、
 *   対応するVBO Allocationを確保する。
 * - 書込サイズの計算にはoverflow検査を適用する。
 * - Allocation Offsetがsizeof(line_vertex_t)の
 *   倍数であることを確認する。
 * - Allocation OffsetとVertex StrideからDraw Rangeを構築する。
 * - CPU GeometryとVBO Allocationを指定Resource Nameで
 *   Geometry Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には対応するGeometry IDを出力する。
 *
 * @par Ownership
 * - 入力AABBのownershipはcallerが保持する。
 * - 生成したCPU Geometryは本Operationが一時的に所有する。
 * - CPU Geometryから取得した頂点配列はborrowであり、
 *   個別には解放しない。
 * - 登録成功時、CPU GeometryとVBO Allocationの
 *   解放責務はGeometry Registryへ移転する。
 * - 登録成功時、callerは対象Geometryの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Failure Contract
 * - Registryへの登録前の回復可能な失敗では、
 *   本Operationが生成したCPU Geometryと
 *   確保済みVBO Allocationを解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDを返す。
 * - DATA_CORRUPTED発生時は通常のcleanupを実行せず、
 *   Resourceの解放・Ownership状態の復元を保証しない。
 * - 失敗時にはout_geometry_id_の値を変更しない。
 *
 * @param[in,out] vbo_manager_ 頂点データの転送とVBO Allocationの
 *                管理に使用するVBO Manager。
 * @param[in,out] geometry_registry_ 登録先のLine Mesh Geometry Registry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in] aabb_ 入力AABB。
 * @param[in] color_ 入力描画色。
 * @param[out] out_geometry_id_ 登録したGeometry IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS 登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 引数、Resource Name、
 *         AABB、または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION Resource Nameが登録済み、
 *         VBO書込条件が成立しない、または下位APIのOperationが不成立。
 * @retval RESOURCE_PIPELINE_OVERFLOW 頂点データサイズ等の計算でoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED Registryの登録数、
 *         VBO容量、または下位Resourceの使用制限を超過。
 * @retval RESOURCE_PIPELINE_NO_MEMORY CPU Resource等のメモリ確保に失敗。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR 下位処理で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED 内部状態の整合性異常、
 *         Allocation Offsetの不整合、またはrollback失敗。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR 下位処理で未分類のエラーが発生。
 *
 * @pre vbo_manager_とgeometry_registry_は利用可能であること。
 * @pre resource_name_は読み取り可能なNUL終端文字列であること。
 * @pre aabb_は有効なAABBを参照していること。
 * @pre VBO ManagerはLine VertexのStrideと整合する
 *      Allocation Offsetを生成できるように構成されていること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、AABBの12辺を表現するLine Mesh Geometryが
 *       CPU / GPU ResourceとしてRegistryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_geometry_id_に登録IDが格納される。
 * @post 入力AABBのownershipは移転しない。
 */
resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(vbo_manager_t* vbo_manager_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const aabb_3d_t* aabb_, vec4u8_t color_, uint16_t* out_geometry_id_);

/**
 * @brief Line Mesh Geometryの利用権を1つ返却する。
 *
 * @par Public API Contract
 * - 指定Geometry IDの利用権をGeometry Registryへ返却する。
 * - Geometry IDの範囲と登録状態の検証は
 *   Geometry Registryのrelease()へ委譲する。
 * - Reference Countが2以上の場合、利用権を1つ減少させ、
 *   CPU GeometryとVBO Allocationは維持する。
 * - Reference Countが1の場合、最後の利用権の返却として
 *   CPU GeometryとVBO Allocationを解放する。
 * - 最後の利用権を返却したEntryはEmpty Stateへ戻る。
 * - Resourceの解放はGeometry Registryが担当する。
 * - 本PipelineはReference CountやRegistryのEntryを直接操作しない。
 * - VBO Manager自体は破棄しない。
 *
 * @par Failure Contract
 * - Geometry Registryのrelease()が失敗した場合、
 *   そのResultをPipelineのResult Codeへ変換して返す。
 * - Commit前の失敗ではRegistryの登録状態を変更しない。
 * - DATA_CORRUPTEDの場合、下位のResource解放処理が
 *   完了したことや内部状態の復元は保証されない。
 *
 * @param[in,out] vbo_manager_ VBO Allocationの解放に使用するVBO Manager。
 * @param[in,out] geometry_registry_ 利用権を返却するGeometry Registry。
 * @param[in] geometry_id_ 利用権を返却するGeometry ID。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS 利用権の返却成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 引数またはGeometry IDの範囲が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION 指定Geometry IDが未登録。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED Registryの内部整合性異常、
 *         またはVBO Allocationの解放失敗。
 *
 * @pre vbo_manager_とgeometry_registry_は利用可能であること。
 * @pre vbo_manager_は対象VBO Allocationを管理するものであること。
 * @pre callerは対象Geometryに対する有効な利用権を保持していること。
 * @pre callerは返却する利用権に基づくborrowの利用を終了していること。
 *
 * @post 成功時、対象Geometryの利用権が1つ返却される。
 * @post 返却前のReference Countが2以上の場合、
 *       Resourceの登録状態は維持される。
 * @post 返却前のReference Countが1の場合、
 *       対象EntryはEmpty Stateとなり、所有Resourceは解放される。
 */
resource_pipeline_result_t line_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, line_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
