// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file model_transform_tree.h
 * @author chocolate-pie24
 * @brief Model内の階層座標系とLocal Transformを管理するModel Transform Treeを提供する
 *
 * @details
 * Model Transform Tree moduleは、Modelを構成する座標系を階層構造として管理し、
 * 各座標系のTranslation / Rotationの設定、および階層関係を反映した
 * World Transform Matrixの取得機能を提供する。
 *
 * 座標系はModel内で一意なCoordinate IDによって識別する。
 * 座標系にはGeometry等のResourceを直接所有させず、
 * Transformの管理とResourceの関連付けを分離する。
 *
 * @section model_transform_tree_boundary_contract Module Boundary Contract
 *
 * Module Scope:
 * - 本moduleは、Model内のCoordinate HierarchyおよびTransformを所有する。
 * - Geometry、Material、GPU Resource等の管理や描画処理は担当しない。
 * - 各Coordinateは、Geometryを持たない階層構造上のAnchorとしても利用できる。
 *
 * Representation:
 * - `model_transform_tree_t`はopaque typeであり、内部構造をmodule外部へ公開しない。
 * - Treeは必ず1つのRoot Coordinateを持つ。
 * - Root以外のCoordinateは、必ず1つのParent Coordinateを持つ。
 * - 各Coordinateは0個以上のChild Coordinateを持つことができる。
 * - Coordinate IDは同一Tree内で一意であり、`uint16_t`の全範囲を使用できる。
 *
 * Transform:
 * - 各CoordinateはParent Coordinateを基準とするLocal TranslationとLocal Rotationを持つ。
 * - Root CoordinateのLocal Transformは、Modelの基準座標系に対するTransformを表す。
 * - Translationは3次元Vector、RotationはX / Y / Z各軸のEuler Angleで表す。
 * - Rotationの単位はDegreeとする。
 * - Translation / RotationはNaN / Infを含まないfiniteな値をContractとする。
 * - Scaleは本moduleのTransform表現に含めない。
 *
 * Ownership / Lifetime:
 * - Model Transform Tree moduleは、Tree自身とその全CoordinateのStorageを所有する。
 * - StorageのAllocation / ReleaseにはGeneral Allocatorを使用する。
 * - CallerはTreeおよびCoordinateのStorageを直接解放しない。
 * - TreeのLifetimeはcreate成功時に開始し、destroyによって終了する。
 * - Treeに登録されたCoordinateは、Treeが破棄されるまで保持される。
 * - APIへ渡すTree Pointerは、有効なLifetime中のobjectを参照するものとする。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_MODEL_TRANSFORM_TREE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_MODEL_TRANSFORM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

/**
 * @brief Model内の階層座標系とLocal Transformを所有するOpaque Object
 */
typedef struct model_transform_tree model_transform_tree_t;

/**
 * @brief Root Coordinateを持つModel Transform Treeを生成する
 *
 * @details
 * 指定されたCoordinate IDをRootとして、新しいTreeを生成する。
 * RootのTranslationおよびRotationはすべて0で初期化する。
 *
 * @par Public API Contract
 *
 * Output Contract:
 * - `*out_transform_tree_`はNULLでなければならない。
 * - 成功時は完全に構築されたTreeのOwnershipをCallerへ渡す。
 * - 失敗時は`*out_transform_tree_`を変更しない。
 *
 * Failure Classification:
 * - 引数と既存Output Stateの不正は、本APIのContractに従って分類する。
 * - Memory Allocationで発生したErrorはGeneral Allocatorからの
 *   Result CodeをRender ModelのResult Codeへ変換して伝播する。
 * - 構築したTreeの内部整合性に異常を検出した場合はDATA_CORRUPTEDとする。
 *
 * @param[in] coordinate_id_ Root CoordinateのID
 * @param[out] out_transform_tree_ 生成したTreeを受け取るPointer
 *
 * @retval RENDER_MODEL_SUCCESS Treeの生成に成功
 * @retval RENDER_MODEL_INVALID_ARGUMENT out_transform_tree_がNULL、またはGeneral Allocator由来のINVALID_ARGUMENT
 * @retval RENDER_MODEL_BAD_OPERATION *out_transform_tree_がNULLではない、またはGeneral Allocator由来のBAD_OPERATION
 * @retval RENDER_MODEL_DATA_CORRUPTED 構築したTreeの内部整合性異常、またはGeneral Allocator由来のDATA_CORRUPTED
 * @retval RENDER_MODEL_NO_MEMORY General AllocatorでMemory不足が発生
 * @retval RENDER_MODEL_LIMIT_EXCEEDED General AllocatorでAllocation Limitを超過
 * @retval RENDER_MODEL_OVERFLOW General AllocatorでOverflowが発生
 * @retval RENDER_MODEL_UNDEFINED_ERROR General Allocatorで未定義のErrorが発生
 *
 * @post 成功時、生成されたTreeには指定されたIDのRoot Coordinateが存在する。
 */
render_model_result_t model_transform_tree_create(uint16_t coordinate_id_, model_transform_tree_t** out_transform_tree_);

/**
 * @brief Model Transform Treeとその全Coordinateを破棄する
 *
 * @details
 * Treeが所有するすべてのCoordinateおよびTree自身のStorageを解放する。
 *
 * @par Public API Contract
 *
 * Lifetime / Ownership:
 * - TreeのOwnershipを終了し、そのBacking Storageを解放する。
 * - 正常なTreeを破棄した後、`*transform_tree_`はNULLとなる。
 * - `transform_tree_ == NULL`または`*transform_tree_ == NULL`の場合は何もしない。
 * - 内部整合性に異常を検出した場合は、破棄処理を中止する。
 *
 * @param[in,out] transform_tree_ 破棄するTreeを保持するPointer
 *
 * @post 正常に破棄した場合、*transform_tree_ == NULLとなる。
 */
void model_transform_tree_destroy(model_transform_tree_t** transform_tree_);

/**
 * @brief 指定されたParent Coordinateの下にChild Coordinateを追加する
 *
 * @details
 * 新しいCoordinateを生成し、指定されたParent CoordinateのChildとして登録する。
 * 新しいCoordinateには、Parentを基準とするLocal Translation / Rotationを設定する。
 *
 * @par Public API Contract
 *
 * Coordinate Identity:
 * - `parent_coordinate_id_`は既存のCoordinateを指定する。
 * - `coordinate_id_`はTree内で未使用のCoordinate IDでなければならない。
 * - Child Coordinateの追加によって、既存CoordinateのIDやLocal Transformは変更されない。
 *
 * Failure Contract:
 * - Parentが存在しない場合、またはCoordinate IDが重複する場合は追加しない。
 * - Memory Allocationに失敗した場合、既存Treeの構造を変更しない。
 *
 * Failure Classification:
 * - Tree PointerおよびParent Coordinateの不正はINVALID_ARGUMENTとする。
 * - Coordinate IDの重複はBAD_OPERATIONとする。
 * - Treeの内部整合性に異常を検出した場合はDATA_CORRUPTEDとする。
 * - Memory Allocationで発生したErrorはGeneral Allocatorからの
 *   Result CodeをRender ModelのResult Codeへ変換して伝播する。
 *
 * @param[in,out] transform_tree_ Childを追加するTree
 * @param[in] parent_coordinate_id_ Parent CoordinateのID
 * @param[in] coordinate_id_ 新しく追加するCoordinateのID
 * @param[in] translate_ Parentを基準とするLocal Translation
 * @param[in] rotation_ Parentを基準とするLocal Rotation（Degree）
 *
 * @retval RENDER_MODEL_SUCCESS Child Coordinateの追加に成功
 * @retval RENDER_MODEL_INVALID_ARGUMENT TreeがNULL、Parent Coordinateが存在しない、またはGeneral Allocator由来のINVALID_ARGUMENT
 * @retval RENDER_MODEL_BAD_OPERATION Coordinate IDが既に登録されている、またはGeneral Allocator由来のBAD_OPERATION
 * @retval RENDER_MODEL_DATA_CORRUPTED Treeの内部整合性異常、またはGeneral Allocator由来のDATA_CORRUPTED
 * @retval RENDER_MODEL_NO_MEMORY General AllocatorでMemory不足が発生
 * @retval RENDER_MODEL_LIMIT_EXCEEDED General AllocatorでAllocation Limitを超過
 * @retval RENDER_MODEL_OVERFLOW General AllocatorでOverflowが発生
 * @retval RENDER_MODEL_UNDEFINED_ERROR General Allocatorで未定義のErrorが発生
 *
 * @pre translate_およびrotation_はfiniteなVectorである。
 * @post 成功時、新しいCoordinateが指定されたParentのChildとして登録される。
 */
render_model_result_t model_transform_tree_add_child(model_transform_tree_t* transform_tree_, uint16_t parent_coordinate_id_, uint16_t coordinate_id_, vec3f_t translate_, vec3f_t rotation_);

/**
 * @brief 指定されたCoordinateのLocal Rotationを更新する
 *
 * @details
 * RotationはParent Coordinateを基準としたEuler Angleであり、
 * X / Y / Z各軸の角度をDegreeで指定する。
 *
 * @param[in,out] transform_tree_ 更新対象のTree
 * @param[in] coordinate_id_ 更新するCoordinateのID
 * @param[in] rotation_ 設定するLocal Rotation（Degree）
 *
 * @retval RENDER_MODEL_SUCCESS Rotationの更新に成功
 * @retval RENDER_MODEL_INVALID_ARGUMENT TreeがNULL、またはCoordinateが存在しない
 * @retval RENDER_MODEL_DATA_CORRUPTED Treeの内部整合性に異常を検出
 *
 * @pre rotation_はfiniteなVectorである。
 * @post 成功時、対象CoordinateのLocal Rotationがrotation_となる。
 */
render_model_result_t model_transform_tree_rotation_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t rotation_);

/**
 * @brief 指定されたCoordinateのLocal Translationを更新する
 *
 * @details
 * TranslationはParent Coordinateの座標系を基準として指定する。
 *
 * @param[in,out] transform_tree_ 更新対象のTree
 * @param[in] coordinate_id_ 更新するCoordinateのID
 * @param[in] translate_ 設定するLocal Translation
 *
 * @retval RENDER_MODEL_SUCCESS Translationの更新に成功
 * @retval RENDER_MODEL_INVALID_ARGUMENT TreeがNULL、またはCoordinateが存在しない
 * @retval RENDER_MODEL_DATA_CORRUPTED Treeの内部整合性に異常を検出
 *
 * @pre translate_はfiniteなVectorである。
 * @post 成功時、対象CoordinateのLocal Translationがtranslate_となる。
 */
render_model_result_t model_transform_tree_translate_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t translate_);

/**
 * @brief 指定されたCoordinateのWorld Transform Matrixを取得する
 *
 * @details
 * 対象CoordinateからRoot CoordinateまでのLocal Transformを合成し、
 * Modelの基準座標系に対するWorld Transform Matrixを求める。
 *
 * @par Public API Contract
 *
 * Transform Composition:
 * - Local TransformはTranslationとRotationの積 T * R とする。
 * - Rotation Matrixは X / Y / Z軸のRotationを Rx * Ry * Rz の順序で合成する。
 * - 階層TransformはRootから対象Coordinateまで、Parent * Childの順序で合成する。
 * - Root自身のLocal Transformも計算結果へ反映する。
 * - 取得結果は各Nodeに保持せず、API呼び出し時に計算する。
 *
 * Output Contract:
 * - 成功時は計算した4x4 Matrixを*out_matrix_へ格納する。
 * - 失敗時は*out_matrix_を変更しない。
 *
 * @param[in] transform_tree_ 参照対象のTree
 * @param[in] coordinate_id_ World Transformを取得するCoordinateのID
 * @param[out] out_matrix_ 計算結果を格納するMatrix
 *
 * @retval RENDER_MODEL_SUCCESS World Transform Matrixの取得に成功
 * @retval RENDER_MODEL_INVALID_ARGUMENT Treeまたは出力PointerがNULL、またはCoordinateが存在しない
 * @retval RENDER_MODEL_DATA_CORRUPTED Treeの内部整合性に異常を検出
 *
 * @post 成功時、*out_matrix_には階層関係を反映したWorld Transform Matrixが格納される。
 */
render_model_result_t model_transform_tree_world_matrix_get(const model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, mat4x4f_t* out_matrix_);

/**
 * @brief Model Transform Treeの内部整合性をCanonical Validationする
 *
 * @details
 * TreeおよびCoordinateのMemory生存状態、階層参照関係、
 * Local Transformの有限性を検査する。
 *
 * @par Public Validator Contract
 *
 * Validation Scope:
 * - Tree自身とRootを含む各CoordinateがGeneral Allocator上のlive allocationであること。
 * - RootのParentおよびSiblingが存在しないこと。
 * - Child / ParentおよびSibling / Parentの参照関係が整合していること。
 * - 各CoordinateのTranslation / Rotationがfiniteであること。
 * - 有限回のTraversalで検査を完了できること。
 *
 * Validation Limitations:
 * - Coordinate IDの重複を網羅的には検出しない。
 * - 訪問済みNodeの集合を保持しないため、Nodeの重複参照を網羅的には検出しない。
 * - Allocationの生存確認は、Allocation SizeやObject Typeの正当性を保証しない。
 * - 不正なMemory Addressや破損した参照関係に対する完全なMemory Safetyを保証するものではない。
 *
 * @param[in] transform_tree_ 検証対象のTree
 *
 * @retval true 実装が対象とするCanonical Validationに成功
 * @retval false NULL、内部不整合、またはTraversal上限超過を検出
 *
 * @pre transform_tree_が非NULLの場合、有効なMemory上のObject Pointer、
 *      またはGeneral Allocatorへ照会可能なPointerであること。
 */
bool model_transform_tree_is_valid(const model_transform_tree_t* transform_tree_);

#ifdef __cplusplus
}
#endif
#endif
