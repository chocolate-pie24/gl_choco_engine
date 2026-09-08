// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_FLIGHT_CAMERA_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_FLIGHT_CAMERA_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"


typedef struct flight_camera_registry flight_camera_registry_t;

typedef struct linear_alloc linear_alloc_t;                                 /**< リニアアロケータのopaque型 */
typedef struct flight_camera flight_camera_t;

camera_registry_result_t flight_camera_registry_initialize(size_t max_flight_camera_count_, linear_alloc_t* allocator_, flight_camera_registry_t** out_registry_);

void flight_camera_registry_deinitialize(flight_camera_registry_t* registry_);

bool flight_camera_registry_find(const flight_camera_registry_t* registry_, const char* name_);

flight_camera_t* flight_camera_registry_flight_camera_get(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_);

camera_registry_result_t flight_camera_registry_id_get(const flight_camera_registry_t* registry_, const char* name_, uint16_t* out_flight_camera_id_);

camera_registry_result_t flight_camera_registry_register(flight_camera_registry_t* registry_, const char* flight_camera_name_, flight_camera_t** flight_camera_, uint16_t* out_flight_camera_id_);

camera_registry_result_t flight_camera_registry_unregister(flight_camera_registry_t* registry_, uint16_t flight_camera_id_);

bool flight_camera_registry_is_valid(const flight_camera_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
