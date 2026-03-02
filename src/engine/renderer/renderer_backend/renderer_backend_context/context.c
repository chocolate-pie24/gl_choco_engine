#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_vbo.h"

#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vbo.h"

#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vbo.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#define TEST_BUILD

/**
 * @brief RendererBackend内部状態管理構造体
 *
 */
struct renderer_backend_context {
    target_graphics_api_t target_api;               /**< 使用グラフィックスAPI */

    const renderer_shader_vtable_t* shader_vtable;  /**< シェーダー機能提供vtable */
    const renderer_vao_vtable_t* vao_vtable;        /**< VAO機能提供vtable */
    const renderer_vbo_vtable_t* vbo_vtable;        /**< VBO機能提供vtable */

    uint32_t current_program_id;                    /**< 現在使用中のリンクされたシェーダープログラムID */
    uint32_t current_bound_vao;                     /**< 現在バインド中のVAO識別子 */
    uint32_t current_bound_vbo;                     /**< 現在バインド中のVBO識別子 */
};

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_);
static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_);
static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_);

static bool graphics_api_valid_check(target_graphics_api_t target_api_);

#ifdef TEST_BUILD
#include <assert.h>
#include <stdlib.h>

typedef struct fail_injection {
    bool use_test_vtable;                               /**< テスト用vtable使用フラグ */
    renderer_result_t rslt_renderer_shader_create;      /**< TEST_BUILDかつ、use_test_vtable == trueで、renderer_shader_create()に強制的出力させる実行結果コード */
    renderer_result_t rslt_renderer_shader_compile;     /**< TEST_BUILDかつ、use_test_vtable == trueで、renderer_shader_compile()に強制的出力させる実行結果コード */
    renderer_result_t rslt_renderer_shader_link;        /**< TEST_BUILDかつ、use_test_vtable == trueで、renderer_shader_link()に強制的出力させる実行結果コード */
    renderer_result_t rslt_renderer_shader_use;         /**< TEST_BUILDかつ、use_test_vtable == trueで、renderer_shader_use()に強制的出力させる実行結果コード */

    renderer_result_t rslt_vertex_array_create;         /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_array_create()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_array_bind;           /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_array_bind()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_array_unbind;         /**< TEST_BUILDかつ、use_test_vtable == trueで、rslt_vertex_array_unbind()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_array_attribute_set;  /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_array_attribute_set()に強制的出力させる実行結果コード */

    renderer_result_t rslt_vertex_buffer_create;        /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_buffer_create()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_buffer_bind;          /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_buffer_bind()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_buffer_unbind;        /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_buffer_unbind()に強制的出力させる実行結果コード */
    renderer_result_t rslt_vertex_buffer_vertex_load;   /**< TEST_BUILDかつ、use_test_vtable == trueで、vertex_buffer_vertex_load()に強制的出力させる実行結果コード */
} fail_injection_t;

static fail_injection_t s_fail_injection;

static void test_linear_allocator_create(linear_alloc_t** allocator_, void** out_memory_pool_, size_t pool_size_);
static void test_linear_allocator_destroy(linear_alloc_t** allocator_, void** memory_pool_);
static void test_renderer_backend_initialize(void);
static void test_renderer_backend_destroy(void);
static void test_renderer_backend_shader_create(void);
static void test_renderer_backend_shader_destroy(void);
static void test_renderer_backend_shader_compile(void);
static void test_renderer_backend_shader_link(void);
static void test_renderer_backend_shader_use(void);
static void test_renderer_backend_vertex_array_create(void);
static void test_renderer_backend_vertex_array_destroy(void);
static void test_renderer_backend_vertex_array_bind(void);
static void test_renderer_backend_vertex_array_unbind(void);
static void test_renderer_backend_vertex_array_attribute_set(void);
static void test_renderer_backend_vertex_buffer_create(void);
static void test_renderer_backend_vertex_buffer_destroy(void);
static void test_renderer_backend_vertex_buffer_bind(void);
static void test_renderer_backend_vertex_buffer_unbind(void);
static void test_renderer_backend_vertex_buffer_vertex_load(void);
static void test_graphics_api_valid_check(void);

// shader vtable関数
static renderer_result_t test_renderer_shader_create(renderer_backend_shader_t** shader_handle_);
static void test_renderer_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_result_t test_renderer_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);

// vao vtable関数
static renderer_result_t test_vertex_array_create(renderer_backend_vao_t** vertex_array_);
static void test_vertex_array_destroy(renderer_backend_vao_t** vertex_array_);
static renderer_result_t test_vertex_array_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_);
static renderer_result_t test_vertex_array_unbind(const renderer_backend_vao_t* vertex_array_);
static renderer_result_t test_vertex_array_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

// vbo vtable関数
static renderer_result_t test_vertex_buffer_create(renderer_backend_vbo_t** vertex_buffer_);
static void test_vertex_buffer_destroy(renderer_backend_vbo_t** vertex_buffer_);
static renderer_result_t test_vertex_buffer_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_);
static renderer_result_t test_vertex_buffer_unbind(const renderer_backend_vbo_t* vertex_buffer_);
static renderer_result_t test_vertex_buffer_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

static const renderer_shader_vtable_t s_test_shader_vtable = {
    .renderer_shader_create = test_renderer_shader_create,
    .renderer_shader_destroy = test_renderer_shader_destroy,
    .renderer_shader_compile = test_renderer_shader_compile,
    .renderer_shader_link = test_renderer_shader_link,
    .renderer_shader_use = test_renderer_shader_use,
};

static const renderer_vao_vtable_t s_test_vao_vtable = {
    .vertex_array_create = test_vertex_array_create,
    .vertex_array_destroy = test_vertex_array_destroy,
    .vertex_array_bind = test_vertex_array_bind,
    .vertex_array_unbind = test_vertex_array_unbind,
    .vertex_array_attribute_set = test_vertex_array_attribute_set,
};

static const renderer_vbo_vtable_t s_test_vbo_vtable = {
    .vertex_buffer_create = test_vertex_buffer_create,
    .vertex_buffer_destroy = test_vertex_buffer_destroy,
    .vertex_buffer_bind = test_vertex_buffer_bind,
    .vertex_buffer_unbind = test_vertex_buffer_unbind,
    .vertex_buffer_vertex_load = test_vertex_buffer_vertex_load,
};
#endif

renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "out_renderer_backend_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "*out_renderer_backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(graphics_api_valid_check(target_api_), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "target_api_")

    // Simulation.
    renderer_backend_context_t* tmp_context = NULL;
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(renderer_backend_context_t), alignof(renderer_backend_context_t), (void**)&tmp_context);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = renderer_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to allocate memory for renderer backend context.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_context, 0, sizeof(renderer_backend_context_t));

    // shaderバックエンドメモリ確保+初期化
    tmp_context->shader_vtable = shader_vtable_get(target_api_);
    if(NULL == tmp_context->shader_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get shader vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // vaoバックエンドメモリ確保+初期化
    tmp_context->vao_vtable = vao_vtable_get(target_api_);
    if(NULL == tmp_context->vao_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vao vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // vboバックエンドメモリ確保+初期化
    tmp_context->vbo_vtable = vbo_vtable_get(target_api_);
    if(NULL == tmp_context->vbo_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vbo vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->target_api = target_api_;
    tmp_context->current_bound_vao = 0;
    tmp_context->current_bound_vbo = 0;
    tmp_context->current_program_id = 0;

    // commit.
    *out_renderer_backend_context_ = tmp_context;
    ret = RENDERER_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void renderer_backend_destroy(renderer_backend_context_t* renderer_context_) {
    if(NULL == renderer_context_) {
        goto cleanup;
    }
    // 現状では特に必要な処理はなし(リニアロケータによるメモリ確保のため、renderer_context_のリソース解放は不要)
cleanup:
    return;
}

renderer_result_t renderer_backend_shader_create(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_create", "renderer_backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "*shader_handle_")

    ret = renderer_backend_context_->shader_vtable->renderer_shader_create(shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_create(%s) - Failed to create shader handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_shader_destroy(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    if(NULL == renderer_backend_context_ || NULL == renderer_backend_context_->shader_vtable) {
        return;
    }
    renderer_backend_context_->shader_vtable->renderer_shader_destroy(shader_handle_);
}

renderer_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_compile", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_compile(shader_type_, shader_source_, shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_compile(%s) - Failed to compile shader source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_link", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_link", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_link", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_link(shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_link(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_use", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_use", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_use(shader_handle_, &backend_context_->current_program_id);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_use(%s) - Failed to use shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_create", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "vertex_array_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "*vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_create(vertex_array_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_create(%s) - Failed to create vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return ret;
}

void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    if(NULL == backend_context_ || NULL == backend_context_->vao_vtable) {
        return;
    }
    backend_context_->vao_vtable->vertex_array_destroy(vertex_array_);
}

renderer_result_t renderer_backend_vertex_array_bind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_bind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_bind(vertex_array_, &backend_context_->current_bound_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_bind(%s) - Failed to bind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_unbind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_unbind", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_unbind(vertex_array_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_unbind(%s) - Failed to unbind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    backend_context_->current_bound_vao = 0;

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_attribute_set(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_attribute_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_attribute_set", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_attribute_set", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_bind(vertex_array_, &backend_context_->current_bound_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_attribute_set(%s) - Failed to bind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->vao_vtable->vertex_array_attribute_set(vertex_array_, layout_, size_, type_, normalized_, stride_, offset_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_attribute_set(%s) - Failed to set vao attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_create", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "vertex_buffer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "*vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_create(vertex_buffer_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_create(%s) - Failed to create vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return ret;
}

void renderer_backend_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    if(NULL == backend_context_ || NULL == backend_context_->vbo_vtable) {
        return;
    }
    backend_context_->vbo_vtable->vertex_buffer_destroy(vertex_buffer_);
}

renderer_result_t renderer_backend_vertex_buffer_bind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_bind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_, &backend_context_->current_bound_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_bind(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_unbind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_unbind", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_unbind(vertex_buffer_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_unbind(%s) - Failed to unbind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    backend_context_->current_bound_vbo = 0;

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_vertex_load(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_load", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_vertex_load", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_load", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_, &backend_context_->current_bound_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_load(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->vbo_vtable->vertex_buffer_vertex_load(vertex_buffer_, load_size_, load_data_, usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_load(%s) - Failed to load vertex.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_) {
#ifdef TEST_BUILD
    if(s_fail_injection.use_test_vtable) {
        return &s_test_shader_vtable;
    }
#endif

    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_shader_vtable_get();
    default:
        return NULL;
    }
}

static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_) {
#ifdef TEST_BUILD
    if(s_fail_injection.use_test_vtable) {
        return &s_test_vao_vtable;
    }
#endif

    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_vao_vtable_get();
    default:
        return NULL;
    }
}

static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_) {
#ifdef TEST_BUILD
    if(s_fail_injection.use_test_vtable) {
        return &s_test_vbo_vtable;
    }
#endif

    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_vbo_vtable_get();
    default:
        return NULL;
    }
}

static bool graphics_api_valid_check(target_graphics_api_t target_api_) {
    bool ret = false;
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

#ifdef TEST_BUILD
void NO_COVERAGE test_renderer_backend_context(void) {
    test_renderer_backend_initialize();
}

static void NO_COVERAGE test_linear_allocator_create(linear_alloc_t** allocator_, void** out_memory_pool_, size_t pool_size_) {
    assert(NULL == *allocator_);
    assert(NULL == *out_memory_pool_);
    assert(0 != pool_size_);

    size_t mem_req = 0;
    size_t align_req = 0;
    linear_allocator_preinit(&mem_req, &align_req);

    *allocator_ = malloc(mem_req);
    assert(NULL != *allocator_);

    *out_memory_pool_ = malloc(pool_size_);
    assert(NULL != *out_memory_pool_);

    assert(LINEAR_ALLOC_SUCCESS == linear_allocator_init(*allocator_, pool_size_, *out_memory_pool_));
}

static void NO_COVERAGE test_linear_allocator_destroy(linear_alloc_t** allocator_, void** memory_pool_) {
    if(NULL != *allocator_) {
        free(*allocator_);
        *allocator_ = NULL;
    }
    if(NULL != *memory_pool_) {
        free(*memory_pool_);
        *memory_pool_ = NULL;
    }
}

static void NO_COVERAGE test_renderer_backend_initialize(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        // allocator_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(NULL, GRAPHICS_API_GL33, &context);
        assert(NULL == context);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // out_renderer_backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // out_renderer_backend_context_ != NULL -> RENDERER_INVALID_ARGUMENT
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        context = malloc(sizeof(renderer_backend_context_t));
        assert(NULL != context);

        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        free(context);
        context = NULL;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // target_api_が規定値外 -> RENDERER_INVALID_ARGUMENT
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, 100, &context);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // linear_allocator_allocateがLINEAR_ALLOC_NO_MEMORY -> RENDERER_NO_MEMORY
        linear_allocator_malloc_fail_set(0);    // 初回のlinear_allocator_allocateで失敗させる

        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        linear_allocator_malloc_fail_reset();
    }
    {
        // 正常系(vtableはテスト用を使用)
        s_fail_injection.use_test_vtable = true;

        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != context);

        renderer_backend_destroy(context);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        s_fail_injection.use_test_vtable = false;
    }
}

static void NO_COVERAGE test_renderer_backend_destroy(void) {
    {
        // renderer_context_ == NULL
        renderer_backend_context_t* context = NULL;
        renderer_backend_destroy(context);
    }
    {
        // renderer_context != NULL
        renderer_backend_context_t* context = NULL;
        context = malloc(sizeof(renderer_backend_context_t));
        assert(NULL != context);

        renderer_backend_destroy(context);

        free(context);
    }
}

static void NO_COVERAGE test_renderer_backend_shader_create(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        // renderer_backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_shader_t* shader_handle = NULL;
        ret = renderer_backend_shader_create(NULL, shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == shader_handle);
    }
    {
        // renderer_backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_backend_shader_t* shader_handle = NULL;
        renderer_backend_context_t* context = NULL;

        context = malloc(sizeof(renderer_backend_context_t));
        assert(NULL != context);
        context->shader_vtable = NULL;

        renderer_backend_shader_create(context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == shader_handle);

        free(context);
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        linear_alloc_t* linear_alloc;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != context);
        assert(NULL != context->shader_vtable);

        ret = renderer_backend_shader_create(context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // *shader_handle_ != NULL -> RENDERER_INVALID_ARGUMENT
        linear_alloc_t* linear_alloc;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != context);
        assert(NULL != context->shader_vtable);

        double* dummy_addr = NULL;
        double a = 0.0;
        dummy_addr = &a;
        ret = renderer_backend_shader_create(context, &dummy_addr);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // renderer_shader_create == RENDERER_RUNTIME_ERROR -> RENDERER_RUNTIME_ERROR
        s_fail_injection.use_test_vtable = true;
        s_fail_injection.rslt_renderer_shader_create = RENDERER_RUNTIME_ERROR;

        linear_alloc_t* linear_alloc;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != context);
        assert(NULL != context->shader_vtable);

        renderer_backend_shader_t* shader = NULL;
        ret = renderer_backend_shader_create(context, &shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        s_fail_injection.rslt_renderer_shader_create = RENDERER_SUCCESS;
        s_fail_injection.use_test_vtable = false;
    }
    {
        // 正常系
        s_fail_injection.use_test_vtable = true;
        s_fail_injection.rslt_renderer_shader_create = RENDERER_SUCCESS;

        linear_alloc_t* linear_alloc;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_backend_context_t* context = NULL;
        ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != context);
        assert(NULL != context->shader_vtable);

        renderer_backend_shader_t* shader = NULL;
        ret = renderer_backend_shader_create(context, &shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        s_fail_injection.use_test_vtable = false;
    }
}

static void NO_COVERAGE test_renderer_backend_shader_destroy(void) {
    {
        // renderer_backend_context_ = NULL
        renderer_backend_shader_t* shader_handle = NULL;
        renderer_backend_shader_destroy(NULL, &shader_handle);
    }
    {
        // renderer_backend_context_->shader_vtable
        renderer_backend_context_t* context = NULL;
        renderer_backend_shader_t* shader_handle = NULL;

        context = malloc(sizeof(renderer_backend_context_t));
        assert(NULL != context);
        context->shader_vtable = NULL;

        renderer_backend_shader_destroy(context, &shader_handle);
    }
    {
        // 正常系
        s_fail_injection.use_test_vtable = true;

        renderer_backend_context_t* context = NULL;
        renderer_backend_shader_t* shader_handle = NULL;

        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        renderer_result_t ret = renderer_backend_initialize(linear_alloc, GRAPHICS_API_GL33, &context);
        assert(RENDERER_SUCCESS == ret);

        renderer_backend_shader_destroy(context, &shader_handle);

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
        s_fail_injection.use_test_vtable = false;
    }
}

static renderer_result_t NO_COVERAGE test_renderer_shader_create(renderer_backend_shader_t** shader_handle_) {
    return s_fail_injection.rslt_renderer_shader_create;
}

static void NO_COVERAGE test_renderer_shader_destroy(renderer_backend_shader_t** shader_handle_) {
    return;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
    return s_fail_injection.rslt_renderer_shader_compile;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_link(renderer_backend_shader_t* shader_handle_) {
    return s_fail_injection.rslt_renderer_shader_link;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_) {
    return s_fail_injection.rslt_renderer_shader_use;
}

static renderer_result_t NO_COVERAGE test_vertex_array_create(renderer_backend_vao_t** vertex_array_) {
    return s_fail_injection.rslt_vertex_array_create;
}

static void NO_COVERAGE test_vertex_array_destroy(renderer_backend_vao_t** vertex_array_) {
    return;
}

static renderer_result_t NO_COVERAGE test_vertex_array_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_) {
    return s_fail_injection.rslt_vertex_array_bind;
}

static renderer_result_t NO_COVERAGE test_vertex_array_unbind(const renderer_backend_vao_t* vertex_array_) {
    return s_fail_injection.rslt_vertex_array_unbind;
}

static renderer_result_t NO_COVERAGE test_vertex_array_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    return s_fail_injection.rslt_vertex_array_attribute_set;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_create(renderer_backend_vbo_t** vertex_buffer_) {
    return s_fail_injection.rslt_vertex_buffer_create;
}

static void NO_COVERAGE test_vertex_buffer_destroy(renderer_backend_vbo_t** vertex_buffer_) {
    return;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_) {
    return s_fail_injection.rslt_vertex_buffer_bind;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_unbind(const renderer_backend_vbo_t* vertex_buffer_) {
    return s_fail_injection.rslt_vertex_buffer_unbind;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    return s_fail_injection.rslt_vertex_buffer_vertex_load;
}

#endif
