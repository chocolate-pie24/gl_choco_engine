/** @ingroup gl33
 *
 * @file gl33_vbo.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VBOを使用するためのラッパーAPIの実装
 *
 * @version 0.1
 * @date 2025-12.19
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
#include "engine/renderer/renderer_backend/renderer_backend_interface/vertex_buffer_object.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vbo.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

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

#ifdef TEST_BUILD
#include <assert.h>
#include "engine/core/memory/choco_memory.h"

static bool s_vertex_buffer_object_test = false;    /**< gl33_vbo API単体テスト有効フラグ */

static void test_gl33_vbo_create(void);
static void test_gl33_vbo_destroy(void);
static void test_gl33_vbo_bind(void);
static void test_gl33_vbo_unbind(void);
static void test_gl33_vbo_vertex_load(void);
#endif

static const renderer_vbo_vtable_t s_gl33_vbo_vtable = {
    .vertex_buffer_create = gl33_vbo_create,
    .vertex_buffer_destroy = gl33_vbo_destroy,
    .vertex_buffer_bind = gl33_vbo_bind,
    .vertex_buffer_unbind = gl33_vbo_unbind,
    .vertex_buffer_vertex_load = gl33_vbo_vertex_load,
};

const renderer_vbo_vtable_t* gl33_vbo_vtable_get(void) {
    return &s_gl33_vbo_vtable;
}

/**
 * @brief VBO構造体インスタンスのメモリを確保し初期化する
 *
 * @param vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
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
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_vbo_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_create", "vertex_buffer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_create", "vertex_buffer_")

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
 * @param vertex_buffer_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
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
 * @retval RENDERER_INVALID_ARGUMENT vertex_buffer_がNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vbo_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_bind", "vertex_buffer_")

    if(vertex_buffer_->vbo_handle != *out_vbo_id_) {
        mock_glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_->vbo_handle);
        *out_vbo_id_ = vertex_buffer_->vbo_handle;
    }

    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

/**
 * @brief VBOアンバインド処理
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vbo_t* vbo = NULL;
 * renderer_result_t ret = gl33_vbo_create(&vbo);
 * // エラー処理
 * ret = gl33_vbo_bind(vbo);
 * // エラー処理
 * gl33_vbo_unbind(vbo);
 * @endcode
 *
 * @param vertex_buffer_ VBOハンドル(OpenGL3.3では使用しない)
 *
 * @retval RENDERER_SUCCESS 現状では内部で呼び出すglBindBufferに対して個別にglGetErrorを行わないため、常に成功
 */
static renderer_result_t gl33_vbo_unbind(const renderer_backend_vbo_t* vertex_buffer_) {
    (void)vertex_buffer_;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    mock_glBindBuffer(GL_ARRAY_BUFFER, 0);

    ret = RENDERER_SUCCESS;

    return ret;
}

/**
 * @brief VBOで管理する頂点情報をGPUへ転送する(glBufferData APIのラッパーAPI)
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 * @note 現在バインド中のVBOに紐づくバッファへ転送されるため、事前に @ref gl33_vbo_bind によってバインドしておくこと
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vbo_t* vbo = NULL;
 * renderer_result_t ret = gl33_vbo_create(&vbo);
 * // エラー処理
 * ret = gl33_vbo_bind(vbo);
 * float data[] = { 1.0f, 2.0f, 3.0f };
 * renderer_result_t ret = gl33_vbo_vertex_load(vbo, sizeof(float) * 3, data, BUFFER_USAGE_STATIC);
 * // エラー処理
 * @endcode
 *
 * @param vertex_buffer_ VBOハンドル(OpenGL3.3では使用しない)
 * @param load_size_ 転送サイズ(byte)
 * @param load_data_ 転送データの先頭アドレス
 * @param usage_ バッファデータの取り扱い @ref buffer_usage_t
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - load_data_ == NULL
 * - load_size_ == 0
 * @retval RENDERER_RUNTIME_ERROR usage_の値が規定範囲外
 * @retval RENDERER_SUCCESS データの転送に成功し、正常終了
 */
static renderer_result_t gl33_vbo_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(load_data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "load_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != load_size_, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "load_size_")

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
    if(s_vertex_buffer_object_test) {
        return;
    } else {
        glGenBuffers(n_, buffer_);
    }
#else
    glGenBuffers(n_, buffer_);
#endif
}

static void NO_COVERAGE mock_glBindBuffer(GLenum target_, GLuint buffer_) {
#ifdef TEST_BUILD
    if(s_vertex_buffer_object_test) {
        return;
    } else {
        glBindBuffer(target_, buffer_);
    }
#else
    glBindBuffer(target_, buffer_);
#endif
}

static void NO_COVERAGE mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_) {
#ifdef TEST_BUILD
    if(s_vertex_buffer_object_test) {
        return;
    } else {
        glDeleteBuffers(n_, buffer_);
    }
#else
    glDeleteBuffers(n_, buffer_);
#endif
}

static void NO_COVERAGE mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_) {
#ifdef TEST_BUILD
    if(s_vertex_buffer_object_test) {
        return;
    } else {
        glBufferData(target_, size_, data_, usage_);
    }
#else
    glBufferData(target_, size_, data_, usage_);
#endif
}

#ifdef TEST_BUILD
void test_gl33_vbo(void) {
    s_vertex_buffer_object_test = true;
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_gl33_vbo_create();
    test_gl33_vbo_destroy();
    test_gl33_vbo_bind();
    test_gl33_vbo_unbind();
    test_gl33_vbo_vertex_load();

    memory_system_destroy();
    s_vertex_buffer_object_test = false;
}

static void NO_COVERAGE test_gl33_vbo_create(void) {
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     ret = gl33_vbo_create(NULL);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    // }
    // {
    //     renderer_backend_vbo_t* test_vbo = NULL;
    //     assert(RENDERER_SUCCESS == render_mem_allocate(sizeof(renderer_backend_vbo_t), (void**)&test_vbo));
    //     assert(NULL != test_vbo);

    //     ret = gl33_vbo_create(&test_vbo);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    //     assert(NULL != test_vbo);

    //     render_mem_free((void*)test_vbo, sizeof(renderer_backend_vbo_t));
    // }
    // {
    //     renderer_backend_vbo_t* test_vbo = NULL;
    //     render_mem_test_param_set(RENDERER_NO_MEMORY);
    //     ret = gl33_vbo_create(&test_vbo);
    //     assert(RENDERER_NO_MEMORY == ret);
    //     assert(NULL == test_vbo);
    //     render_mem_test_param_reset();
    // }
    // {
    //     renderer_backend_vbo_t* test_vbo = NULL;
    //     ret = gl33_vbo_create(&test_vbo);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != test_vbo);
    //     gl33_vbo_destroy(&test_vbo);
    // }
}

static void NO_COVERAGE test_gl33_vbo_destroy(void) {
    // {
    //     gl33_vbo_destroy(NULL);
    // }
    // {
    //     renderer_backend_vbo_t* tmp = NULL;
    //     gl33_vbo_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
    // {
    //     renderer_backend_vbo_t* tmp = NULL;
    //     renderer_result_t ret = gl33_vbo_create(&tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != tmp);
    //     gl33_vbo_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
}

static void NO_COVERAGE test_gl33_vbo_bind(void) {
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     ret = gl33_vbo_bind(NULL);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    // }
    // {
    //     renderer_backend_vbo_t* tmp = NULL;
    //     ret = gl33_vbo_create(&tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != tmp);
    //     ret = gl33_vbo_bind(tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     gl33_vbo_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
}

static void NO_COVERAGE test_gl33_vbo_unbind(void) {
    // assert(RENDERER_SUCCESS == gl33_vbo_unbind());
}

static void NO_COVERAGE test_gl33_vbo_vertex_load(void) {
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     char* data = NULL;
    //     ret = render_mem_allocate(128, (void**)&data);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != data);
    //     ret = gl33_vbo_vertex_load(0, data, BUFFER_USAGE_STATIC);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    //     render_mem_free(data, 128);
    // }
    // {
    //     ret = gl33_vbo_vertex_load(128, NULL, BUFFER_USAGE_STATIC);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    // }
    // {
    //     char* data = NULL;
    //     ret = render_mem_allocate(128, (void**)&data);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != data);
    //     ret = gl33_vbo_vertex_load(128, data, 100);
    //     assert(RENDERER_RUNTIME_ERROR == ret);
    //     render_mem_free(data, 128);
    // }
    // {
    //     char* data = NULL;
    //     ret = render_mem_allocate(128, (void**)&data);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != data);
    //     ret = gl33_vbo_vertex_load(128, data, BUFFER_USAGE_STATIC);
    //     assert(RENDERER_SUCCESS == ret);
    //     render_mem_free(data, 128);
    // }
    // {
    //     char* data = NULL;
    //     ret = render_mem_allocate(128, (void**)&data);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != data);
    //     ret = gl33_vbo_vertex_load(128, data, BUFFER_USAGE_DYNAMIC);
    //     assert(RENDERER_SUCCESS == ret);
    //     render_mem_free(data, 128);
    // }
}

#endif
