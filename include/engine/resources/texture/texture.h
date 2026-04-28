#ifndef GLCE_ENGINE_RESOURCES_TEXTURE_TEXTURE_H
#define GLCE_ENGINE_RESOURCES_TEXTURE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/resources/resources_core/resource_types.h"

typedef struct texture texture_t; /**< テクスチャCPU側リソース内部状態管理構造体前方宣言 */

texture_result_t texture_create(const char* name_, texture_t** texture_);

void texture_destroy(texture_t** texture_);

// NOTE: 以下のテクスチャ名称は特殊で、filepath_, extension_はNULLでok
// - test_texture_red
// - test_texture_green
// - test_texture_blue
// NOTE: テクスチャファイルフルパス: <fullpath_><texutre_->name>/<extension_>
texture_result_t texture_pixel_load(texture_t* texture_, const char* filepath_, const char* extension_);

texture_result_t texture_pixel_unload(texture_t* texture_);

texture_result_t texture_pixel_get(texture_t* texture_, uint8_t** out_pixels_);

texture_result_t texture_pixel_size_get(texture_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_);

const char* texture_name_get(const texture_t* texture_);

#ifdef __cplusplus
}
#endif
#endif
