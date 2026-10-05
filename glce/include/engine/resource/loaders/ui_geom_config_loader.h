// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file glce_config_utility.h
 * @author chocolate-pie24
 * @brief GLCE configuration textの低水準な解析機能を提供するEngine内部utility module
 * @details configuration textのline classificationおよびkey/value representationの解析を提供する。
 *          configuration format固有のkey、value semantic、required field等は扱わず、
 *          それらは本moduleを利用する上位moduleが所有する。
 *
 * @section glce_config_utility_boundary_contract Module Boundary Contract
 *
 * Module Scope:
 * - GLCE Config UtilityはEngine内部で使用するlow-level utility moduleであり、
 *   Applicationレイヤーから直接使用しない。
 * - 本moduleはGLCE configuration textに共通する低水準な
 *   lexical / syntactic representationを扱う。
 * - supported key、required field、value range、value interpretation等の
 *   format-specific semanticは本moduleでは所有せず、
 *   各configuration formatを扱う上位moduleが所有する。
 *
 * Trust Boundary:
 * - 本moduleはExternal Trust Boundaryを所有せず、
 *   external configurationをGLCE内部trusted representationとして
 *   admissionする責務を持たない。
 * - external configurationに対するtrust promotionは、
 *   そのconfiguration formatを所有する上位Loader等のboundary ownerが行う。
 * - 本moduleが返すclassificationまたはparsed viewは、
 *   source representation自体のtrust promotionを意味しない。
 *
 * Line Representation:
 * - lineはcaller-ownedのread-only borrowed storageとして扱い、
 *   本moduleはそのstorageを所有、変更、解放しない。
 * - line bodyはchar_count byteで構成され、
 *   body内にNUL、CR、LFを含まない。
 * - line[char_count]は終端NULである。
 * - callerはline[0]からline[char_count]までを
 *   読み取り可能なstorageとして提供しなければならない。
 * - char_count == 0のlineはempty line representationとして許可する。
 *
 * Borrowed View:
 * - key / value viewはsource line storageの一部を参照するnon-owning borrowed viewであり、
 *   key / value textのcopyまたは新しいstorageのallocationを行わない。
 * - viewが参照するstorageのownershipはsource lineのownerに残る。
 * - viewはsource line storageが生存し、かつ参照範囲の内容が変更されない間だけ有効である。
 * - viewが保持するpointerをcallerが個別に解放してはならない。
 *
 * State / Resource Ownership:
 * - 本moduleはpersistent stateを持たず、
 *   callerから渡されたtext storageまたはその他のresourceを所有しない。
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

#include "engine/resource/core/resource_types.h"

/**
 * @brief UI描画用ジオメトリ設定値格納構造体
 *
 */
typedef struct ui_geom_config {
    uint16_t icon_width;    /**< 矩形領域形状指定(幅) */
    uint16_t icon_height;   /**< 矩形領域形状指定(高さ) */
} ui_geom_config_t;

resource_result_t ui_geom_config_loader_load(const char* config_fullpath_, ui_geom_config_t* out_config_);

#ifdef __cplusplus
}
#endif
#endif
