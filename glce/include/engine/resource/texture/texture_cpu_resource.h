// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file texture_cpu_resource.h
 * @author chocolate-pie24
 * @brief texture描画に使用するCPU側pixel resourceを扱うopaque objectと操作を提供する
 *
 * @details
 * Texture CPU Resource moduleは、texture imageを構成するCPU側pixel dataと、
 * image width、height、channel count等のmetadataを保持する
 * `texture_cpu_resource_t`と、そのlifetime管理およびresource参照機能を提供する。
 *
 * `texture_cpu_resource_t`は内部表現を公開しないopaque objectであり、
 * create成功後はtexture処理に使用可能な完成済みCPU resourceとして扱う。
 *
 * 本moduleはLoader等によって生成済みのtrusted pixel dataを受け取り、
 * pixel storageのownershipをmoveによって取得する。
 *
 * @section texture_cpu_resource_boundary_contract Module Boundary Contract
 *
 * - `texture_cpu_resource_t`はopaque typeとして公開し、
 *   内部表現をmodule外部へ公開しない。
 * - Texture CPU Resource moduleは`texture_cpu_resource_t` object自身と、
 *   そのresourceが保持するCPU側pixel storageのownershipおよびlifetimeを管理する。
 * - callerはTexture CPU Resource moduleが所有するinternal resourceを
 *   直接変更または解放しない。
 * - module APIへ渡す`texture_cpu_resource_t*`は、
 *   Texture CPU Resource moduleによって生成され、
 *   lifetime中にあるobjectを参照するものとする。
 *
 * - create成功時に公開される`texture_cpu_resource_t`は、
 *   texture CPU resourceとして通常利用可能な完成済みresourceを表す。
 * - publicなpartial initialization stateは持たない。
 *
 * - createへ渡されるpixel dataは、Loader等のupstream trust boundaryで
 *   GLCE内部representationとして受理するために必要なvalidationが
 *   完了していることを前提とする。
 * - Texture CPU Resource moduleはexternal dataをtrusted internal representationへ
 *   昇格させるtrust boundaryとしては扱わない。
 * - pixel elementの内容そのものについて、
 *   upstreamで成立済みのsemantic contractを再認証する目的のvalidationは行わない。
 *
 * - createへ渡される`pixels_`はownership move対象である。
 * - create開始時点では`*pixels_`が指すpixel storageのownershipはcallerに残る。
 * - create成功時にのみpixel storageのownershipを
 *   Texture CPU Resource moduleへ移転し、`*pixels_`をNULLへ変更する。
 * - create失敗時にはpixel storageのownershipを取得せず、
 *   `*pixels_`およびそのstorageのownershipはcallerに残る。
 *
 * - move対象となるpixel storageはGeneral Allocatorから
 *   `GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE`で取得されたlive allocationであることをcaller contractとする。
 * - 現在のGeneral Allocator APIでは、pointerからallocation時のmemory tagを
 *   完全に再確認する仕組みを提供していないため、Texture CPU Resource moduleはこのallocation provenanceを外部契約として信頼する。
 * - 将来的にownership transferをより明示的かつ検証可能にする必要が生じた場合は、
 *   pointer、allocation size、memory tag等のallocation metadataを保持する
 *   ownership descriptor等を用いた仕組みの導入を検討する。
 *
 * - image widthおよびheightは0より大きいものとする。
 * - channel countはRGBを表す3、またはRGBAを表す4のみを使用する。
 * - pixel data sizeは
 *   `width * height * channel_count`
 *   と一致するものとする。
 *
 * - moduleから公開されるCPU側pixel dataへの参照はborrowed viewとして扱う。
 * - borrowed pixel dataのownershipはTexture CPU Resource moduleに残り、
 *   callerはそのstorageを変更または解放しない。
 * - borrowed viewは、そのownerである`texture_cpu_resource_t`の
 *   lifetime終了後には使用しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_TEXTURE_TEXTURE_CPU_RESOURCE_H
#define GLCE_ENGINE_RESOURCE_TEXTURE_TEXTURE_CPU_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/resource/core/resource_types.h"

typedef struct texture_cpu_resource texture_cpu_resource_t; /**< テクスチャCPU側リソース内部状態管理構造体前方宣言 */

resource_result_t texture_cpu_resource_create(const texture_resource_info_t* resource_info_, uint8_t** pixels_, texture_cpu_resource_t** out_texture_resource_);

void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_resource_);

resource_result_t texture_cpu_resource_pixels_get(const texture_cpu_resource_t* texture_resource_, const uint8_t** out_pixels_);

resource_result_t texture_cpu_resource_resource_info_get(const texture_cpu_resource_t* texture_resource_, texture_resource_info_t* out_resource_info_);

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_resource_);

#ifdef __cplusplus
}
#endif
#endif
