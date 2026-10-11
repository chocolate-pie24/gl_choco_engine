// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file untextured_material_config_loader.h
 * @author chocolate-pie24
 * @brief Untextured Material configuration fileを読み込み、Material設定値を生成する
 *
 * @details
 * Untextured Material Config Loader moduleは、外部configuration fileを読み込み、
 * configuration textを解析してuntextured_material_config_tへ変換する。
 *
 * 本moduleはUntextured Material Configuration Formatを所有し、
 * supported key、value representationおよびMaterial固有のsemanticを定義する。
 *
 * @section untextured_material_config_loader_boundary_contract Module Boundary Contract
 *
 * Trust Boundary:
 * - 本moduleはExternal Trust Boundaryを所有する。
 * - configuration fileの内容はexternal / untrusted representationとして扱う。
 * - file contentのformatおよびMaterial configurationのsemantic validityを検証し、
 *   untextured_material_config_loader_load()が成功した場合にのみ、
 *   GLCE内部のtrusted representationとして受理する。
 * - RESOURCE_SUCCESS以外の場合、external representationはcaller-visible outputへ
 *   commitされない。
 *
 * Ownership:
 * - 本moduleはpersistent module stateを持たない。
 * - file読み込みに使用するtemporary resourceは本moduleが管理する。
 * - callerが所有するpathおよびoutput storageのownershipを取得しない。
 * - outputのuntextured_material_config_tは、owned resourceを持たないvalue typeである。
 *
 * @section untextured_material_config_format Untextured Material Configuration Format
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
 * - `ambient`
 * - `diffuse`
 * - `specular`
 * - `shininess`
 * - 上記4つのkeyはすべて必須である。
 * - 各keyはfile内にちょうど1回だけ記述しなければならない。
 * - keyの記述順序は問わない。
 * - unsupported keyを含むfileは受理しない。
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
 * Temporary Numeric Input Contract:
 * - 現行実装は`sscanf()`の`%f`変換を使用する。
 * - callerは、変換結果がfloatで表現不能となる数値文字列を
 *   configuration fileへ含めてはならない。
 * - この条件は変換前にautomatic validationされない。
 *   違反した場合の動作は保証されず、未定義動作が生じ得る。
 * - この制約は、将来のSafe Floating-Point Text Parsing導入までの暫定条件である。
 *
 * Example:
 * @code
 * # Untextured material
 * ambient = [0.1, 0.1, 0.1]
 * diffuse = [0.8, 0.5, 0.3]
 * specular = [1.0, 1.0, 1.0]
 * shininess = 32.0
 * @endcode
 *
 * File Validity:
 * - 上記syntax、supported keyおよびrequired fieldの条件を満たし、
 *   validなuntextured_material_config_tを構築できるfileのみを受理する。
 *
 * @par AI支援:
 * - 本DocumentationはChatGPTによる草案である。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_UNTEXTURED_MATERIAL_CONFIG_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_UNTEXTURED_MATERIAL_CONFIG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/resource/core/resource_types.h"

/**
 * @brief Untextured Material configurationを保持するvalue type
 *
 * @details
 * ambient、diffuse、specularの反射係数と、shininessを保持する。
 *
 * Valid Representation:
 * - ambient、diffuse、specularのすべての成分はfiniteかつ[0.0f, 1.0f]内である。
 * - shininessはfiniteかつ0.0f以上である。
 *
 * `untextured_material_config_is_valid()`が本typeのcanonical validityを定義する。
 */
typedef struct untextured_material_config {
    vec3f_t ambient;    /**< Ambient反射係数 */
    vec3f_t diffuse;    /**< Diffuse反射係数 */
    vec3f_t specular;   /**< Specular反射係数 */
    float shininess;    /**< 鏡面反射の光沢指数 */
} untextured_material_config_t;

/**
 * @brief Configuration fileを読み込み、Untextured Material設定値を生成する
 *
 * @details
 * config_fullpath_で指定されたfileを、Untextured Material Configuration Formatに
 * 従って解析し、validなuntextured_material_config_tを生成する。
 *
 * @par Public API Contract
 *
 * Output Contract:
 * - RESOURCE_SUCCESSの場合、out_config_にcanonical validityを満たす値を格納する。
 * - RESOURCE_SUCCESS以外の場合、out_config_の既存内容は変更しない。
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
 * @retval RESOURCE_SUCCESS
 *         configurationの読み込み、解析およびvalidationに成功した。
 * @retval RESOURCE_INVALID_ARGUMENT
 *         public API argumentがcontractを満たしていない。
 * @retval RESOURCE_UNSUPPORTED_FILE
 *         file contentがConfiguration FormatまたはMaterialのsemantic条件を満たしていない。
 * @return 上記以外では、file access、temporary resource operation等に対応する
 *         Resource layer result codeを返す。
 *
 * @pre config_fullpath_は、API実行中に読み取り可能なNUL terminated stringを
 *      参照していなければならない。
 * @pre out_config_は、API実行中に書き込み可能な
 *      untextured_material_config_t storageを参照していなければならない。
 *
 * @post RESOURCE_SUCCESSの場合、
 *       `untextured_material_config_is_valid(out_config_) == true`が成立する。
 * @post RESOURCE_SUCCESS以外の場合、out_config_の既存内容は変更されない。
 */
resource_result_t untextured_material_config_loader_load(const char* config_fullpath_, untextured_material_config_t* out_config_);

/**
 * @brief untextured_material_config_tのcanonical validityを判定する
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
 * @param[in] config_ 判定対象のUntextured Material configuration
 *
 * @retval true  validなuntextured_material_config_tである。
 * @retval false invalidなuntextured_material_config_tである、またはconfig_がNULL。
 *
 * @pre config_が非NULLの場合、読み取り可能かつlifetime内の
 *      untextured_material_config_t storageを参照していなければならない。
 * @post config_の内容を変更しない。
 */
bool untextured_material_config_is_valid(const untextured_material_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
