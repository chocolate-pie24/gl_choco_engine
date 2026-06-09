/** @ingroup application
 *
 * @file application_err_utils.c
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @version 0.1
 * @date 2026-03-25
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "application/application_core/application_err_utils.h"

#include "application/application_core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/systems/camera_system/camera_core/camera_types.h"
#include "engine/systems/platform/platform_core/platform_types.h"
#include "engine/systems/renderer/renderer_core/renderer_types.h"
#include "engine/systems/texture_system/texture_manager.h"

#include "engine/resource/resource_core/resource_types.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"
#include "engine/base/choco_macros.h"

#include "application/application_core/test_application_err_utils.h"

// application_err_utils用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_app_rslt_convert_mem_sys;              /**< app_rslt_convert_mem_sys()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_linear_alloc;         /**< app_rslt_convert_linear_alloc()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_platform;             /**< app_rslt_convert_platform()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_ring_queue;           /**< app_rslt_convert_ring_queue()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_renderer;             /**< app_rslt_convert_renderer()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_camera;               /**< app_rslt_convert_camera()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_texture_system;       /**< app_rslt_convert_texture_system()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_resource;             /**< app_rslt_convert_resource()テスト設定 */
static test_call_control_t s_test_config_app_rslt_convert_geometry_primitive;   /**< app_rslt_convert_geometry_primitive()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_app_rslt_to_str(void);
static void test_app_rslt_convert_mem_sys(void);
static void test_app_rslt_convert_linear_alloc(void);
static void test_app_rslt_convert_platform(void);
static void test_app_rslt_convert_ring_queue(void);
static void test_app_rslt_convert_renderer(void);
static void test_app_rslt_convert_camera(void);
static void test_app_rslt_convert_texture_system(void);
static void test_app_rslt_convert_resource(void);
static void test_app_rslt_convert_geometry_primitive(void);
#endif

static const char* const s_rslt_str_success = "SUCCESS";                    /**< アプリケーション実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< アプリケーション実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< アプリケーション実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< アプリケーション実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< アプリケーション実行結果コード(メモリ破損,未初期化)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< アプリケーション実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< アプリケーション実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< アプリケーション実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";  /**< アプリケーション実行結果コード(未対応のファイル形式)に対応する文字列 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";    /**< アプリケーション実行結果コード(ファイルオープンエラー)に対応する文字列 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";  /**< アプリケーション実行結果コード(ファイルクローズエラー)に対応する文字列 */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";    /**< アプリケーション実行結果コード(ファイル読み込みエラー)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< アプリケーション実行結果コード(未定義エラー)に対応する文字列 */

const char* app_rslt_to_str(application_result_t rslt_) {
    switch(rslt_) {
    case APPLICATION_SUCCESS:
        return s_rslt_str_success;
    case APPLICATION_NO_MEMORY:
        return s_rslt_str_no_memory;
    case APPLICATION_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case APPLICATION_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case APPLICATION_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case APPLICATION_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case APPLICATION_OVERFLOW:
        return s_rslt_str_overflow;
    case APPLICATION_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case APPLICATION_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case APPLICATION_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case APPLICATION_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case APPLICATION_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case APPLICATION_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

application_result_t app_rslt_convert_mem_sys(memory_system_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_mem_sys.call_count++;
    if(s_test_config_app_rslt_convert_mem_sys.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_mem_sys.call_count == s_test_config_app_rslt_convert_mem_sys.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_mem_sys.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_linear_alloc.call_count++;
    if(s_test_config_app_rslt_convert_linear_alloc.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_linear_alloc.call_count == s_test_config_app_rslt_convert_linear_alloc.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_linear_alloc.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return APPLICATION_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_platform(platform_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_platform.call_count++;
    if(s_test_config_app_rslt_convert_platform.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_platform.call_count == s_test_config_app_rslt_convert_platform.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_platform.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return APPLICATION_SUCCESS;
    case PLATFORM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case PLATFORM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case PLATFORM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case PLATFORM_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case PLATFORM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case PLATFORM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case PLATFORM_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case PLATFORM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case PLATFORM_WINDOW_CLOSE:
        return APPLICATION_SUCCESS; // これはエラーではないので、成功扱いにする
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_ring_queue(ring_queue_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_ring_queue.call_count++;
    if(s_test_config_app_rslt_convert_ring_queue.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_ring_queue.call_count == s_test_config_app_rslt_convert_ring_queue.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_ring_queue.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case RING_QUEUE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case RING_QUEUE_EMPTY:
        return APPLICATION_RUNTIME_ERROR;   // リングキュー空はRuntime errorに変換
    case RING_QUEUE_OVERFLOW:
        return APPLICATION_RUNTIME_ERROR;   // オーバーフローもRuntime errorに変換
    case RING_QUEUE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RING_QUEUE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RING_QUEUE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_renderer(renderer_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_renderer.call_count++;
    if(s_test_config_app_rslt_convert_renderer.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_renderer.call_count == s_test_config_app_rslt_convert_renderer.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_renderer.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case RENDERER_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDERER_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDERER_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDERER_SHADER_COMPILE_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_SHADER_LINK_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDERER_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDERER_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDERER_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_camera(camera_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_camera.call_count++;
    if(s_test_config_app_rslt_convert_camera.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_camera.call_count == s_test_config_app_rslt_convert_camera.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_camera.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case CAMERA_SUCCESS:
        return APPLICATION_SUCCESS;
    case CAMERA_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case CAMERA_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case CAMERA_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_texture_system(texture_system_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_texture_system.call_count++;
    if(s_test_config_app_rslt_convert_texture_system.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_texture_system.call_count == s_test_config_app_rslt_convert_texture_system.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_texture_system.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case TEXTURE_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case TEXTURE_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case TEXTURE_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case TEXTURE_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case TEXTURE_SYSTEM_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case TEXTURE_SYSTEM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case TEXTURE_SYSTEM_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case TEXTURE_SYSTEM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case TEXTURE_SYSTEM_FILE_OPEN_ERROR:
        return APPLICATION_FILE_OPEN_ERROR;
    case TEXTURE_SYSTEM_FILE_READ_ERROR:
        return APPLICATION_FILE_READ_ERROR;
    case TEXTURE_SYSTEM_UNSUPPORTED_FILE:
        return APPLICATION_UNSUPPORTED_FILE;
    case TEXTURE_SYSTEM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_resource(resource_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_resource.call_count++;
    if(s_test_config_app_rslt_convert_resource.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_resource.call_count == s_test_config_app_rslt_convert_resource.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_resource.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RESOURCE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RESOURCE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RESOURCE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RESOURCE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RESOURCE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RESOURCE_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RESOURCE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RESOURCE_FILE_OPEN_ERROR:
        return APPLICATION_FILE_OPEN_ERROR;
    case RESOURCE_FILE_READ_ERROR:
        return APPLICATION_FILE_READ_ERROR;
    case RESOURCE_FILE_CLOSE_ERROR:
        return APPLICATION_FILE_CLOSE_ERROR;
    case RESOURCE_UNSUPPORTED_FILE:
        return APPLICATION_UNSUPPORTED_FILE;
    case RESOURCE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_geometry_primitive(geometry_primitive_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_app_rslt_convert_geometry_primitive.call_count++;
    if(s_test_config_app_rslt_convert_geometry_primitive.fail_on_call != 0) {
        if(s_test_config_app_rslt_convert_geometry_primitive.call_count == s_test_config_app_rslt_convert_geometry_primitive.fail_on_call) {
            return (application_result_t)s_test_config_app_rslt_convert_geometry_primitive.forced_result;
        }
    }
#endif

    switch(rslt_) {
    case GEOMETRY_PRIMITIVE_SUCCESS:
        return APPLICATION_SUCCESS;
    case GEOMETRY_PRIMITIVE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case GEOMETRY_PRIMITIVE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case GEOMETRY_PRIMITIVE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case GEOMETRY_PRIMITIVE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case GEOMETRY_PRIMITIVE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case GEOMETRY_PRIMITIVE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD
void NO_COVERAGE test_app_rslt_convert_mem_sys_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_mem_sys.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_mem_sys.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_linear_alloc_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_linear_alloc.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_linear_alloc.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_platform_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_platform.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_platform.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_ring_queue_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_ring_queue.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_ring_queue.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_renderer_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_renderer.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_renderer.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_camera_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_camera.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_camera.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_texture_system_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_texture_system.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_texture_system.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_resource_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_resource.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_resource.forced_result = config_->forced_result;
}

void NO_COVERAGE test_app_rslt_convert_geometry_primitive_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_app_rslt_convert_geometry_primitive.fail_on_call = config_->fail_on_call;
    s_test_config_app_rslt_convert_geometry_primitive.forced_result = config_->forced_result;
}

void NO_COVERAGE test_application_err_utils_config_reset(void) {
    test_call_control_reset(&s_test_config_app_rslt_convert_mem_sys);
    test_call_control_reset(&s_test_config_app_rslt_convert_linear_alloc);
    test_call_control_reset(&s_test_config_app_rslt_convert_platform);
    test_call_control_reset(&s_test_config_app_rslt_convert_ring_queue);
    test_call_control_reset(&s_test_config_app_rslt_convert_renderer);
    test_call_control_reset(&s_test_config_app_rslt_convert_camera);
    test_call_control_reset(&s_test_config_app_rslt_convert_texture_system);
    test_call_control_reset(&s_test_config_app_rslt_convert_resource);
    test_call_control_reset(&s_test_config_app_rslt_convert_geometry_primitive);
}

void NO_COVERAGE test_application_err_utils(void) {
    test_app_rslt_to_str();
    test_app_rslt_convert_mem_sys();
    test_app_rslt_convert_linear_alloc();
    test_app_rslt_convert_platform();
    test_app_rslt_convert_ring_queue();
    test_app_rslt_convert_renderer();
    test_app_rslt_convert_camera();
    test_app_rslt_convert_texture_system();
    test_app_rslt_convert_resource();
    test_app_rslt_convert_geometry_primitive();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_to_str(void) {
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_SUCCESS), "SUCCESS"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_NO_MEMORY), "NO_MEMORY"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_RUNTIME_ERROR), "RUNTIME_ERROR"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "INVALID_ARGUMENT"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_DATA_CORRUPTED), "DATA_CORRUPTED"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_BAD_OPERATION), "BAD_OPERATION"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_OVERFLOW), "OVERFLOW"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_LIMIT_EXCEEDED), "LIMIT_EXCEEDED"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_UNSUPPORTED_FILE), "UNSUPPORTED_FILE"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_FILE_OPEN_ERROR), "FILE_OPEN_ERROR"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_FILE_CLOSE_ERROR), "FILE_CLOSE_ERROR"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_FILE_READ_ERROR), "FILE_READ_ERROR"));
    assert(0 == strcmp(app_rslt_to_str(APPLICATION_UNDEFINED_ERROR), "UNDEFINED_ERROR"));

    assert(0 == strcmp(app_rslt_to_str((application_result_t)-1), "UNDEFINED_ERROR"));
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_mem_sys(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_mem_sys(MEMORY_SYSTEM_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_mem_sys(MEMORY_SYSTEM_INVALID_ARGUMENT));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_mem_sys(MEMORY_SYSTEM_RUNTIME_ERROR));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_mem_sys(MEMORY_SYSTEM_NO_MEMORY));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_mem_sys(MEMORY_SYSTEM_LIMIT_EXCEEDED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_mem_sys(MEMORY_SYSTEM_BAD_OPERATION));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_mem_sys((memory_system_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_RUNTIME_ERROR;
    test_app_rslt_convert_mem_sys_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_mem_sys(MEMORY_SYSTEM_SUCCESS));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_mem_sys(MEMORY_SYSTEM_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_linear_alloc(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_linear_alloc((linear_allocator_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_RUNTIME_ERROR;
    test_app_rslt_convert_linear_alloc_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_platform(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_platform(PLATFORM_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_platform(PLATFORM_INVALID_ARGUMENT));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_platform(PLATFORM_RUNTIME_ERROR));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_platform(PLATFORM_NO_MEMORY));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_platform(PLATFORM_DATA_CORRUPTED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_platform(PLATFORM_BAD_OPERATION));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_platform(PLATFORM_UNDEFINED_ERROR));
    assert(APPLICATION_OVERFLOW == app_rslt_convert_platform(PLATFORM_OVERFLOW));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_platform(PLATFORM_LIMIT_EXCEEDED));
    assert(APPLICATION_SUCCESS == app_rslt_convert_platform(PLATFORM_WINDOW_CLOSE));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_platform((platform_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_RUNTIME_ERROR;
    test_app_rslt_convert_platform_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_platform(PLATFORM_SUCCESS));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_platform(PLATFORM_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_ring_queue(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_ring_queue(RING_QUEUE_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_ring_queue(RING_QUEUE_INVALID_ARGUMENT));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_ring_queue(RING_QUEUE_NO_MEMORY));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_ring_queue(RING_QUEUE_RUNTIME_ERROR));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_ring_queue(RING_QUEUE_UNDEFINED_ERROR));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_ring_queue(RING_QUEUE_EMPTY));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_ring_queue(RING_QUEUE_OVERFLOW));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_ring_queue(RING_QUEUE_LIMIT_EXCEEDED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_ring_queue(RING_QUEUE_BAD_OPERATION));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_ring_queue(RING_QUEUE_DATA_CORRUPTED));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_ring_queue((ring_queue_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_NO_MEMORY;
    test_app_rslt_convert_ring_queue_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_ring_queue(RING_QUEUE_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_ring_queue(RING_QUEUE_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_renderer(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_renderer(RENDERER_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_renderer(RENDERER_INVALID_ARGUMENT));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_renderer(RENDERER_RUNTIME_ERROR));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_renderer(RENDERER_NO_MEMORY));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_renderer(RENDERER_SHADER_COMPILE_ERROR));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_renderer(RENDERER_SHADER_LINK_ERROR));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_renderer(RENDERER_LIMIT_EXCEEDED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_renderer(RENDERER_BAD_OPERATION));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_renderer(RENDERER_DATA_CORRUPTED));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_renderer(RENDERER_UNDEFINED_ERROR));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_renderer((renderer_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_NO_MEMORY;
    test_app_rslt_convert_renderer_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_renderer(RENDERER_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_renderer(RENDERER_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_camera(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_camera(CAMERA_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_camera(CAMERA_INVALID_ARGUMENT));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_camera(CAMERA_RUNTIME_ERROR));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_camera(CAMERA_BAD_OPERATION));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_camera(CAMERA_NO_MEMORY));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_camera(CAMERA_LIMIT_EXCEEDED));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_camera(CAMERA_DATA_CORRUPTED));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_camera(CAMERA_UNDEFINED_ERROR));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_camera((camera_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_NO_MEMORY;
    test_app_rslt_convert_camera_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_camera(CAMERA_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_camera(CAMERA_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_texture_system(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_texture_system(TEXTURE_SYSTEM_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_texture_system(TEXTURE_SYSTEM_NO_MEMORY));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_texture_system(TEXTURE_SYSTEM_RUNTIME_ERROR));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_texture_system(TEXTURE_SYSTEM_INVALID_ARGUMENT));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_texture_system(TEXTURE_SYSTEM_DATA_CORRUPTED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_texture_system(TEXTURE_SYSTEM_BAD_OPERATION));
    assert(APPLICATION_OVERFLOW == app_rslt_convert_texture_system(TEXTURE_SYSTEM_OVERFLOW));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_texture_system(TEXTURE_SYSTEM_LIMIT_EXCEEDED));
    assert(APPLICATION_FILE_OPEN_ERROR == app_rslt_convert_texture_system(TEXTURE_SYSTEM_FILE_OPEN_ERROR));
    assert(APPLICATION_FILE_READ_ERROR == app_rslt_convert_texture_system(TEXTURE_SYSTEM_FILE_READ_ERROR));
    assert(APPLICATION_UNSUPPORTED_FILE == app_rslt_convert_texture_system(TEXTURE_SYSTEM_UNSUPPORTED_FILE));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_texture_system(TEXTURE_SYSTEM_UNDEFINED_ERROR));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_texture_system((texture_system_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_NO_MEMORY;
    test_app_rslt_convert_texture_system_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_texture_system(TEXTURE_SYSTEM_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_texture_system(TEXTURE_SYSTEM_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_resource(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_resource(RESOURCE_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_resource(RESOURCE_NO_MEMORY));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_resource(RESOURCE_RUNTIME_ERROR));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_resource(RESOURCE_INVALID_ARGUMENT));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_resource(RESOURCE_DATA_CORRUPTED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_resource(RESOURCE_BAD_OPERATION));
    assert(APPLICATION_OVERFLOW == app_rslt_convert_resource(RESOURCE_OVERFLOW));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_resource(RESOURCE_LIMIT_EXCEEDED));
    assert(APPLICATION_FILE_OPEN_ERROR == app_rslt_convert_resource(RESOURCE_FILE_OPEN_ERROR));
    assert(APPLICATION_FILE_READ_ERROR == app_rslt_convert_resource(RESOURCE_FILE_READ_ERROR));
    assert(APPLICATION_FILE_CLOSE_ERROR == app_rslt_convert_resource(RESOURCE_FILE_CLOSE_ERROR));
    assert(APPLICATION_UNSUPPORTED_FILE == app_rslt_convert_resource(RESOURCE_UNSUPPORTED_FILE));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_resource(RESOURCE_UNDEFINED_ERROR));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_resource((resource_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_NO_MEMORY;
    test_app_rslt_convert_resource_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_resource(RESOURCE_SUCCESS));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_resource(RESOURCE_SUCCESS));

    test_application_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_app_rslt_convert_geometry_primitive(void) {
    test_call_control_t config;

    test_application_err_utils_config_reset();

    assert(APPLICATION_SUCCESS == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_SUCCESS));
    assert(APPLICATION_INVALID_ARGUMENT == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_RUNTIME_ERROR));
    assert(APPLICATION_LIMIT_EXCEEDED == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED));
    assert(APPLICATION_BAD_OPERATION == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_BAD_OPERATION));
    assert(APPLICATION_NO_MEMORY == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_NO_MEMORY));
    assert(APPLICATION_DATA_CORRUPTED == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_DATA_CORRUPTED));
    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_UNDEFINED_ERROR));

    assert(APPLICATION_UNDEFINED_ERROR == app_rslt_convert_geometry_primitive((geometry_primitive_result_t)-1));

    test_application_err_utils_config_reset();

    config.fail_on_call = 2;
    config.forced_result = APPLICATION_RUNTIME_ERROR;
    test_app_rslt_convert_geometry_primitive_config_set(&config);

    assert(APPLICATION_SUCCESS == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_SUCCESS));
    assert(APPLICATION_RUNTIME_ERROR == app_rslt_convert_geometry_primitive(GEOMETRY_PRIMITIVE_SUCCESS));

    test_application_err_utils_config_reset();
}
#endif
