/** @ingroup gl33
 *
 * @file vertex_buffer_object.c
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

#include <GL/glew.h>

#include "engine/renderer/renderer_base/renderer_types.h"

#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_memory.h"

#include "engine/renderer/renderer_backend/gl33/vertex_buffer_object.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief VBOモジュール内部状態管理構造体
 *
 */
struct vertex_buffer_object {
    GLuint vbo_handle;  /**< VBO */
};

static void mock_glGenBuffers(GLsizei n_, GLuint* buffer_);
static void mock_glBindBuffer(GLenum target_, GLuint buffer_);
static void mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_);
static void mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_);

#ifdef TEST_BUILD
#include <assert.h>
#include "engine/core/memory/choco_memory.h"

static bool s_vertex_buffer_object_test = false;    /**< vertex_buffer_object API単体テスト有効フラグ */

static void test_vertex_buffer_create(void);
static void test_vertex_buffer_destroy(void);
static void test_vertex_buffer_bind(void);
static void test_vertex_buffer_unbind(void);
static void test_vertex_buffer_vertex_load(void);
#endif

renderer_result_t vertex_buffer_create(vertex_buffer_object_t** vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    vertex_buffer_object_t* tmp = NULL;

    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, RENDERER_INVALID_ARGUMENT, "vertex_buffer_create", "vertex_buffer_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, RENDERER_INVALID_ARGUMENT, "vertex_buffer_create", "vertex_buffer_")

    ret = render_mem_allocate(sizeof(vertex_buffer_object_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("vertex_buffer_create(%s) - Failed to allocate memory for 'tmp'.", renderer_result_to_str(ret));
        goto cleanup;
    }

    mock_glGenBuffers(1, &tmp->vbo_handle);

    *vertex_buffer_ = tmp;
    ret = RENDERER_SUCCESS;

cleanup:

    // NOTE: 現状ではrender_mem_allocate以降で失敗することはないので、エラー発生かつtmpがNULL以外になることはないためコメントアウト
    // if(RENDERER_SUCCESS != ret) {
    //     if(NULL != tmp) {
    //         render_mem_free(tmp, sizeof(vertex_buffer_object_t));
    //         tmp = NULL;
    //     }
    // }

    return ret;
}

void vertex_buffer_destroy(vertex_buffer_object_t** vertex_buffer_) {
    if(NULL == vertex_buffer_) {
        goto cleanup;
    }
    if(NULL == *vertex_buffer_) {
        goto cleanup;
    }
    vertex_buffer_unbind();
    mock_glDeleteBuffers(1, &(*vertex_buffer_)->vbo_handle);
    render_mem_free(*vertex_buffer_, sizeof(vertex_buffer_object_t));
    *vertex_buffer_ = NULL;
cleanup:
    return;
}

renderer_result_t vertex_buffer_bind(const vertex_buffer_object_t* vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, RENDERER_INVALID_ARGUMENT, "vertex_buffer_bind", "vertex_buffer_")

    mock_glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_->vbo_handle);
    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

renderer_result_t vertex_buffer_unbind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    mock_glBindBuffer(GL_ARRAY_BUFFER, 0);

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

renderer_result_t vertex_buffer_vertex_load(size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(load_data_, RENDERER_INVALID_ARGUMENT, "vertex_buffer_vertex_load", "load_data_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != load_size_, RENDERER_INVALID_ARGUMENT, "vertex_buffer_vertex_load", "load_size_")

    switch(usage_) {
    case BUFFER_USAGE_STATIC:
        mock_glBufferData(GL_ARRAY_BUFFER, load_size_, load_data_, GL_STATIC_DRAW);
        break;
    case BUFFER_USAGE_DYNAMIC:
        mock_glBufferData(GL_ARRAY_BUFFER, load_size_, load_data_, GL_DYNAMIC_DRAW);
        break;
    default:
        ERROR_MESSAGE("vertex_buffer_vertex_load(%s) - Provided usage_ is not valid.", renderer_result_to_str(RENDERER_RUNTIME_ERROR));
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
        glBindBuffer(target_, buffer_);;
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
void test_vertex_buffer_object(void) {
    s_vertex_buffer_object_test = true;
    memory_system_create();

    test_vertex_buffer_create();
    test_vertex_buffer_destroy();
    test_vertex_buffer_bind();
    test_vertex_buffer_unbind();
    test_vertex_buffer_vertex_load();

    memory_system_destroy();
    s_vertex_buffer_object_test = false;
}

static void NO_COVERAGE test_vertex_buffer_create(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        ret = vertex_buffer_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        vertex_buffer_object_t* test_vbo = NULL;
        render_mem_allocate(sizeof(vertex_buffer_object_t), (void**)&test_vbo);
        assert(NULL != test_vbo);

        ret = vertex_buffer_create(&test_vbo);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL != test_vbo);

        render_mem_free((void*)test_vbo, sizeof(vertex_buffer_object_t));
    }
    {
        vertex_buffer_object_t* test_vbo = NULL;
        render_mem_test_param_set(RENDERER_NO_MEMORY);
        ret = vertex_buffer_create(&test_vbo);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == test_vbo);
        render_mem_test_param_reset();
    }
    {
        vertex_buffer_object_t* test_vbo = NULL;
        ret = vertex_buffer_create(&test_vbo);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != test_vbo);
        vertex_buffer_destroy(&test_vbo);
    }
}

static void NO_COVERAGE test_vertex_buffer_destroy(void) {
    {
        vertex_buffer_destroy(NULL);
    }
    {
        vertex_buffer_object_t* tmp = NULL;
        vertex_buffer_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        vertex_buffer_object_t* tmp = NULL;
        renderer_result_t ret = vertex_buffer_create(&tmp);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != tmp);
        vertex_buffer_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_vertex_buffer_bind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        ret = vertex_buffer_bind(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        vertex_buffer_object_t* tmp = NULL;
        ret = vertex_buffer_create(&tmp);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != tmp);
        ret = vertex_buffer_bind(tmp);
        assert(RENDERER_SUCCESS == ret);
        vertex_buffer_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_vertex_buffer_unbind(void) {
    vertex_buffer_unbind();
}

static void NO_COVERAGE test_vertex_buffer_vertex_load(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        char* data = NULL;
        ret = render_mem_allocate(128, (void**)&data);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != data);
        ret = vertex_buffer_vertex_load(0, data, BUFFER_USAGE_STATIC);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        render_mem_free(data, 128);
    }
    {
        ret = vertex_buffer_vertex_load(128, NULL, BUFFER_USAGE_STATIC);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        char* data = NULL;
        ret = render_mem_allocate(128, (void**)&data);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != data);
        ret = vertex_buffer_vertex_load(128, data, 100);
        assert(RENDERER_RUNTIME_ERROR == ret);
        render_mem_free(data, 128);
    }
    {
        char* data = NULL;
        ret = render_mem_allocate(128, (void**)&data);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != data);
        ret = vertex_buffer_vertex_load(128, data, BUFFER_USAGE_STATIC);
        assert(RENDERER_SUCCESS == ret);
        render_mem_free(data, 128);
    }
    {
        char* data = NULL;
        ret = render_mem_allocate(128, (void**)&data);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != data);
        ret = vertex_buffer_vertex_load(128, data, BUFFER_USAGE_DYNAMIC);
        assert(RENDERER_SUCCESS == ret);
        render_mem_free(data, 128);
    }
}

#endif
