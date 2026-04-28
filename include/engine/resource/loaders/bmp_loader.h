#ifndef GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/resource/resource_core/resource_types.h"

typedef struct bmp_loader bmp_loader_t;

resource_result_t bmp_loader_create(bmp_loader_t** bmp_loader_);

void bmp_loader_destroy(bmp_loader_t** bmp_loader_);

resource_result_t bmp_loader_load(const char* fullpath_, bmp_loader_t* bmp_loader_);

// bmp_loader_->pixelsの管理権限をout_pixels_へ移譲
resource_result_t bmp_loader_pixel_move(bmp_loader_t* bmp_loader_, uint8_t** out_pixels_);

resource_result_t bmp_loader_bmp_size_get(const bmp_loader_t* bmp_loader_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_);

#ifdef __cplusplus
}
#endif
#endif
