// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file ui_geom_config_loader.h
 * @author chocolate-pie24
 * @brief UI geometry configuration fileを読み込み、UI geometry設定値を生成する
 *
 * @details
 * UI Geometry Config Loader moduleは、UI geometry configuration fileを読み込み、
 * file内のconfiguration textを解析してui_geom_config_tへ変換する。
 *
 * 本moduleはUI geometry configuration formatを所有し、
 * supported keyおよび各configuration valueのsemantic interpretationを
 * 定義するLoaderとして機能する。
 *
 * @section ui_geom_config_loader_boundary_contract Module Boundary Contract
 *
 * Trust Boundary:
 * - UI Geometry Config Loader moduleはExternal Trust Boundaryを所有する。
 * - `.ui_geom` fileの内容はexternal / untrusted representationとして扱う。
 * - file contentは、UI Geometry Configuration Formatに従うsyntax、
 *   supported key、required field、duplicate key、value representationおよび
 *   ui_geom_config_tのsemantic validityを検証した後にのみ、
 *   GLCE内部のtrusted representationとして受理する。
 * - trust promotionはui_geom_config_loader_load()の成功時に完了する。
 * - RESOURCE_SUCCESS以外の場合、external representationはtrusted stateへ
 *   commitされない。
 *
 * @section ui_geom_config_format UI Geometry Configuration Format
 *
 * Line Format:
 * - fileはlogical line単位で解析する。
 * - blank lineは無視する。
 * - comment lineは、任意のleading space / horizontal tabの後に
 *   '#'を置くことで記述する。
 * - blank / comment以外のlineはkey/value lineでなければならない。
 * - key/value lineは`key = value`形式で記述する。
 * - key/value lineには'='がちょうど1つ存在しなければならない。
 * - key、value、および'='の周囲にはspace / horizontal tabを記述できる。
 * - keyまたはvalue token内部にspace / horizontal tabを含めることはできない。
 * - inline commentはサポートしない。
 *
 * Supported Keys:
 * - supported keyはcase-sensitiveである。
 * - `icon_width`
 * - `icon_height`
 * - unsupported keyを含むfileは受理しない。
 * - `icon_width`と`icon_height`はそれぞれ必須である。
 * - 各keyはfile内にちょうど1回だけ記述しなければならない。
 * - keyの記述順序は問わない。
 *
 * Value Representation:
 * - `icon_width`および`icon_height`はdecimal integer textとして記述する。
 * - valueはASCII digit ('0'..'9')のみで構成する。
 * - '+'および'-' signはサポートしない。
 * - value tokenは1文字以上10文字以下とする。
 * - numeric valueは1以上INT32_MAX以下でなければならない。
 * - leading zeroは現在のformatでは許可する。
 *
 * File Validity:
 * - 上記syntaxおよびsupported keyの条件をすべて満たし、
 *   `icon_width`と`icon_height`の両方からvalidなui_geom_config_tを
 *   構築できるfileのみをsupported `.ui_geom` fileとして扱う。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_UI_GEOM_CONFIG_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_UI_GEOM_CONFIG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/resource/core/resource_types.h"

/**
 * @brief UI geometry configurationを保持するvalue type
 *
 * @details
 * `.ui_geom` fileから読み込まれたUI geometryのconfiguration valueを保持する。
 *
 * Valid Representation:
 * - icon_width > 0
 * - icon_height > 0
 *
 * `ui_geom_config_is_valid()`が本typeのcanonical validityを定義する。
 */
typedef struct ui_geom_config {
    int32_t icon_width;    /**< UI geometryの幅 */
    int32_t icon_height;   /**< UI geometryの高さ */
} ui_geom_config_t;

/**
 * @brief `.ui_geom` fileを読み込み、UI geometry configurationを生成する
 *
 * @details
 * config_fullpath_で指定されたfileを読み込み、
 * UI Geometry Configuration Formatに従ってconfiguration textを解析する。
 *
 * supported keyの収集、required fieldの確認、value parsingおよび
 * semantic validationを行い、validなui_geom_config_tへ変換する。
 *
 * external file由来のrepresentationは、本APIによる解析および
 * `ui_geom_config_is_valid()`によるResult validationが成功した時点で、
 * GLCE内部で使用可能なtrusted ui_geom_config_tとして受理される。
 *
 * @param[in]  config_fullpath_
 *             読み込む`.ui_geom` fileのNUL terminated full path。
 *             NULLまたはempty stringは許可しない。
 *
 * @param[out] out_config_
 *             読み込んだconfigurationの出力先。
 *             NULLは許可しない。
 *             success時にのみ書き換えられ、failure時は既存内容を変更しない。
 *
 * @retval RESOURCE_SUCCESS
 *         configurationの読み込み、解析およびvalidationに成功した。
 *
 * @retval RESOURCE_INVALID_ARGUMENT
 *         public API argumentがcontractを満たしていない。
 *
 * @retval RESOURCE_UNSUPPORTED_FILE
 *         file contentがUI Geometry Configuration Formatまたは
 *         ui_geom_config_tのsemantic requirementsを満たしていない。
 *
 * @return 上記以外では、file access、temporary resource operation等で発生した
 *         Resource layer result codeを返す。
 *
 * @pre config_fullpath_は、API実行中に読み取り可能な
 *      NUL terminated stringを参照していなければならない。
 * @pre out_config_は、API実行中に書き込み可能な
 *      ui_geom_config_t storageを参照していなければならない。
 *
 * @post RESOURCE_SUCCESSの場合、
 *       `ui_geom_config_is_valid(out_config_) == true`が成立する。
 * @post RESOURCE_SUCCESS以外の場合、out_config_の既存内容は変更されない。
 */
resource_result_t ui_geom_config_loader_load(const char* config_fullpath_, ui_geom_config_t* out_config_);

/**
 * @brief ui_geom_config_tのcanonical validityを判定する
 *
 * @details
 * ui_geom_config_tがGLCE内部で使用可能な
 * UI geometry configuration representationであるかを判定する。
 *
 * Validity Definition:
 * - config_はNULLではない。
 * - icon_width > 0
 * - icon_height > 0
 *
 * 本validatorはconfiguration fileのtext representation、
 * supported key、required field、duplicate key等のfile-level validityは扱わない。
 * それらは`ui_geom_config_loader_load()`によるexternal file admission時に検証する。
 *
 * @param[in] config_ 判定対象のUI geometry configuration
 *
 * @retval true  validなui_geom_config_tである。
 * @retval false invalidなui_geom_config_tである。
 *
 * @post config_の内容を変更しない。
 */
bool ui_geom_config_is_valid(const ui_geom_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
