/** @ingroup gl33
 *
 * @file gl33_vao.c
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
#include "engine/renderer/renderer_backend/renderer_backend_interface/vertex_array_object.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vao.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief VAOモジュール内部状態管理構造体
 *
 */
struct renderer_backend_vao {
    GLuint vao_handle;  /**< VAO */
};

static renderer_result_t gl33_vao_create(renderer_backend_vao_t** vertex_array_);
static void gl33_vao_destroy(renderer_backend_vao_t** vertex_array_);
static renderer_result_t gl33_vao_bind(const renderer_backend_vao_t* vertex_array_);
static renderer_result_t gl33_vao_unbind(const renderer_backend_vao_t* vertex_array_);
static renderer_result_t gl33_vao_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

static void mock_glGenVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glBindVertexArray(GLuint array_);
static void mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_);
static void mock_glEnableVertexAttribArray(GLuint index_);

#ifdef TEST_BUILD
#include <assert.h>
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

    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_array_, RENDERER_INVALID_ARGUMENT, "gl33_vao_create", "vertex_array_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, RENDERER_INVALID_ARGUMENT, "gl33_vao_create", "vertex_array_")

    ret = render_mem_allocate(sizeof(renderer_backend_vao_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_vao_create(%s) - Failed to allocate memory for 'tmp'.", renderer_result_to_str(ret));
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
 * @param vertex_array_ Bind対象vao
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
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_がNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_bind(const renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_array_, RENDERER_INVALID_ARGUMENT, "gl33_vao_bind", "vertex_array_")

    mock_glBindVertexArray(vertex_array_->vao_handle);

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
 * gl33_vao_unbind();
 * @endcode
 *
 * @param vertex_array_ VAOハンドル(OpenGL3.3では使用しない)
 *
 * @retval RENDERER_SUCCESS 現状では内部で呼び出すglBindVertexArrayに対して個別にglGetErrorを行わないため、常に成功
 */
static renderer_result_t gl33_vao_unbind(const renderer_backend_vao_t* vertex_array_) {
    (void)vertex_array_;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    mock_glBindVertexArray(0);

    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

/**
 * @brief glVertexAttribPointerのラッパーAPIであり、layout設定の後に設定の有効化(glEnableVertexAttribArray)を行う
 *
 * @param vertex_array_ VAOハンドル(OpenGL3.3では使用しない)
 * @param layout_ シェーダープログラム内のどのバッファ変数の設定値かを指定
 * @param size_ 頂点情報に含まれるデータの数([x, y, z]の3次元座標のみであれば3)
 * @param type_ バッファに格納されているデータの型 @ref renderer_type_t
 * @param normalized_ 与えられた頂点データを正規化するかどうかを指定
 * @param stride_ 頂点情報1つあたりのサイズを指定(GLfloat型の[x, y, z]であれば、sizeof(GLfloat) x 3を指定)
 * @param offset_ 「この頂点属性の先頭が、現在GL_ARRAY_BUFFERにバインドされているバッファの先頭から何バイト目にあるか」を指定
 *
 * 使用例:
 * @code{.c}
 * static const GLfloat vertex_buffer_data[] = {
 * -1.0f, -1.0f, 0.0f,
 * 1.0f, -1.0f, 0.0f,
 * 0.0f,  1.0f, 0.0f,
 * };
 *
 * glVertexAttribPointer(
 * 0,
 * 3,
 * GL_FLOAT,
 * GL_FALSE,
 * sizeof(GLfloat) * 3,
 * (void*)0
 * );
 * @endcode
 *
 * @retval RENDERER_RUNTIME_ERROR type_の値が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    (void)vertex_array_;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
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
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     ret = gl33_vao_create(NULL);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    // }
    // {
    //     renderer_backend_vao_t* tmp = NULL;
    //     assert(RENDERER_SUCCESS == render_mem_allocate(sizeof(renderer_backend_vao_t), (void**)&tmp));
    //     assert(NULL != tmp);
    //     ret = gl33_vao_create(&tmp);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    //     render_mem_free(tmp, sizeof(renderer_backend_vao_t));
    // }
    // {
    //     memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);
    //     renderer_backend_vao_t* tmp = NULL;
    //     ret = gl33_vao_create(&tmp);
    //     assert(RENDERER_NO_MEMORY == ret);
    //     assert(NULL == tmp);
    //     memory_system_test_param_reset();
    // }
    // {
    //     renderer_backend_vao_t* tmp = NULL;
    //     ret = gl33_vao_create(&tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != tmp);
    //     gl33_vao_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
}

static void NO_COVERAGE test_gl33_vao_destroy(void) {
    // {
    //     gl33_vao_destroy(NULL);
    // }
    // {
    //     renderer_backend_vao_t* tmp = NULL;
    //     gl33_vao_destroy(&tmp);
    // }
    // {
    //     renderer_backend_vao_t* tmp = NULL;
    //     renderer_result_t ret = gl33_vao_create(&tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != tmp);
    //     gl33_vao_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
}

static void NO_COVERAGE test_gl33_vao_bind(void) {
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     ret = gl33_vao_bind(NULL);
    //     assert(RENDERER_INVALID_ARGUMENT == ret);
    // }
    // {
    //     renderer_backend_vao_t* tmp = NULL;
    //     ret = gl33_vao_create(&tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     assert(NULL != tmp);
    //     ret = gl33_vao_bind(tmp);
    //     assert(RENDERER_SUCCESS == ret);
    //     gl33_vao_destroy(&tmp);
    //     assert(NULL == tmp);
    // }
}

static void NO_COVERAGE test_gl33_vao_unbind(void) {
    // renderer_result_t ret = gl33_vao_unbind();
    // assert(RENDERER_SUCCESS == ret);
}

static void NO_COVERAGE test_gl33_vao_attribute_set(void) {
    // renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    // {
    //     ret = gl33_vao_attribute_set(0, 128, 100, false, 16, 0);
    //     assert(RENDERER_RUNTIME_ERROR == ret);
    // }
    // {
    //     ret = gl33_vao_attribute_set(0, 128, RENDERER_TYPE_FLOAT, false, 16, 0);
    //     assert(RENDERER_SUCCESS == ret);
    // }
    // {
    //     ret = gl33_vao_attribute_set(0, 128, RENDERER_TYPE_FLOAT, true, 16, 0);
    //     assert(RENDERER_SUCCESS == ret);
    // }
}
#endif
