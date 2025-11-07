/** @ingroup application
 *
 * @file application.c
 * @author chocolate-pie24
 * @brief プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行うAPIの実装
 *
 * @todo application.cのエラー文字列周りを別ファイルに移す
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stddef.h> // for NULL
#include <string.h> // for memset

#include "application/application.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/platform_context/platform_context.h"

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理構造体インスタンスを保持する
 *
 */
typedef struct app_state {
    // application status
    int window_width;           /**< ウィンドウ幅 */
    int window_height;          /**< ウィンドウ高さ */

    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;    /**< リニアアロケータ構造体インスタンスに必要なメモリ量 */
    size_t linear_alloc_align_req;  /**< リニアアロケータ構造体インスタンスが要求するメモリアライメント */
    size_t linear_alloc_pool_size;  /**< リニアアロケータ構造体インスタンスが使用するメモリプールのサイズ */
    void* linear_alloc_pool;        /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_alloc_t* linear_alloc;   /**< リニアアロケータ構造体インスタンス */

    // platform/platform_context
    platform_context_t* platform_context; /**< プラットフォームStrategyパターンへの窓口としてのコンテキスト構造体インスタンス */
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

static const char* rslt_to_str(application_result_t rslt_);
static application_result_t rslt_convert_mem_sys(memory_system_result_t rslt_);
static application_result_t rslt_convert_linear_alloc(linear_allocator_result_t rslt_);
static application_result_t rslt_convert_platform(platform_result_t rslt_);

static const char* const s_rslt_str_success = "SUCCESS";                    /**< アプリケーション実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< アプリケーション実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< アプリケーション実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< アプリケーション実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< アプリケーション実行結果コード(未定義エラー)に対応する文字列 */

application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", s_rslt_str_runtime_error);
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create memory system.", rslt_to_str(ret));
        goto cleanup;
    }

    // begin Simulation
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for application state.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin Simulation -> launch all systems.(Don't use s_app_state here.)


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create linear allocator.(Don't use s_app_state here.)
    // [NOTE] linear_allocatorのプールサイズについて
    //   全サブシステムのpreinitを先に実行し、リニアアロケータで必要な容量を計算可能だが、
    //   各サブシステムのアライメント要件を考慮すると単純に総和を取れば良いと言うものではなく、ちょっと複雑
    //   当面は実施せず、多めにメモリを確保する方針にする
    INFO_MESSAGE("Initializing linear allocator...");
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate linear allocator memory.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp->linear_alloc_pool_size = 1 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for the linear allocator pool.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize linear allocator.", rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("linear_allocator initialized successfully.");

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create platform.(Don't use s_app_state here.)
    INFO_MESSAGE("Initializing platform state...");
    ret_platform = platform_initialize(tmp->linear_alloc, PLATFORM_USE_GLFW, &tmp->platform_context);
    if(PLATFORM_SUCCESS != ret_platform) {
        ret = rslt_convert_platform(ret_platform);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize platform.", rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("platform_backend initialized successfully.");

    // begin temporary
    // TODO: ウィンドウ生成はレンダラー作成時にそっちに移す
    tmp->window_width = 1024;
    tmp->window_height = 768;
    ret_platform = platform_window_create(tmp->platform_context, "test_window", 1024, 768);
    if(PLATFORM_SUCCESS != ret_platform) {
        ret = rslt_convert_platform(ret_platform);
        ERROR_MESSAGE("application_create(%s) - Failed to create window.", rslt_to_str(ret));
        goto cleanup;
    }
    // end temporary

    // commit
    s_app_state = tmp;
    INFO_MESSAGE("Application created successfully.");
    memory_system_report();
    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp) {
            if(NULL != tmp->platform_context) {
                platform_destroy(tmp->platform_context);
            }
            if(NULL != tmp->linear_alloc_pool) {
                memory_system_free(tmp->linear_alloc_pool, tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
            }
            if(NULL != tmp->linear_alloc) {
                memory_system_free(tmp->linear_alloc, tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
            }
            memory_system_free(tmp, sizeof(*tmp), MEMORY_TAG_SYSTEM);
            tmp = NULL;
        }
        memory_system_destroy();
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    if(NULL != s_app_state->platform_context) {
        platform_destroy(s_app_state->platform_context);
    }
    if(NULL != s_app_state->linear_alloc_pool) {
        memory_system_free(s_app_state->linear_alloc_pool, s_app_state->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc_pool = NULL;
    }
    if(NULL != s_app_state->linear_alloc) {
        memory_system_free(s_app_state->linear_alloc, s_app_state->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc = NULL;
    }

    memory_system_free(s_app_state, sizeof(*s_app_state), MEMORY_TAG_SYSTEM);
    s_app_state = NULL;
    INFO_MESSAGE("Freed all memory.");
    memory_system_report();
    memory_system_destroy();
    // end cleanup all systems.

    INFO_MESSAGE("Application destroyed successfully.");
cleanup:
    return;
}

application_result_t application_run(void) {
    application_result_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", rslt_to_str(ret));
        goto cleanup;
    }
    while(1) {
    }
cleanup:
    return ret;
}

/**
 * @brief アプリケーション実行結果コードを文字列に変換し出力する
 *
 * @param rslt_ アプリケーション実行結果コード
 * @return const char* 変換された文字列
 */
static const char* rslt_to_str(application_result_t rslt_) {
    switch(rslt_) {
    case APPLICATION_SUCCESS:
        return s_rslt_str_success;
    case APPLICATION_NO_MEMORY:
        return s_rslt_str_no_memory;
    case APPLICATION_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case APPLICATION_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case APPLICATION_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief エラー伝播のため、メモリシステム実行結果コードをアプリケーション実行結果コードに変換する
 *
 * @param rslt_ メモリシステム実行結果コード
 * @return application_result_t 変換されたアプリケーション実行結果コード
 */
static application_result_t rslt_convert_mem_sys(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

/**
 * @brief エラー伝播のため、リニアアロケータ実行結果コードをアプリケーション実行結果コードに変換する
 *
 * @param rslt_ リニアアロケータ実行結果コード
 * @return application_result_t 変換されたアプリケーション実行結果コード
 */
static application_result_t rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
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

/**
 * @brief エラー伝播のため、プラットフォームシステム実行結果コードをアプリケーション実行結果コードに変換する
 *
 * @param rslt_ プラットフォームシステム実行結果コード
 * @return application_result_t 変換されたアプリケーション実行結果コード
 */
static application_result_t rslt_convert_platform(platform_result_t rslt_) {
    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return APPLICATION_SUCCESS;
    case PLATFORM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case PLATFORM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case PLATFORM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case PLATFORM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case PLATFORM_WINDOW_CLOSE:
        return APPLICATION_SUCCESS; // これはエラーではないので、成功扱いにする
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
