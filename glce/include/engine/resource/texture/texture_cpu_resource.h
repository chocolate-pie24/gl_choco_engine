// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file texture_cpu_resource.h
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソースを操作するモジュールAPIの定義
 *
 * @date 2026-05-14
 *
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

resource_result_t texture_cpu_resource_create(texture_cpu_resource_t** texture_);

void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_);

resource_result_t texture_cpu_resource_pixel_load(texture_cpu_resource_t* texture_, const char* texture_source_);

resource_result_t texture_cpu_resource_pixel_unload(texture_cpu_resource_t* texture_);

resource_result_t texture_cpu_resource_pixel_get(const texture_cpu_resource_t* texture_, const uint8_t** out_pixels_);

resource_result_t texture_cpu_resource_pixel_size_get(const texture_cpu_resource_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_);

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_);

bool texture_cpu_resource_is_loaded(const texture_cpu_resource_t* texture_);

#ifdef __cplusplus
}
#endif
#endif
