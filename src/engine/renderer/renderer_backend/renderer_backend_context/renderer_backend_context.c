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
#include "engine/renderer/renderer_backend/renderer_backend_context/context_texture.h"

#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"

#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_texture.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>

#include "test_controller.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_renderer_backend_context.h"

// renderer_backend_context用モジュール専用テスト制御構造体定義

// テスト用vtable関数ポインタ

// shader vtable関数
static renderer_result_t test_renderer_shader_create(renderer_backend_shader_t** shader_handle_);
static void test_renderer_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_result_t test_renderer_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_result_t test_renderer_shader_use(const renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);
static renderer_result_t test_renderer_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_result_t test_renderer_mat4f_uniform_set(const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_);

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
    .renderer_shader_mat4f_uniform_set = test_renderer_mat4f_uniform_set,
    .renderer_shader_uniform_location_get = test_renderer_uniform_location_get,
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
static test_call_control_t s_test_config_renderer_backend_vertex_buffer_vertex_subload; /**< renderer_backend_vertex_buffer_vertex_subload()テスト設定 */

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
    target_graphics_api_t target_api;                   /**< 使用グラフィックスAPI */

    const renderer_shader_vtable_t* shader_vtable;      /**< シェーダー機能提供vtable */
    const renderer_vao_vtable_t* vao_vtable;            /**< VAO機能提供vtable */
    const renderer_vbo_vtable_t* vbo_vtable;            /**< VBO機能提供vtable */
    const renderer_texture_vtable_t* texture_vtable;    /**< Texture機能提供vtable */

    uint32_t current_program_id;                        /**< 現在使用中のリンクされたシェーダープログラムID */
    uint32_t current_bound_vao;                         /**< 現在バインド中のVAO識別子 */
    uint32_t current_bound_vbo;                         /**< 現在バインド中のVBO識別子 */

    int32_t current_texture_unit;
    int32_t current_bound_texture;                     /**< 現在バインド中のTextureハンドル */
};

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_);
static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_);
static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_);
static const renderer_texture_vtable_t* texture_vtable_get(target_graphics_api_t target_api_);

static bool graphics_api_valid_check(target_graphics_api_t target_api_);
static bool texture_type_valid_check(texture_type_t texture_type_);

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

    // Textureバックエンドメモリ確保+初期化
    tmp_context->texture_vtable = texture_vtable_get(target_api_);
    if(NULL == tmp_context->texture_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get texture vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->target_api = target_api_;
    tmp_context->current_bound_vao = 0;
    tmp_context->current_bound_vbo = 0;
    tmp_context->current_program_id = 0;
    tmp_context->current_texture_unit = 0;
    tmp_context->current_bound_texture = 0;

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
    // NOTE: shader_handle_のNULLチェックは下位に任せる
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
    // NOTE: vertex_array_のNULLチェックは下位で行う
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
    // NOTE: vertex_buffer_のNULLチェックは下位に任せる
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

renderer_result_t renderer_backend_vertex_buffer_vertex_subload(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_, size_t offset_, size_t size_, void* load_data_) {
#ifdef TEST_BUILD
    s_test_config_renderer_backend_vertex_buffer_vertex_subload.call_count++;
    if(s_test_config_renderer_backend_vertex_buffer_vertex_subload.fail_on_call != 0) {
        if(s_test_config_renderer_backend_vertex_buffer_vertex_subload.call_count == s_test_config_renderer_backend_vertex_buffer_vertex_subload.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_backend_vertex_buffer_vertex_subload.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_subload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_vertex_subload", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_subload", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_, &backend_context_->current_bound_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_subload(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->vbo_vtable->vertex_buffer_vertex_subload(vertex_buffer_, offset_, size_, load_data_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_subload(%s) - Failed to load vertex.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_texture_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_texture_create", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_create", "*texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_create(unit_num_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, texture_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_create(%s) - Failed to create texture.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_texture_destroy(renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_) {
    if(NULL == backend_context_ || NULL == backend_context_->texture_vtable) {
        return;
    }
    // NOTE: texture_handle_のNULLチェックは下位に任せる
    backend_context_->texture_vtable->renderer_texture_destroy(texture_handle_);
}

renderer_result_t renderer_backend_texture_bind(renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_texture_bind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_bind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_bind(texture_handle_, &backend_context_->current_texture_unit, &backend_context_->current_bound_texture);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_bind(%s) - Failed to bind texture.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_texture_unbind(renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_texture_unbind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_unbind(texture_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_unbind(%s) - Failed to unbind texture.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_texture_pixel_upload(renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_texture_pixel_upload", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "texture_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "pixels_")

    ret = backend_context_->texture_vtable->renderer_texture_bind(texture_handle_, &backend_context_->current_texture_unit, &backend_context_->current_bound_texture);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Failed to bind texture.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->texture_vtable->renderer_texture_pixel_upload(width_, height_, channel_count_, pixels_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Failed to upload texture pixels.", renderer_rslt_to_str(ret));
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

static const renderer_texture_vtable_t* texture_vtable_get(target_graphics_api_t target_api_) {
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_texture_vtable_get();
    default:
        return NULL;
    }
}

static bool graphics_api_valid_check(target_graphics_api_t target_api_) {
#ifdef TEST_BUILD
    s_test_config_graphics_api_valid_check.call_count++;
    if(s_test_config_graphics_api_valid_check.fail_on_call != 0) {
        if(s_test_config_graphics_api_valid_check.call_count == s_test_config_graphics_api_valid_check.fail_on_call) {
            return s_test_config_graphics_api_valid_check.forced_result;
        }
    }
#endif
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

static bool texture_type_valid_check(texture_type_t texture_type_) {
    bool ret = false;
    switch(texture_type_) {
    case TEXTURE_TYPE_DIFFUSE:
        ret = true;
        break;
    case TEXTURE_TYPE_NORMAL:
        ret = true;
        break;
    case TEXTURE_TYPE_SPECULAR:
        ret = true;
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

static renderer_result_t NO_COVERAGE test_renderer_shader_use(const renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_) {
    (void)shader_handle_;
    (void)out_program_id_;

    return s_test_config_test_renderer_shader_use;
}

static renderer_result_t NO_COVERAGE test_renderer_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
    (void)shader_handle_;
    (void)name_;
    (void)out_location_;

    return s_test_config_test_renderer_shader_uniform_location_get;
}

static renderer_result_t NO_COVERAGE test_renderer_mat4f_uniform_set(const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_) {
    (void)shader_handle_;
    (void)location_;
    (void)should_transpose_;
    (void)data_;
    (void)out_program_id_;

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

void NO_COVERAGE test_renderer_backend_context_config_reset(void) {
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
    test_call_control_reset(&s_test_config_renderer_backend_vertex_buffer_vertex_subload);
    s_test_config_shader_vtable_get.forced_result = shader_vtable_get(GRAPHICS_API_GL33);
    s_test_config_shader_vtable_get.call_count = 0;
    s_test_config_shader_vtable_get.fail_on_call = 0;
    s_test_config_vao_vtable_get.forced_result = vao_vtable_get(GRAPHICS_API_GL33);
    s_test_config_vao_vtable_get.call_count = 0;
    s_test_config_vao_vtable_get.fail_on_call = 0;
    s_test_config_vbo_vtable_get.forced_result = vbo_vtable_get(GRAPHICS_API_GL33);
    s_test_config_vbo_vtable_get.call_count = 0;
    s_test_config_vbo_vtable_get.fail_on_call = 0;
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

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_initialize(void) {
    {
        // renderer_backend_initialize() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;
        test_call_control_t config = {0};

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_initialize_config_set(&config);

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(0U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // allocator_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_initialize(NULL, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(0U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // out_renderer_backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(0U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // *out_renderer_backend_context_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t dummy_context = {0};
        renderer_backend_context_t* out_context = &dummy_context;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(&dummy_context == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(0U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // target_api_ が既定値外 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_initialize(
            allocator,
            (target_graphics_api_t)99999,
            &out_context
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // allocator 容量不足 -> RENDERER_NO_MEMORY
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char small_pool_storage[sizeof(renderer_backend_context_t) - 1U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(small_pool_storage, 0, sizeof(small_pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(
            allocator,
            sizeof(small_pool_storage),
            small_pool_storage
        );
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(0U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_vtable_get() が NULL を返す -> RENDERER_RUNTIME_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 1U;
        s_test_config_shader_vtable_get.forced_result = NULL;

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 0U;

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 0U;

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(1U == s_test_config_shader_vtable_get.call_count);
        assert(0U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vao_vtable_get() が NULL を返す -> RENDERER_RUNTIME_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 0U;
        s_test_config_shader_vtable_get.forced_result = gl33_shader_vtable_get();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 1U;
        s_test_config_vao_vtable_get.forced_result = NULL;

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 0U;

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(1U == s_test_config_shader_vtable_get.call_count);
        assert(1U == s_test_config_vao_vtable_get.call_count);
        assert(0U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vbo_vtable_get() が NULL を返す -> RENDERER_RUNTIME_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 0U;
        s_test_config_shader_vtable_get.forced_result = gl33_shader_vtable_get();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 0U;
        s_test_config_vao_vtable_get.forced_result = gl33_vao_vtable_get();

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 1U;
        s_test_config_vbo_vtable_get.forced_result = NULL;

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(NULL == out_context);
        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(1U == s_test_config_shader_vtable_get.call_count);
        assert(1U == s_test_config_vao_vtable_get.call_count);
        assert(1U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 正常系
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t* out_context = NULL;

        size_t allocator_mem_req = 0U;
        size_t allocator_align_req = 0U;
        size_t allocator_storage_count = 0U;
        linear_alloc_t* allocator = NULL;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;

        linear_allocator_preinit(&allocator_mem_req, &allocator_align_req);
        (void)allocator_align_req;
        allocator_storage_count =
            (allocator_mem_req + sizeof(max_align_t) - 1U) / sizeof(max_align_t);

        max_align_t allocator_storage[allocator_storage_count];
        unsigned char pool_storage[sizeof(renderer_backend_context_t) * 2U];

        memset(allocator_storage, 0, sizeof(allocator_storage));
        memset(pool_storage, 0, sizeof(pool_storage));

        allocator = (linear_alloc_t*)allocator_storage;
        ret_linear = linear_allocator_init(allocator, sizeof(pool_storage), pool_storage);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 0U;
        s_test_config_shader_vtable_get.forced_result = gl33_shader_vtable_get();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 0U;
        s_test_config_vao_vtable_get.forced_result = gl33_vao_vtable_get();

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 0U;
        s_test_config_vbo_vtable_get.forced_result = gl33_vbo_vtable_get();

        ret = renderer_backend_initialize(allocator, GRAPHICS_API_GL33, &out_context);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != out_context);

        assert(GRAPHICS_API_GL33 == out_context->target_api);
        assert(gl33_shader_vtable_get() == out_context->shader_vtable);
        assert(gl33_vao_vtable_get() == out_context->vao_vtable);
        assert(gl33_vbo_vtable_get() == out_context->vbo_vtable);
        assert(0U == out_context->current_program_id);
        assert(0U == out_context->current_bound_vao);
        assert(0U == out_context->current_bound_vbo);

        assert(1U == s_test_config_renderer_backend_initialize.call_count);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);
        assert(1U == s_test_config_shader_vtable_get.call_count);
        assert(1U == s_test_config_vao_vtable_get.call_count);
        assert(1U == s_test_config_vbo_vtable_get.call_count);

        renderer_backend_destroy(out_context);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_destroy(void) {
    {
        // renderer_context_ == NULL -> no-op
        test_renderer_backend_context_config_reset();

        renderer_backend_destroy(NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // 非NULLでも現状は no-op
        renderer_backend_context_t dummy_context = {0};

        dummy_context.target_api = GRAPHICS_API_GL33;
        dummy_context.shader_vtable = gl33_shader_vtable_get();
        dummy_context.vao_vtable = gl33_vao_vtable_get();
        dummy_context.vbo_vtable = gl33_vbo_vtable_get();
        dummy_context.current_program_id = 111U;
        dummy_context.current_bound_vao = 222U;
        dummy_context.current_bound_vbo = 333U;

        test_renderer_backend_context_config_reset();

        renderer_backend_destroy(&dummy_context);

        // 現状は何もしないため状態は変化しない
        assert(GRAPHICS_API_GL33 == dummy_context.target_api);
        assert(gl33_shader_vtable_get() == dummy_context.shader_vtable);
        assert(gl33_vao_vtable_get() == dummy_context.vao_vtable);
        assert(gl33_vbo_vtable_get() == dummy_context.vbo_vtable);
        assert(111U == dummy_context.current_program_id);
        assert(222U == dummy_context.current_bound_vao);
        assert(333U == dummy_context.current_bound_vbo);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_create(void) {
    {
        // renderer_backend_shader_create() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = NULL;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_create_config_set(&config);

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // renderer_backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_create(NULL, &shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // renderer_backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_create(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // *shader_handle_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert((renderer_backend_shader_t*)(uintptr_t)0x1U == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_NO_MEMORY を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_create = RENDERER_NO_MEMORY;

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_LIMIT_EXCEEDED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_create = RENDERER_LIMIT_EXCEEDED;

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系: wrapper としては SUCCESS を返す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_create = RENDERER_SUCCESS;

        ret = renderer_backend_shader_create(&context, &shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_renderer_backend_shader_create.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_destroy(void) {
    {
        // renderer_backend_context_ == NULL, shader_handle_ == NULL -> no-op
        test_renderer_backend_context_config_reset();

        renderer_backend_shader_destroy(NULL, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // renderer_backend_context_ == NULL -> no-op
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        renderer_backend_shader_destroy(NULL, &shader_handle);

        assert((renderer_backend_shader_t*)(uintptr_t)0x1U == shader_handle);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_vtable == NULL -> no-op
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;

        test_renderer_backend_context_config_reset();

        renderer_backend_shader_destroy(&context, &shader_handle);

        assert((renderer_backend_shader_t*)(uintptr_t)0x1U == shader_handle);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL でも、有効な vtable があればそのまま下位へ委譲してよい
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_shader_destroy(&context, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // 有効な context + 有効な vtable -> 下位 destroy に委譲
        // 現在の test_renderer_shader_destroy() は no-op なので、shader_handle は変化しない
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_shader_destroy(&context, &shader_handle);

        assert((renderer_backend_shader_t*)(uintptr_t)0x1U == shader_handle);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_compile(void) {
    {
        // renderer_backend_shader_compile() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_compile_config_set(&config);

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            NULL,
            shader_handle
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_source_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            NULL,
            &context,
            shader_handle
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            NULL
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_shader_compile(
            (shader_type_t)99999,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_BAD_OPERATION;

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_SHADER_COMPILE_ERROR を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_SHADER_COMPILE_ERROR;

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_FRAGMENT,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_NO_MEMORY を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_NO_MEMORY;

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_NO_MEMORY == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_LIMIT_EXCEEDED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_LIMIT_EXCEEDED;

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* shader_source = "dummy shader source";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_compile = RENDERER_SUCCESS;

        ret = renderer_backend_shader_compile(
            SHADER_TYPE_VERTEX,
            shader_source,
            &context,
            shader_handle
        );
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_renderer_backend_shader_compile.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_link(void) {
    {
        // renderer_backend_shader_link() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_link_config_set(&config);

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_link(NULL, shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_link(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_link = RENDERER_BAD_OPERATION;

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_SHADER_LINK_ERROR を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_link = RENDERER_SHADER_LINK_ERROR;

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_NO_MEMORY を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_link = RENDERER_NO_MEMORY;

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_NO_MEMORY == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_LIMIT_EXCEEDED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_link = RENDERER_LIMIT_EXCEEDED;

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_link = RENDERER_SUCCESS;

        ret = renderer_backend_shader_link(&context, shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_renderer_backend_shader_link.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_use(void) {
    {
        // renderer_backend_shader_use() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_use_config_set(&config);

        ret = renderer_backend_shader_use(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_use(NULL, shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;
        context.current_program_id = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_use(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_use(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_use = RENDERER_BAD_OPERATION;

        ret = renderer_backend_shader_use(&context, shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_DATA_CORRUPTED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_use = RENDERER_DATA_CORRUPTED;

        ret = renderer_backend_shader_use(&context, shader_handle);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(222U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_renderer_shader_use() は out_program_id_ を更新しないため、
        // current_program_id は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_use = RENDERER_SUCCESS;

        ret = renderer_backend_shader_use(&context, shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(333U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_use.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_uniform_location_get(void) {
    {
        // renderer_backend_shader_uniform_location_get() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        test_call_control_t config = {0};
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_uniform_location_get_config_set(&config);

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        const char* name = "u_mvp";

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_uniform_location_get(
            NULL,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        int32_t location = -999;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            NULL,
            name,
            &location
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // name_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            NULL,
            &location
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // out_location_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            NULL
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_RUNTIME_ERROR を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_uniform_location_get = RENDERER_RUNTIME_ERROR;

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_uniform_location_get = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_renderer_uniform_location_get() は out_location_ を更新しないため、
        // location は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        int32_t location = -999;
        const char* name = "u_mvp";

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_uniform_location_get = RENDERER_SUCCESS;

        ret = renderer_backend_shader_uniform_location_get(
            &context,
            shader_handle,
            name,
            &location
        );
        assert(RENDERER_SUCCESS == ret);
        assert(-999 == location);
        assert(1U == s_test_config_renderer_backend_shader_uniform_location_get.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_shader_mat4f_uniform_set(void) {
    {
        // renderer_backend_shader_mat4f_uniform_set() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {0.0f};
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_shader_mat4f_uniform_set_config_set(&config);

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            false,
            data
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {0.0f};

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_mat4f_uniform_set(
            NULL,
            shader_handle,
            7,
            false,
            data
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->shader_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = NULL;
        context.current_program_id = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            false,
            data
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        float data[16] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            NULL,
            7,
            false,
            data
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // data_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 999U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            false,
            NULL
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(999U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_mat4f_uniform_set = RENDERER_BAD_OPERATION;

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            false,
            data
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_DATA_CORRUPTED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_mat4f_uniform_set = RENDERER_DATA_CORRUPTED;

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            true,
            data
        );
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(222U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_renderer_mat4f_uniform_set() は out_program_id_ を更新しないため、
        // current_program_id は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_shader_t* shader_handle =
            (renderer_backend_shader_t*)(uintptr_t)0x1U;
        float data[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        context.target_api = GRAPHICS_API_GL33;
        context.shader_vtable = &s_test_shader_vtable;
        context.current_program_id = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_renderer_shader_mat4f_uniform_set = RENDERER_SUCCESS;

        ret = renderer_backend_shader_mat4f_uniform_set(
            &context,
            shader_handle,
            7,
            false,
            data
        );
        assert(RENDERER_SUCCESS == ret);
        assert(333U == context.current_program_id);
        assert(1U == s_test_config_renderer_backend_shader_mat4f_uniform_set.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_array_create(void) {
    {
        // renderer_backend_vertex_array_create() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array = NULL;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_array_create_config_set(&config);

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vertex_array = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_create(NULL, &vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vao_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_create(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // *vertex_array_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert((renderer_backend_vao_t*)(uintptr_t)0x1U == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_NO_MEMORY を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_create = RENDERER_NO_MEMORY;

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_LIMIT_EXCEEDED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_create = RENDERER_LIMIT_EXCEEDED;

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(NULL == vertex_array);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系: wrapper としては SUCCESS を返す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_create = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_array_create(&context, &vertex_array);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_renderer_backend_vertex_array_create.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_array_destroy(void) {
    {
        // backend_context_ == NULL, vertex_array_ == NULL -> no-op
        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_array_destroy(NULL, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> no-op
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_array_destroy(NULL, &vertex_array);

        assert((renderer_backend_vao_t*)(uintptr_t)0x1U == vertex_array);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vao_vtable == NULL -> no-op
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = NULL;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_array_destroy(&context, &vertex_array);

        assert((renderer_backend_vao_t*)(uintptr_t)0x1U == vertex_array);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_array_ == NULL でも、有効な vtable があればそのまま下位へ委譲してよい
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_array_destroy(&context, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // 有効な context + 有効な vtable -> 下位 destroy に委譲
        // 現在の test_vertex_array_destroy() は no-op なので、vertex_array は変化しない
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_array_destroy(&context, &vertex_array);

        assert((renderer_backend_vao_t*)(uintptr_t)0x1U == vertex_array);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_array_bind(void) {
    {
        // renderer_backend_vertex_array_bind() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_array_bind_config_set(&config);

        ret = renderer_backend_vertex_array_bind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_bind(NULL, vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vao_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = NULL;
        context.current_bound_vao = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_bind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_bind(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_BAD_OPERATION;

        ret = renderer_backend_vertex_array_bind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_vertex_array_bind(&context, vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(222U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_vertex_array_bind() は out_vao_id_ を更新しないため、
        // current_bound_vao は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_array_bind(&context, vertex_array);
        assert(RENDERER_SUCCESS == ret);
        assert(333U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_array_unbind(void) {
    {
        // renderer_backend_vertex_array_unbind() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_array_unbind_config_set(&config);

        ret = renderer_backend_vertex_array_unbind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_unbind(NULL, vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vao_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = NULL;
        context.current_bound_vao = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_unbind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_unbind(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_unbind = RENDERER_BAD_OPERATION;

        ret = renderer_backend_vertex_array_unbind(&context, vertex_array);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_unbind = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_vertex_array_unbind(&context, vertex_array);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(222U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系: wrapper 成功時は current_bound_vao を 0 に更新
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_unbind = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_array_unbind(&context, vertex_array);
        assert(RENDERER_SUCCESS == ret);
        assert(0U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_array_attribute_set(void) {
    {
        // renderer_backend_vertex_array_attribute_set() 冒頭で強制的に
        // RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_array_attribute_set_config_set(&config);

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            vertex_array,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_attribute_set(
            NULL,
            vertex_array,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vao_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = NULL;
        context.current_bound_vao = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            vertex_array,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            NULL,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 bind が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_BAD_OPERATION;
        s_test_config_test_vertex_array_attribute_set = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            vertex_array,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // bind 成功後に下位 attribute_set が RENDERER_RUNTIME_ERROR を返す -> そのまま伝播
        // NOTE: 現在の test_vertex_array_bind() は out_vao_id_ を更新しないため、
        // current_bound_vao は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_SUCCESS;
        s_test_config_test_vertex_array_attribute_set = RENDERER_RUNTIME_ERROR;

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            vertex_array,
            1U,
            4,
            RENDERER_TYPE_FLOAT,
            true,
            sizeof(float) * 8U,
            sizeof(float) * 4U
        );
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(222U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_vertex_array_bind() は out_vao_id_ を更新しないため、
        // current_bound_vao は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vao_t* vertex_array =
            (renderer_backend_vao_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vao_vtable = &s_test_vao_vtable;
        context.current_bound_vao = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_array_bind = RENDERER_SUCCESS;
        s_test_config_test_vertex_array_attribute_set = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_array_attribute_set(
            &context,
            vertex_array,
            0U,
            3,
            RENDERER_TYPE_FLOAT,
            false,
            sizeof(float) * 3U,
            0U
        );
        assert(RENDERER_SUCCESS == ret);
        assert(333U == context.current_bound_vao);
        assert(1U == s_test_config_renderer_backend_vertex_array_attribute_set.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_buffer_create(void) {
    {
        // renderer_backend_vertex_buffer_create() 冒頭で強制的に
        // RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer = NULL;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_buffer_create_config_set(&config);

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vbo_t* vertex_buffer = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_create(NULL, &vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vbo_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = NULL;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_buffer_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_create(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // *vertex_buffer_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert((renderer_backend_vbo_t*)(uintptr_t)0x1U == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_NO_MEMORY を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_create = RENDERER_NO_MEMORY;

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_LIMIT_EXCEEDED を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_create = RENDERER_LIMIT_EXCEEDED;

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(NULL == vertex_buffer);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系: wrapper としては SUCCESS を返す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer = NULL;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_create = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_buffer_create(&context, &vertex_buffer);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_create.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_buffer_destroy(void) {
    {
        // backend_context_ == NULL, vertex_buffer_ == NULL -> no-op
        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_buffer_destroy(NULL, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> no-op
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_buffer_destroy(NULL, &vertex_buffer);

        assert((renderer_backend_vbo_t*)(uintptr_t)0x1U == vertex_buffer);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vbo_vtable == NULL -> no-op
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = NULL;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_buffer_destroy(&context, &vertex_buffer);

        assert((renderer_backend_vbo_t*)(uintptr_t)0x1U == vertex_buffer);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_buffer_ == NULL でも、有効な vtable があればそのまま下位へ委譲してよい
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_buffer_destroy(&context, NULL);

        test_renderer_backend_context_config_reset();
    }
    {
        // 有効な context + 有効な vtable -> 下位 destroy に委譲
        // 現在の test_vertex_buffer_destroy() は no-op なので、vertex_buffer は変化しない
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;

        test_renderer_backend_context_config_reset();

        renderer_backend_vertex_buffer_destroy(&context, &vertex_buffer);

        assert((renderer_backend_vbo_t*)(uintptr_t)0x1U == vertex_buffer);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_buffer_bind(void) {
    {
        // renderer_backend_vertex_buffer_bind() 冒頭で強制的に
        // RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_buffer_bind_config_set(&config);

        ret = renderer_backend_vertex_buffer_bind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_bind(NULL, vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vbo_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = NULL;
        context.current_bound_vbo = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_bind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_buffer_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_bind(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_BAD_OPERATION;

        ret = renderer_backend_vertex_buffer_bind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_vertex_buffer_bind(&context, vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(222U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_vertex_buffer_bind() は out_vbo_id_ を更新しないため、
        // current_bound_vbo は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_buffer_bind(&context, vertex_buffer);
        assert(RENDERER_SUCCESS == ret);
        assert(333U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_bind.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_buffer_unbind(void) {
    {
        // renderer_backend_vertex_buffer_unbind() 冒頭で強制的に
        // RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_buffer_unbind_config_set(&config);

        ret = renderer_backend_vertex_buffer_unbind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_unbind(NULL, vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vbo_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = NULL;
        context.current_bound_vbo = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_unbind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_buffer_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_unbind(&context, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_unbind = RENDERER_BAD_OPERATION;

        ret = renderer_backend_vertex_buffer_unbind(&context, vertex_buffer);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 vtable が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_unbind = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_vertex_buffer_unbind(&context, vertex_buffer);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(222U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系: wrapper 成功時は current_bound_vbo を 0 に更新
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_unbind = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_buffer_unbind(&context, vertex_buffer);
        assert(RENDERER_SUCCESS == ret);
        assert(0U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_unbind.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_backend_vertex_buffer_vertex_load(void) {
    {
        // renderer_backend_vertex_buffer_vertex_load() 冒頭で強制的に
        // RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[9] = {0.0f};
        test_call_control_t config = {0};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 123U;

        test_renderer_backend_context_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)RENDERER_BAD_OPERATION;
        test_renderer_backend_vertex_buffer_vertex_load_config_set(&config);

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[9] = {0.0f};

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_vertex_load(
            NULL,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // backend_context_->vbo_vtable == NULL -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[9] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = NULL;
        context.current_bound_vbo = 456U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(456U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // vertex_buffer_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        float vertex_data[9] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 789U;

        test_renderer_backend_context_config_reset();

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            NULL,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(789U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 下位 bind が RENDERER_BAD_OPERATION を返す -> そのまま伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[9] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 111U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_BAD_OPERATION;
        s_test_config_test_vertex_buffer_vertex_load = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_BAD_OPERATION == ret);
        assert(111U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // bind 成功後に下位 vertex_load が RENDERER_RUNTIME_ERROR を返す -> そのまま伝播
        // NOTE: 現在の test_vertex_buffer_bind() は out_vbo_id_ を更新しないため、
        // current_bound_vbo は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[9] = {0.0f};

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 222U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_SUCCESS;
        s_test_config_test_vertex_buffer_vertex_load = RENDERER_RUNTIME_ERROR;

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            (buffer_usage_t)99999
        );
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(222U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // bind 成功後に下位 vertex_load が RENDERER_INVALID_ARGUMENT を返す -> そのまま伝播
        // NOTE: 現在の test_vertex_buffer_bind() は out_vbo_id_ を更新しないため、
        // current_bound_vbo は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 333U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_SUCCESS;
        s_test_config_test_vertex_buffer_vertex_load = RENDERER_INVALID_ARGUMENT;

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            0U,
            NULL,
            BUFFER_USAGE_STATIC
        );
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(333U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 成功系
        // NOTE: 現在の test_vertex_buffer_bind() は out_vbo_id_ を更新しないため、
        // current_bound_vbo は変化しない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_context_t context = {0};
        renderer_backend_vbo_t* vertex_buffer =
            (renderer_backend_vbo_t*)(uintptr_t)0x1U;
        float vertex_data[12] = {
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f
        };

        context.target_api = GRAPHICS_API_GL33;
        context.vbo_vtable = &s_test_vbo_vtable;
        context.current_bound_vbo = 444U;

        test_renderer_backend_context_config_reset();

        s_test_config_test_vertex_buffer_bind = RENDERER_SUCCESS;
        s_test_config_test_vertex_buffer_vertex_load = RENDERER_SUCCESS;

        ret = renderer_backend_vertex_buffer_vertex_load(
            &context,
            vertex_buffer,
            sizeof(vertex_data),
            vertex_data,
            BUFFER_USAGE_DYNAMIC
        );
        assert(RENDERER_SUCCESS == ret);
        assert(444U == context.current_bound_vbo);
        assert(1U == s_test_config_renderer_backend_vertex_buffer_vertex_load.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_shader_vtable_get(void) {
    {
        // shader_vtable_get() 冒頭で強制的に NULL を返させる
        const renderer_shader_vtable_t* vtable = (const renderer_shader_vtable_t*)0x1;
        const renderer_shader_vtable_t* forced_result = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 1U;
        s_test_config_shader_vtable_get.forced_result = forced_result;

        vtable = shader_vtable_get(GRAPHICS_API_GL33);
        assert(NULL == vtable);
        assert(1U == s_test_config_shader_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // GRAPHICS_API_GL33 -> gl33_shader_vtable_get() を返す
        const renderer_shader_vtable_t* vtable = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 0U;
        s_test_config_shader_vtable_get.forced_result = NULL;

        vtable = shader_vtable_get(GRAPHICS_API_GL33);
        assert(NULL != vtable);
        assert(gl33_shader_vtable_get() == vtable);
        assert(1U == s_test_config_shader_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 既定値外の target_api_ -> NULL
        const renderer_shader_vtable_t* vtable = (const renderer_shader_vtable_t*)0x1;

        test_renderer_backend_context_config_reset();

        s_test_config_shader_vtable_get.call_count = 0U;
        s_test_config_shader_vtable_get.fail_on_call = 0U;
        s_test_config_shader_vtable_get.forced_result = NULL;

        vtable = shader_vtable_get((target_graphics_api_t)99999);
        assert(NULL == vtable);
        assert(1U == s_test_config_shader_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_vao_vtable_get(void) {
    {
        // vao_vtable_get() 冒頭で強制的に NULL を返させる
        const renderer_vao_vtable_t* vtable = (const renderer_vao_vtable_t*)0x1;
        const renderer_vao_vtable_t* forced_result = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 1U;
        s_test_config_vao_vtable_get.forced_result = forced_result;

        vtable = vao_vtable_get(GRAPHICS_API_GL33);
        assert(NULL == vtable);
        assert(1U == s_test_config_vao_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // GRAPHICS_API_GL33 -> gl33_vao_vtable_get() を返す
        const renderer_vao_vtable_t* vtable = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 0U;
        s_test_config_vao_vtable_get.forced_result = NULL;

        vtable = vao_vtable_get(GRAPHICS_API_GL33);
        assert(NULL != vtable);
        assert(gl33_vao_vtable_get() == vtable);
        assert(1U == s_test_config_vao_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 既定値外の target_api_ -> NULL
        const renderer_vao_vtable_t* vtable = (const renderer_vao_vtable_t*)0x1;

        test_renderer_backend_context_config_reset();

        s_test_config_vao_vtable_get.call_count = 0U;
        s_test_config_vao_vtable_get.fail_on_call = 0U;
        s_test_config_vao_vtable_get.forced_result = NULL;

        vtable = vao_vtable_get((target_graphics_api_t)99999);
        assert(NULL == vtable);
        assert(1U == s_test_config_vao_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_vbo_vtable_get(void) {
    {
        // vbo_vtable_get() 冒頭で強制的に NULL を返させる
        const renderer_vbo_vtable_t* vtable = (const renderer_vbo_vtable_t*)0x1;
        const renderer_vbo_vtable_t* forced_result = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 1U;
        s_test_config_vbo_vtable_get.forced_result = forced_result;

        vtable = vbo_vtable_get(GRAPHICS_API_GL33);
        assert(NULL == vtable);
        assert(1U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // GRAPHICS_API_GL33 -> gl33_vbo_vtable_get() を返す
        const renderer_vbo_vtable_t* vtable = NULL;

        test_renderer_backend_context_config_reset();

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 0U;
        s_test_config_vbo_vtable_get.forced_result = NULL;

        vtable = vbo_vtable_get(GRAPHICS_API_GL33);
        assert(NULL != vtable);
        assert(gl33_vbo_vtable_get() == vtable);
        assert(1U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 既定値外の target_api_ -> NULL
        const renderer_vbo_vtable_t* vtable = (const renderer_vbo_vtable_t*)0x1;

        test_renderer_backend_context_config_reset();

        s_test_config_vbo_vtable_get.call_count = 0U;
        s_test_config_vbo_vtable_get.fail_on_call = 0U;
        s_test_config_vbo_vtable_get.forced_result = NULL;

        vtable = vbo_vtable_get((target_graphics_api_t)99999);
        assert(NULL == vtable);
        assert(1U == s_test_config_vbo_vtable_get.call_count);

        test_renderer_backend_context_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_graphics_api_valid_check(void) {
    {
        // graphics_api_valid_check() 冒頭で強制的に false を返させる
        bool ret = true;

        test_renderer_backend_context_config_reset();

        s_test_config_graphics_api_valid_check.call_count = 0U;
        s_test_config_graphics_api_valid_check.fail_on_call = 1U;
        s_test_config_graphics_api_valid_check.forced_result = false;

        ret = graphics_api_valid_check(GRAPHICS_API_GL33);
        assert(false == ret);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // GRAPHICS_API_GL33 -> true
        bool ret = false;

        test_renderer_backend_context_config_reset();

        s_test_config_graphics_api_valid_check.call_count = 0U;
        s_test_config_graphics_api_valid_check.fail_on_call = 0U;
        s_test_config_graphics_api_valid_check.forced_result = false;

        ret = graphics_api_valid_check(GRAPHICS_API_GL33);
        assert(true == ret);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);

        test_renderer_backend_context_config_reset();
    }
    {
        // 既定値外の target_api_ -> false
        bool ret = true;

        test_renderer_backend_context_config_reset();

        s_test_config_graphics_api_valid_check.call_count = 0U;
        s_test_config_graphics_api_valid_check.fail_on_call = 0U;
        s_test_config_graphics_api_valid_check.forced_result = true;

        ret = graphics_api_valid_check((target_graphics_api_t)99999);
        assert(false == ret);
        assert(1U == s_test_config_graphics_api_valid_check.call_count);

        test_renderer_backend_context_config_reset();
    }
}
#endif
