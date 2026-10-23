// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 * @brief Untextured Mesh Geometryの生成、GPU頂点データ転送、
 *        Registry登録および利用権返却を提供する。
 *
 * @details
 * ASCII STLファイルから頂点座標と法線情報を読み込み、
 * CPU Geometryの生成、VBO Managerへの頂点データ転送、
 * Geometry Registryへの登録を一連のOperationとして実行する。
 *
 * 本PipelineはResourceの生成・登録手順を調整するものであり、
 * 登録済みResourceやVBO Manager自体を恒久的に所有しない。
 *
 * @section untextured_mesh_geometry_pipeline_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - ASCII STLファイルの読み込みと頂点データの生成は
 *   STL ASCII Loaderへ委譲する。
 * - CPU Geometryの生成はUntextured Mesh Geometry Moduleへ委譲する。
 * - GPU頂点データの転送とVBO Allocationの確保は
 *   VBO Managerへ委譲する。
 * - Resource Nameの複製と登録状態の管理は
 *   Geometry Registryへ委譲する。
 * - VBO ManagerとGeometry Registryはcallerが所有し、
 *   本PipelineはOperation中にborrowする。
 * - 新規登録成功時、CPU GeometryとVBO Allocationの
 *   解放責務はGeometry Registryへ移転する。
 * - 登録済みResourceのlifetimeは、
 *   Geometry RegistryのReference Countによって管理される。
 *
 * @par File Loading / Trust Boundary
 * - 入力ファイルはASCII STL形式とする。
 * - Binary STL形式は本Pipelineの対応対象に含まない。
 * - ファイルのsyntax、triangle構造、頂点座標、
 *   法線情報のsemantic validityはSTL ASCII Loaderが検証する。
 * - Loaderの成功によって、取得した頂点配列は
 *   trusted internal representationとして受理される。
 * - 本PipelineはLoaderのsemantic validationを重複しない。
 * - 入力ファイルおよびFile Pathのownershipは取得しない。
 *
 * @par Temporary Vertex Storage
 * - STL ASCII Loaderはpoint_normal_vertex_tの配列を生成する。
 * - 頂点配列はGeneral Allocatorから確保され、
 *   ownershipはLoaderから本Operationへ移転する。
 * - CPU GeometryはLoaderが返した頂点配列をコピーして生成する。
 * - VBOへの頂点データ転送にもLoaderが返した配列を使用する。
 * - CPU Geometryの生成とVBO転送が完了した後、
 *   一時頂点配列をGeneral Allocatorへ返却する。
 * - Loaderが返した頂点配列のownershipは
 *   Geometry Registryへ移転しない。
 *
 * @par Geometry Construction
 * - 頂点データはpoint_normal_vertex_tによって表現する。
 * - 各頂点は位置情報と法線情報を保持する。
 * - 3頂点を1つの三角形として扱う。
 * - CPU Geometryの頂点数は正の3の倍数とする。
 * - 頂点数のsemantic validityは
 *   Untextured Mesh Geometry Moduleが所有する。
 * - CPU GeometryはMaterial Resourceを所有しない。
 *
 * @par VBO Allocation / Draw Range
 * - VBOへの書込サイズは
 *   vertex_count * sizeof(point_normal_vertex_t)とする。
 * - VBO Allocation Offsetはsizeof(point_normal_vertex_t)で
 *   割り切れる必要がある。
 * - Draw Rangeの開始頂点は、
 *   Allocation Offset / sizeof(point_normal_vertex_t)として算出する。
 * - Draw RangeのVertex Countは生成されたCPU Geometryの
 *   頂点数に対応する。
 * - callerは、この条件を満たすAllocationを返す
 *   VBO Managerの構成と利用方法を保証する。
 * - OffsetとVertex Strideの不整合を検出した場合は
 *   DATA_CORRUPTEDとして扱う。
 *
 * @par Reference Counting / Release
 * - 本Pipelineは登録済みResourceのReference Countを保持しない。
 * - 新規登録成功時、Reference Countは1となり、
 *   callerは最初の利用権を取得する。
 * - release()はGeometry Registryのrelease()へ処理を委譲する。
 * - Reference Countが2以上の場合、利用権を1つ返却するだけで、
 *   CPU GeometryとVBO Allocationを維持する。
 * - 最後の利用権を返却した場合、Geometry Registryが
 *   CPU GeometryとVBO Allocationを解放する。
 * - 他のcallerによる利用権の取得はGeometry Registryの
 *   acquire()によって行う。
 *
 * @par Failure / Rollback
 * - Loaderによるファイルの読み込み・解析に失敗した場合、
 *   CPU GeometryとVBO Allocationの生成を開始しない。
 * - Registryへの登録前に発生した回復可能な失敗では、
 *   本Operationが所有する一時頂点配列、CPU Geometry、
 *   およびVBO Allocationを解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDとして扱う。
 * - DATA_CORRUPTED発生時は、所有関係や内部状態を
 *   安全に判断できない可能性があるため、
 *   通常のcleanupによる解放を行わない。
 * - Import失敗時はGeometry IDを出力しない。
 * - Registryへの登録成功後は、Resourceの解放を
 *   Geometry Registryのlifetime管理へ委譲する。
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
 *
 * @date 2026-06-30
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UNTEXTURED_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UNTEXTURED_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct vbo_manager vbo_manager_t;
typedef struct untextured_mesh_geometry_registry untextured_mesh_geometry_registry_t; /**< 単色ライティング描画用ジオメトリレジストリのopaque型 */

/**
 * @brief ASCII STLファイルからUntextured Mesh Geometryを生成し、
 *        CPU / GPU Resourceとして登録する。
 *
 * @par Public API Contract
 * - 指定されたASCII STLファイルを読み込み、
 *   三角形の頂点座標と法線情報を取得する。
 * - ファイルの読み込み、解析、semantic validationは
 *   stl_ascii_loader_load()へ委譲する。
 * - Loaderが返す頂点数は正の3の倍数とする。
 * - Loaderが返した頂点配列から独立したCPU Geometryを生成する。
 * - CPU Geometryの生成は
 *   untextured_mesh_geometry_create_from_vertices()へ委譲する。
 * - 頂点データをVBO Managerへ書き込み、
 *   対応するVBO Allocationを確保する。
 * - 書込サイズの計算にはoverflow検査を適用する。
 * - Allocation Offsetがsizeof(point_normal_vertex_t)の
 *   倍数であることを確認する。
 * - Allocation OffsetとVertex StrideからDraw Rangeを構築する。
 * - CPU GeometryとVBO Allocationを指定Resource Nameで
 *   Geometry Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には対応するGeometry IDを出力する。
 *
 * @par Ownership
 * - 入力ファイルとResource Full Pathのownershipは
 *   callerが保持する。
 * - STL ASCII Loaderから返された頂点配列のownershipは
 *   本Operationが一時的に取得する。
 * - 頂点配列はGeneral Allocatorの
 *   GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRYを使用して解放する。
 * - CPU Geometryは頂点配列をコピーするため、
 *   Loaderの頂点配列とは独立したstorageを所有する。
 * - VBO転送とDraw Rangeの構築が完了した後、
 *   Loaderの一時頂点配列を解放する。
 * - 本Operationが生成したCPU GeometryとVBO Allocationは、
 *   Registryへの登録成功時にRegistryの管理下へ移転する。
 * - 登録成功時、callerは対象Geometryの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 *
 * @par Failure Contract
 * - Loaderの失敗時はResult Codeを変換して返す。
 * - Registryへの登録前の回復可能な失敗では、
 *   確保済みVBO Allocation、CPU Geometry、
 *   Loaderの一時頂点配列を解放する。
 * - VBO Allocationのrollbackが必要な場合は、
 *   CPU Geometryや頂点配列の解放より先に実行する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDを返す。
 * - DATA_CORRUPTED発生時は通常のcleanupを実行せず、
 *   Resourceの解放・Ownership状態の復元を保証しない。
 * - Registryのregister()がDATA_CORRUPTEDを返した場合も、
 *   Ownership Transferの状態を安全に判断できないため、
 *   通常のcleanupを行わない。
 * - 失敗時にはout_geometry_id_の値を変更しない。
 *
 * @param[in,out] vbo_manager_
 *                頂点データの転送とVBO Allocationの
 *                管理に使用するVBO Manager。
 * @param[in,out] geometry_registry_
 *                登録先のUntextured Mesh Geometry Registry。
 * @param[in] resource_name_
 *            登録するResource Name。
 * @param[in] resource_fullpath_
 *            読み込むASCII STLファイルのFull Path。
 * @param[out] out_geometry_id_
 *             登録したGeometry IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         Untextured Mesh Geometryの登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         引数、Resource Name、File Path、
 *         または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         Resource Nameが登録済み、
 *         VBO書込条件が成立しない、または下位Operationが不成立。
 * @retval RESOURCE_PIPELINE_FILE_OPEN_ERROR
 *         ASCII STLファイルを開けない。
 * @retval RESOURCE_PIPELINE_FILE_READ_ERROR
 *         ASCII STLファイルの読み込みに失敗。
 * @retval RESOURCE_PIPELINE_UNSUPPORTED_FILE
 *         ASCII STLファイルが対応形式または
 *         semantic requirementsを満たさない。
 * @retval RESOURCE_PIPELINE_OVERFLOW
 *         頂点データサイズ等の計算でoverflowが発生。
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED
 *         Registryの登録数、VBO容量、
 *         または下位Resourceの使用制限を超過。
 * @retval RESOURCE_PIPELINE_NO_MEMORY
 *         CPU Resource等のメモリ確保に失敗。
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR
 *         下位処理で実行時エラーが発生。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         内部状態の整合性異常、
 *         Allocation Offsetの不整合、またはrollback失敗。
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR
 *         下位処理で未分類のエラーが発生。
 *
 * @pre vbo_manager_とgeometry_registry_は利用可能であること。
 * @pre resource_name_は空文字列ではない
 *      読み取り可能なNUL終端文字列であること。
 * @pre resource_fullpath_は読み取り可能な
 *      NUL終端のFile Pathであること。
 * @pre VBO ManagerはUntextured Mesh VertexのStrideと整合する
 *      Allocation Offsetを生成できるように構成されていること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、ASCII STLファイルから生成された
 *       Untextured Mesh GeometryがRegistryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_geometry_id_に登録IDが格納される。
 * @post Loaderから取得した一時頂点配列は解放済みとなる。
 * @post 入力ファイルとResource Full Pathのownershipは移転しない。
 */
resource_pipeline_result_t untextured_mesh_geometry_pipeline_import_from_file(vbo_manager_t* vbo_manager_, untextured_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

/**
 * @brief Untextured Mesh Geometryの利用権を1つ返却する。
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
 * @param[in,out] vbo_manager_
 *                VBO Allocationの解放に使用するVBO Manager。
 * @param[in,out] geometry_registry_
 *                利用権を返却するGeometry Registry。
 * @param[in] geometry_id_
 *            利用権を返却するGeometry ID。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         利用権の返却成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         引数またはGeometry IDの範囲が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         指定Geometry IDが未登録。
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED
 *         Registryの内部整合性異常、
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
resource_pipeline_result_t untextured_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, untextured_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
