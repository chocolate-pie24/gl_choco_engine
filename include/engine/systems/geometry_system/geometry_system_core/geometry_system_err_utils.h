#ifndef GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_SYSTEM_CORE_GEOMETRY_SYSTEM_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_SYSTEM_CORE_GEOMETRY_SYSTEM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/geometry_system/geometry_system_core/geometry_system_types.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/resource/resource_core/resource_types.h"

const char* geometry_system_rslt_to_str(geometry_system_result_t rslt_);

geometry_system_result_t geometry_system_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

geometry_system_result_t geometry_system_rslt_convert_resource(resource_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
