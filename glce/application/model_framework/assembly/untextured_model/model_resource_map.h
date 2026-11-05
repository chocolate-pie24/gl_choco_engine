// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file model_resource_map.h
 * @author chocolate-pie24
 * @brief Untextured Modelの共有Resourceと初期Transform Treeを管理するResource Mapを提供する
 *
 * @details
 * Untextured Model Resource Mapは、Model Assembly Configに基づいて
 * Renderer FrontendへMesh ResourceをImportし、
 * Modelを構成するMeshとCoordinate IDの対応関係を保持する。
 *
 * また、Model個体が独立したTransform Treeを生成できるよう、
 * 初期Transform Treeを保持し、そのClone機能を提供する。
 *
 * @section untextured_model_resource_map_boundary_contract Module Boundary Contract
 *
 * Module Scope:
 * - Untextured MeshおよびDebug表示用Meshの構築と管理を担当する。
 * - 各MeshとCoordinate IDの対応関係を保持する。
 * - Model個体生成用の初期Transform Treeを保持する。
 * - Model個体の姿勢、配置および描画処理は担当しない。
 *
 * Resource Representation:
 * - 通常のUntextured Meshを保持する。
 * - 各Untextured Meshに対応するDebug AABB Line Meshを保持する。
 * - 各Coordinateに対応するX / Y / ZのDebug Axis Meshを保持する。
 * - Mesh BindingはMeshとCoordinate IDの組み合わせで表現する。
 * - Mesh Bindingの取得では値をコピーし、ResourceのOwnershipは移転しない。
 *
 * Transform Tree:
 * - Assembly Configから初期Transform Treeを構築する。
 * - 初期Transform TreeはResource Mapが所有する。
 * - Cloneによって生成されたTransform TreeはCallerが所有する。
 * - CloneされたTransform Treeの変更は、初期Transform Treeへ影響しない。
 *
 * Ownership / Lifetime:
 * - Resource Map自身のStorageはGeneral Allocatorで確保する。
 * - Resource Mapは自身が保持するMesh Binding群と初期Transform Treeを管理する。
 * - Renderer ResourceのImport / ReleaseはRenderer Frontendを通して行う。
 * - Renderer FrontendのOwnershipは取得しない。
 * - Createに渡したAssembly ConfigのOwnershipは取得しない。
 * - Resource MapのLifetimeはCreate成功時に開始し、Destroy成功時に終了する。
 * - Resource Mapは、それを利用するModel個体より後にDestroyする。
 * - Resource MapのDestroyには、Resource構築時と同じRenderer Frontendを使用する。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

#ifndef GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_RESOURCE_MAP_H
#define GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_RESOURCE_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "application/model_framework/core/model_framework_types.h"

/**
 * @brief Untextured Modelの共有Mesh Resourceと初期Transform Treeを管理するOpaque Object
 *
 * @details
 * Model Assembly Configから構築したMesh Binding群と初期Transform Treeを保持する。
 * 複数のModel個体で共有することを想定しており、
 * 各個体のTransform TreeはCloneによって独立して生成する。
 *
 * Resource Map自身と保持するResourceのLifetimeは本moduleが管理する。
 */
typedef struct untextured_model_resource_map untextured_model_resource_map_t;

typedef struct untextured_model_assembly_config untextured_model_assembly_config_t;
typedef struct line_mesh line_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;
typedef struct model_transform_tree model_transform_tree_t;
typedef struct cone cone_t;
typedef struct cylinder cylinder_t;
typedef struct renderer_frontend renderer_frontend_t;

/**
 * @brief Model Assembly ConfigからUntextured Model Resource Mapを生成する
 *
 * @details
 * Assembly Configに基づき、以下のResourceを構築する。
 *
 * - 通常のUntextured Mesh
 * - 各Untextured Meshに対応するDebug AABB Line Mesh
 * - 各Coordinateに対応するX / Y / ZのDebug Axis Mesh
 * - Model個体生成用の初期Transform Tree
 *
 * Mesh ResourceはRenderer FrontendへImportする。
 * Resource Mapは、各MeshとCoordinate IDの対応関係を保持する。
 *
 * @par Public API Contract
 *
 * Input Contract:
 * - resource_map_name_は空でないNUL終端文字列である。
 * - config_は有効なModel Assembly Configである。
 * - asset_root_path_はAssetの基準Directoryを表すNUL終端文字列である。
 * - debug_axis_cone_とdebug_axis_cylinder_は有効なGeometry Primitiveである。
 * - debug_axis_segment_count_は3以上である。
 * - 現在の実装ではDebug AABBおよびDebug Axisの生成は必須である。
 *
 * Ownership / Lifetime:
 * - Renderer FrontendはBorrowし、そのOwnershipを取得しない。
 * - Assembly Config、Cone、Cylinderおよび入力文字列のOwnershipは取得しない。
 * - Resource Mapは、ImportしたMesh ResourceのReleaseを管理する。
 * - Resource Map自身と初期Transform TreeのStorageを所有する。
 *
 * Output Contract:
 * - out_resource_map_はNULLであってはならない。
 * - *out_resource_map_はNULLでなければならない。
 * - 成功時は完全に構築されたResource MapのOwnershipをCallerへ渡す。
 * - 成功後、Assembly ConfigはCallerが独立して破棄できる。
 * - 失敗時は*out_resource_map_を変更しない。
 *
 * Failure Contract:
 * - 通常の構築失敗では、取得途中のResourceをRollbackする。
 * - 内部整合性の破損が確定した場合はDATA_CORRUPTEDとし、
 *   通常のRollbackを停止する。
 *
 * @param[in,out] renderer_frontend_ Mesh ResourceのImport先となるRenderer Frontend
 * @param[in] resource_map_name_ Resource Mapの名前
 * @param[in] config_ Modelを構築するためのAssembly Config
 * @param[in] asset_root_path_ Assetの基準Directory Path
 * @param[in] debug_aabb_color_ Debug AABBの描画色
 * @param[in] debug_axis_cone_ Debug Axisの先端を構成するCone
 * @param[in] debug_axis_cylinder_ Debug Axisの軸部分を構成するCylinder
 * @param[in] debug_axis_segment_count_ ConeおよびCylinderの分割数
 * @param[out] out_resource_map_ 生成したResource Mapを受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Resource Mapの生成に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 入力引数または構築設定が不正
 * @retval MODEL_FRAMEWORK_BAD_OPERATION Output先の状態が不正、または下位Operationの契約違反
 * @retval MODEL_FRAMEWORK_NO_MEMORY Resource構築中にMemory不足が発生
 * @retval MODEL_FRAMEWORK_OVERFLOW Size計算等でOverflowが発生
 * @retval MODEL_FRAMEWORK_LIMIT_EXCEEDED Resourceの容量上限を超過
 * @retval MODEL_FRAMEWORK_RUNTIME_ERROR ResourceのImportまたは構築処理が失敗
 * @retval MODEL_FRAMEWORK_DATA_CORRUPTED 内部整合性の異常、またはRollback失敗
 * @retval MODEL_FRAMEWORK_UNDEFINED_ERROR 下位Operationから未定義のErrorを受信
 *
 * @post 成功時、通常Mesh、Debug Meshおよび初期Transform Treeが利用可能となる。
 */
model_framework_result_t untextured_model_resource_map_create(
    renderer_frontend_t* renderer_frontend_,
    const char* resource_map_name_,
    const untextured_model_assembly_config_t* config_,
    const char* asset_root_path_,
    vec4u8_t debug_aabb_color_,
    const cone_t* debug_axis_cone_,
    const cylinder_t* debug_axis_cylinder_,
    uint16_t debug_axis_segment_count_,
    untextured_model_resource_map_t** out_resource_map_);

/**
 * @brief Untextured Model Resource Mapと所有Resourceを破棄する
 *
 * @details
 * Resource Mapが管理するDebug Axis Mesh、Debug AABB Mesh、
 * 通常Untextured MeshのResourceをRenderer Frontend経由でReleaseする。
 *
 * その後、初期Transform Tree、Resource Map Nameおよび
 * Resource Map自身のStorageを解放する。
 *
 * @par Public API Contract
 *
 * Ownership / Lifetime:
 * - Resource Map自身のLifetimeと所有ResourceのLifetimeを終了する。
 * - Resource Mapを参照するModel個体は、事前に破棄されている必要がある。
 * - renderer_frontend_にはResource構築時と同じRenderer Frontendを指定する。
 * - Renderer Frontend自身は破棄しない。
 *
 * Output Contract:
 * - 成功時、*resource_map_はNULLとなる。
 * - Resource Mapから取得した情報は、破棄後に利用できない。
 *
 * Failure Contract:
 * - ResourceのReleaseに失敗した場合、破棄処理を中止する。
 * - 破棄途中の失敗では、一部のResourceがRelease済みとなる可能性がある。
 * - この場合、Resource Mapが引き続き有効であることは保証しない。
 * - 内部整合性の破損が確定した場合は、通常の解放処理を継続しない。
 *
 * @param[in,out] renderer_frontend_ ResourceのReleaseに使用するRenderer Frontend
 * @param[in,out] resource_map_ 破棄するResource Mapを保持するPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Resource Mapの破棄に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL
 * @retval MODEL_FRAMEWORK_DATA_CORRUPTED Resource Mapの内部整合性異常、またはResourceのRelease失敗
 *
 * @post 成功時、*resource_map_ == NULLとなる。
 */
model_framework_result_t untextured_model_resource_map_destroy(renderer_frontend_t* renderer_frontend_, untextured_model_resource_map_t** resource_map_);

/**
 * @brief Resource Mapが保持する初期Transform Treeを複製する
 *
 * @details
 * 初期Transform TreeのCoordinate HierarchyおよびLocal Transformを
 * 複製し、独立したModel Transform Treeを生成する。
 *
 * @par Public API Contract
 *
 * Ownership / Lifetime:
 * - Resource Mapが所有する初期Transform Treeは変更しない。
 * - CloneされたTreeのOwnershipはCallerへ移転する。
 * - Callerは生成したTreeをmodel_transform_tree_destroy()で破棄する。
 * - CloneされたTreeはResource Mapとは独立して変更できる。
 *
 * Output Contract:
 * - *out_transform_tree_はNULLでなければならない。
 * - 成功時は完全に構築されたTreeを返す。
 * - 失敗時は*out_transform_tree_を変更しない。
 *
 * @param[in] resource_map_ Clone元の初期Transform Treeを保持するResource Map
 * @param[out] out_transform_tree_ 生成したTransform Treeを受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Transform TreeのCloneに成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 入力PointerまたはClone元Treeが不正
 * @retval MODEL_FRAMEWORK_BAD_OPERATION Output先の状態が不正
 * @retval MODEL_FRAMEWORK_DATA_CORRUPTED 構築したTreeの内部整合性に異常を検出
 *
 * @return その他、Tree構築処理から伝播するModel Framework Result Code
 *
 * @post 成功時、複製先Treeは複製元と同じCoordinate HierarchyおよびLocal Transformを持つ。
 */
model_framework_result_t untextured_model_resource_map_transform_tree_clone(const untextured_model_resource_map_t* resource_map_, model_transform_tree_t** out_transform_tree_);

// ============================================================
// for Untextured Mesh.
// ============================================================
/**
 * @brief 指定されたUntextured MeshのBinding情報を取得する
 *
 * @details
 * 指定Indexに対応するUntextured MeshとCoordinate IDを取得する。
 *
 * Meshは値としてコピーし、GeometryおよびMaterial Resourceの
 * OwnershipはCallerへ移転しない。
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[in] mesh_index_ 取得するUntextured MeshのIndex
 * @param[out] out_mesh_ Mesh情報を受け取るPointer
 * @param[out] out_coordinate_id_ Meshに対応するCoordinate IDを受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Bindingの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL、またはMesh Indexが範囲外
 *
 * @post 成功時、指定IndexのMeshとCoordinate IDが出力される。
 */
model_framework_result_t untextured_model_resource_map_untextured_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_, uint16_t* out_coordinate_id_);

/**
 * @brief Resource Mapが保持するUntextured Meshの数を取得する
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[out] out_count_ Untextured Meshの数を受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Countの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL
 *
 * @post 成功時、*out_count_にUntextured Meshの数が格納される。
 */
model_framework_result_t untextured_model_resource_map_untextured_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_);

// ============================================================
// for Debug AABB Line Mesh.
// ============================================================
/**
 * @brief 指定されたDebug AABB MeshのBinding情報を取得する
 *
 * @details
 * 指定Indexに対応するDebug AABB Line MeshとCoordinate IDを取得する。
 *
 * Debug AABBは通常のUntextured Meshごとに生成され、
 * 対応するUntextured Meshと同じCoordinate IDを持つ。
 *
 * Meshは値としてコピーし、Geometry ResourceのOwnershipは
 * Callerへ移転しない。
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[in] mesh_index_ 取得するDebug AABB MeshのIndex
 * @param[out] out_mesh_ Line Mesh情報を受け取るPointer
 * @param[out] out_coordinate_id_ Meshに対応するCoordinate IDを受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Bindingの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL、またはMesh Indexが範囲外
 *
 * @post 成功時、指定IndexのDebug AABB MeshとCoordinate IDが出力される。
 */
model_framework_result_t untextured_model_resource_map_debug_aabb_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, line_mesh_t* out_mesh_, uint16_t* out_coordinate_id_);

/**
 * @brief Resource Mapが保持するDebug AABB Meshの数を取得する
 *
 * @details
 * 現在の実装では、Debug AABB Meshは通常のUntextured Meshと
 * 1対1で生成されるため、両者のMesh Countは等しい。
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[out] out_count_ Debug AABB Meshの数を受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Countの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL
 *
 * @post 成功時、*out_count_にDebug AABB Meshの数が格納される。
 */
model_framework_result_t untextured_model_resource_map_debug_aabb_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_);

// ============================================================
// for Debug AXIS Untextured Mesh.
// ============================================================
/**
 * @brief 指定されたDebug Axis MeshのBinding情報を取得する
 *
 * @details
 * 指定Indexに対応するDebug Axis Untextured MeshとCoordinate IDを取得する。
 *
 * 各CoordinateにはX / Y / Zの3つのAxis Meshが対応する。
 * 同じ軸のMeshはGeometryおよびMaterial Resourceを共有する。
 *
 * Meshは値としてコピーし、GeometryおよびMaterial Resourceの
 * OwnershipはCallerへ移転しない。
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[in] mesh_index_ 取得するDebug Axis MeshのIndex
 * @param[out] out_mesh_ Mesh情報を受け取るPointer
 * @param[out] out_coordinate_id_ Meshに対応するCoordinate IDを受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Bindingの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL、またはMesh Indexが範囲外
 *
 * @post 成功時、指定IndexのDebug Axis MeshとCoordinate IDが出力される。
 */
model_framework_result_t untextured_model_resource_map_debug_axis_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_, uint16_t* out_coordinate_id_);

/**
 * @brief Resource Mapが保持するDebug Axis Meshの数を取得する
 *
 * @details
 * 各CoordinateにX / Y / Zの3つのAxis Meshを生成するため、
 * Debug Axis Mesh CountはAssembly ConfigのCoordinate Countの3倍となる。
 *
 * @param[in] resource_map_ 取得対象のResource Map
 * @param[out] out_count_ Debug Axis Meshの数を受け取るPointer
 *
 * @retval MODEL_FRAMEWORK_SUCCESS Mesh Countの取得に成功
 * @retval MODEL_FRAMEWORK_INVALID_ARGUMENT 必須PointerがNULL
 *
 * @post 成功時、*out_count_にDebug Axis Meshの数が格納される。
 */
model_framework_result_t untextured_model_resource_map_debug_axis_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_);

// ============================================================
// Validator.
// ============================================================
/**
 * @brief Untextured Model Resource MapのCanonical Validityを検証する
 *
 * @details
 * Resource Map自身が保証するStructural / Semantic Invariantを検証する。
 *
 * 主な検証対象は以下のとおり。
 *
 * - Resource Map NameのValidity
 * - 初期Transform TreeのValidity
 * - 各Mesh群のCountとPointerの整合性
 * - Mesh配列およびCoordinate ID配列のAllocation Validity
 * - Debug AABB Mesh Countと通常Mesh Countの対応関係
 * - Debug Axis Mesh Countの構造的整合性
 *
 * Renderer Frontendが管理するGeometry / Material Resourceの
 * 登録状態や内部整合性までは検証しない。
 *
 * @param[in] resource_map_ 検証対象のResource Map
 *
 * @retval true Resource MapのCanonical Invariantが成立している
 * @retval false Resource MapがNULL、またはCanonical Invariantが成立していない
 */
bool untextured_model_resource_map_is_valid(const untextured_model_resource_map_t* resource_map_);

#ifdef __cplusplus
}
#endif
#endif
