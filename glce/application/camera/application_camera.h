// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_CAMERA_APPLICATION_CAMERA_H
#define GLCE_APPLICATION_CAMERA_APPLICATION_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/core/application_types.h"

typedef struct application_camera application_camera_t;

application_result_t application_camera_create(application_camera_t** out_application_camera_);

void application_camera_destroy(application_camera_t** application_camera_);

#ifdef __cplusplus
}
#endif
#endif
