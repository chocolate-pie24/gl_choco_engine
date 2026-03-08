#ifndef GLCE_ENGINE_CAMERA_CAMERA_H
#define GLCE_ENGINE_CAMERA_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct camera camera_t;

typedef enum {
    CAMERA_SUCCESS = 0,
    CAMERA_INVALID_ARGUMENT,
    CAMERA_RUNTIME_ERROR,
    CAMERA_BAD_OPERATION,
    CAMERA_NO_MEMORY,
    CAMERA_LIMIT_EXCEEDED,
    CAMREA_DATA_CORRUPTED,
    CAMERA_UNDEFINED_ERROR,
} camera_result_t;

camera_result_t camera_create(const char* name_, camera_t** out_camera_);

void camera_destroy(camera_t** camera_);

// 取得した文字列のメモリを解放してはいけない。メモリ解放は必ずcamera_destroyを介して行うこと
const char* camera_name_get(const camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
