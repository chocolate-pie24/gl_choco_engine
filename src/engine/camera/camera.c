#include <stdbool.h>

#include "engine/camera/camera.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

struct camera {
    float speed;
    vec3f_t euler;
    vec3f_t position;
    bool dirty;
    choco_string_t* name;
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過) */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ破損) */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

static const char* rslt_to_str(camera_result_t rslt_);
static camera_result_t rslt_convert_choco_memory(memory_system_result_t rslt_);
static camera_result_t rslt_convert_choco_string(choco_string_result_t rslt_);

camera_result_t camera_create(const char* name_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    camera_t* tmp_camera = NULL;
    memory_system_result_t mem_ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t string_ret = CHOCO_STRING_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "out_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "*out_camera_")

    mem_ret = memory_system_allocate(sizeof(camera_t), MEMORY_TAG_CAMERA, (void**)&tmp_camera);
    if(MEMORY_SYSTEM_SUCCESS != mem_ret) {
        ret = rslt_convert_choco_memory(mem_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to allocate memory for camera.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_camera->name = NULL;

    string_ret = choco_string_create_from_c_string(&tmp_camera->name, name_);
    if(CHOCO_STRING_SUCCESS != string_ret) {
        ret = rslt_convert_choco_string(string_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to create string for camera name.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_camera->dirty = false;
    tmp_camera->speed = 0.0f;
    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->euler);
    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->position);

    *out_camera_ = tmp_camera;
    ret = CAMERA_SUCCESS;
cleanup:
    if(CAMERA_SUCCESS != ret) {
        if(NULL != tmp_camera) {
            if(NULL != tmp_camera->name) {
                choco_string_destroy(&tmp_camera->name);
            }
            camera_destroy(&tmp_camera);
        }
    }
    return ret;
}

void camera_destroy(camera_t** camera_) {
    if(NULL == camera_) {
        return;
    }
    if(NULL == *camera_) {
        return;
    }
    if(NULL != (*camera_)->name) {
        choco_string_destroy(&(*camera_)->name);
    }
    memory_system_free(*camera_, sizeof(camera_t), MEMORY_TAG_CAMERA);
    *camera_ = NULL;
}

const char* camera_name_get(const camera_t* camera_) {
    if(NULL == camera_) {
        ERROR_MESSAGE("camera_name_get(%s) - Argument camera_ requires a valid pointer.", rslt_to_str(CAMERA_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == camera_->name) {
        ERROR_MESSAGE("camera_name_get(%s) - Provided camera_ is corrupted.", rslt_to_str(CAMREA_DATA_CORRUPTED));
        return NULL;
    }
    return choco_string_c_str(camera_->name);
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(camera_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_SUCCESS:
        return s_rslt_str_success;
    case CAMERA_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CAMERA_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CAMERA_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CAMERA_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case CAMERA_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case CAMERA_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CAMREA_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    default:
        return s_rslt_str_undefined_error;
    }
}

static camera_result_t rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return CAMERA_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}

static camera_result_t rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return CAMERA_SUCCESS;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case CHOCO_STRING_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case CHOCO_STRING_OVERFLOW:
        return CAMERA_BAD_OPERATION;    // 長すぎる文字列指定はAPIの誤用とする
    case CHOCO_STRING_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case CHOCO_STRING_DATA_CORRUPTED:
        return CAMREA_DATA_CORRUPTED;
    case CHOCO_STRING_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return CAMERA_UNDEFINED_ERROR;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}
