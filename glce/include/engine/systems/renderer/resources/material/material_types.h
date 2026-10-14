// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

 /** @ingroup renderer
  *
  * @file material_types.h
  * @author chocolate-pie24
  * @brief Untextured / Textured Meshで利用するMaterialの値型を定義する。
  *
  * @details
  * Phong系の基本反射特性、Texture Map参照、およびその組み合わせを
  * 非opaqueなvalue typeとして表現する。
  * Materialの登録identity、Resource Name、Texture Resourceの所有権は保持しない。
  *
  * @section material_types_boundary_contract Module Boundary Contract
  *
  * @par Representation / Ownership
  * - `untextured_material_t`、`texture_map_t`、`textured_material_t`は、
  *   callerが値として生成・コピーできる非opaqueのデータ型である。
  * - Texture IDは別のRegistryが管理するResourceへの参照情報であり、
  *   値のコピーによってTextureの所有権は移動・複製されない。
  * - Texture IDが示すResourceの登録状態とlifetimeの整合は、
  *   Materialを使用するApplication側のcontractに委ねる。
  *
 * @par Validity
 * - `untextured_material_t`は、各反射係数が0.0以上1.0以下、
 *   `shininess`が0.0以上の有限値であることを要求する。
 * - `untextured_material_is_valid()`は、上記のsemantic validityを検証する。
 * - `texture_map_t`および`textured_material_t`のsemantic validity、
 *   Texture IDの未使用状態の表現については、最終的な検証規則が未実装である。
 * - `texture_map_is_valid()`および`textured_material_is_valid()`は
 *   暫定実装であり、Texture参照やMaterialの健全性を保証しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MATERIAL_MATERIAL_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MATERIAL_MATERIAL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

 /**
  * @brief Textureを使用しないPhong系Materialの基本反射特性。
  *
  * この値は`textured_material_t`の基本反射特性としても利用する。
  */
typedef struct untextured_material {
    vec3f_t ambient;        /**< 環境光の反射特性 */
    vec3f_t diffuse;        /**< 拡散反射特性 */
    vec3f_t specular;       /**< 鏡面反射特性 */
    float shininess;        /**< 鏡面反射の光沢指数 */
} untextured_material_t;

 /**
  * @brief Diffuse / Specular / Normal用Texture ResourceのIDを保持する。
  *
  * 各IDはTexture Resourceへの非所有参照であり、IDだけからResourceの
  * 存在やlifetimeが保証されるわけではない。
  * 未使用Mapの表現と、Mapの組み合わせのsemantic validityは今後定義する。
  */
typedef struct texture_map {
    uint16_t diffuse_texture_id;    /**< Diffuse MapのTexture ID */
    uint16_t specular_texture_id;   /**< Specular MapのTexture ID */
    uint16_t normal_texture_id;     /**< Normal MapのTexture ID */
} texture_map_t;

 /**
  * @brief Phong系の基本反射特性とTexture Map参照を組み合わせたMaterial。
  *
  * 自身ではTexture Resourceの所有権やlifetimeを管理しない。
  */
typedef struct textured_material {
    untextured_material_t untextured_material;  /**< 基本反射特性 */
    texture_map_t texture_map;                  /**< 使用するTexture Mapの参照情報 */
} textured_material_t;

/**
 * @brief Untextured Materialのcanonical validityを判定する。
 *
 * @par Validation Scope
 * 以下の条件を検証する。
 * - `material_`がNULLではない。
 * - `ambient`、`diffuse`、`specular`の全要素が有限値である。
 * - 各反射係数が0.0以上1.0以下である。
 * - `shininess`が有限値かつ0.0以上である。
 *
 * @param[in] material_ 検証対象のUntextured Material。
 *
 * @return true 全てのcanonical invariantを満たす。
 * @return false いずれかのcanonical invariantを満たさない。
 */
bool untextured_material_is_valid(const untextured_material_t* material_);

 /**
  * @brief Texture Mapのvalidityを判定するための暫定API。
  *
  * @par Validation Scope
  * 現在の実装は引数を参照・検査せず、常にtrueを返す。
  * IDの予約値、登録状態、Texture Resourceのlifetimeは検査しない。
  *
  * @param[in] material_ 検査対象のTexture Map（現時点では参照されない）。
  * @return 現在の実装では常にtrue。
  */
bool texture_map_is_valid(const texture_map_t* material_);

 /**
  * @brief Textured Materialのvalidityを判定するための暫定API。
  *
  * @par Validation Scope
  * 現在の実装は引数を参照・検査せず、常にtrueを返す。
  * 基本反射特性、各Texture ID、Mapの組み合わせの整合性は検査しない。
  *
  * @param[in] material_ 検査対象のTextured Material（現時点では参照されない）。
  * @return 現在の実装では常にtrue。
  */
bool textured_material_is_valid(const textured_material_t* material_);

#ifdef __cplusplus
}
#endif
#endif
