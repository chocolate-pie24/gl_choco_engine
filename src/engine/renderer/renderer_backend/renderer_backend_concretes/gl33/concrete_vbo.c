/** @ingroup gl33
 *
 * @file concrete_vbo.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VBOを使用するためのラッパーAPIの実装
 *
 * @version 0.1
 * @date 2025-12-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_memory.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vbo.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"

#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_vbo.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/core/memory/choco_memory.h"

// concrete_vbo用モジュール専用テスト制御構造体定義
/**
 * @brief void型の関数を強制的にNo-Opで終了させるための設定値構造体
 *
 */
typedef struct test_call_control_no_op {
    uint32_t call_count;    /**< 関数呼び出し回数 */
    uint32_t fail_on_call;  /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はNo-Opで関数を終了させる) */
} test_call_control_no_op_t;

// 外部公開APIテスト設定

// プライベート関数テスト設定
static test_call_control_t s_test_config_gl33_vbo_create;               /**< gl33_vbo_create()テスト設定 */
static test_call_control_no_op_t s_test_config_gl33_vbo_destroy;        /**< gl33_vbo_destroy()テスト設定 */
static test_call_control_t s_test_config_gl33_vbo_bind;                 /**< gl33_vbo_bind()テスト設定 */
static test_call_control_t s_test_config_gl33_vbo_unbind;               /**< gl33_vbo_unbind()テスト設定 */
static test_call_control_t s_test_config_gl33_vbo_vertex_load;          /**< gl33_vbo_vertex_load()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glGenBuffers;       /**< mock_glGenBuffers()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glBindBuffer;       /**< mock_glBindBuffer()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glDeleteBuffers;    /**< mock_glDeleteBuffers()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glBufferData;       /**< mock_glBufferData()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_gl33_vbo_create(void);
static void test_gl33_vbo_destroy(void);
static void test_gl33_vbo_bind(void);
static void test_gl33_vbo_unbind(void);
static void test_gl33_vbo_vertex_load(void);

static void test_call_control_no_op_reset(test_call_control_no_op_t* config_);
#endif

/**
 * @brief VBOモジュール内部状態管理構造体
 *
 */
struct renderer_backend_vbo {
    GLuint vbo_handle;  /**< VBO */
};

static renderer_result_t gl33_vbo_create(renderer_backend_vbo_t** vertex_buffer_);
static void gl33_vbo_destroy(renderer_backend_vbo_t** vertex_buffer_);
static renderer_result_t gl33_vbo_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_);
static renderer_result_t gl33_vbo_unbind(const renderer_backend_vbo_t* vertex_buffer_);
static renderer_result_t gl33_vbo_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

static void mock_glGenBuffers(GLsizei n_, GLuint* buffer_);
static void mock_glBindBuffer(GLenum target_, GLuint buffer_);
static void mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_);
static void mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_);

static const renderer_vbo_vtable_t s_gl33_vbo_vtable = {
    .vertex_buffer_create = gl33_vbo_create,
    .vertex_buffer_destroy = gl33_vbo_destroy,
    .vertex_buffer_bind = gl33_vbo_bind,
    .vertex_buffer_unbind = gl33_vbo_unbind,
    .vertex_buffer_vertex_load = gl33_vbo_vertex_load,
};

const renderer_vbo_vtable_t* gl33_vbo_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_vbo_vtable;
}

/**
 * @brief VBO構造体インスタンスのメモリを確保し初期化する
 *
 * @param[out] vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vbo_t* vbo = NULL;
 * renderer_result_t ret = gl33_vbo_create(&vbo);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_buffer_がNULL
 * - *vertex_buffer_が非NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vbo_create(renderer_backend_vbo_t** vertex_buffer_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vbo_create.call_count++;
    if(s_test_config_gl33_vbo_create.fail_on_call != 0) {
        if(s_test_config_gl33_vbo_create.call_count == s_test_config_gl33_vbo_create.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vbo_create.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_vbo_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_create", "vertex_buffer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_create", "vertex_buffer_")

    ret = render_mem_allocate(sizeof(renderer_backend_vbo_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_vbo_create(%s) - Failed to allocate memory for 'tmp'.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glGenBuffers(1, &tmp->vbo_handle);

    *vertex_buffer_ = tmp;

    ret = RENDERER_SUCCESS;

cleanup:
#ifdef TEST_BUILD
    // NOTE: 将来的に仕様変更でrender_mem_allocate成功した後で失敗することを想定し、cleanup漏れ検出を追加
    // ここはカバレッジ到達不可だけど許容する
    if(RENDERER_SUCCESS != ret && NULL != tmp) {
        assert(false);
    }
#endif
    return ret;
}

/**
 * @brief renderer_backend_vbo_t構造体インスタンスのメモリを解放し、OpenGLContext内のVBOも削除する
 *
 * @param[out] vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vbo_t* vbo = NULL;
 * renderer_result_t ret = gl33_vbo_create(&vbo);
 * // エラー処理
 * gl33_vbo_destroy(&vbo);
 * @endcode
 */
static void gl33_vbo_destroy(renderer_backend_vbo_t** vertex_buffer_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vbo_destroy.call_count++;
    if(s_test_config_gl33_vbo_destroy.fail_on_call != 0) {
        if(s_test_config_gl33_vbo_destroy.call_count == s_test_config_gl33_vbo_destroy.fail_on_call) {
            return;
        }
    }
#endif
    if(NULL == vertex_buffer_) {
        goto cleanup;
    }
    if(NULL == *vertex_buffer_) {
        goto cleanup;
    }

    // NOTE: 現状ではgl33_vbo_unbindはエラーを返さないため、このif文内は到達不可でカバレッジは100にはならないが許容する
    if(RENDERER_SUCCESS != gl33_vbo_unbind(*vertex_buffer_)) {
        WARN_MESSAGE("gl33_vbo_destroy(RUNTIME_ERROR) - Failed to unbind vertex buffer.");
    }
    mock_glDeleteBuffers(1, &(*vertex_buffer_)->vbo_handle);
    render_mem_free(*vertex_buffer_, sizeof(renderer_backend_vbo_t));
    *vertex_buffer_ = NULL;
cleanup:
    return;
}

/**
 * @brief glBindBuffer APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param[in] vertex_buffer_ bind対象vbo
 * @param[in,out] out_vbo_id_ bindしたvbo id格納先
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vbo_t* vbo = NULL;
 * renderer_result_t ret = gl33_vbo_create(&vbo);
 * // エラー処理
 * ret = gl33_vbo_bind(vbo);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT
 * - vertex_buffer_ == NULL
 * - out_vbo_id_ == NULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vbo_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vbo_bind.call_count++;
    if(s_test_config_gl33_vbo_bind.fail_on_call != 0) {
        if(s_test_config_gl33_vbo_bind.call_count == s_test_config_gl33_vbo_bind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vbo_bind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_bind", "vertex_buffer_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vbo_id_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_bind", "out_vbo_id_")

    if(0 == vertex_buffer_->vbo_handle) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_vbo_bind(%s) - Provided vertex_buffer is not valid.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    if(vertex_buffer_->vbo_handle != *out_vbo_id_) {
        mock_glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_->vbo_handle);
        *out_vbo_id_ = vertex_buffer_->vbo_handle;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

static renderer_result_t gl33_vbo_unbind(const renderer_backend_vbo_t* vertex_buffer_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vbo_unbind.call_count++;
    if(s_test_config_gl33_vbo_unbind.fail_on_call != 0) {
        if(s_test_config_gl33_vbo_unbind.call_count == s_test_config_gl33_vbo_unbind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vbo_unbind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_unbind", "vertex_buffer_")
    if(0 == vertex_buffer_->vbo_handle) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_vbo_unbind(%s) - Provided vertex_buffer is not valid.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glBindBuffer(GL_ARRAY_BUFFER, 0);

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

static renderer_result_t gl33_vbo_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vbo_vertex_load.call_count++;
    if(s_test_config_gl33_vbo_vertex_load.fail_on_call != 0) {
        if(s_test_config_gl33_vbo_vertex_load.call_count == s_test_config_gl33_vbo_vertex_load.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vbo_vertex_load.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "vertex_buffer_")
    IF_ARG_NULL_GOTO_CLEANUP(load_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "load_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != load_size_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "load_size_")
    if(0 == vertex_buffer_->vbo_handle) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_vbo_vertex_load(%s) - Provided vertex_buffer is not valid.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    switch(usage_) {
    case BUFFER_USAGE_STATIC:
        mock_glBufferData(GL_ARRAY_BUFFER, load_size_, load_data_, GL_STATIC_DRAW);
        break;
    case BUFFER_USAGE_DYNAMIC:
        mock_glBufferData(GL_ARRAY_BUFFER, load_size_, load_data_, GL_DYNAMIC_DRAW);
        break;
    default:
        ERROR_MESSAGE("gl33_vbo_vertex_load(%s) - Provided usage_ is not valid.", renderer_rslt_to_str(RENDERER_RUNTIME_ERROR));
        ret = RENDERER_RUNTIME_ERROR;
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

static void NO_COVERAGE mock_glGenBuffers(GLsizei n_, GLuint* buffer_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGenBuffers.call_count++;
    if(s_test_config_mock_glGenBuffers.fail_on_call != 0) {
        if(s_test_config_mock_glGenBuffers.call_count == s_test_config_mock_glGenBuffers.fail_on_call) {
            return;
        }
    }
#endif
    glGenBuffers(n_, buffer_);
}

static void NO_COVERAGE mock_glBindBuffer(GLenum target_, GLuint buffer_) {
#ifdef TEST_BUILD
    s_test_config_mock_glBindBuffer.call_count++;
    if(s_test_config_mock_glBindBuffer.fail_on_call != 0) {
        if(s_test_config_mock_glBindBuffer.call_count == s_test_config_mock_glBindBuffer.fail_on_call) {
            return;
        }
    }
#endif
    glBindBuffer(target_, buffer_);
}

static void NO_COVERAGE mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_) {
#ifdef TEST_BUILD
    s_test_config_mock_glDeleteBuffers.call_count++;
    if(s_test_config_mock_glDeleteBuffers.fail_on_call != 0) {
        if(s_test_config_mock_glDeleteBuffers.call_count == s_test_config_mock_glDeleteBuffers.fail_on_call) {
            return;
        }
    }
#endif
    glDeleteBuffers(n_, buffer_);
}

static void NO_COVERAGE mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_) {
#ifdef TEST_BUILD
    s_test_config_mock_glBufferData.call_count++;
    if(s_test_config_mock_glBufferData.fail_on_call != 0) {
        if(s_test_config_mock_glBufferData.call_count == s_test_config_mock_glBufferData.fail_on_call) {
            return;
        }
    }
#endif
    glBufferData(target_, size_, data_, usage_);
}

#ifdef TEST_BUILD
void test_concrete_vbo_config_reset(void) {
    test_call_control_reset(&s_test_config_gl33_vbo_create);
    test_call_control_no_op_reset(&s_test_config_gl33_vbo_destroy);
    test_call_control_reset(&s_test_config_gl33_vbo_bind);
    test_call_control_reset(&s_test_config_gl33_vbo_unbind);
    test_call_control_reset(&s_test_config_gl33_vbo_vertex_load);
    test_call_control_no_op_reset(&s_test_config_mock_glGenBuffers);
    test_call_control_no_op_reset(&s_test_config_mock_glBindBuffer);
    test_call_control_no_op_reset(&s_test_config_mock_glDeleteBuffers);
    test_call_control_no_op_reset(&s_test_config_mock_glBufferData);
}

void test_concrete_vbo(void) {
    test_gl33_vbo_create();
    test_gl33_vbo_destroy();
    test_gl33_vbo_bind();
    test_gl33_vbo_unbind();
    test_gl33_vbo_vertex_load();
}

static void NO_COVERAGE test_gl33_vbo_create(void) {

}

static void NO_COVERAGE test_gl33_vbo_destroy(void) {

}

static void NO_COVERAGE test_gl33_vbo_bind(void) {

}

static void NO_COVERAGE test_gl33_vbo_unbind(void) {

}

static void NO_COVERAGE test_gl33_vbo_vertex_load(void) {

}

static void NO_COVERAGE test_call_control_no_op_reset(test_call_control_no_op_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
}
#endif
