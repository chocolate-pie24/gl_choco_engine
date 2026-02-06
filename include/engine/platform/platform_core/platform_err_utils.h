#ifndef GLCE_ENGINE_PLATFORM_PLATFORM_CORE_PLATFORM_ERR_UTILS_H
#define GLCE_ENGINE_PLATFORM_PLATFORM_CORE_PLATFORM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/platform/platform_core/platform_types.h"

#include "engine/containers/choco_string.h"
#include "engine/core/memory/linear_allocator.h"

const char* platform_rslt_to_str(platform_result_t rslt_);

platform_result_t platform_rslt_convert_choco_string(choco_string_result_t rslt_);
platform_result_t platform_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
