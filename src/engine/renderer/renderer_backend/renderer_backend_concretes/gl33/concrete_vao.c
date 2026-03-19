/** @ingroup gl33
 *
 * @file concrete_vao.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VAOを使用するためのラッパーAPIの実装
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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

#include <GL/glew.h>

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_memory.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vao.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

/**
 * @brief VAOモジュール内部状態管理構造体
 *
 */
struct renderer_backend_vao {
    GLuint vao_handle;  /**< VAO */
};

static renderer_result_t gl33_vao_create(renderer_backend_vao_t** vertex_array_);
static void gl33_vao_destroy(renderer_backend_vao_t** vertex_array_);
static renderer_result_t gl33_vao_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_);
static renderer_result_t gl33_vao_unbind(const renderer_backend_vao_t* vertex_array_);
static renderer_result_t gl33_vao_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

static void mock_glGenVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glBindVertexArray(GLuint array_);
static void mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_);
static void mock_glEnableVertexAttribArray(GLuint index_);

#ifdef TEST_BUILD
#include <assert.h>
#include <stdlib.h>
#include "engine/core/memory/choco_memory.h"

static bool s_vertex_array_object_test = false;

static void test_gl33_vao_create(void);
static void test_gl33_vao_destroy(void);
static void test_gl33_vao_bind(void);
static void test_gl33_vao_unbind(void);
static void test_gl33_vao_attribute_set(void);

#endif

static const renderer_vao_vtable_t s_gl33_vao_vtable = {
    .vertex_array_create = gl33_vao_create,
    .vertex_array_destroy = gl33_vao_destroy,
    .vertex_array_bind = gl33_vao_bind,
    .vertex_array_unbind = gl33_vao_unbind,
    .vertex_array_attribute_set = gl33_vao_attribute_set,
};

const renderer_vao_vtable_t* gl33_vao_vtable_get(void) {
    return &s_gl33_vao_vtable;
}

/**
 * @brief VAO構造体インスタンスのメモリを確保し初期化する
 *
 * @param vertex_array_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vao_t* vao = NULL;
 * renderer_result_t ret = gl33_vao_create(&vao);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_がNULL
 * - *vertex_array_が非NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_create(renderer_backend_vao_t** vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_vao_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_create", "vertex_array_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_create", "vertex_array_")

    ret = render_mem_allocate(sizeof(renderer_backend_vao_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_vao_create(%s) - Failed to allocate memory for 'tmp'.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glGenVertexArrays(1, &tmp->vao_handle);

    *vertex_array_ = tmp;
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
 * @brief renderer_backend_vao_t構造体インスタンスのメモリを解放し、OpenGLContext内のVAOも削除する
 *
 * @param vertex_array_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vao_t* vao = NULL;
 * renderer_result_t ret = gl33_vao_create(&vao);
 * // エラー処理
 * gl33_vao_destroy(&vao);
 * @endcode
 */
static void gl33_vao_destroy(renderer_backend_vao_t** vertex_array_) {
    if(NULL == vertex_array_) {
        goto cleanup;
    }
    if(NULL == *vertex_array_) {
        goto cleanup;
    }

    // NOTE: 現状ではvertex_buffer_unbindはエラーを返さないため、このif文内は到達不可でカバレッジは100にはならないが許容する
    if(RENDERER_SUCCESS != gl33_vao_unbind(*vertex_array_)) {
        WARN_MESSAGE("gl33_vao_destroy(RUNTIME_ERROR) - Failed to unbind vertex array.");
    }
    mock_glDeleteVertexArrays(1, &(*vertex_array_)->vao_handle);
    render_mem_free(*vertex_array_, sizeof(renderer_backend_vao_t));
    *vertex_array_ = NULL;
cleanup:
    return;
}

/**
 * @brief glBindVertexArray APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param[in] vertex_array_ bind対象vao
 * @param[in,out] out_vao_id_ bindされたvao id格納先
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vao_t* vao = NULL;
 * renderer_result_t ret = gl33_vao_create(&vao);
 * // エラー処理
 * ret = gl33_vao_bind(vao);
 * // エラー処理
 * @endcode
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_ == NULL
 * - out_vao_id_ == NULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_bind", "vertex_array_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vao_id_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_bind", "out_vao_id_")
    if(*out_vao_id_ != vertex_array_->vao_handle) {
        mock_glBindVertexArray(vertex_array_->vao_handle);
        *out_vao_id_ = vertex_array_->vao_handle;
    }

    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

/**
 * @brief VAOアンバインド処理
 *
 * 使用例:
 * @code{.c}
 * renderer_backend_vao_t* vao = NULL;
 * renderer_result_t ret = gl33_vao_create(&vao);
 * // エラー処理
 * ret = gl33_vao_bind(vao);
 * // エラー処理
 * gl33_vao_unbind(vao);
 * @endcode
 *
 * @param vertex_array_ VAOハンドル(OpenGL3.3では使用しない)
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_ == NULL
 * @retval RENDERER_SUCCESS 現状では内部で呼び出すglBindVertexArrayに対して個別にglGetErrorを行わないため、常に成功
 */
static renderer_result_t gl33_vao_unbind(const renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_unbind", "vertex_array_")

    mock_glBindVertexArray(0);

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

static renderer_result_t gl33_vao_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_attribute_set", "vertex_array_")

    switch(type_) {
    case RENDERER_TYPE_FLOAT:
        mock_glVertexAttribPointer(layout_, size_, GL_FLOAT, normalized_ ? GL_TRUE : GL_FALSE, stride_, (void*)offset_);
        break;
    default:
        ret = RENDERER_RUNTIME_ERROR;
        goto cleanup;
    }
    mock_glEnableVertexAttribArray(layout_);
    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

static void NO_COVERAGE mock_glGenVertexArrays(GLsizei n_, GLuint* array_) {
#ifdef TEST_BUILD
    if(s_vertex_array_object_test) {
        return;
    } else {
        glGenVertexArrays(n_, array_);
    }
#else
    glGenVertexArrays(n_, array_);
#endif
}

static void NO_COVERAGE mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_) {
#ifdef TEST_BUILD
    if(s_vertex_array_object_test) {
        return;
    } else {
        glDeleteVertexArrays(n_, array_);
    }
#else
    glDeleteVertexArrays(n_, array_);
#endif
}

static void NO_COVERAGE mock_glBindVertexArray(GLuint array_) {
#ifdef TEST_BUILD
    if(s_vertex_array_object_test) {
        return;
    } else {
        glBindVertexArray(array_);
    }
#else
    glBindVertexArray(array_);
#endif
}

static void NO_COVERAGE mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_) {
#ifdef TEST_BUILD
    if(s_vertex_array_object_test) {
        return;
    } else {
        glVertexAttribPointer(index_, size_, type_, normalized_, stride_, pointer_);
    }
#else
    glVertexAttribPointer(index_, size_, type_, normalized_, stride_, pointer_);
#endif
}

static void NO_COVERAGE mock_glEnableVertexAttribArray(GLuint index_) {
#ifdef TEST_BUILD
    if(s_vertex_array_object_test) {
        return;
    } else {
        glEnableVertexAttribArray(index_);
    }
#else
    glEnableVertexAttribArray(index_);
#endif
}

#ifdef TEST_BUILD
void test_gl33_vao(void) {
    s_vertex_array_object_test = true;
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_gl33_vao_create();
    test_gl33_vao_destroy();
    test_gl33_vao_bind();
    test_gl33_vao_unbind();
    test_gl33_vao_attribute_set();

    memory_system_destroy();
    s_vertex_array_object_test = false;
}

static void NO_COVERAGE test_gl33_vao_create(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    s_vertex_array_object_test = true;  // OpenGL未初期化でもテストできるようOpenGL関数をスルーさせる
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        ret = gl33_vao_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // *vertex_array != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_vao_t* vertex_array = NULL;
        vertex_array = malloc(sizeof(renderer_backend_vao_t));
        assert(NULL != vertex_array);

        ret = gl33_vao_create(&vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        free(vertex_array);
        vertex_array = NULL;
    }
    {
        // render_mem_allocateがRENDERER_LIMIT_EXCEEDEDでRENDERER_LIMIT_EXCEEDED, vertex_arrayはNULLのまま
        // memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        // renderer_backend_vao_t* vertex_array = NULL;
        // ret = gl33_vao_create(&vertex_array);
        // assert(RENDERER_LIMIT_EXCEEDED == ret);
        // assert(NULL == vertex_array);

        // memory_system_test_param_reset();
    }
    {
        // 正常系
        renderer_backend_vao_t* vertex_array = NULL;
        ret = gl33_vao_create(&vertex_array);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vertex_array);

        gl33_vao_destroy(&vertex_array);
        assert(NULL == vertex_array);
    }
    s_vertex_array_object_test = false;
}

static void NO_COVERAGE test_gl33_vao_destroy(void) {
    s_vertex_array_object_test = true;  // OpenGL未初期化でもテストできるようOpenGL関数をスルーさせる
    {
        // vertex_array_ == NULL -> No-op
        renderer_backend_vao_t* vao = NULL;
        gl33_vao_destroy(&vao);
    }
    {
        // vertex_array_ == NULL -> No-op
        gl33_vao_destroy(NULL);
    }
    {
        // 正常系
        renderer_backend_vao_t* vao = NULL;
        renderer_result_t ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    s_vertex_array_object_test = false;
}


static void NO_COVERAGE test_gl33_vao_bind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    s_vertex_array_object_test = true;  // OpenGL未初期化でもテストできるようOpenGL関数をスルーさせる
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        uint32_t id = 0;
        ret = gl33_vao_bind(NULL, &id);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // out_vao_id_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        ret = gl33_vao_bind(vao, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    {
        // *out_vao_id_ == vertex_array_->vao_handle -> RENDERER_SUCCESS
        // NOTE: if文を通らないことを確認する
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        vao->vao_handle = 1;
        uint32_t id = 1;

        ret = gl33_vao_bind(vao, &id);
        assert(RENDERER_SUCCESS == ret);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    {
        // *out_vao_id_ != vertex_array_->vao_handle -> RENDERER_SUCCESS
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);
        vao->vao_handle = 5;
        uint32_t id = 1;
        assert(vao->vao_handle != id);

        ret = gl33_vao_bind(vao, &id);
        assert(RENDERER_SUCCESS == ret);
        assert(vao->vao_handle == id);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    s_vertex_array_object_test = false;
}

static void NO_COVERAGE test_gl33_vao_unbind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    s_vertex_array_object_test = true;
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        ret = gl33_vao_unbind(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 正常系
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        ret = gl33_vao_unbind(vao);
        assert(RENDERER_SUCCESS == ret);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    s_vertex_array_object_test = false;
}

static void NO_COVERAGE test_gl33_vao_attribute_set(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    s_vertex_array_object_test = true;
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        ret = gl33_vao_attribute_set(NULL, 4, 8, RENDERER_TYPE_FLOAT, true, 8, 8);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // type_が既定値外 -> RENDERER_RUNTIME_ERROR
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        ret = gl33_vao_attribute_set(vao, 4, 8, 100, true, 8, 8);
        assert(RENDERER_RUNTIME_ERROR == ret);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }
    {
        // 正常系
        renderer_backend_vao_t* vao = NULL;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        ret = gl33_vao_attribute_set(vao, 4, 8, RENDERER_TYPE_FLOAT, true, 8, 8);
        assert(RENDERER_SUCCESS == ret);

        gl33_vao_destroy(&vao);
        assert(NULL == vao);
    }

    s_vertex_array_object_test = false;
}
#endif
