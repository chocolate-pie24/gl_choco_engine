// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_RESOURCE_RENDER_MODEL_RENDER_MODEL_PIPELINE_RENDER_MODE_PIPELINE_H
#define GLCE_APPLICATION_RESOURCE_RENDER_MODEL_RENDER_MODEL_PIPELINE_RENDER_MODE_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/core/application_types.h"

typedef struct render_model_config render_model_config_t;
typedef struct renderer_frontend renderer_frontend_t;
typedef struct render_model render_model_t;

application_result_t render_model_import(const render_model_config_t* config_, renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, render_model_t** out_model_);

#ifdef __cplusplus
}
#endif
#endif
