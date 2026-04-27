#ifndef GLCE_ENGINE_CORE_BUFFER_UTILS_BUFFER_UTILS_H
#define GLCE_ENGINE_CORE_BUFFER_UTILS_BUFFER_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// WARNING: 当面はリトルエンディアン用のみ

int16_t buffer_utils_le_int16_t_get(const char* buff_);

int32_t buffer_utils_le_int32_t_get(const char* buff_);

int64_t buffer_utils_le_int64_t_get(const char* buff_);

uint16_t buffer_utils_le_uint16_t_get(const char* buff_);

uint32_t buffer_utils_le_uint32_t_get(const char* buff_);

uint64_t buffer_utils_le_uint64_t_get(const char* buff_);

#ifdef __cplusplus
}
#endif
#endif
