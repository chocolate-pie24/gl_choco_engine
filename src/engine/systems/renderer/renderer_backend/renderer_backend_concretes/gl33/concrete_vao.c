/** @ingroup gl33
 *
 * @file concrete_vao.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VAOを使用するためのラッパーAPIの実装
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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

#include <GL/glew.h>

#include "engine/systems/renderer/renderer_core/renderer_types.h"
#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_memory.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vao.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_vao.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/core/memory/choco_memory.h"

// concrete_vao用モジュール専用テスト制御構造体定義
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
static test_call_control_t s_test_config_gl33_vao_create;                       /**< gl33_vao_create()テスト設定 */
static test_call_control_no_op_t s_test_config_gl33_vao_destroy;                /**< gl33_vao_destroy()テスト設定 */
static test_call_control_t s_test_config_gl33_vao_bind;                         /**< gl33_vao_bind()テスト設定 */
static test_call_control_t s_test_config_gl33_vao_unbind;                       /**< gl33_vao_unbind()テスト設定 */
static test_call_control_t s_test_config_gl33_vao_attribute_set;                /**< gl33_vao_attribute_set()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glGenVertexArrays;          /**< mock_glGenVertexArrays()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glDeleteVertexArrays;       /**< mock_glDeleteVertexArrays()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glBindVertexArray;          /**< mock_glBindVertexArray()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glVertexAttribPointer;      /**< mock_glVertexAttribPointer()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glEnableVertexAttribArray;  /**< mock_glEnableVertexAttribArray()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_gl33_vao_create(void);
static void test_gl33_vao_destroy(void);
static void test_gl33_vao_bind(void);
static void test_gl33_vao_unbind(void);
static void test_gl33_vao_attribute_set(void);

static void test_call_control_no_op_reset(test_call_control_no_op_t* config_);
#endif

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

static const renderer_vao_vtable_t s_gl33_vao_vtable = {
    .vertex_array_create = gl33_vao_create,
    .vertex_array_destroy = gl33_vao_destroy,
    .vertex_array_bind = gl33_vao_bind,
    .vertex_array_unbind = gl33_vao_unbind,
    .vertex_array_attribute_set = gl33_vao_attribute_set,
};  /**< OpenGL3.3用VAO操作仮想関数テーブル */

const renderer_vao_vtable_t* gl33_vao_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_vao_vtable;
}

/**
 * @brief VAO構造体インスタンスのメモリを確保し、VAOハンドルを生成する
 *
 * @param[out] vertex_array_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_がNULL
 * - *vertex_array_が非NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_create(renderer_backend_vao_t** vertex_array_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vao_create.call_count++;
    if(s_test_config_gl33_vao_create.fail_on_call != 0) {
        if(s_test_config_gl33_vao_create.call_count == s_test_config_gl33_vao_create.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vao_create.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_vao_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_create", "vertex_array_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_create", "vertex_array_")

    ret = renderer_mem_allocate(sizeof(renderer_backend_vao_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_vao_create(%s) - Failed to allocate memory for 'tmp'.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glGenVertexArrays(1, &tmp->vao_handle);
    *vertex_array_ = tmp;

    ret = RENDERER_SUCCESS;

cleanup:
#ifdef TEST_BUILD
    // NOTE: 将来的に仕様変更でrenderer_mem_allocate成功した後で失敗することを想定し、cleanup漏れ検出を追加
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
 * @param[in,out] vertex_array_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 */
static void gl33_vao_destroy(renderer_backend_vao_t** vertex_array_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vao_destroy.call_count++;
    if(s_test_config_gl33_vao_destroy.fail_on_call != 0) {
        if(s_test_config_gl33_vao_destroy.call_count == s_test_config_gl33_vao_destroy.fail_on_call) {
            return;
        }
    }
#endif
    if(NULL == vertex_array_) {
        goto cleanup;
    }
    if(NULL == *vertex_array_) {
        goto cleanup;
    }

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
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_ == NULL
 * - out_vao_id_ == NULL
 * @retval RENDERER_BAD_OPERATION 未初期化のvertex_array_が渡された
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_bind(const renderer_backend_vao_t* vertex_array_, uint32_t* out_vao_id_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vao_bind.call_count++;
    if(s_test_config_gl33_vao_bind.fail_on_call != 0) {
        if(s_test_config_gl33_vao_bind.call_count == s_test_config_gl33_vao_bind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vao_bind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_bind", "vertex_array_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vao_id_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_bind", "out_vao_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_array_->vao_handle, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_vao_bind", "vertex_array_->vao_handle")

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
 * @param[in] vertex_array_ VAOリソース管理構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_ == NULL
 * @retval RENDERER_BAD_OPERATION 未初期化のvertex_array_が渡された
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_unbind(const renderer_backend_vao_t* vertex_array_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vao_unbind.call_count++;
    if(s_test_config_gl33_vao_unbind.fail_on_call != 0) {
        if(s_test_config_gl33_vao_unbind.call_count == s_test_config_gl33_vao_unbind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vao_unbind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_unbind", "vertex_array_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_array_->vao_handle, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_vao_unbind", "vertex_array_->vao_handle")

    mock_glBindVertexArray(0);

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief OpenGL3.3用VAOアトリビュート設定
 *
 * @param[in] vertex_array_ VAOリソース管理構造体インスタンスへのポインタ
 * @param[in] layout_ 設定対象変数のlayoutロケーション番号
 * @param[in] size_ 頂点属性のコンポーネントの数
 * @param[in] type_ 頂点属性のデータ型
 * @param[in] normalized_ true: アクセス時に固定小数点データ値を正規化する / false: 正規化しない
 * @param[in] stride_ 連続する頂点属性間のバイトオフセット
 * @param[in] offset_ 設定対象頂点属性が格納されているバイトオフセット
 *
 * @retval RENDERER_INVALID_ARGUMENT vertex_array_ == NULL
 * @retval RENDERER_BAD_OPERATION vao_handleが未初期化
 * @retval RENDERER_RUNTIME_ERROR type_が規定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_vao_attribute_set(const renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
#ifdef TEST_BUILD
    s_test_config_gl33_vao_attribute_set.call_count++;
    if(s_test_config_gl33_vao_attribute_set.fail_on_call != 0) {
        if(s_test_config_gl33_vao_attribute_set.call_count == s_test_config_gl33_vao_attribute_set.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_vao_attribute_set.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_vao_attribute_set", "vertex_array_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_array_->vao_handle, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_vao_attribute_set", "0 == vertex_array_->vao_handle")

    switch(type_) {
    case RENDERER_TYPE_FLOAT:
        mock_glVertexAttribPointer(layout_, size_, GL_FLOAT, normalized_ ? GL_TRUE : GL_FALSE, (GLsizei)stride_, (void*)offset_);
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
    s_test_config_mock_glGenVertexArrays.call_count++;
    if(s_test_config_mock_glGenVertexArrays.fail_on_call != 0) {
        if(s_test_config_mock_glGenVertexArrays.call_count == s_test_config_mock_glGenVertexArrays.fail_on_call) {
            return;
        }
    }
#endif
    glGenVertexArrays(n_, array_);
}

static void NO_COVERAGE mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_) {
#ifdef TEST_BUILD
    s_test_config_mock_glDeleteVertexArrays.call_count++;
    if(s_test_config_mock_glDeleteVertexArrays.fail_on_call != 0) {
        if(s_test_config_mock_glDeleteVertexArrays.call_count == s_test_config_mock_glDeleteVertexArrays.fail_on_call) {
            return;
        }
    }
#endif
    glDeleteVertexArrays(n_, array_);
}

static void NO_COVERAGE mock_glBindVertexArray(GLuint array_) {
#ifdef TEST_BUILD
    s_test_config_mock_glBindVertexArray.call_count++;
    if(s_test_config_mock_glBindVertexArray.fail_on_call != 0) {
        if(s_test_config_mock_glBindVertexArray.call_count == s_test_config_mock_glBindVertexArray.fail_on_call) {
            return;
        }
    }
#endif
    glBindVertexArray(array_);
}

static void NO_COVERAGE mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_) {
#ifdef TEST_BUILD
    s_test_config_mock_glVertexAttribPointer.call_count++;
    if(s_test_config_mock_glVertexAttribPointer.fail_on_call != 0) {
        if(s_test_config_mock_glVertexAttribPointer.call_count == s_test_config_mock_glVertexAttribPointer.fail_on_call) {
            return;
        }
    }
#endif
    glVertexAttribPointer(index_, size_, type_, normalized_, stride_, pointer_);
}

static void NO_COVERAGE mock_glEnableVertexAttribArray(GLuint index_) {
#ifdef TEST_BUILD
    s_test_config_mock_glEnableVertexAttribArray.call_count++;
    if(s_test_config_mock_glEnableVertexAttribArray.fail_on_call != 0) {
        if(s_test_config_mock_glEnableVertexAttribArray.call_count == s_test_config_mock_glEnableVertexAttribArray.fail_on_call) {
            return;
        }
    }
#endif
    glEnableVertexAttribArray(index_);
}

#ifdef TEST_BUILD
void NO_COVERAGE test_concrete_vao_config_reset(void) {
    test_call_control_reset(&s_test_config_gl33_vao_create);
    test_call_control_no_op_reset(&s_test_config_gl33_vao_destroy);
    test_call_control_reset(&s_test_config_gl33_vao_bind);
    test_call_control_reset(&s_test_config_gl33_vao_unbind);
    test_call_control_reset(&s_test_config_gl33_vao_attribute_set);
    test_call_control_no_op_reset(&s_test_config_mock_glGenVertexArrays);
    test_call_control_no_op_reset(&s_test_config_mock_glDeleteVertexArrays);
    test_call_control_no_op_reset(&s_test_config_mock_glBindVertexArray);
    test_call_control_no_op_reset(&s_test_config_mock_glVertexAttribPointer);
    test_call_control_no_op_reset(&s_test_config_mock_glEnableVertexAttribArray);
}

void NO_COVERAGE test_concrete_vao(void) {
    test_gl33_vao_create();
    test_gl33_vao_destroy();
    test_gl33_vao_bind();
    test_gl33_vao_unbind();
    test_gl33_vao_attribute_set();
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_vao_create(void) {
    {
        // gl33_vao_create() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_gl33_vao_create.fail_on_call = 1U;
        s_test_config_gl33_vao_create.forced_result = (int)RENDERER_BAD_OPERATION;

        ret = gl33_vao_create(&vao);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vao);
        assert(1U == s_test_config_gl33_vao_create.call_count);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_vao_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *vertex_array_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t dummy = { 0 };
        renderer_backend_vao_t* vao = &dummy;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_vao_create(&vao);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(&dummy == vao);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // メモリシステム未初期化 -> renderer_mem_allocate() 経由で RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_vao_create(&vao);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == vao);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で MEMORY_SYSTEM_NO_MEMORY を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;
        test_call_control_t config = { 0 };

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_vao_create(&vao);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == vao);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で未定義値を返させる -> RENDERER_UNDEFINED_ERROR
        renderer_result_t ret = RENDERER_SUCCESS;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;
        test_call_control_t config = { 0 };

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = 99999;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_vao_create(&vao);
        assert(RENDERER_UNDEFINED_ERROR == ret);
        assert(NULL == vao);
        assert(0U == s_test_config_mock_glGenVertexArrays.call_count);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: VAO構造体確保成功、mock_glGenVertexArrays() が1回呼ばれる
        // 実 OpenGL 呼び出しを避けるため、mock_glGenVertexArrays() は no-op にする
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        s_test_config_mock_glGenVertexArrays.fail_on_call = 1U;

        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);
        assert(1U == s_test_config_mock_glGenVertexArrays.call_count);

        // renderer_mem_allocate() のゼロ初期化により、no-op の場合は vao_handle は 0 のまま
        assert(0U == vao->vao_handle);

        // destroy 時の実 OpenGL 呼び出しを避ける
        s_test_config_gl33_vao_unbind.fail_on_call = 1U;
        s_test_config_gl33_vao_unbind.forced_result = (int)RENDERER_RUNTIME_ERROR;
        s_test_config_mock_glDeleteVertexArrays.fail_on_call = 1U;

        gl33_vao_destroy(&vao);
        assert(NULL == vao);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_vao_destroy(void) {
    {
        // gl33_vao_destroy() 冒頭で No-Op 終了させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        s_test_config_mock_glGenVertexArrays.fail_on_call = 1U;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        s_test_config_gl33_vao_destroy.fail_on_call = 1U;

        gl33_vao_destroy(&vao);

        // No-Op なのでポインタはそのまま
        assert(NULL != vao);
        assert(1U == s_test_config_gl33_vao_destroy.call_count);
        assert(0U == s_test_config_gl33_vao_unbind.call_count);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);
        assert(0U == s_test_config_mock_glDeleteVertexArrays.call_count);

        // 後片付け
        test_concrete_vao_config_reset();
        s_test_config_mock_glDeleteVertexArrays.fail_on_call = 1U;
        gl33_vao_destroy(&vao);
        assert(NULL == vao);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_array_ == NULL -> no-op
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_vao_destroy(NULL);

        assert(1U == s_test_config_gl33_vao_destroy.call_count);
        assert(0U == s_test_config_gl33_vao_unbind.call_count);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);
        assert(0U == s_test_config_mock_glDeleteVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *vertex_array_ == NULL -> no-op
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_vao_destroy(&vao);

        assert(NULL == vao);
        assert(1U == s_test_config_gl33_vao_destroy.call_count);
        assert(0U == s_test_config_gl33_vao_unbind.call_count);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);
        assert(0U == s_test_config_mock_glDeleteVertexArrays.call_count);

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vao_handle == 0 -> gl33_vao_unbind() は BAD_OPERATION だが、delete/free は継続される
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        // create 成功、ただし glGen は no-op なので vao_handle は 0 のまま
        s_test_config_mock_glGenVertexArrays.fail_on_call = 1U;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);
        assert(0U == vao->vao_handle);

        s_test_config_mock_glDeleteVertexArrays.fail_on_call = 1U;

        gl33_vao_destroy(&vao);

        assert(NULL == vao);
        assert(1U == s_test_config_gl33_vao_destroy.call_count);
        assert(1U == s_test_config_gl33_vao_unbind.call_count);
        // unbind は BAD_OPERATION で cleanup へ行くので glBindVertexArray(0) は呼ばれない
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);
        assert(1U == s_test_config_mock_glDeleteVertexArrays.call_count);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vao_handle != 0 なら unbind -> delete -> free -> NULL化
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_vao_t* vao = NULL;

        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        // 実 OpenGL 呼び出しを避けるため glGen は no-op、生成後に有効ハンドルを手動設定
        s_test_config_mock_glGenVertexArrays.fail_on_call = 1U;
        ret = gl33_vao_create(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != vao);

        vao->vao_handle = 321U;

        s_test_config_mock_glBindVertexArray.fail_on_call = 1U;
        s_test_config_mock_glDeleteVertexArrays.fail_on_call = 1U;

        gl33_vao_destroy(&vao);

        assert(NULL == vao);
        assert(1U == s_test_config_gl33_vao_destroy.call_count);
        assert(1U == s_test_config_gl33_vao_unbind.call_count);
        assert(1U == s_test_config_mock_glBindVertexArray.call_count);
        assert(1U == s_test_config_mock_glDeleteVertexArrays.call_count);

        memory_system_destroy();
        test_concrete_vao_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_vao_bind(void) {
    {
        // gl33_vao_bind() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };
        uint32_t out_vao_id = 0U;

        test_concrete_vao_config_reset();

        vao.vao_handle = 123U;
        s_test_config_gl33_vao_bind.fail_on_call = 1U;
        s_test_config_gl33_vao_bind.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_vao_bind(&vao, &out_vao_id);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == out_vao_id);
        assert(1U == s_test_config_gl33_vao_bind.call_count);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint32_t out_vao_id = 999U;

        test_concrete_vao_config_reset();

        ret = gl33_vao_bind(NULL, &out_vao_id);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(999U == out_vao_id);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // out_vao_id_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 123U;

        ret = gl33_vao_bind(&vao, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vao_handle == 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };
        uint32_t out_vao_id = 999U;

        test_concrete_vao_config_reset();

        vao.vao_handle = 0U;

        ret = gl33_vao_bind(&vao, &out_vao_id);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(999U == out_vao_id);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 既に同じ vao id が bind 済み -> glBindVertexArray は呼ばれず成功
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };
        uint32_t out_vao_id = 456U;

        test_concrete_vao_config_reset();

        vao.vao_handle = 456U;

        ret = gl33_vao_bind(&vao, &out_vao_id);
        assert(RENDERER_SUCCESS == ret);
        assert(456U == out_vao_id);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 異なる vao id が入っている -> bind して out_vao_id_ を更新
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };
        uint32_t out_vao_id = 111U;

        test_concrete_vao_config_reset();

        vao.vao_handle = 789U;
        s_test_config_mock_glBindVertexArray.fail_on_call = 1U;   // 実 OpenGL 呼び出しを避ける

        ret = gl33_vao_bind(&vao, &out_vao_id);
        assert(RENDERER_SUCCESS == ret);
        assert(789U == out_vao_id);
        assert(1U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_vao_unbind(void) {
    {
        // gl33_vao_unbind() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 123U;
        s_test_config_gl33_vao_unbind.fail_on_call = 1U;
        s_test_config_gl33_vao_unbind.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_vao_unbind(&vao);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_gl33_vao_unbind.call_count);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_vao_config_reset();

        ret = gl33_vao_unbind(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vao_handle == 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 0U;

        ret = gl33_vao_unbind(&vao);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 正常系: 有効な vao_handle なら glBindVertexArray(0) を呼んで成功
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 456U;
        s_test_config_mock_glBindVertexArray.fail_on_call = 1U;   // 実 OpenGL 呼び出しを避ける

        ret = gl33_vao_unbind(&vao);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_mock_glBindVertexArray.call_count);

        test_concrete_vao_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_vao_attribute_set(void) {
    {
        // gl33_vao_attribute_set() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 123U;
        s_test_config_gl33_vao_attribute_set.fail_on_call = 1U;
        s_test_config_gl33_vao_attribute_set.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_vao_attribute_set(&vao, 0U, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3U, 0U);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_gl33_vao_attribute_set.call_count);
        assert(0U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(0U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vertex_array_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_vao_config_reset();

        ret = gl33_vao_attribute_set(NULL, 0U, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3U, 0U);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(0U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // vao_handle == 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 0U;

        ret = gl33_vao_attribute_set(&vao, 1U, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 3U, 0U);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(0U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 未対応 type_ -> RENDERER_RUNTIME_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 456U;

        ret = gl33_vao_attribute_set(&vao, 2U, 3, (renderer_type_t)99999, false, sizeof(float) * 3U, 0U);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(0U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 正常系: normalized_ == false
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 789U;
        s_test_config_mock_glVertexAttribPointer.fail_on_call = 1U;      // 実 OpenGL 呼び出しを避ける
        s_test_config_mock_glEnableVertexAttribArray.fail_on_call = 1U;  // 実 OpenGL 呼び出しを避ける

        ret = gl33_vao_attribute_set(&vao, 0U, 3, RENDERER_TYPE_FLOAT, false, sizeof(float) * 5U, sizeof(float) * 2U);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(1U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
    {
        // 正常系: normalized_ == true
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_vao_t vao = { 0 };

        test_concrete_vao_config_reset();

        vao.vao_handle = 987U;
        s_test_config_mock_glVertexAttribPointer.fail_on_call = 1U;      // 実 OpenGL 呼び出しを避ける
        s_test_config_mock_glEnableVertexAttribArray.fail_on_call = 1U;  // 実 OpenGL 呼び出しを避ける

        ret = gl33_vao_attribute_set(&vao, 1U, 4, RENDERER_TYPE_FLOAT, true, sizeof(float) * 8U, sizeof(float) * 4U);
        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_mock_glVertexAttribPointer.call_count);
        assert(1U == s_test_config_mock_glEnableVertexAttribArray.call_count);

        test_concrete_vao_config_reset();
    }
}

static void NO_COVERAGE test_call_control_no_op_reset(test_call_control_no_op_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
}
#endif
