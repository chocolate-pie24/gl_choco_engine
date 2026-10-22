// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file untextured_mesh_material_config_loader.h
 * @author chocolate-pie24
 * @brief Untextured Mesh Material configuration fileを読み込み、Material設定値を生成する
 *
 * @details
 * Untextured Mesh Material Config Loader moduleは、外部configuration fileを読み込み、
 * configuration textを解析してuntextured_mesh_material_config_tへ変換する。
 *
 * 本moduleはUntextured Mesh Material Configuration Formatを所有し、
 * supported key、value representationおよびMaterial固有のsemanticを定義する。
 *
 * @section untextured_mesh_material_config_loader_boundary_contract Module Boundary Contract
 *
 * Trust Boundary:
 * - 本moduleはExternal Trust Boundaryを所有する。
 * - configuration fileの内容はexternal / untrusted representationとして扱う。
 * - file contentのformatおよびMaterial configurationのsemantic validityを検証し、
 *   untextured_mesh_material_config_loader_load()が成功した場合にのみ、
 *   GLCE内部のtrusted representationとして受理する。
 * - LOADER_SUCCESS以外の場合、external representationはcaller-visible outputへcommitされない。
 *
 * Ownership:
 * - 本moduleはpersistent module stateを持たない。
 * - file読み込みに使用するtemporary resourceは本moduleが管理する。
 * - callerが所有するpathおよびoutput storageのownershipを取得しない。
 * - outputのuntextured_mesh_material_config_tは、owned resourceを持たないvalue typeである。
 *
 * @section untextured_mesh_material_config_format Untextured Mesh Material Configuration Format
 *
 * Line Format:
 * - fileはlogical line単位で解析する。
 * - blank lineは無視する。
 * - comment lineは、任意のleading space / horizontal tabの後に
 *   '#'を置くことで記述する。
 * - blank / comment以外のlineはkey/value lineでなければならない。
 * - key/value lineは`key = value`形式で記述する。
 * - key/value lineには'='がちょうど1つ存在しなければならない。
 * - key、valueおよび'='の周囲にはspace / horizontal tabを記述できる。
 * - key token内部にspace / horizontal tabを含めることはできない。
 * - value token内部のspaceは許可するが、horizontal tabは許可しない。
 * - key/value tokenは、それぞれ1文字以上31文字以下とする。
 * - inline commentはサポートしない。
 *
 * Supported Keys:
 * - supported keyはcase-sensitiveである。
 * - `diffuse`はRequiredであり、file内にちょうど1回指定する。
 * - `ambient`、`specular`、`shininess`はOptionalであり、file内にそれぞれ0回または1回指定できる。
 * - keyの記述順序は問わない。
 * - duplicate keyおよびunsupported keyを含むfileは受理しない。
 *
 * Value Representation:
 * - `ambient`、`diffuse`、`specular`は3成分のfloat vectorとする。
 * - vectorは`[r, g, b]`形式で記述し、各成分をcommaで区切る。
 * - vector内部では、数値、comma、角括弧の間にspaceを記述できる。
 * - commaおよび開始・終了の角括弧は省略できない。
 * - `shininess`は単一のfloat値として記述する。
 * - 数値文字列の構文は、Cのsscanf()による`%f`変換に従う。
 * - vectorの閉じ括弧またはscalar値の後に余分な文字を記述できない。
 *
 * Numeric Range:
 * - `ambient`、`diffuse`、`specular`の各成分はfiniteであり、
 *   0.0f以上1.0f以下でなければならない。
 * - `shininess`はfiniteかつ0.0f以上でなければならない。
 * - `shininess`に、floatの表現範囲とは別の固定上限は設けない。
 * - NaNおよびInfinityはvalidなMaterial値として受理しない。
 *
 * Default Values:
 * - `diffuse`はRequiredであり、デフォルト値を持たない。
 * - `ambient`省略時: `diffuse * 0.1f`（RGB各成分に適用）。
 * - `specular`省略時: `(0.0f, 0.0f, 0.0f)`。
 * - `shininess`省略時: `32.0f`。
 * - デフォルト値はkeyが省略された場合にのみ適用する。
 *   明示的なゼロ値はvalidな指定として扱い、デフォルト値で上書きしない。
 * - `specular`と`shininess`は独立して指定・省略できる。
 * - デフォルト値の補完は本Loaderが担い、成功時には4 fieldすべてが確定する。
 *   untextured_mesh_material_config_t自体に未設定状態を導入しない。
 *
 * Default Value Rationale:
 * - AmbientをDiffuseの10%とするのは、基本色との関係を維持した
 *   Ambient反射係数を生成し、暗部の視認性を補助するためである。
 *   固定RGB値を採用すると、Materialの基本色と無関係な
 *   Ambient反射係数になり得る。
 *   0.1fは省略時のMaterial値を導出するための係数であり、
 *   Ambient Lightの色・強度やShader内のambient strengthを
 *   定義するものではない。
 *
 * - Specularをゼロとするのは、Untextured meshの主な用途であるSTL表示では
 *   Geometryの形状や面の向きの把握を優先し、
 *   光沢のないマットな表示をデフォルトとするためである。
 *   光沢が必要な場合はSpecularを明示的に指定できる。
 *
 * - Shininessを32.0fとするのは、Specularだけを指定した場合にも、
 *   Shininessの追加指定なしで光沢表現を利用できるようにするためである。
 *   32.0fは表示用途に合わせた既定の光沢指数であり、
 *   物理的に導出された値ではない。
 *   Specularがゼロの場合、Shininessは描画結果へ寄与しない。
 *
 * - これらはConfiguration Fileの省略時動作であり、
 *   callerが直接構築するMaterial valueに
 *   暗黙のデフォルト適用を要求するものではない。
 *
 * Temporary Numeric Input Contract:
 * - 現行実装は`sscanf()`の`%f`変換を使用する。
 * - callerは、変換結果がfloatで表現不能となる数値文字列を
 *   configuration fileへ含めてはならない。
 * - この条件は変換前にautomatic validationされない。
 *   違反した場合の動作は保証されず、未定義動作が生じ得る。
 * - この制約は、将来のSafe Floating-Point Text Parsing導入までの暫定条件である。
 *
 * Example (Diffuse only / defaults applied):
 * @code
 * # Matte untextured mesh material
 * diffuse = [0.8, 0.5, 0.3]
 * @endcode
 *
 * この場合、以下のデフォルト値が適用される。
 * - ambient = [0.08, 0.05, 0.03]
 * - specular = [0.0, 0.0, 0.0]
 * - shininess = 32.0
 *
 * Example (Explicit optional values):
 * @code
 * diffuse = [0.8, 0.5, 0.3]
 * ambient = [0.0, 0.0, 0.0]
 * specular = [1.0, 1.0, 1.0]
 * shininess = 64.0
 * @endcode
 *
 * File Validity:
 * - 上記syntax、supported keyおよびrequired fieldの条件を満たし、
 *   validなuntextured_mesh_material_config_tを構築できるfileのみを受理する。
 *
 * @par AI支援:
 * - 本DocumentationはChatGPTによる草案である。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_LOADERS_UNTEXTURED_MESH_MATERIAL_CONFIG_LOADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_LOADERS_UNTEXTURED_MESH_MATERIAL_CONFIG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"

/**
 * @brief Untextured Mesh Material configurationを保持するvalue type
 *
 * @details
 * ambient、diffuse、specularの反射係数と、shininessを保持する。
 *
 * Valid Representation:
 * - ambient、diffuse、specularのすべての成分はfiniteかつ[0.0f, 1.0f]内である。
 * - shininessはfiniteかつ0.0f以上である。
 *
 * `untextured_mesh_material_config_is_valid()`が本typeのcanonical validityを定義する。
 */
typedef struct untextured_mesh_material_config {
    vec3f_t ambient;    /**< Ambient反射係数 */
    vec3f_t diffuse;    /**< Diffuse反射係数 */
    vec3f_t specular;   /**< Specular反射係数 */
    float shininess;    /**< 鏡面反射の光沢指数 */
} untextured_mesh_material_config_t;

/**
 * @brief Configuration fileを読み込み、Untextured Mesh Material設定値を生成する
 *
 * @details
 * config_fullpath_で指定されたfileを、Untextured Mesh Material Configuration Formatに
 * 従って解析し、validなuntextured_mesh_material_config_tを生成する。
 *
 * @par Public API Contract
 *
 * Output Contract:
 * - LOADER_SUCCESSの場合、out_config_にcanonical validityを満たす値を格納する。
 * - LOADER_SUCCESS以外の場合、out_config_の既存内容は変更しない。
 *
 * Lifetime / Ownership:
 * - config_fullpath_は呼び出し中だけ参照し、保持しない。
 * - out_config_はcaller-owned storageであり、ownershipを取得しない。
 * - temporary resourceのlifetimeは本API内で管理する。
 *
 * @param[in] config_fullpath_
 *            読み込むconfiguration fileのNUL terminated path。
 *            NULLまたはempty stringは許可しない。
 * @param[out] out_config_
 *             生成したconfigurationの格納先。NULLは許可しない。
 *
 * @retval LOADER_SUCCESS
 *         configurationの読み込み、解析およびvalidationに成功した。
 * @retval LOADER_INVALID_ARGUMENT
 *         public API argumentがcontractを満たしていない。
 * @retval LOADER_UNSUPPORTED_FILE
 *         file contentがConfiguration FormatまたはMaterialのsemantic条件を満たしていない。
 * @return 上記以外では、file access、temporary resource operation等に対応する
 *         loader layer result codeを返す。
 *
 * @pre config_fullpath_は、API実行中に読み取り可能なNUL terminated stringを
 *      参照していなければならない。
 * @pre out_config_は、API実行中に書き込み可能な
 *      untextured_mesh_material_config_t storageを参照していなければならない。
 *
 * @post LOADER_SUCCESSの場合、
 *       `untextured_mesh_material_config_is_valid(out_config_) == true`が成立する。
 * @post LOADER_SUCCESS以外の場合、out_config_の既存内容は変更されない。
 */
loader_result_t untextured_mesh_material_config_loader_load(const char* config_fullpath_, untextured_mesh_material_config_t* out_config_);

/**
 * @brief untextured_mesh_material_config_tのcanonical validityを判定する
 *
 * @details
 * Material configuration valueがGLCE内部で使用可能なsemantic representationかを判定する。
 *
 * Validity Definition:
 * - config_はNULLではない。
 * - ambient、diffuse、specularの各成分はfiniteかつ[0.0f, 1.0f]内である。
 * - shininessはfiniteかつ0.0f以上である。
 *
 * Validation Scope:
 * - configuration fileのsyntax、supported key、required field、duplicate key等の
 *   file-level validityは検査しない。
 * - storageのlifetimeやaddressabilityは検査しない。
 *
 * @param[in] config_ 判定対象のUntextured Mesh Material configuration
 *
 * @retval true  validなuntextured_mesh_material_config_tである。
 * @retval false invalidなuntextured_mesh_material_config_tである、またはconfig_がNULL。
 *
 * @pre config_が非NULLの場合、読み取り可能かつlifetime内の
 *      untextured_mesh_material_config_t storageを参照していなければならない。
 * @post config_の内容を変更しない。
 */
bool untextured_mesh_material_config_is_valid(const untextured_mesh_material_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
