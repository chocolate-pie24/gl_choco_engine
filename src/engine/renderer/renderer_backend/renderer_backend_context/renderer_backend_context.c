#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
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

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"
#include "engine/base/choco_macros.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_renderer_backend_context.h"

// renderer_err_utils用モジュール専用テスト制御構造体定義

// テスト用vtable関数ポインタ

// shader vtable関数
static renderer_result_t test_renderer_shader_create(renderer_backend_shader_t** shader_handle_);
static void test_renderer_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_result_t test_renderer_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);
static renderer_result_t test_renderer_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_result_t test_renderer_shader_mat4f_uniform_set(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_);

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
    .renderer_shader_mat4f_uniform_set = test_renderer_shader_mat4f_uniform_set,
    .renderer_shader_uniform_location_get = test_renderer_shader_uniform_location_get,
};  /**< テスト用shader_vtable */

static const renderer_vao_vtable_t s_test_vao_vtable = {
    .vertex_array_create = test_vertex_array_create,
    .vertex_array_destroy = test_vertex_array_destroy,
    .vertex_array_bind = test_vertex_array_bind,
    .vertex_array_unbind = test_vertex_array_unbind,
    .vertex_array_attribute_set = test_vertex_array_attribute_set,
};  /**< テスト用vao_vtable */

static const renderer_vbo_vtable_t s_test_vbo_vtable = {
    .vertex_buffer_create = test_vertex_buffer_create,
    .vertex_buffer_destroy = test_vertex_buffer_destroy,
    .vertex_buffer_bind = test_vertex_buffer_bind,
    .vertex_buffer_unbind = test_vertex_buffer_unbind,
    .vertex_buffer_vertex_load = test_vertex_buffer_vertex_load,
};  /**< テスト用vbo_vtable */

/**
 * @brief renderer_shader_vtable_t*型の関数の失敗注入設定構造体
 *
 */
typedef struct test_call_control_renderer_shader_vtable_t {
    uint32_t call_count;                              /**< 関数呼び出し回数 */
    uint32_t fail_on_call;                            /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    const renderer_shader_vtable_t* forced_result;    /**< 関数を強制的に失敗させる際の戻り値 */
} test_call_control_renderer_shader_vtable_t_t;

/**
 * @brief renderer_vao_vtable_t*型の関数の失敗注入設定構造体
 *
 */
typedef struct test_call_control_renderer_vao_vtable_t {
    uint32_t call_count;                              /**< 関数呼び出し回数 */
    uint32_t fail_on_call;                            /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    const renderer_vao_vtable_t* forced_result;       /**< 関数を強制的に失敗させる際の戻り値 */
} test_call_control_renderer_vao_vtable_t_t;

/**
 * @brief renderer_vbo_vtable_t*型の関数の失敗注入設定構造体
 *
 */
typedef struct test_call_control_renderer_vbo_vtable_t {
    uint32_t call_count;                              /**< 関数呼び出し回数 */
    uint32_t fail_on_call;                            /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    const renderer_vbo_vtable_t* forced_result;       /**< 関数を強制的に失敗させる際の戻り値 */
} test_call_control_renderer_vbo_vtable_t_t;

// 外部公開APIテスト設定
static test_call_control_t s_test_config_renderer_backend_initialize;                   /**< renderer_backend_initialize()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_create;                /**< renderer_backend_shader_create()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_compile;               /**< renderer_backend_shader_compile()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_link;                  /**< renderer_backend_shader_link()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_use;                   /**< renderer_backend_shader_use()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_uniform_location_get;  /**< renderer_backend_shader_uniform_location_get()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_shader_mat4f_uniform_set;     /**< renderer_backend_shader_mat4f_uniform_set()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_array_create;          /**< renderer_backend_vertex_array_create()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_array_bind;            /**< renderer_backend_vertex_array_bind()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_array_unbind;          /**< renderer_backend_vertex_array_unbind()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_array_attribute_set;   /**< renderer_backend_vertex_array_attribute_set()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_buffer_create;         /**< renderer_backend_vertex_buffer_create()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_buffer_bind;           /**< renderer_backend_vertex_buffer_bind()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_buffer_unbind;         /**< renderer_backend_vertex_buffer_unbind()テスト設定 */
static test_call_control_t s_test_config_renderer_backend_vertex_buffer_vertex_load;    /**< renderer_backend_vertex_buffer_vertex_load()テスト設定 */

// プライベート関数テスト設定
static test_call_control_renderer_shader_vtable_t_t s_test_config_shader_vtable_get;    /**< shader_vtable_get()テスト設定 */
static test_call_control_renderer_vao_vtable_t_t s_test_config_vao_vtable_get;          /**< vao_vtable_get()テスト設定 */
static test_call_control_renderer_vbo_vtable_t_t s_test_config_vbo_vtable_get;          /**< vbo_vtable_get()テスト設定 */
static test_call_control_bool_t s_test_config_graphics_api_valid_check;                 /**< graphics_api_valid_check()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_create;                     /**< test_renderer_shader_create()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_compile;                    /**< test_renderer_shader_compile()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_link;                       /**< test_renderer_shader_link()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_use;                        /**< test_renderer_shader_use()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_uniform_location_get;       /**< test_renderer_shader_uniform_location_get()テスト設定 */
static renderer_result_t s_test_config_test_renderer_shader_mat4f_uniform_set;          /**< test_renderer_shader_mat4f_uniform_set()テスト設定 */
static renderer_result_t s_test_config_test_vertex_array_create;                        /**< test_vertex_array_create()テスト設定 */
static renderer_result_t s_test_config_test_vertex_array_bind;                          /**< test_vertex_array_bind()テスト設定 */
static renderer_result_t s_test_config_test_vertex_array_unbind;                        /**< test_vertex_array_unbind()テスト設定 */
static renderer_result_t s_test_config_test_vertex_array_attribute_set;                 /**< test_vertex_array_attribute_set()テスト設定 */
static renderer_result_t s_test_config_test_vertex_buffer_create;                       /**< test_vertex_buffer_create()テスト設定 */
static renderer_result_t s_test_config_test_vertex_buffer_bind;                         /**< test_vertex_buffer_bind()テスト設定 */
static renderer_result_t s_test_config_test_vertex_buffer_unbind;                       /**< test_vertex_buffer_unbind()テスト設定 */
static renderer_result_t s_test_config_test_vertex_buffer_vertex_load;                  /**< test_vertex_buffer_vertex_load()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_renderer_backend_initialize(void);
static void test_renderer_backend_destroy(void);
static void test_renderer_backend_shader_create(void);
static void test_renderer_backend_shader_destroy(void);
static void test_renderer_backend_shader_compile(void);
static void test_renderer_backend_shader_link(void);
static void test_renderer_backend_shader_use(void);
static void test_renderer_backend_shader_uniform_location_get(void);
static void test_renderer_backend_shader_mat4f_uniform_set(void);
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
static void test_shader_vtable_get(void);
static void test_vao_vtable_get(void);
static void test_vbo_vtable_get(void);
static void test_graphics_api_valid_check(void);
#endif

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

renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_initialize.call_count++;
    if(s_test_config_renderer_backend_initialize.fail_on_call != 0) {
        if(s_test_config_renderer_backend_initialize.call_count == s_test_config_renderer_backend_initialize.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_initialize.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    renderer_backend_context_t* tmp_context = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "out_renderer_backend_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "*out_renderer_backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(graphics_api_valid_check(target_api_), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "target_api_")

    // Simulation.
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_create.call_count++;
    if(s_test_config_renderer_backend_shader_create.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_create.call_count == s_test_config_renderer_backend_shader_create.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_compile.call_count++;
    if(s_test_config_renderer_backend_shader_compile.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_compile.call_count == s_test_config_renderer_backend_shader_compile.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_compile.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_link.call_count++;
    if(s_test_config_renderer_backend_shader_link.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_link.call_count == s_test_config_renderer_backend_shader_link.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_link.forced_result;
        }
    }
#endif
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

renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_use.call_count++;
    if(s_test_config_renderer_backend_shader_use.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_use.call_count == s_test_config_renderer_backend_shader_use.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_use.forced_result;
        }
    }
#endif
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

renderer_result_t renderer_backend_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_uniform_location_get.call_count++;
    if(s_test_config_renderer_backend_shader_uniform_location_get.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_uniform_location_get.call_count == s_test_config_renderer_backend_shader_uniform_location_get.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_uniform_location_get.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_uniform_location_get", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "out_location_")

    ret = backend_context_->shader_vtable->renderer_shader_uniform_location_get(shader_handle_, name_, out_location_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_uniform_location_get(%s) - Failed to get uniform location. name: %s", renderer_rslt_to_str(ret), name_);
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_shader_mat4f_uniform_set(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count++;
    if(s_test_config_renderer_backend_shader_mat4f_uniform_set.fail_on_call != 0) {
        if(s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count == s_test_config_renderer_backend_shader_mat4f_uniform_set.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_shader_mat4f_uniform_set.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_mat4f_uniform_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_mat4f_uniform_set", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_mat4f_uniform_set", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_mat4f_uniform_set", "data_")

    ret = backend_context_->shader_vtable->renderer_shader_mat4f_uniform_set(shader_handle_, location_, should_transpose_, data_, &backend_context_->current_program_id);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_mat4f_uniform_set(%s) - Failed to set mat4f uniform.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_array_create.call_count++;
    if(s_test_config_renderer_backend_vertex_array_create.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_array_create.call_count == s_test_config_renderer_backend_vertex_array_create.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_array_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_array_bind.call_count++;
    if(s_test_config_renderer_backend_vertex_array_bind.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_array_bind.call_count == s_test_config_renderer_backend_vertex_array_bind.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_array_bind.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_array_unbind.call_count++;
    if(s_test_config_renderer_backend_vertex_array_unbind.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_array_unbind.call_count == s_test_config_renderer_backend_vertex_array_unbind.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_array_unbind.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_array_attribute_set.call_count++;
    if(s_test_config_renderer_backend_vertex_array_attribute_set.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_array_attribute_set.call_count == s_test_config_renderer_backend_vertex_array_attribute_set.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_array_attribute_set.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_buffer_create.call_count++;
    if(s_test_config_renderer_backend_vertex_buffer_create.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_buffer_create.call_count == s_test_config_renderer_backend_vertex_buffer_create.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_buffer_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_buffer_bind.call_count++;
    if(s_test_config_renderer_backend_vertex_buffer_bind.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_buffer_bind.call_count == s_test_config_renderer_backend_vertex_buffer_bind.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_buffer_bind.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_buffer_unbind.call_count++;
    if(s_test_config_renderer_backend_vertex_buffer_unbind.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_buffer_unbind.call_count == s_test_config_renderer_backend_vertex_buffer_unbind.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_buffer_unbind.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count++;
    if(s_test_config_renderer_backend_vertex_buffer_vertex_load.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count == s_test_config_renderer_backend_vertex_buffer_vertex_load.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_buffer_vertex_load.forced_result;
        }
    }
#endif
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
    s_test_config_shader_vtable_get.call_count++;
    if(s_test_config_shader_vtable_get.fail_on_call != 0) {
        if(s_test_config_shader_vtable_get.call_count == s_test_config_shader_vtable_get.fail_on_call) {
            return s_test_config_shader_vtable_get.forced_result;
        }
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
    s_test_config_vao_vtable_get.call_count++;
    if(s_test_config_vao_vtable_get.fail_on_call != 0) {
        if(s_test_config_vao_vtable_get.call_count == s_test_config_vao_vtable_get.fail_on_call) {
            return s_test_config_vao_vtable_get.forced_result;
        }
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
    s_test_config_vbo_vtable_get.call_count++;
    if(s_test_config_vbo_vtable_get.fail_on_call != 0) {
        if(s_test_config_vbo_vtable_get.call_count == s_test_config_vbo_vtable_get.fail_on_call) {
            return s_test_config_vbo_vtable_get.forced_result;
        }
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

static renderer_result_t NO_COVERAGE test_renderer_shader_create(renderer_backend_shader_t** shader_handle_) {
    (void)shader_handle_;

    return s_test_config_test_renderer_shader_create;
}

static void NO_COVERAGE test_renderer_shader_destroy(renderer_backend_shader_t** shader_handle_) {
    (void)shader_handle_;

    return;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
    (void)shader_type_;
    (void)shader_source_;
    (void)shader_handle_;

    return s_test_config_test_renderer_shader_compile;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_link(renderer_backend_shader_t* shader_handle_) {
    (void)shader_handle_;

    return s_test_config_test_renderer_shader_link;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_) {
    (void)shader_handle_;
    (void)out_program_id_;

    return s_test_config_test_renderer_shader_use;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
    (void)backend_context_;
    (void)shader_handle_;
    (void)name_;
    (void)out_location_;

    return s_test_config_test_renderer_shader_uniform_location_get;
}

static renderer_result_t NO_COVERAGE test_renderer_shader_mat4f_uniform_set(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_) {
    (void)backend_context_;
    (void)shader_handle_;
    (void)location_;
    (void)should_transpose_;
    (void)data_;

    return s_test_config_test_renderer_shader_mat4f_uniform_set;
}

static renderer_result_t NO_COVERAGE test_vertex_array_create(renderer_backend_vao_t** vertex_array_) {
    (void)vertex_array_;

    return s_test_config_test_vertex_array_create;
}

static void NO_COVERAGE test_vertex_array_destroy(renderer_backend_vao_t** vertex_array_) {
    (void)vertex_array_;

    return;
}

static renderer_result_t NO_COVERAGE test_vertex_array_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_) {
    (void)vertex_array_;
    (void)out_vao_id_;

    return s_test_config_test_vertex_array_bind;
}

static renderer_result_t NO_COVERAGE test_vertex_array_unbind(const renderer_backend_vao_t* vertex_array_) {
    (void)vertex_array_;

    return s_test_config_test_vertex_array_unbind;
}

static renderer_result_t NO_COVERAGE test_vertex_array_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    (void)vertex_array_;
    (void)layout_;
    (void)size_;
    (void)type_;
    (void)normalized_;
    (void)stride_;
    (void)offset_;

    return s_test_config_test_vertex_array_attribute_set;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_create(renderer_backend_vbo_t** vertex_buffer_) {
    (void)vertex_buffer_;

    return s_test_config_test_vertex_buffer_create;
}

static void NO_COVERAGE test_vertex_buffer_destroy(renderer_backend_vbo_t** vertex_buffer_) {
    (void)vertex_buffer_;

    return;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_bind(const renderer_backend_vbo_t* vertex_buffer_, uint32_t* out_vbo_id_) {
    (void)vertex_buffer_;
    (void)out_vbo_id_;

    return s_test_config_test_vertex_buffer_bind;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_unbind(const renderer_backend_vbo_t* vertex_buffer_) {
    (void)vertex_buffer_;

    return s_test_config_test_vertex_buffer_unbind;
}

static renderer_result_t NO_COVERAGE test_vertex_buffer_vertex_load(const renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    (void)vertex_buffer_;
    (void)load_size_;
    (void)load_data_;
    (void)usage_;

    return s_test_config_test_vertex_buffer_vertex_load;
}

void NO_COVERAGE test_renderer_backend_shader_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_create.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_shader_compile_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_compile.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_compile.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_shader_link_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_link.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_link.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_shader_use_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_use.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_use.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_shader_uniform_location_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_uniform_location_get.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_uniform_location_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_shader_mat4f_uniform_set_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_shader_mat4f_uniform_set.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_shader_mat4f_uniform_set.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_array_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_array_create.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_array_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_array_bind_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_array_bind.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_array_bind.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_array_unbind_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_array_unbind.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_array_unbind.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_array_attribute_set_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_array_attribute_set.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_array_attribute_set.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_buffer_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_buffer_create.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_buffer_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_buffer_bind_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_buffer_bind.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_buffer_bind.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_buffer_unbind_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_buffer_unbind.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_buffer_unbind.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_vertex_buffer_vertex_load_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_vertex_buffer_vertex_load.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_vertex_buffer_vertex_load.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_backend_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_backend_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_backend_context_reset(void) {
    test_call_control_reset(&s_test_config_renderer_backend_initialize);
    test_call_control_reset(&s_test_config_renderer_backend_shader_create);
    test_call_control_reset(&s_test_config_renderer_backend_shader_compile);
    test_call_control_reset(&s_test_config_renderer_backend_shader_link);
    test_call_control_reset(&s_test_config_renderer_backend_shader_use);
    test_call_control_reset(&s_test_config_renderer_backend_shader_uniform_location_get);
    test_call_control_reset(&s_test_config_renderer_backend_shader_mat4f_uniform_set);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_array_create);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_array_bind);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_array_unbind);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_array_attribute_set);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_buffer_create);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_buffer_bind);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_buffer_unbind);
    test_call_control_reset(&s_test_config_renderer_backend_vertex_buffer_vertex_load);
    s_test_config_shader_vtable_get.forced_result = shader_vtable_get(GRAPHICS_API_GL33);
    s_test_config_vao_vtable_get.forced_result = vao_vtable_get(GRAPHICS_API_GL33);
    s_test_config_vbo_vtable_get.forced_result = vbo_vtable_get(GRAPHICS_API_GL33);
    test_call_control_bool_reset(&s_test_config_graphics_api_valid_check);
    s_test_config_test_renderer_shader_create = RENDERER_SUCCESS;
    s_test_config_test_renderer_shader_compile = RENDERER_SUCCESS;
    s_test_config_test_renderer_shader_link = RENDERER_SUCCESS;
    s_test_config_test_renderer_shader_use = RENDERER_SUCCESS;
    s_test_config_test_renderer_shader_uniform_location_get = RENDERER_SUCCESS;
    s_test_config_test_renderer_shader_mat4f_uniform_set = RENDERER_SUCCESS;
    s_test_config_test_vertex_array_create = RENDERER_SUCCESS;
    s_test_config_test_vertex_array_bind = RENDERER_SUCCESS;
    s_test_config_test_vertex_array_unbind = RENDERER_SUCCESS;
    s_test_config_test_vertex_array_attribute_set = RENDERER_SUCCESS;
    s_test_config_test_vertex_buffer_create = RENDERER_SUCCESS;
    s_test_config_test_vertex_buffer_bind = RENDERER_SUCCESS;
    s_test_config_test_vertex_buffer_unbind = RENDERER_SUCCESS;
    s_test_config_test_vertex_buffer_vertex_load = RENDERER_SUCCESS;
}

void NO_COVERAGE test_renderer_backend_context(void) {
    test_renderer_backend_initialize();
    test_renderer_backend_destroy();
    test_renderer_backend_shader_create();
    test_renderer_backend_shader_destroy();
    test_renderer_backend_shader_compile();
    test_renderer_backend_shader_link();
    test_renderer_backend_shader_use();
    test_renderer_backend_shader_uniform_location_get();
    test_renderer_backend_shader_mat4f_uniform_set();
    test_renderer_backend_vertex_array_create();
    test_renderer_backend_vertex_array_destroy();
    test_renderer_backend_vertex_array_bind();
    test_renderer_backend_vertex_array_unbind();
    test_renderer_backend_vertex_array_attribute_set();
    test_renderer_backend_vertex_buffer_create();
    test_renderer_backend_vertex_buffer_destroy();
    test_renderer_backend_vertex_buffer_bind();
    test_renderer_backend_vertex_buffer_unbind();
    test_renderer_backend_vertex_buffer_vertex_load();
    test_shader_vtable_get();
    test_vao_vtable_get();
    test_vbo_vtable_get();
    test_graphics_api_valid_check();
}

static void NO_COVERAGE test_renderer_backend_initialize(void) {

}

static void NO_COVERAGE test_renderer_backend_destroy(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_create(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_destroy(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_compile(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_link(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_use(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_uniform_location_get(void) {

}

static void NO_COVERAGE test_renderer_backend_shader_mat4f_uniform_set(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_array_create(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_array_destroy(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_array_bind(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_array_unbind(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_array_attribute_set(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_buffer_create(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_buffer_destroy(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_buffer_bind(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_buffer_unbind(void) {

}

static void NO_COVERAGE test_renderer_backend_vertex_buffer_vertex_load(void) {

}

static void NO_COVERAGE test_shader_vtable_get(void) {

}

static void NO_COVERAGE test_vao_vtable_get(void) {

}

static void NO_COVERAGE test_vbo_vtable_get(void) {

}

static void NO_COVERAGE test_graphics_api_valid_check(void) {

}
#endif
