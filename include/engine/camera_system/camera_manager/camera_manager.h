#ifndef GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_MANAGER_CAMERA_MANAGER_H
#define GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_MANAGER_CAMERA_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct camera_manager camera_manager_t;

#include <stddef.h>
#include <stdint.h>

#include "engine/core/memory/linear_allocator.h"

#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/camera_system/camera/camera.h"

camera_result_t camera_manager_initialize(linear_alloc_t* allocator_, size_t max_camera_count_, camera_manager_t** out_camera_manager_);

void camera_manager_deinitialize(camera_manager_t** camera_manager_);

camera_result_t camera_manager_regist(const char* camera_name_, camera_manager_t* camera_manager_, int16_t* out_camera_id_);

camera_result_t camera_manager_unregist(int16_t camera_id_);

camera_result_t camera_manager_camera_get(int16_t camera_id_, camera_manager_t* camera_manager_, camera_t** out_camera_);

#ifdef __cplusplus
}
#endif
#endif
