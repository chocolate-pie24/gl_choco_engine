#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_TEXTURE_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

renderer_result_t renderer_backend_texture_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);

void renderer_backend_texture_destroy(renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_);

renderer_result_t renderer_backend_texture_bind(renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

renderer_result_t renderer_backend_texture_unbind(renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

renderer_result_t renderer_backend_texture_pixel_upload(renderer_backend_context_t* backend_context_, const uint8_t* pixels_, const renderer_backend_texture_t* texture_handle_);

#ifdef __cplusplus
}
#endif
#endif
