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

#define INVALID_CAMERA_ID (-1)

typedef struct camera camera_t;

// リニアアロケータ経由でのメモリ確保を行うため、初期化に失敗した場合は続行不可。リニアアロケータ自身を破棄すること。
camera_result_t camera_manager_initialize(linear_alloc_t* allocator_, int16_t max_camera_count_, camera_manager_t** out_camera_manager_);

// 内部メモリのうち、リニアアロケータ経由で確保したメモリは解放しない。このため、リニアアロケータを破棄しないと再度initializeは不可
void camera_manager_deinitialize(camera_manager_t* camera_manager_);

camera_result_t camera_manager_register(const char* camera_name_, camera_manager_t* camera_manager_, int16_t* out_camera_id_);

camera_result_t camera_manager_unregister(int16_t camera_id_, camera_manager_t* camera_manager_);

camera_result_t camera_manager_unregister_by_name(const char* name_, camera_manager_t* camera_manager_);

int16_t camera_manager_camera_id_get(const char* name_, const camera_manager_t* camera_manager_);

camera_result_t camera_manager_camera_get(int16_t camera_id_, camera_manager_t* camera_manager_, camera_t** out_camera_);

camera_result_t camera_manager_camera_get_by_name(const char* name_, camera_manager_t* camera_manager_, camera_t** out_camera_);

#ifdef __cplusplus
}
#endif
#endif
