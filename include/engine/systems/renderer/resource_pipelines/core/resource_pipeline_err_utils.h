#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_CORE_RESOURCE_PIPELINE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_CORE_RESOURCE_PIPELINE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/resource/resource_core/resource_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

const char* resource_pipeline_rslt_to_str(resource_pipeline_result_t rslt_);

resource_pipeline_result_t resource_pipeline_rslt_convert_resource(resource_result_t rslt_);

resource_pipeline_result_t resource_pipeline_rslt_convert_renderer(renderer_result_t rslt_);

resource_pipeline_result_t resource_pipeline_rslt_convert_resource_registry(resource_registry_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
