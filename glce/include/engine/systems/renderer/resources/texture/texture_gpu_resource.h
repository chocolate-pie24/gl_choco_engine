#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURE_TEXTURE_GPU_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURE_TEXTURE_GPU_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/resources/texture/core/texture_gpu_resource_types.h"

typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct renderer_backend_context renderer_backend_context_t;

texture_gpu_resource_result_t texture_gpu_resource_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, texture_gpu_resource_t** out_texture_gpu_resource_);
void texture_gpu_resource_destroy(renderer_backend_context_t* backend_context_, texture_gpu_resource_t** texture_gpu_resource_);
texture_gpu_resource_result_t texture_gpu_resource_bind(const renderer_backend_context_t* backend_context_, const texture_gpu_resource_t* texture_gpu_resource_);
texture_gpu_resource_result_t texture_gpu_resource_unbind(const renderer_backend_context_t* backend_context_, const texture_gpu_resource_t* texture_gpu_resource_);
texture_gpu_resource_result_t texture_gpu_resource_upload(const renderer_backend_context_t* backend_context_, texture_gpu_resource_t* texture_gpu_resource_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);
bool texture_gpu_resource_is_valid(const texture_gpu_resource_t* texture_gpu_resource_);

#ifdef __cplusplus
}
#endif
#endif
