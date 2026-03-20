#ifndef GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_ERR_UTILS_H
#define GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/application_core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/view/view_core/view_types.h"

#include "engine/platform/platform_core/platform_types.h"

#include "engine/renderer/renderer_core/renderer_types.h"

const char* app_rslt_to_str(application_result_t rslt_);

application_result_t app_rslt_convert_mem_sys(memory_system_result_t rslt_);

application_result_t app_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

application_result_t app_rslt_convert_platform(platform_result_t rslt_);

application_result_t app_rslt_convert_ring_queue(ring_queue_result_t rslt_);

application_result_t app_rslt_convert_renderer(renderer_result_t rslt_);

application_result_t app_rslt_convert_view(view_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
