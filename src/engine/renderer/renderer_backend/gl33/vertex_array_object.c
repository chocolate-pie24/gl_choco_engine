/** @ingroup gl33
 *
 * @file vertex_array_object.c
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

#include <GL/glew.h>

#include "engine/renderer/renderer_base/renderer_types.h"

#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_memory.h"

#include "engine/renderer/renderer_backend/gl33/vertex_array_object.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief VAOモジュール内部状態管理構造体
 *
 */
struct vertex_array_object {
    GLuint vao_handle;  /**< VAO */
};

static void mock_glGenVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glBindVertexArray(GLuint array_);
static void mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_);
static void mock_glEnableVertexAttribArray(GLuint index_);

#ifdef TEST_BUILD
#include <assert.h>
#include "engine/core/memory/choco_memory.h"

static bool s_vertex_array_object_test = false;

static void test_vertex_array_create(void);
static void test_vertex_array_destroy(void);
static void test_vertex_array_bind(void);
static void test_vertex_array_unbind(void);
static void test_vertex_array_attribute_set(void);

#endif

renderer_result_t vertex_array_create(vertex_array_object_t** vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    vertex_array_object_t* tmp = NULL;

    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_array_, RENDERER_INVALID_ARGUMENT, "vertex_array_create", "vertex_array_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, RENDERER_INVALID_ARGUMENT, "vertex_array_create", "vertex_array_")

    ret = render_mem_allocate(sizeof(vertex_array_object_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("vertex_array_create(%s) - Failed to allocate memory for 'tmp'.", renderer_result_to_str(ret));
        goto cleanup;
    }

    mock_glGenVertexArrays(1, &tmp->vao_handle);

    *vertex_array_ = tmp;
    ret = RENDERER_SUCCESS;

cleanup:
    // NOTE: 現状ではrender_mem_allocate以降で失敗することはないので、エラー発生かつtmpがNULL以外になることはないためコメントアウト
    // if(RENDERER_SUCCESS != ret) {
    //     if(NULL != tmp) {
    //         render_mem_free(tmp, sizeof(vertex_array_object_t));
    //         tmp = NULL;
    //     }
    // }
    return ret;
}

void vertex_array_destroy(vertex_array_object_t** vertex_array_) {
    if(NULL == vertex_array_) {
        goto cleanup;
    }
    if(NULL == *vertex_array_) {
        goto cleanup;
    }
    vertex_array_unbind();
    mock_glDeleteVertexArrays(1, &(*vertex_array_)->vao_handle);
    render_mem_free(*vertex_array_, sizeof(vertex_array_object_t));
    *vertex_array_ = NULL;
cleanup:
    return;
}

renderer_result_t vertex_array_bind(const vertex_array_object_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(vertex_array_, RENDERER_INVALID_ARGUMENT, "vertex_array_bind", "vertex_array_")

    mock_glBindVertexArray(vertex_array_->vao_handle);

    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

renderer_result_t vertex_array_unbind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    mock_glBindVertexArray(0);

    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

renderer_result_t vertex_array_attribute_set(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
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
void test_vertex_array_object(void) {
    s_vertex_array_object_test = true;
    memory_system_create();

    test_vertex_array_create();
    test_vertex_array_destroy();
    test_vertex_array_bind();
    test_vertex_array_unbind();
    test_vertex_array_attribute_set();

    memory_system_destroy();
    s_vertex_array_object_test = false;
}

static void NO_COVERAGE test_vertex_array_create(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        ret = vertex_array_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        vertex_array_object_t* tmp = NULL;
        render_mem_allocate(sizeof(vertex_array_object_t), (void**)&tmp);
        assert(NULL != tmp);
        ret = vertex_array_create(&tmp);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        render_mem_free(tmp, sizeof(vertex_array_object_t));
    }
    {
        memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);
        vertex_array_object_t* tmp = NULL;
        ret = vertex_array_create(&tmp);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == tmp);
        memory_system_test_param_reset();
    }
    {
        vertex_array_object_t* tmp = NULL;
        ret = vertex_array_create(&tmp);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != tmp);
        vertex_array_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_vertex_array_destroy(void) {
    {
        vertex_array_destroy(NULL);
    }
    {
        vertex_array_object_t* tmp = NULL;
        vertex_array_destroy(&tmp);
    }
    {
        vertex_array_object_t* tmp = NULL;
        renderer_result_t ret = vertex_array_create(&tmp);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != tmp);
        vertex_array_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_vertex_array_bind(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        ret = vertex_array_bind(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        vertex_array_object_t* tmp = NULL;
        ret = vertex_array_create(&tmp);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != tmp);
        ret = vertex_array_bind(tmp);
        assert(RENDERER_SUCCESS == ret);
        vertex_array_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_vertex_array_unbind(void) {
    renderer_result_t ret = vertex_array_unbind();
    assert(RENDERER_SUCCESS == ret);
}

static void NO_COVERAGE test_vertex_array_attribute_set(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        ret = vertex_array_attribute_set(0, 128, 100, false, 16, 0);
        assert(RENDERER_RUNTIME_ERROR == ret);
    }
    {
        ret = vertex_array_attribute_set(0, 128, RENDERER_TYPE_FLOAT, false, 16, 0);
        assert(RENDERER_SUCCESS == ret);
    }
    {
        ret = vertex_array_attribute_set(0, 128, RENDERER_TYPE_FLOAT, true, 16, 0);
        assert(RENDERER_SUCCESS == ret);
    }
}
#endif
