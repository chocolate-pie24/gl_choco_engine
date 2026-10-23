// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file ui_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 * @brief UI Mesh Geometryの生成、GPU頂点データ転送、
 *        Registry登録および利用権返却を提供する。
 *
 * @details
 * UI Geometry Configuration Fileから矩形の構成情報を読み込み、
 * UI Mesh Geometryの生成、VBO Managerへの頂点データ転送、
 * Geometry Registryへの登録を一連のOperationとして実行する。
 *
 * 本PipelineはResourceの生成・登録手順を調整するものであり、
 * 登録済みResourceやVBO Manager自体を恒久的に所有しない。
 *
 * @section ui_mesh_geometry_pipeline_boundary_contract Module Boundary Contract
 *
 * @par Resource Ownership
 * - UI Geometry Configuration Fileの読み込みと解析は
 *   UI Geometry Config Loaderへ委譲する。
 * - CPU Geometryの生成はUI Mesh Geometry Moduleへ委譲する。
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
 * @par Configuration / Trust Boundary
 * - 入力ファイルはUI Geometry Configuration Formatに従う
 *   `.ui_geom`ファイルとする。
 * - ファイルのsyntax、supported key、required field、
 *   value representationおよびsemantic validityの検証は
 *   UI Geometry Config Loaderが所有する。
 * - Loaderの成功によって、取得したui_geom_config_tは
 *   trusted internal representationとして受理される。
 * - 本PipelineはLoaderによるValidationを重複しない。
 * - 入力ファイルやその内容のownershipは取得しない。
 *
 * @par Geometry Construction
 * - 1つのUI Mesh Geometryは1つの矩形領域を表す。
 * - 矩形領域は2枚の三角形、合計6頂点で構成する。
 * - 矩形の幅と高さには、読み込んだicon_widthと
 *   icon_heightを使用する。
 * - 頂点位置とTexture UV座標を構築し、
 *   ui_mesh_geometry_create_from_vertices()へ渡す。
 * - 生成するCPU Geometryは頂点位置とUV座標を保持する。
 * - Texture Resource自体の生成、登録、Ownership管理は行わない。
 *
 * @par VBO Allocation / Draw Range
 * - UI Meshの各頂点はui_vertex_tによって表現する。
 * - VBOへの書込サイズは6 * sizeof(ui_vertex_t)とする。
 * - VBO Allocation Offsetはsizeof(ui_vertex_t)で
 *   割り切れる必要がある。
 * - Draw Rangeの開始頂点は、
 *   Allocation Offset / sizeof(ui_vertex_t)として算出する。
 * - Draw RangeのVertex Countは6とする。
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
 *   本Operationで確保したCPU GeometryとVBO Allocationを解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDとして扱う。
 * - DATA_CORRUPTED発生時は、所有関係や内部状態を
 *   安全に判断できない可能性があるため、
 *   通常のcleanupによる解放を行わない。
 * - Import失敗時はGeometry IDを出力しない。
 * - Registryへの登録後は、Resourceの解放を
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
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct vbo_manager vbo_manager_t;
typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t; /**< UI描画用ジオメトリレジストリのopaque型 */

/**
 * @brief UI Geometry Configuration Fileから矩形を生成し、
 *        CPU / GPU Resourceとして登録する。
 *
 * @par Public API Contract
 * - 指定された`.ui_geom`ファイルを読み込み、
 *   UI Geometryの幅と高さを取得する。
 * - ファイルの読み込み、解析、semantic validationは
 *   ui_geom_config_loader_load()へ委譲する。
 * - 取得したicon_widthとicon_heightから
 *   2枚の三角形で構成される矩形の頂点配列を生成する。
 * - 生成する頂点数は6固定とする。
 * - 頂点位置は(0, 0)から
 *   (icon_width, icon_height)までの矩形を表す。
 * - Texture UV座標には0.0から1.0の範囲を使用する。
 * - 生成した頂点配列から独立したCPU Geometryを生成する。
 * - CPU Geometryの生成はui_mesh_geometry_create_from_vertices()
 *   へ委譲する。
 * - 頂点データをVBO Managerへ書き込み、
 *   対応するVBO Allocationを確保する。
 * - 書込サイズの計算にはoverflow検査を適用する。
 * - Allocation Offsetがsizeof(ui_vertex_t)の
 *   倍数であることを確認する。
 * - Allocation OffsetとVertex StrideからDraw Rangeを構築する。
 * - CPU GeometryとVBO Allocationを指定Resource Nameで
 *   Geometry Registryへ新規登録する。
 * - 同一Resource Nameの重複登録は許可しない。
 * - 成功時には対応するGeometry IDを出力する。
 *
 * @par Ownership
 * - 入力ファイルおよびResource Full Pathのownershipは
 *   callerが保持する。
 * - Configurationと一時頂点配列は本Operation内で管理する。
 * - 本Operationが生成したCPU GeometryとVBO Allocationは、
 *   Registryへの登録成功時にRegistryの管理下へ移転する。
 * - 登録成功時、callerは対象Geometryの最初の利用権を取得する。
 * - callerは取得した利用権をrelease()で返却する責務を持つ。
 * - Texture Resourceの所有権は取得しない。
 *
 * @par Failure Contract
 * - Loaderによるファイルの読み込み・解析に失敗した場合、
 *   下位LoaderのResultをPipelineのResult Codeへ変換して返す。
 * - Registryへの登録前の回復可能な失敗では、
 *   本Operationが生成したCPU Geometryと
 *   確保済みVBO Allocationを解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDを返す。
 * - DATA_CORRUPTED発生時は通常のcleanupを実行せず、
 *   Resourceの解放・Ownership状態の復元を保証しない。
 * - 失敗時にはout_geometry_id_の値を変更しない。
 *
 * @param[in,out] vbo_manager_
 *                頂点データの転送とVBO Allocationの
 *                管理に使用するVBO Manager。
 * @param[in,out] geometry_registry_
 *                登録先のUI Mesh Geometry Registry。
 * @param[in] resource_name_
 *            登録するResource Name。
 * @param[in] resource_fullpath_
 *            読み込むUI Geometry Configuration FileのFull Path。
 * @param[out] out_geometry_id_
 *             登録したGeometry IDの出力先。
 *
 * @retval RESOURCE_PIPELINE_SUCCESS
 *         UI Mesh Geometryの登録成功。
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT
 *         引数、Resource Name、File Path、
 *         または下位Resourceの入力条件が不正。
 * @retval RESOURCE_PIPELINE_BAD_OPERATION
 *         Resource Nameが登録済み、
 *         VBO書込条件が成立しない、または下位Operationが不成立。
 * @retval RESOURCE_PIPELINE_FILE_OPEN_ERROR
 *         Configuration Fileを開けない。
 * @retval RESOURCE_PIPELINE_FILE_READ_ERROR
 *         Configuration Fileの読み込みに失敗。
 * @retval RESOURCE_PIPELINE_UNSUPPORTED_FILE
 *         Configuration Fileがsupported formatまたは
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
 * @pre VBO ManagerはUI VertexのStrideと整合する
 *      Allocation Offsetを生成できるように構成されていること。
 * @pre out_geometry_id_は書き込み可能な出力先を指していること。
 *
 * @post 成功時、Configuration Fileで指定された寸法の
 *       UI Mesh GeometryがRegistryへ登録される。
 * @post 成功時、Reference Countは1となり、
 *       out_geometry_id_に登録IDが格納される。
 * @post 入力ファイルとResource Full Pathのownershipは移転しない。
 */
resource_pipeline_result_t ui_mesh_geometry_pipeline_import_from_file(vbo_manager_t* vbo_manager_, ui_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

/**
 * @brief UI Mesh Geometryの利用権を1つ返却する。
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
 *                利用権を返却するUI Mesh Geometry Registry。
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
resource_pipeline_result_t ui_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, ui_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
