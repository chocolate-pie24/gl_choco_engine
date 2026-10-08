// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file texture_cpu_resource.h
 * @author chocolate-pie24
 * @brief texture描画に使用するCPU側pixel resourceを扱うopaque objectと操作を提供する
 *
 * @details
 * Texture CPU Resource moduleは、texture imageを構成するCPU側pixel dataと
 * image metadataを保持する`texture_cpu_resource_t`のlifetimeおよび
 * ownershipを管理する。
 *
 * `texture_cpu_resource_t`は内部表現を公開しないopaque objectであり、
 * create成功後はtexture処理に使用可能な完成済みCPU resourceとして扱う。
 *
 * @section texture_cpu_resource_boundary_contract Module Boundary Contract
 *
 * - `texture_cpu_resource_t`はopaque typeとして公開し、
 *   内部表現をmodule外部へ公開しない。
 * - publicなpartial initialization stateは持たず、
 *   moduleから公開される`texture_cpu_resource_t`は完成済みresourceである。
 *
 * - Texture CPU Resource moduleは`texture_cpu_resource_t` object自身と、
 *   そのresourceが保持するCPU側pixel storageのownershipおよびlifetimeを管理する。
 * - callerはTexture CPU Resource moduleが所有するinternal resourceを
 *   直接変更または解放しない。
 *
 * - `texture_cpu_resource_t`が保持するresource_infoはvalidである。
 * - resource_infoのvalidityはResource Coreの
 *   `texture_resource_info_is_valid()`によって定義する。
 * - `texture_resource_info_t`を構成する各fieldおよびfield間relationの
 *   semantic ownershipはResource Coreに属する。
 * - Texture CPU Resource moduleはresource_info固有のsemanticを
 *   独自に再定義しない。
 *
 * - Texture CPU Resource moduleが保持するpixel dataは、
 *   Loader等のupstream trust boundaryでGLCE内部representationとして
 *   受理済みのtrusted dataとして扱う。
 * - Texture CPU Resource moduleはexternal dataをtrusted internal representationへ
 *   昇格させるtrust boundaryとしては扱わない。
 * - pixel elementの内容そのものについて、
 *   upstreamで成立済みのsemantic contractを再認証しない。
 *
 * - owned pixel storageはGeneral Allocatorから
 *   `GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE`で取得されたlive allocationであることを
 *   module boundary contractとする。
 * - 現在のGeneral Allocator APIではpointerからallocation時のmemory tagを
 *   完全に再確認できないため、このallocation provenanceは外部契約として信頼する。
 *
 * - moduleから公開されるCPU側pixel dataへの参照はborrowed viewとして扱う。
 * - borrowed pixel dataのownershipはTexture CPU Resource moduleに残る。
 * - borrowed viewはownerである`texture_cpu_resource_t`のlifetimeを超えて使用しない。
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
