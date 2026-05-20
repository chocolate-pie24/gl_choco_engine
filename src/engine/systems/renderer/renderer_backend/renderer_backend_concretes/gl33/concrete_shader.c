/** @ingroup gl33
 *
 * @file concrete_shader.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダーオブジェクト/シェーダープログラム操作関数の実装
 *
 * @version 0.1
 * @date 2026-01-03
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_shader.h"

#include "engine/systems/renderer/renderer_core/renderer_memory.h"
#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>
#include <stdlib.h> // for malloc / free
#include <string.h> // for memset

#include "test_controller.h"

#include "engine/base/choco_macros.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_shader.h"

#include "engine/core/memory/test_choco_memory.h"

// concrete_shader用モジュール専用テスト制御構造体定義

#define TEST_INFO_LOG_LENGTH 128    /**< テスト用ログ格納配列サイズ */

/**
 * @brief void型の関数を強制的にNo-Opで終了させるための設定値構造体
 *
 */
typedef struct test_call_control_no_op {
    uint32_t call_count;    /**< 関数呼び出し回数 */
    uint32_t fail_on_call;  /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はNo-Opで関数を終了させる) */
} test_call_control_no_op_t;

/**
 * @brief GLuint型の関数の失敗注入設定構造体
 *
 */
typedef struct test_call_control_GLuint {
    uint32_t call_count;        /**< 関数呼び出し回数 */
    uint32_t fail_on_call;      /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    GLuint forced_result;       /**< 関数を強制的に失敗させる際の戻り値 */
} test_call_control_GLuint_t;

/**
 * @brief GLint型の関数の失敗注入設定構造体
 *
 */
typedef struct test_call_control_GLint {
    uint32_t call_count;        /**< 関数呼び出し回数 */
    uint32_t fail_on_call;      /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    GLint forced_result;       /**< 関数を強制的に失敗させる際の戻り値 */
} test_call_control_GLint_t;

/**
 * @brief テスト用ログ情報出力制御構造体
 *
 */
typedef struct test_call_control_log_info {
    uint32_t call_count;            /**< 関数呼び出し回数 */
    uint32_t fail_on_call;          /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はNo-Opで関数を終了させる) */
    GLsizei max_length;             /**< 格納可能なログ文字列長さ */
    char log[TEST_INFO_LOG_LENGTH]; /**< ログ格納配列 */
} test_call_control_log_info_t;

// 外部公開APIテスト設定

// プライベート関数テスト設定
static test_call_control_t s_test_config_gl33_shader_create;                        /**< gl33_shader_create()テスト設定 */
static test_call_control_t s_test_config_gl33_shader_compile;                       /**< gl33_shader_compile()テスト設定 */
static test_call_control_t s_test_config_gl33_shader_link;                          /**< gl33_shader_link()テスト設定 */
static test_call_control_t s_test_config_gl33_shader_use;                           /**< gl33_shader_use()テスト設定 */
static test_call_control_t s_test_config_gl33_uniform_location_get;                 /**< gl33_uniform_location_get()テスト設定 */
static test_call_control_t s_test_config_gl33_mat4f_uniform_set;                    /**< gl33_mat4f_uniform_set()テスト設定 */
static test_call_control_t s_test_config_gl33_shader_handle_addr_get;               /**< gl33_shader_handle_addr_get()テスト設定 */
static test_call_control_t s_test_config_gl33_shader_resolve_target;                /**< gl33_shader_resolve_target()テスト設定*/
static test_call_control_t s_test_config_shader_compile_status_get;                 /**< shader_compile_status_get()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glDeleteShader;                 /**< mock_glDeleteShader()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glDeleteProgram;                /**< mock_glDeleteProgram()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glShaderSource;                 /**< mock_glShaderSource()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glCompileShader;                /**< mock_glCompileShader()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glAttachShader;                 /**< mock_glAttachShader()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glLinkProgram;                  /**< mock_glLinkProgram()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glUseProgram;                   /**< mock_glUseProgram()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glUniformMatrix4fv;             /**< mock_glUniformMatrix4fv()テスト設定 */
static test_call_control_GLuint_t s_test_config_mock_glCreateShader;                /**< mock_glCreateShader()テスト設定 */
static test_call_control_GLuint_t s_test_config_mock_glCreateProgram;               /**< mock_glCreateProgram()テスト設定 */
static test_call_control_GLint_t s_test_config_mock_glGetShaderiv;                  /**< mock_glGetShaderiv()テスト設定 */
static test_call_control_GLint_t s_test_config_mock_glGetProgramiv;                 /**< mock_glGetProgramiv()テスト設定*/
static test_call_control_GLint_t s_test_config_mock_glGetUniformLocation;           /**< mock_glGetUniformLocation()テスト設定 */
static test_call_control_log_info_t s_test_config_mock_glGetShaderInfoLog;          /**< mock_glGetShaderInfoLog()テスト設定 */
static test_call_control_log_info_t s_test_config_mock_glGetProgramInfoLog;         /**< mock_glGetProgramInfoLog()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_gl33_shader_vtable_get(void);
static void test_gl33_shader_create(void);
static void test_gl33_shader_destroy(void);
static void test_gl33_shader_compile(void);
static void test_gl33_shader_link(void);
static void test_gl33_shader_use(void);
static void test_gl33_uniform_location_get(void);
static void test_gl33_mat4f_uniform_set(void);
static void test_gl33_shader_handle_addr_get(void);
static void test_gl33_shader_resolve_target(void);
static void test_shader_compile_status_get(void);
static void test_mock_glDeleteShader(void);
static void test_mock_glDeleteProgram(void);
static void test_mock_glCreateShader(void);
static void test_mock_glCreateProgram(void);
static void test_mock_glShaderSource(void);
static void test_mock_glCompileShader(void);
static void test_mock_glGetShaderiv(void);
static void test_mock_glGetShaderInfoLog(void);
static void test_mock_glAttachShader(void);
static void test_mock_glLinkProgram(void);
static void test_mock_glGetProgramiv(void);
static void test_mock_glGetProgramInfoLog(void);
static void test_mock_glUseProgram(void);
static void test_mock_glUniformMatrix4fv(void);
static void test_mock_glGetUniformLocation(void);

// その他テスト専用関数
static void test_call_control_no_op_reset(test_call_control_no_op_t* config_);
static void test_call_control_GLuint_reset(test_call_control_GLuint_t* config_);
static void test_call_control_GLint_reset(test_call_control_GLint_t* config_);
static void test_call_control_log_info_reset(test_call_control_log_info_t* config_);
#endif

/**
 * @brief シェーダープログラム／シェーダーオブジェクトのハンドルを保持する構造体
 *
 * @details
 * - リンクされたシェーダープログラムのハンドルを保持する
 * - コンパイルされた各シェーダーステージのシェーダーオブジェクトハンドルを保持する
 *
 */
struct renderer_backend_shader {
    GLuint program_id;              /**< リンクしたOpenGLシェーダープログラムへのハンドル */
    GLuint vertex_shader_handle;    /**< コンパイルしたバーテックスシェーダーオブジェクトへのハンドル */
    GLuint fragment_shader_handle;  /**< コンパイルしたフラグメントシェーダーオブジェクトへのハンドル */
};

/**
 * @brief シェーダーオブジェクトコンパイル状況列挙体
 *
 */
typedef enum shader_compile_status {
    SHADER_COMPILE_STATUS_NOT_COMPILED,                 /**< 未コンパイル状態 */
    SHADER_COMPILE_STATUS_COMPILED,                     /**< コンパイル済み状態 */
    SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE,      /**< サポート対象外のシェーダー種別 */
    SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE,        /**< 入力されたシェーダーハンドルが不正 */
} shader_compile_status_t;

static renderer_result_t gl33_shader_create(renderer_backend_shader_t** shader_handle_);
static void gl33_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_result_t gl33_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_result_t gl33_shader_use(const renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);
static renderer_result_t gl33_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_result_t gl33_mat4f_uniform_set(const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_);

static renderer_result_t gl33_shader_handle_addr_get(renderer_backend_shader_t* shader_handle_, shader_type_t shader_type_, GLuint** out_handle_addr_);
static renderer_result_t gl33_shader_resolve_target(shader_type_t shader_type_, GLenum* out_gl33_type_);
static shader_compile_status_t shader_compile_status_get(shader_type_t shader_type_, const renderer_backend_shader_t* shader_handle_);

static void mock_glDeleteShader(GLuint shader_);
static void mock_glDeleteProgram(GLuint program_);
static GLuint mock_glCreateShader(GLenum shader_type_);
static GLuint mock_glCreateProgram(void);
static void mock_glShaderSource(GLuint shader_, GLsizei count_, const GLchar **string_, const GLint *length_);
static void mock_glCompileShader(GLuint shader_);
static void mock_glGetShaderiv(GLuint shader_, GLenum pname_, GLint *params_);
static void mock_glGetShaderInfoLog(GLuint shader_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_);
static void mock_glAttachShader(GLuint program_, GLuint shader_);
static void mock_glLinkProgram(GLuint program_);
static void mock_glGetProgramiv(GLuint program_, GLenum pname_, GLint *params_);
static void mock_glGetProgramInfoLog(GLuint program_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_);
static void mock_glUseProgram(GLuint program_);
static void mock_glUniformMatrix4fv(GLint location_, GLsizei count_, GLboolean transpose_, const GLfloat *value_);
static GLint mock_glGetUniformLocation(GLuint program_, const GLchar *name_);

static const renderer_shader_vtable_t s_gl33_shader_vtable = {
    .renderer_shader_create = gl33_shader_create,
    .renderer_shader_destroy = gl33_shader_destroy,
    .renderer_shader_compile = gl33_shader_compile,
    .renderer_shader_link = gl33_shader_link,
    .renderer_shader_use = gl33_shader_use,
    .renderer_shader_uniform_location_get = gl33_uniform_location_get,
    .renderer_shader_mat4f_uniform_set = gl33_mat4f_uniform_set,
};  /**< OpenGL3.3用シェーダー操作仮想関数テーブル */

const renderer_shader_vtable_t* gl33_shader_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_shader_vtable;
}

/**
 * @brief OpenGL3.3用シェーダーGPUリソース内部状態管理構造体インスタンスのメモリを確保し、フィールドを0で初期化する
 *
 * @param[out] shader_handle_ GPUリソース内部状態管理構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_create(renderer_backend_shader_t** shader_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_create.call_count++;
    if(s_test_config_gl33_shader_create.fail_on_call != 0) {
        if(s_test_config_gl33_shader_create.call_count == s_test_config_gl33_shader_create.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_create.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_shader_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_create", "*shader_handle_")

    ret = renderer_mem_allocate(sizeof(renderer_backend_shader_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_create(%s) - Failed to allocate memory for shader handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->program_id = 0;
    tmp->vertex_shader_handle = 0;
    tmp->fragment_shader_handle = 0;
    *shader_handle_ = tmp;

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
 * @brief OpenGL3.3用シェーダーGPUリソース内部状態管理構造体が管理するリソースを破棄し、自身のメモリも解放する。メモリ解放後はNULLで初期化する
 *
 * @note 以下のGPUリソースを破棄する
 * - バーテックスシェーダー
 * - フラグメントシェーダー
 * - OpenGLシェーダープログラム
 * @note 2重destroyを許可する
 *
 * @param[in,out] shader_handle_ GPUリソース内部状態管理構造体インスタンスへのダブルポインタ
 */
static void gl33_shader_destroy(renderer_backend_shader_t** shader_handle_) {
    if(NULL == shader_handle_) {
        return;
    }
    if(NULL == *shader_handle_) {
        return;
    }
    if(0 != (*shader_handle_)->vertex_shader_handle) {
        mock_glDeleteShader((*shader_handle_)->vertex_shader_handle);
    }
    if(0 != (*shader_handle_)->fragment_shader_handle) {
        mock_glDeleteShader((*shader_handle_)->fragment_shader_handle);
    }
    if(0 != (*shader_handle_)->program_id) {
        mock_glDeleteProgram((*shader_handle_)->program_id);
    }

    render_mem_free(*shader_handle_, sizeof(renderer_backend_shader_t));
    *shader_handle_ = NULL;
}

/**
 * @brief OpenGL3.3用シェーダーオブジェクトを生成し、シェーダーソースをコンパイルする
 *
 * @param[in] shader_type_ シェーダー種別指定値
 * @param[in] shader_source_ シェーダーソース
 * @param[in,out] shader_handle_ シェーダー関連リソース管理構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_source_ == NULL
 * - shader_handle_ == NULL
 * - shader_type_が規定値外
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - 指定したシェーダー種別はすでにコンパイル済み
 * - シェーダープログラムがすでにリンク済み
 * - メモリシステム未初期化
 * @retval RENDERER_SHADER_COMPILE_ERROR シェーダーソースコンパイルエラー
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_compile.call_count++;
    if(s_test_config_gl33_shader_compile.fail_on_call != 0) {
        if(s_test_config_gl33_shader_compile.call_count == s_test_config_gl33_shader_compile.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_compile.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLenum gl33_shader_type;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;
    GLuint tmp_handle = 0;
    GLuint* handle_addr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_compile", "shader_handle_")

    // シェーダーオブジェクトのコンパイル状況チェック
    if(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(shader_type_, shader_handle_)) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader object is already compiled.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムのリンク状況チェック
    if(0 != shader_handle_->program_id) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader program is already linked.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーオブジェクトハンドルを取得
    ret = gl33_shader_handle_addr_get(shader_handle_, shader_type_, &handle_addr);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_handle_addr_get).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダー種別をOpenGLで使用可能な値に変換
    ret = gl33_shader_resolve_target(shader_type_, &gl33_shader_type);
    if(RENDERER_SUCCESS != ret) {
        // NOTE: gl33_shader_handle_addr_getで既にエラー処理されているため、ここに来ることはないが将来的な変更のために残しておく
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_resolve_target).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_handle = mock_glCreateShader(gl33_shader_type);
    if(0 == tmp_handle) {
        ret = RENDERER_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to create shader object handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソースをコンパイル
    mock_glShaderSource(tmp_handle, 1, &shader_source_ , NULL);
    mock_glCompileShader(tmp_handle);

    // コンパイル結果をチェック
    // TODO: そのうちやる(mock_glGetProgramivの失敗注入は、GL_LINK_STATUS / GL_INFO_LOG_LENGTHに応じて個別に設定可能にしないと以下の分岐を全て通すことはできない)
    mock_glGetShaderiv(tmp_handle, GL_COMPILE_STATUS, &result);   // コンパイル結果正常でresult = GL_TRUE
    mock_glGetShaderiv(tmp_handle, GL_INFO_LOG_LENGTH, &info_log_length); // コンパイル結果正常でinfo_log_length = 0
    if(0 < info_log_length) {
        ret = renderer_mem_allocate((size_t)info_log_length, (void**)&err_mes);
        if(RENDERER_SUCCESS != ret) {
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to allocate memory for shader info log.", renderer_rslt_to_str(ret));
            goto cleanup;
        }
        mock_glGetShaderInfoLog(tmp_handle, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_SHADER_COMPILE_ERROR;
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source: '%s'", renderer_rslt_to_str(ret), err_mes);
            render_mem_free(err_mes, (size_t)info_log_length);
            err_mes = NULL;
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_compile - info log: %s", err_mes);
            render_mem_free(err_mes, (size_t)info_log_length);
            err_mes = NULL;
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    *handle_addr = tmp_handle;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret && 0 != tmp_handle) {
        mock_glDeleteShader(tmp_handle);
    }
    return ret;
}

/**
 * @brief コンパイル済みシェーダーオブジェクトをリンクし、OpenGLシェーダープログラムを生成する
 *
 * @param[in,out] shader_handle_ シェーダー関連リソース管理構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT shader_handle_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムがすでにリンク済み
 * - バーテックスシェーダーが未コンパイル
 * - フラグメントシェーダーが未コンパイル
 * @retval RENDERER_SHADER_LINK_ERROR シェーダーリンクエラー
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_link.call_count++;
    if(s_test_config_gl33_shader_link.fail_on_call != 0) {
        if(s_test_config_gl33_shader_link.call_count == s_test_config_gl33_shader_link.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_link.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLuint tmp_program_id = 0;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_link", "shader_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == shader_handle_->program_id, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_shader_link", "shader_handle_->program_id")
    // バーテックスシェーダーとフラグメントシェーダーは必須なので、有効な状態でなければエラー
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_TYPE_VERTEX, shader_handle_), ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_shader_link", "vertex_shader_handle")
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_TYPE_FRAGMENT, shader_handle_), ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_shader_link", "fragment_shader_handle")

    // プログラムをリンク
    tmp_program_id = mock_glCreateProgram();
    if(0 == tmp_program_id) {
        ret = RENDERER_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to create shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glAttachShader(tmp_program_id, shader_handle_->vertex_shader_handle);
    mock_glAttachShader(tmp_program_id, shader_handle_->fragment_shader_handle);
    mock_glLinkProgram(tmp_program_id);

    // プログラムをチェック
    // TODO: そのうちやる(mock_glGetProgramivの失敗注入は、GL_LINK_STATUS / GL_INFO_LOG_LENGTHに応じて個別に設定可能にしないと以下の分岐を全て通すことはできない)
    mock_glGetProgramiv(tmp_program_id, GL_LINK_STATUS, &result);
    mock_glGetProgramiv(tmp_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(0 < info_log_length) {
        ret = renderer_mem_allocate((size_t)info_log_length, (void**)&err_mes);
        if(RENDERER_SUCCESS != ret) {
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to allocate memory for program info log.", renderer_rslt_to_str(ret));
            goto cleanup;
        }
        mock_glGetProgramInfoLog(tmp_program_id, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_SHADER_LINK_ERROR;
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program: '%s'", renderer_rslt_to_str(ret), err_mes);
            render_mem_free(err_mes, (size_t)info_log_length);
            err_mes = NULL;
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_link - info log: %s", err_mes);
            render_mem_free(err_mes, (size_t)info_log_length);
            err_mes = NULL;
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    // use関数呼び出し時に、バリデーション用にprogram_id != 0かつshader_object_handle == 0でDATA_CORRUPTEDにするため、シェーダーオブジェクトのデストロイは行わない(shader_destroy APIでまとめて破棄する)
    shader_handle_->program_id = tmp_program_id;

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret && 0 != tmp_program_id) {
        mock_glDeleteProgram(tmp_program_id);
    }
    return ret;
}

/**
 * @brief OpenGLシェーダープログラムを切り替える
 *
 * @note 切り替え先シェーダープログラムがすでに使用中であれば切り替えは行わない
 *
 * @param[in] shader_handle_ 切り替え先シェーダープログラムを管理する内部状態管理構造体インスタンスへのポインタ
 * @param[in,out] out_program_id_ 現在使用中のシェーダープログラム識別子
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - out_program_id_ == NULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - program_idが設定されているにもかかわらず、バーテックスシェーダーオブジェクトハンドルが未設定
 * - program_idが設定されているにもかかわらず、フラグメントシェーダーオブジェクトハンドルが未設定
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_use(const renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_use.call_count++;
    if(s_test_config_gl33_shader_use.fail_on_call != 0) {
        if(s_test_config_gl33_shader_use.call_count == s_test_config_gl33_shader_use.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_use.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_use", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(out_program_id_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_use", "out_program_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != shader_handle_->program_id, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "gl33_shader_use", "shader_handle_->program_id")

    if(*out_program_id_ != shader_handle_->program_id) {
        if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_TYPE_VERTEX, shader_handle_)) {
            // 既にprogram_idが0ではなく、リンクされているのにvertex_shaderがコンパイル済みではないのは異常
            ret = RENDERER_DATA_CORRUPTED;
            ERROR_MESSAGE("gl33_shader_use(%s) - Vertex shader object is not compiled.", renderer_rslt_to_str(ret));
            goto cleanup;
        }
        // TODO: 現状の失敗注入では、shader_compile_status_getの連続呼び出しに対して両方とも強制出力をさせることができないため、下のifはテスト不可(失敗注入方式を引数のシェーダー種別に応じて切り替えるように修正する)
        if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_TYPE_FRAGMENT, shader_handle_)) {
            // 既にprogram_idが0ではなく、リンクされているのにfragment_shaderがコンパイル済みではないのは異常
            ret = RENDERER_DATA_CORRUPTED;
            ERROR_MESSAGE("gl33_shader_use(%s) - Fragment shader object is not compiled.", renderer_rslt_to_str(ret));
            goto cleanup;
        }
        mock_glUseProgram(shader_handle_->program_id);
        *out_program_id_ = shader_handle_->program_id;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムのユニフォーム変数のLocationを取得する
 *
 * @note OpenGL3.3実装
 *
 * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
 * @param[in] name_ ユニフォーム変数名称
 * @param[out] out_location_ Location格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - name_ == NULL
 * - out_location_ == NULL
 * @retval RENDERER_RUNTIME_ERROR ユニフォーム変数が存在しない、未使用として最適化された、またはLocation取得に失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
#ifdef TEST_BUILD
    s_test_config_gl33_uniform_location_get.call_count++;
    if(s_test_config_gl33_uniform_location_get.fail_on_call != 0) {
        if(s_test_config_gl33_uniform_location_get.call_count == s_test_config_gl33_uniform_location_get.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_uniform_location_get.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    int32_t tmp_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "out_location_")

    tmp_location = mock_glGetUniformLocation(shader_handle_->program_id, name_);
    if(-1 == tmp_location) {
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("gl33_uniform_location_get(%s) - Failed to get uniform location. name: %s", renderer_rslt_to_str(ret), name_);
        goto cleanup;
    }
    *out_location_ = tmp_location;

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
 *
 * @note OpenGL 3.3実装
 *
 * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
 * @param[in] data_ 送信データへのポインタ
 * @param[in,out] out_program_id_ 現在使用中のOpenGLプログラム識別子
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - data_ == NULL
 * - out_program_id_ == NULL
 * @retval RENDERER_DATA_CORRUPTED シェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク状態
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_mat4f_uniform_set(const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_) {
#ifdef TEST_BUILD
    s_test_config_gl33_mat4f_uniform_set.call_count++;
    if(s_test_config_gl33_mat4f_uniform_set.fail_on_call != 0) {
        if(s_test_config_gl33_mat4f_uniform_set.call_count == s_test_config_gl33_mat4f_uniform_set.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_mat4f_uniform_set.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "data_")
    IF_ARG_NULL_GOTO_CLEANUP(out_program_id_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "out_program_id_")

    ret = gl33_shader_use(shader_handle_, out_program_id_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_mat4f_uniform_set(%s) - Failed to switch shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glUniformMatrix4fv(location_, 1, should_transpose_, data_);

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダーオブジェクトハンドルのアドレスを取得する
 *
 * @note *out_handle_addr_への値の上書きは許可する
 *
 * @param[in] shader_handle_ シェーダーオブジェクトハンドルを格納する構造体インスタンスへのポインタ
 * @param[in] shader_type_ アドレスを取得したいシェーダーオブジェクト種別
 * @param[out] out_handle_addr_ シェーダーオブジェクトハンドルのアドレス格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - out_handle_addr_ == NULL
 * - shader_type_が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_handle_addr_get(renderer_backend_shader_t* shader_handle_, shader_type_t shader_type_, GLuint** out_handle_addr_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_handle_addr_get.call_count++;
    if(s_test_config_gl33_shader_handle_addr_get.fail_on_call != 0) {
        if(s_test_config_gl33_shader_handle_addr_get.call_count == s_test_config_gl33_shader_handle_addr_get.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_handle_addr_get.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLuint* tmp_handle = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(out_handle_addr_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "out_handle_addr_")

    switch(shader_type_) {
    case SHADER_TYPE_VERTEX:
        tmp_handle = &shader_handle_->vertex_shader_handle;
        break;
    case SHADER_TYPE_FRAGMENT:
        tmp_handle = &shader_handle_->fragment_shader_handle;
        break;
    default:
        ret = RENDERER_INVALID_ARGUMENT;
        goto cleanup;
    }
    *out_handle_addr_ = tmp_handle;

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief GLCE内で扱うシェーダー種別をOpenGLシェーダー種別に変換する
 *
 * @param[in] shader_type_ 変換元シェーダー種別
 * @param[out] out_gl33_type_ 変換結果格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - out_gl33_type_ == NULL
 * - shader_type_が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_shader_resolve_target(shader_type_t shader_type_, GLenum* out_gl33_type_) {
#ifdef TEST_BUILD
    s_test_config_gl33_shader_resolve_target.call_count++;
    if(s_test_config_gl33_shader_resolve_target.fail_on_call != 0) {
        if(s_test_config_gl33_shader_resolve_target.call_count == s_test_config_gl33_shader_resolve_target.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_shader_resolve_target.forced_result;
        }
    }
#endif
    GLenum tmp_type = GL_VERTEX_SHADER;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_gl33_type_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_resolve_target", "out_gl33_type_")

    switch(shader_type_) {
    case SHADER_TYPE_VERTEX:
        tmp_type = GL_VERTEX_SHADER;
        break;
    case SHADER_TYPE_FRAGMENT:
        tmp_type = GL_FRAGMENT_SHADER;
        break;
    default:
        ret = RENDERER_INVALID_ARGUMENT;
        goto cleanup;
    }
    *out_gl33_type_ = tmp_type;

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダーオブジェクトのコンパイル状況を取得する
 *
 * @param[in] shader_type_ 判定対象シェーダー種別を指定する
 * @param[in] shader_handle_ シェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE shader_handle_ == NULL
 * @retval SHADER_COMPILE_STATUS_NOT_COMPILED シェーダーオブジェクトは未コンパイル状態
 * @retval SHADER_COMPILE_STATUS_COMPILED シェーダーオブジェクトはコンパイル済み
 * @retval SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE サポート対象外のシェーダー種別
 */
static shader_compile_status_t shader_compile_status_get(shader_type_t shader_type_, const renderer_backend_shader_t* shader_handle_) {
#ifdef TEST_BUILD
    s_test_config_shader_compile_status_get.call_count++;
    if(s_test_config_shader_compile_status_get.fail_on_call != 0) {
        if(s_test_config_shader_compile_status_get.call_count == s_test_config_shader_compile_status_get.fail_on_call) {
            return (shader_compile_status_t)s_test_config_shader_compile_status_get.forced_result;
        }
    }
#endif
    shader_compile_status_t status = SHADER_COMPILE_STATUS_NOT_COMPILED;

    if(NULL == shader_handle_) {
        status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
    } else {
        switch(shader_type_) {
        case SHADER_TYPE_VERTEX:
            status = (0 == shader_handle_->vertex_shader_handle) ? SHADER_COMPILE_STATUS_NOT_COMPILED : SHADER_COMPILE_STATUS_COMPILED;
            break;
        case SHADER_TYPE_FRAGMENT:
            status = (0 == shader_handle_->fragment_shader_handle) ? SHADER_COMPILE_STATUS_NOT_COMPILED : SHADER_COMPILE_STATUS_COMPILED;
            break;
        default:
            status = SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE;
        }
    }

    return status;
}

static void NO_COVERAGE mock_glDeleteShader(GLuint shader_) {
#ifdef TEST_BUILD
    s_test_config_mock_glDeleteShader.call_count++;
    if(s_test_config_mock_glDeleteShader.fail_on_call != 0) {
        if(s_test_config_mock_glDeleteShader.call_count == s_test_config_mock_glDeleteShader.fail_on_call) {
            return;
        }
    }
#endif
    glDeleteShader(shader_);
}

static void NO_COVERAGE mock_glDeleteProgram(GLuint program_) {
#ifdef TEST_BUILD
    s_test_config_mock_glDeleteProgram.call_count++;
    if(s_test_config_mock_glDeleteProgram.fail_on_call != 0) {
        if(s_test_config_mock_glDeleteProgram.call_count == s_test_config_mock_glDeleteProgram.fail_on_call) {
            return;
        }
    }
#endif
    glDeleteProgram(program_);
}

static GLuint NO_COVERAGE mock_glCreateShader(GLenum shader_type_) {
#ifdef TEST_BUILD
    s_test_config_mock_glCreateShader.call_count++;
    if(s_test_config_mock_glCreateShader.fail_on_call != 0) {
        if(s_test_config_mock_glCreateShader.call_count == s_test_config_mock_glCreateShader.fail_on_call) {
            return s_test_config_mock_glCreateShader.forced_result;
        }
    }
#endif
    return glCreateShader(shader_type_);
}

static GLuint NO_COVERAGE mock_glCreateProgram(void) {
#ifdef TEST_BUILD
    s_test_config_mock_glCreateProgram.call_count++;
    if(s_test_config_mock_glCreateProgram.fail_on_call != 0) {
        if(s_test_config_mock_glCreateProgram.call_count == s_test_config_mock_glCreateProgram.fail_on_call) {
            return s_test_config_mock_glCreateProgram.forced_result;
        }
    }
#endif
    return glCreateProgram();
}

static void NO_COVERAGE mock_glShaderSource(GLuint shader_, GLsizei count_, const GLchar **string_, const GLint *length_) {
#ifdef TEST_BUILD
    s_test_config_mock_glShaderSource.call_count++;
    if(s_test_config_mock_glShaderSource.fail_on_call != 0) {
        if(s_test_config_mock_glShaderSource.call_count == s_test_config_mock_glShaderSource.fail_on_call) {
            return;
        }
    }
#endif
    glShaderSource(shader_, count_, string_, length_);
}

static void NO_COVERAGE mock_glCompileShader(GLuint shader_) {
#ifdef TEST_BUILD
    s_test_config_mock_glCompileShader.call_count++;
    if(s_test_config_mock_glCompileShader.fail_on_call != 0) {
        if(s_test_config_mock_glCompileShader.call_count == s_test_config_mock_glCompileShader.fail_on_call) {
            return;
        }
    }
#endif
    glCompileShader(shader_);
}

static void NO_COVERAGE mock_glGetShaderiv(GLuint shader_, GLenum pname_, GLint *params_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGetShaderiv.call_count++;
    if(s_test_config_mock_glGetShaderiv.fail_on_call != 0) {
        if(s_test_config_mock_glGetShaderiv.call_count == s_test_config_mock_glGetShaderiv.fail_on_call) {
            *params_ = s_test_config_mock_glGetShaderiv.forced_result;
            return;
        }
    }
#endif
    glGetShaderiv(shader_, pname_, params_);
}

static void NO_COVERAGE mock_glGetShaderInfoLog(GLuint shader_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGetShaderInfoLog.call_count++;
    if(s_test_config_mock_glGetShaderInfoLog.fail_on_call != 0) {
        if(s_test_config_mock_glGetShaderInfoLog.call_count == s_test_config_mock_glGetShaderInfoLog.fail_on_call) {
            if(0 >= s_test_config_mock_glGetShaderInfoLog.max_length) {
                return;
            }
            GLsizei copy_length = (s_test_config_mock_glGetShaderInfoLog.max_length > TEST_INFO_LOG_LENGTH) ? TEST_INFO_LOG_LENGTH : s_test_config_mock_glGetShaderInfoLog.max_length;
            for(GLsizei i = 0; i != (copy_length - 1); ++i) {
                infoLog_[i] = s_test_config_mock_glGetShaderInfoLog.log[i];
            }
            infoLog_[copy_length - 1] = '\0';
            return;
        }
    }
#endif
    glGetShaderInfoLog(shader_, maxLength_, length_, infoLog_);
}

static void NO_COVERAGE mock_glAttachShader(GLuint program_, GLuint shader_) {
#ifdef TEST_BUILD
    s_test_config_mock_glAttachShader.call_count++;
    if(s_test_config_mock_glAttachShader.fail_on_call != 0) {
        if(s_test_config_mock_glAttachShader.call_count == s_test_config_mock_glAttachShader.fail_on_call) {
            return;
        }
    }
#endif
    glAttachShader(program_, shader_);
}

static void NO_COVERAGE mock_glLinkProgram(GLuint program_) {
#ifdef TEST_BUILD
    s_test_config_mock_glLinkProgram.call_count++;
    if(s_test_config_mock_glLinkProgram.fail_on_call != 0) {
        if(s_test_config_mock_glLinkProgram.call_count == s_test_config_mock_glLinkProgram.fail_on_call) {
            return;
        }
    }
#endif
    glLinkProgram(program_);
}

static void NO_COVERAGE mock_glGetProgramiv(GLuint program_, GLenum pname_, GLint *params_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGetProgramiv.call_count++;
    if(s_test_config_mock_glGetProgramiv.fail_on_call != 0) {
        if(s_test_config_mock_glGetProgramiv.call_count == s_test_config_mock_glGetProgramiv.fail_on_call) {
            *params_ = s_test_config_mock_glGetProgramiv.forced_result;
            return;
        }
    }
#endif
    glGetProgramiv(program_, pname_, params_);
}

static void NO_COVERAGE mock_glGetProgramInfoLog(GLuint program_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGetProgramInfoLog.call_count++;
    if(s_test_config_mock_glGetProgramInfoLog.fail_on_call != 0) {
        if(s_test_config_mock_glGetProgramInfoLog.call_count == s_test_config_mock_glGetProgramInfoLog.fail_on_call) {
            if(0 >= s_test_config_mock_glGetProgramInfoLog.max_length) {
                return;
            }
            GLsizei copy_length = (s_test_config_mock_glGetProgramInfoLog.max_length > TEST_INFO_LOG_LENGTH) ? TEST_INFO_LOG_LENGTH : s_test_config_mock_glGetProgramInfoLog.max_length;
            for(GLsizei i = 0; i != (copy_length - 1); ++i) {
                infoLog_[i] = s_test_config_mock_glGetProgramInfoLog.log[i];
            }
            infoLog_[copy_length - 1] = '\0';
            return;
        }
    }
#endif
    glGetProgramInfoLog(program_, maxLength_, length_, infoLog_);
}

static void NO_COVERAGE mock_glUseProgram(GLuint program_) {
#ifdef TEST_BUILD
    s_test_config_mock_glUseProgram.call_count++;
    if(s_test_config_mock_glUseProgram.fail_on_call != 0) {
        if(s_test_config_mock_glUseProgram.call_count == s_test_config_mock_glUseProgram.fail_on_call) {
            return;
        }
    }
#endif
    glUseProgram(program_);
}

static void NO_COVERAGE mock_glUniformMatrix4fv(GLint location_, GLsizei count_, GLboolean transpose_, const GLfloat *value_) {
#ifdef TEST_BUILD
    s_test_config_mock_glUniformMatrix4fv.call_count++;
    if(s_test_config_mock_glUniformMatrix4fv.fail_on_call != 0) {
        if(s_test_config_mock_glUniformMatrix4fv.call_count == s_test_config_mock_glUniformMatrix4fv.fail_on_call) {
            return;
        }
    }
#endif
    glUniformMatrix4fv(location_, count_, transpose_, value_);
}

static GLint NO_COVERAGE mock_glGetUniformLocation(GLuint program_, const GLchar *name_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGetUniformLocation.call_count++;
    if(s_test_config_mock_glGetUniformLocation.fail_on_call != 0) {
        if(s_test_config_mock_glGetUniformLocation.call_count == s_test_config_mock_glGetUniformLocation.fail_on_call) {
            return s_test_config_mock_glGetUniformLocation.forced_result;
        }
    }
#endif
    return glGetUniformLocation(program_, name_);
}

#ifdef TEST_BUILD
void NO_COVERAGE test_concrete_shader_config_reset(void) {
    test_call_control_reset(&s_test_config_gl33_shader_create);
    test_call_control_reset(&s_test_config_gl33_shader_compile);
    test_call_control_reset(&s_test_config_gl33_shader_link);
    test_call_control_reset(&s_test_config_gl33_shader_use);
    test_call_control_reset(&s_test_config_gl33_uniform_location_get);
    test_call_control_reset(&s_test_config_gl33_mat4f_uniform_set);
    test_call_control_reset(&s_test_config_gl33_shader_handle_addr_get);
    test_call_control_reset(&s_test_config_gl33_shader_resolve_target);
    test_call_control_reset(&s_test_config_shader_compile_status_get);
    test_call_control_no_op_reset(&s_test_config_mock_glDeleteShader);
    test_call_control_no_op_reset(&s_test_config_mock_glDeleteProgram);
    test_call_control_no_op_reset(&s_test_config_mock_glShaderSource);
    test_call_control_no_op_reset(&s_test_config_mock_glCompileShader);
    test_call_control_no_op_reset(&s_test_config_mock_glAttachShader);
    test_call_control_no_op_reset(&s_test_config_mock_glLinkProgram);
    test_call_control_no_op_reset(&s_test_config_mock_glUseProgram);
    test_call_control_no_op_reset(&s_test_config_mock_glUniformMatrix4fv);
    test_call_control_GLuint_reset(&s_test_config_mock_glCreateShader);
    test_call_control_GLuint_reset(&s_test_config_mock_glCreateProgram);
    test_call_control_GLint_reset(&s_test_config_mock_glGetShaderiv);
    test_call_control_GLint_reset(&s_test_config_mock_glGetProgramiv);
    test_call_control_GLint_reset(&s_test_config_mock_glGetUniformLocation);
    test_call_control_log_info_reset(&s_test_config_mock_glGetShaderInfoLog);
    test_call_control_log_info_reset(&s_test_config_mock_glGetProgramInfoLog);
}

void NO_COVERAGE test_concrete_shader(void) {
    test_gl33_shader_vtable_get();
    test_gl33_shader_create();
    test_gl33_shader_destroy();
    test_gl33_shader_compile();
    test_gl33_shader_link();
    test_gl33_shader_use();
    test_gl33_uniform_location_get();
    test_gl33_mat4f_uniform_set();
    test_gl33_shader_handle_addr_get();
    test_gl33_shader_resolve_target();
    test_shader_compile_status_get();
    test_mock_glDeleteShader();
    test_mock_glDeleteProgram();
    test_mock_glCreateShader();
    test_mock_glCreateProgram();
    test_mock_glShaderSource();
    test_mock_glCompileShader();
    test_mock_glGetShaderiv();
    test_mock_glGetShaderInfoLog();
    test_mock_glAttachShader();
    test_mock_glLinkProgram();
    test_mock_glGetProgramiv();
    test_mock_glGetProgramInfoLog();
    test_mock_glUseProgram();
    test_mock_glUniformMatrix4fv();
    test_mock_glGetUniformLocation();
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_vtable_get(void) {
    const renderer_shader_vtable_t* vtable1 = NULL;
    const renderer_shader_vtable_t* vtable2 = NULL;

    vtable1 = gl33_shader_vtable_get();
    vtable2 = gl33_shader_vtable_get();

    assert(NULL != vtable1);
    assert(NULL != vtable2);

    // 常に同じ静的vtableを返すこと
    assert(vtable1 == vtable2);
    assert(vtable1 == &s_gl33_shader_vtable);

    // 各関数ポインタが期待する具体実装を指していること
    assert(vtable1->renderer_shader_create == gl33_shader_create);
    assert(vtable1->renderer_shader_destroy == gl33_shader_destroy);
    assert(vtable1->renderer_shader_compile == gl33_shader_compile);
    assert(vtable1->renderer_shader_link == gl33_shader_link);
    assert(vtable1->renderer_shader_use == gl33_shader_use);
    assert(vtable1->renderer_shader_uniform_location_get == gl33_uniform_location_get);
    assert(vtable1->renderer_shader_mat4f_uniform_set == gl33_mat4f_uniform_set);
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_create(void) {
    {
        // gl33_shader_create() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_gl33_shader_create.fail_on_call = 1U;
        s_test_config_gl33_shader_create.forced_result = (int)RENDERER_BAD_OPERATION;

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == shader_handle);
        assert(1U == s_test_config_gl33_shader_create.call_count);

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_shader_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *shader_handle_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        shader_handle = (renderer_backend_shader_t*)malloc(sizeof(renderer_backend_shader_t));
        assert(NULL != shader_handle);
        memset(shader_handle, 0, sizeof(renderer_backend_shader_t));

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL != shader_handle);

        free(shader_handle);
        shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // メモリシステム未初期化 -> renderer_mem_allocate() 経由で RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == shader_handle);

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で MEMORY_SYSTEM_NO_MEMORY を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;
        test_call_control_t config = { 0 };

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == shader_handle);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で未定義値を返させる -> RENDERER_UNDEFINED_ERROR
        renderer_result_t ret = RENDERER_SUCCESS;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;
        test_call_control_t config = { 0 };

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = 99999;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_UNDEFINED_ERROR == ret);
        assert(NULL == shader_handle);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: ハンドル確保成功、内部フィールドはゼロ初期化される
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader_handle);

        assert(0U == shader_handle->program_id);
        assert(0U == shader_handle->vertex_shader_handle);
        assert(0U == shader_handle->fragment_shader_handle);

        gl33_shader_destroy(&shader_handle);
        assert(NULL == shader_handle);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_destroy(void) {
    {
        // shader_handle_ == NULL -> no-op
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_shader_destroy(NULL);

        assert(0U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
    }
    {
        // *shader_handle_ == NULL -> no-op
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_shader_destroy(&shader_handle);

        assert(NULL == shader_handle);
        assert(0U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
    }
    {
        // 全ハンドル 0 の状態 -> render_mem_free() のみ行われ、delete 系は呼ばれない
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader_handle);

        assert(0U == shader_handle->vertex_shader_handle);
        assert(0U == shader_handle->fragment_shader_handle);
        assert(0U == shader_handle->program_id);

        gl33_shader_destroy(&shader_handle);

        assert(NULL == shader_handle);
        assert(0U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_shader_handle != 0 -> mock_glDeleteShader() が1回呼ばれる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader_handle);

        shader_handle->vertex_shader_handle = 111U;

        // 実 OpenGL 呼び出しを避けるため、1回目を no-op にする
        s_test_config_mock_glDeleteShader.fail_on_call = 1U;

        gl33_shader_destroy(&shader_handle);

        assert(NULL == shader_handle);
        assert(1U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // fragment_shader_handle != 0 -> mock_glDeleteShader() が1回呼ばれる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader_handle);

        shader_handle->fragment_shader_handle = 222U;

        // 実 OpenGL 呼び出しを避けるため、1回目を no-op にする
        s_test_config_mock_glDeleteShader.fail_on_call = 1U;

        gl33_shader_destroy(&shader_handle);

        assert(NULL == shader_handle);
        assert(1U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // program_id != 0 -> mock_glDeleteProgram() が1回呼ばれる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_shader_t* shader_handle = NULL;

        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = gl33_shader_create(&shader_handle);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader_handle);

        shader_handle->program_id = 333U;

        // 実 OpenGL 呼び出しを避けるため、1回目を no-op にする
        s_test_config_mock_glDeleteProgram.fail_on_call = 1U;

        gl33_shader_destroy(&shader_handle);

        assert(NULL == shader_handle);
        assert(0U == s_test_config_mock_glDeleteShader.call_count);
        assert(1U == s_test_config_mock_glDeleteProgram.call_count);

        memory_system_destroy();
        test_concrete_shader_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_compile(void) {
    {
        // gl33_shader_compile() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_compile.fail_on_call = 1U;
        s_test_config_gl33_shader_compile.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == shader_handle.program_id);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(1U == s_test_config_gl33_shader_compile.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_source_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, NULL, &shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // 既に vertex shader がコンパイル済み -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 1U;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(1U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // 既に fragment shader がコンパイル済み -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_shader_compile(SHADER_TYPE_FRAGMENT, shader_source, &shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(2U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // 既に program_id != 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        shader_handle.program_id = 10U;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(10U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // gl33_shader_handle_addr_get() が失敗 -> その戻り値を伝播
        renderer_result_t ret = RENDERER_SUCCESS;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_handle_addr_get.fail_on_call = 1U;
        s_test_config_gl33_shader_handle_addr_get.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // gl33_shader_resolve_target() が失敗 -> その戻り値を伝播
        renderer_result_t ret = RENDERER_SUCCESS;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_resolve_target.fail_on_call = 1U;
        s_test_config_gl33_shader_resolve_target.forced_result = (int)RENDERER_INVALID_ARGUMENT;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // shader_type_ が既定値外 -> gl33_shader_handle_addr_get() で RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_SUCCESS;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        ret = gl33_shader_compile((shader_type_t)99999, shader_source, &shader_handle);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // mock_glCreateShader() が 0 を返す -> RENDERER_SHADER_COMPILE_ERROR
        renderer_result_t ret = RENDERER_SUCCESS;
        renderer_backend_shader_t shader_handle = { 0 };
        const char* shader_source = "dummy shader source";

        test_concrete_shader_config_reset();

        s_test_config_mock_glCreateShader.fail_on_call = 1U;
        s_test_config_mock_glCreateShader.forced_result = 0U;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, &shader_handle);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(1U == s_test_config_mock_glCreateShader.call_count);
        assert(0U == s_test_config_mock_glDeleteShader.call_count);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_link(void) {
    {
        // gl33_shader_link() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_link.fail_on_call = 1U;
        s_test_config_gl33_shader_link.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_gl33_shader_link.call_count);
        assert(0U == shader_handle.program_id);
        assert(0U == shader_handle.vertex_shader_handle);
        assert(0U == shader_handle.fragment_shader_handle);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_shader_config_reset();

        ret = gl33_shader_link(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program_id != 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.program_id = 10U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
        assert(10U == shader_handle.program_id);
        assert(1U == shader_handle.vertex_shader_handle);
        assert(2U == shader_handle.fragment_shader_handle);

        test_concrete_shader_config_reset();
    }
    {
        // vertex shader 未コンパイル -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 0U;
        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // fragment shader 未コンパイル -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 0U;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // shader_compile_status_get() を強制的に未コンパイル扱い -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        s_test_config_shader_compile_status_get.fail_on_call = 1U;
        s_test_config_shader_compile_status_get.forced_result = (int)SHADER_COMPILE_STATUS_NOT_COMPILED;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
        assert(0U == shader_handle.program_id);

        test_concrete_shader_config_reset();
    }
    {
        // mock_glCreateProgram() が 0 を返す -> RENDERER_SHADER_LINK_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        s_test_config_mock_glCreateProgram.fail_on_call = 1U;
        s_test_config_mock_glCreateProgram.forced_result = 0U;

        ret = gl33_shader_link(&shader_handle);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(1U == s_test_config_mock_glCreateProgram.call_count);
        assert(0U == s_test_config_mock_glDeleteProgram.call_count);
        assert(0U == shader_handle.program_id);
        assert(1U == shader_handle.vertex_shader_handle);
        assert(2U == shader_handle.fragment_shader_handle);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_use(void) {
    {
        // gl33_shader_use() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_use.fail_on_call = 1U;
        s_test_config_gl33_shader_use.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == out_program_id);
        assert(1U == s_test_config_gl33_shader_use.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        ret = gl33_shader_use(NULL, &out_program_id);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == out_program_id);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // out_program_id_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        ret = gl33_shader_use(&shader_handle, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program_id == 0 -> RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 123U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(123U == out_program_id);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program_id != 0 かつ vertex が未コンパイル -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 10U;
        shader_handle.vertex_shader_handle = 0U;
        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(0U == out_program_id);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program_id != 0 かつ fragment が未コンパイル -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 20U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 0U;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(0U == out_program_id);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // 現在使用中 program_id と一致 -> glUseProgram を呼ばずに成功
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 30U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 30U;
        shader_handle.vertex_shader_handle = 0U;    // この分岐では compile 状態チェックも通らない
        shader_handle.fragment_shader_handle = 0U;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(30U == out_program_id);
        assert(0U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program 切り替え成功 -> glUseProgram を1回呼び、out_program_id_ を更新
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 40U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        // 実 OpenGL 呼び出しを避けるため、1回目を no-op にする
        s_test_config_mock_glUseProgram.fail_on_call = 1U;

        ret = gl33_shader_use(&shader_handle, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(40U == out_program_id);
        assert(1U == s_test_config_mock_glUseProgram.call_count);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_uniform_location_get(void) {
    {
        // gl33_uniform_location_get() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        int32_t location = -999;

        test_concrete_shader_config_reset();

        s_test_config_gl33_uniform_location_get.fail_on_call = 1U;
        s_test_config_gl33_uniform_location_get.forced_result = (int)RENDERER_BAD_OPERATION;

        ret = gl33_uniform_location_get(&shader_handle, "u_mvp", &location);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(-999 == location);
        assert(1U == s_test_config_gl33_uniform_location_get.call_count);
        assert(0U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        int32_t location = -999;

        test_concrete_shader_config_reset();

        ret = gl33_uniform_location_get(NULL, "u_mvp", &location);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(0U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // name_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        int32_t location = -999;

        test_concrete_shader_config_reset();

        ret = gl33_uniform_location_get(&shader_handle, NULL, &location);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-999 == location);
        assert(0U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // out_location_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        ret = gl33_uniform_location_get(&shader_handle, "u_mvp", NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // mock_glGetUniformLocation() が -1 を返す -> RENDERER_RUNTIME_ERROR
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        int32_t location = -999;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 100U;
        s_test_config_mock_glGetUniformLocation.fail_on_call = 1U;
        s_test_config_mock_glGetUniformLocation.forced_result = -1;

        ret = gl33_uniform_location_get(&shader_handle, "u_mvp", &location);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(-999 == location);
        assert(1U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // mock_glGetUniformLocation() が有効な location を返す -> RENDERER_SUCCESS
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        int32_t location = -999;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 200U;
        s_test_config_mock_glGetUniformLocation.fail_on_call = 1U;
        s_test_config_mock_glGetUniformLocation.forced_result = 7;

        ret = gl33_uniform_location_get(&shader_handle, "u_view_proj", &location);
        assert(RENDERER_SUCCESS == ret);
        assert(7 == location);
        assert(1U == s_test_config_mock_glGetUniformLocation.call_count);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_mat4f_uniform_set(void) {
    {
        // gl33_mat4f_uniform_set() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = { 0.0f };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        s_test_config_gl33_mat4f_uniform_set.fail_on_call = 1U;
        s_test_config_gl33_mat4f_uniform_set.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_mat4f_uniform_set(&shader_handle, 3, false, data, &out_program_id);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == out_program_id);
        assert(1U == s_test_config_gl33_mat4f_uniform_set.call_count);
        assert(0U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        float data[16] = { 0.0f };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        ret = gl33_mat4f_uniform_set(NULL, 3, false, data, &out_program_id);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == out_program_id);
        assert(0U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // data_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        ret = gl33_mat4f_uniform_set(&shader_handle, 3, false, NULL, &out_program_id);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == out_program_id);
        assert(0U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // out_program_id_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = { 0.0f };

        test_concrete_shader_config_reset();

        ret = gl33_mat4f_uniform_set(&shader_handle, 3, false, data, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // gl33_shader_use() が失敗 -> その戻り値を伝播
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = { 0.0f };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_use.fail_on_call = 1U;
        s_test_config_gl33_shader_use.forced_result = (int)RENDERER_DATA_CORRUPTED;

        ret = gl33_mat4f_uniform_set(&shader_handle, 3, false, data, &out_program_id);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(0U == out_program_id);
        assert(1U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // program_id == 0 -> gl33_shader_use() 経由で RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = { 0.0f };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 0U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        ret = gl33_mat4f_uniform_set(&shader_handle, 5, true, data, &out_program_id);
        assert(RENDERER_BAD_OPERATION == ret);
        assert(0U == out_program_id);
        assert(1U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // 成功系: program 切り替え後に uniform 送信成功
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        uint32_t out_program_id = 0U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 40U;
        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        // 実 OpenGL 呼び出しを避ける
        s_test_config_mock_glUseProgram.fail_on_call = 1U;
        s_test_config_mock_glUniformMatrix4fv.fail_on_call = 1U;

        ret = gl33_mat4f_uniform_set(&shader_handle, 7, false, data, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(40U == out_program_id);
        assert(1U == s_test_config_gl33_shader_use.call_count);
        assert(1U == s_test_config_mock_glUseProgram.call_count);
        assert(1U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // 成功系: 既に同じ program_id 使用中なら glUseProgram は呼ばれず、uniform 送信のみ行う
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        float data[16] = {
            2.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 2.0f
        };
        uint32_t out_program_id = 50U;

        test_concrete_shader_config_reset();

        shader_handle.program_id = 50U;
        shader_handle.vertex_shader_handle = 0U;
        shader_handle.fragment_shader_handle = 0U;

        // gl33_shader_use() は program_id 一致時、compile 状態を見ずに成功する
        s_test_config_mock_glUniformMatrix4fv.fail_on_call = 1U;

        ret = gl33_mat4f_uniform_set(&shader_handle, 9, true, data, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(50U == out_program_id);
        assert(1U == s_test_config_gl33_shader_use.call_count);
        assert(0U == s_test_config_mock_glUseProgram.call_count);
        assert(1U == s_test_config_mock_glUniformMatrix4fv.call_count);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_handle_addr_get(void) {
    {
        // gl33_shader_handle_addr_get() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        GLuint* out_handle_addr = NULL;

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_handle_addr_get.fail_on_call = 1U;
        s_test_config_gl33_shader_handle_addr_get.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_handle_addr_get(&shader_handle, SHADER_TYPE_VERTEX, &out_handle_addr);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(NULL == out_handle_addr);
        assert(1U == s_test_config_gl33_shader_handle_addr_get.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        GLuint* out_handle_addr = NULL;

        test_concrete_shader_config_reset();

        ret = gl33_shader_handle_addr_get(NULL, SHADER_TYPE_VERTEX, &out_handle_addr);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_handle_addr);

        test_concrete_shader_config_reset();
    }
    {
        // out_handle_addr_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        ret = gl33_shader_handle_addr_get(&shader_handle, SHADER_TYPE_VERTEX, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_concrete_shader_config_reset();
    }
    {
        // SHADER_TYPE_VERTEX -> vertex_shader_handle のアドレスを取得
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        GLuint* out_handle_addr = NULL;

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 11U;
        shader_handle.fragment_shader_handle = 22U;

        ret = gl33_shader_handle_addr_get(&shader_handle, SHADER_TYPE_VERTEX, &out_handle_addr);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != out_handle_addr);
        assert(out_handle_addr == &shader_handle.vertex_shader_handle);
        assert(11U == *out_handle_addr);

        // 取得したアドレス経由で更新できること
        *out_handle_addr = 33U;
        assert(33U == shader_handle.vertex_shader_handle);
        assert(22U == shader_handle.fragment_shader_handle);

        test_concrete_shader_config_reset();
    }
    {
        // SHADER_TYPE_FRAGMENT -> fragment_shader_handle のアドレスを取得
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        GLuint* out_handle_addr = NULL;

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 44U;
        shader_handle.fragment_shader_handle = 55U;

        ret = gl33_shader_handle_addr_get(&shader_handle, SHADER_TYPE_FRAGMENT, &out_handle_addr);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != out_handle_addr);
        assert(out_handle_addr == &shader_handle.fragment_shader_handle);
        assert(55U == *out_handle_addr);

        // 取得したアドレス経由で更新できること
        *out_handle_addr = 66U;
        assert(44U == shader_handle.vertex_shader_handle);
        assert(66U == shader_handle.fragment_shader_handle);

        test_concrete_shader_config_reset();
    }
    {
        // *out_handle_addr_ の上書きは許可される
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        GLuint dummy = 999U;
        GLuint* out_handle_addr = &dummy;

        test_concrete_shader_config_reset();

        ret = gl33_shader_handle_addr_get(&shader_handle, SHADER_TYPE_VERTEX, &out_handle_addr);
        assert(RENDERER_SUCCESS == ret);
        assert(out_handle_addr == &shader_handle.vertex_shader_handle);
        assert(out_handle_addr != &dummy);

        test_concrete_shader_config_reset();
    }
    {
        // 既定値外の shader_type_ -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_shader_t shader_handle = { 0 };
        GLuint* out_handle_addr = NULL;

        test_concrete_shader_config_reset();

        ret = gl33_shader_handle_addr_get(&shader_handle, (shader_type_t)99999, &out_handle_addr);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_handle_addr);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_shader_resolve_target(void) {
    {
        // gl33_shader_resolve_target() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        GLenum gl33_type = 0U;

        test_concrete_shader_config_reset();

        s_test_config_gl33_shader_resolve_target.fail_on_call = 1U;
        s_test_config_gl33_shader_resolve_target.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_shader_resolve_target(SHADER_TYPE_VERTEX, &gl33_type);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(0U == gl33_type);
        assert(1U == s_test_config_gl33_shader_resolve_target.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // out_gl33_type_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_shader_config_reset();

        ret = gl33_shader_resolve_target(SHADER_TYPE_VERTEX, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        test_concrete_shader_config_reset();
    }
    {
        // SHADER_TYPE_VERTEX -> GL_VERTEX_SHADER
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        GLenum gl33_type = 0U;

        test_concrete_shader_config_reset();

        ret = gl33_shader_resolve_target(SHADER_TYPE_VERTEX, &gl33_type);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_VERTEX_SHADER == gl33_type);

        test_concrete_shader_config_reset();
    }
    {
        // SHADER_TYPE_FRAGMENT -> GL_FRAGMENT_SHADER
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        GLenum gl33_type = 0U;

        test_concrete_shader_config_reset();

        ret = gl33_shader_resolve_target(SHADER_TYPE_FRAGMENT, &gl33_type);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_FRAGMENT_SHADER == gl33_type);

        test_concrete_shader_config_reset();
    }
    {
        // 既定値外の shader_type_ -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        GLenum gl33_type = 12345U;

        test_concrete_shader_config_reset();

        ret = gl33_shader_resolve_target((shader_type_t)99999, &gl33_type);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(12345U == gl33_type);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_shader_compile_status_get(void) {
    {
        // shader_compile_status_get() 冒頭で強制的に COMPILED を返させる
        shader_compile_status_t status = SHADER_COMPILE_STATUS_NOT_COMPILED;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        s_test_config_shader_compile_status_get.fail_on_call = 1U;
        s_test_config_shader_compile_status_get.forced_result = (int)SHADER_COMPILE_STATUS_COMPILED;

        status = shader_compile_status_get(SHADER_TYPE_VERTEX, &shader_handle);
        assert(SHADER_COMPILE_STATUS_COMPILED == status);
        assert(1U == s_test_config_shader_compile_status_get.call_count);

        test_concrete_shader_config_reset();
    }
    {
        // shader_handle_ == NULL -> SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE
        shader_compile_status_t status = SHADER_COMPILE_STATUS_NOT_COMPILED;

        test_concrete_shader_config_reset();

        status = shader_compile_status_get(SHADER_TYPE_VERTEX, NULL);
        assert(SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE == status);

        test_concrete_shader_config_reset();
    }
    {
        // vertex_shader_handle == 0 -> SHADER_COMPILE_STATUS_NOT_COMPILED
        shader_compile_status_t status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 0U;
        shader_handle.fragment_shader_handle = 999U;

        status = shader_compile_status_get(SHADER_TYPE_VERTEX, &shader_handle);
        assert(SHADER_COMPILE_STATUS_NOT_COMPILED == status);

        test_concrete_shader_config_reset();
    }
    {
        // vertex_shader_handle != 0 -> SHADER_COMPILE_STATUS_COMPILED
        shader_compile_status_t status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 123U;
        shader_handle.fragment_shader_handle = 0U;

        status = shader_compile_status_get(SHADER_TYPE_VERTEX, &shader_handle);
        assert(SHADER_COMPILE_STATUS_COMPILED == status);

        test_concrete_shader_config_reset();
    }
    {
        // fragment_shader_handle == 0 -> SHADER_COMPILE_STATUS_NOT_COMPILED
        shader_compile_status_t status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 999U;
        shader_handle.fragment_shader_handle = 0U;

        status = shader_compile_status_get(SHADER_TYPE_FRAGMENT, &shader_handle);
        assert(SHADER_COMPILE_STATUS_NOT_COMPILED == status);

        test_concrete_shader_config_reset();
    }
    {
        // fragment_shader_handle != 0 -> SHADER_COMPILE_STATUS_COMPILED
        shader_compile_status_t status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 0U;
        shader_handle.fragment_shader_handle = 456U;

        status = shader_compile_status_get(SHADER_TYPE_FRAGMENT, &shader_handle);
        assert(SHADER_COMPILE_STATUS_COMPILED == status);

        test_concrete_shader_config_reset();
    }
    {
        // 既定値外の shader_type_ -> SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE
        shader_compile_status_t status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
        renderer_backend_shader_t shader_handle = { 0 };

        test_concrete_shader_config_reset();

        shader_handle.vertex_shader_handle = 1U;
        shader_handle.fragment_shader_handle = 2U;

        status = shader_compile_status_get((shader_type_t)99999, &shader_handle);
        assert(SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE == status);

        test_concrete_shader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glDeleteShader(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glDeleteProgram(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glCreateShader(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glCreateProgram(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glShaderSource(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glCompileShader(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glGetShaderiv(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glGetShaderInfoLog(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glAttachShader(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glLinkProgram(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glGetProgramiv(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glGetProgramInfoLog(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glUseProgram(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glUniformMatrix4fv(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

// Generated by ChatGPT
static void NO_COVERAGE test_mock_glGetUniformLocation(void) {
    // モック関数のテストは当面は不要(ほとんどが失敗注入だけのため他のテストケースで代用可能であるため)
}

static void NO_COVERAGE test_call_control_no_op_reset(test_call_control_no_op_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
}

static void NO_COVERAGE test_call_control_GLuint_reset(test_call_control_GLuint_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
    config_->forced_result = 0;
}

static void NO_COVERAGE test_call_control_GLint_reset(test_call_control_GLint_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
    config_->forced_result = 0;
}

static void NO_COVERAGE test_call_control_log_info_reset(test_call_control_log_info_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
    config_->max_length = 0;
    config_->log[0] = '\0';
}
#endif
