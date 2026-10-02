// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file bmp_loader.h
 * @author chocolate-pie24
 * @brief BMPファイルをGLCE内部texture representationへloadするAPIを提供する
 *
 * @details
 * BMP Loader moduleはexternal BMP fileを読み込み、
 * GLCE内部で使用可能なtexture resource metadataおよびpixel dataへ変換する。
 *
 * @section bmp_loader_boundary_contract Module Boundary Contract
 *
 * - BMP Loader moduleはexternal BMP resourceとGLCE内部representationの間にある
 *   resource trust boundaryとして動作する。
 * - load成功時に公開されるresource metadataおよびpixel dataは、
 *   GLCE内部で使用可能なtrusted representationとして扱う。
 *
 * - 本moduleが受理するBMP fileはBITMAPINFOHEADERを使用する。
 * - 非圧縮BMPのみを対象とする。
 * - image widthは0より大きい。
 * - image heightは0ではない。
 * - 24bit RGBおよび32bit RGBAの非圧縮BMPをサポートする。
 *
 * - load成功時に返されるtexture_resource_info_tはvalidである。
 * - texture_resource_info_tのvalidityはResource Coreの
 *   texture_resource_info_is_valid()によって定義する。
 *
 * - load成功時に返されるpixel dataはBMP row paddingを含まない。
 * - pixel channel orderはGLCE内部で使用するRGBまたはRGBAへnormalizeされる。
 * - BMP file内のorientationにかかわらず、
 *   GLCE内部で使用するorientationへnormalizeされたpixel dataを返す。
 *
 * - load成功時に返されるpixel storageはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで取得されたallocationである。
 * - output pixel storageのownershipはcallerへ移転する。
 * - callerは取得したpixel storageのlifetime管理およびreleaseに責任を持つ。
 *
 * - load失敗時にはoutputとして有効なtexture resourceを公開しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/resource/core/resource_types.h"

resource_result_t bmp_loader_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_);

#ifdef __cplusplus
}
#endif
#endif
