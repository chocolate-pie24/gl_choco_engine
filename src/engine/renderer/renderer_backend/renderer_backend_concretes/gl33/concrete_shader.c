/** @ingroup gl33
 *
 * @file concrete_shader.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダーオブジェクト/シェーダープログラム操作関数の実装
 *
 * @version 0.1
 * @date 2026-01-03
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_shader.h"

#include "engine/renderer/renderer_core/renderer_memory.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <stdlib.h> // for malloc / free
#include <string.h> // for memset

#include "engine/core/memory/choco_memory.h"

#define TEST_INFO_LOG_LENGTH 128

/**
 * @brief 呼び出し側のテスト用強制エラー出力データ構造体
 *
 */
typedef struct fail_injection {
    renderer_result_t result_code;                              /**< 強制的に出力する実行結果コード */
    bool is_enabled;                                            /**< 実行結果コード強制出力機能有効/無効フラグ */

    GLuint result_glCreateShader;                               /**< glCreateShaderのモック関数に強制的に出力させる値 */
    bool is_enabled_glCreateShader;                             /**< glCreateShaderのモック関数の強制出力機能有効フラグ */

    GLuint result_glCreateProgram;                              /**< glCreateProgramのモック関数に強制的に出力させる値 */
    bool is_enabled_glCreateProgram;                            /**< glCreateProgramのモック関数の強制出力機能有効フラグ */

    bool is_enabled_glDeleteShader_no_op;                       /**< glDeleteShaderのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glDeleteProgram_no_op;                      /**< glDeleteProgramのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glShaderSource_no_op;                       /**< glShaderSourceのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glCompileShader_no_op;                      /**< glCompileShaderのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glGetShaderiv;                              /**< glGetShaderivのモック関数で引数のresultに格納する値を強制的にresult_glGetShaderivにするフラグ */
    GLint status_glGetShaderiv;                                 /**< is_enabled_glGetShaderivがtrueかつ、glGetShaderivの第二引数がGL_COMPILE_STATUSの時に、glGetShaderivの引数*params_にこの値をセットする */
    GLint length_glGetShaderiv;                                 /**< is_enabled_glGetShaderivがtrueかつ、glGetShaderivの第二引数がGL_INFO_LOG_LENGTHの時に、glGetShaderivの引数*params_にこの値をセットする */

    bool is_enabled_glGetShaderInfoLog;                         /**< mock_glGetShaderInfoLogの第四引数にセットする文字列を強制的にlog_str_glGetShaderInfoLogにするフラグ */
    char log_str_glGetShaderInfoLog[TEST_INFO_LOG_LENGTH];      /**< is_enabled_glGetShaderInfoLogがtrueの時に、glGetShaderInfoLogの第四引数にコピーする文字列 */

    bool is_enabled_glAttachShader_no_op;                       /**< glAttachShaderのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glLinkProgram_no_op;                        /**< glLinkProgramのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glGetProgramiv;                             /**< glGetShaderivのモック関数で引数のresultに格納する値を強制的にresult_glGetShaderivにするフラグ */
    GLint status_glGetProgramiv;                                /**< is_enabled_glGetProgramivがtrueかつ、glGetProgramivの第二引数がGL_LINK_STATUSの時に、glGetProgramivの引数*params_にこの値をセットする */
    GLint length_glGetProgramiv;                                /**< is_enabled_glGetProgramivがtrueかつ、glGetProgramivの第二引数がGL_INFO_LOG_LENGTHの時に、glGetProgramivの引数*params_にこの値をセットする */

    bool is_enabled_glGetProgramInfoLog;                        /**< mock_glGetProgramInfoLogの第四引数にセットする文字列を強制的にlog_str_glGetProgramInfoLogにするフラグ */
    char log_str_glGetProgramInfoLog[TEST_INFO_LOG_LENGTH];     /**< is_enabled_glGetProgramInfoLogがtrueの時に、glGetProgramInfoLogの第四引数にコピーする文字列 */

    bool is_enabled_glUseProgram_no_op;                         /**< glUseProgramのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glUniformMatrix4fv_no_op;                   /**< glUniformMatrix4fvのモック関数で強制的にNo-opにするフラグ */

    bool is_enabled_glGetUniformLocation;                       /**< trueでglGetUniformLocationの返り値を強制的にlocation_glGetUniformLocationにする */
    GLint location_glGetUniformLocation;                        /**< is_enabled_glGetUniformLocationがtrueの時にglGetUniformLocationは強制的にこの値を返す */
} fail_injection_t;

static fail_injection_t s_fail_injection;   /**< 上位モジュールテスト用構造体インスタンス */

// テスト用private関数プロトタイプ宣言
static void NO_COVERAGE test_gl33_shader_create(void);
static void NO_COVERAGE test_gl33_shader_destroy(void);
static void NO_COVERAGE test_gl33_shader_compile(void);
static void test_gl33_shader_link(void);
static void test_gl33_shader_use(void);
static void test_gl33_shader_resolve_target(void);
static void NO_COVERAGE test_shader_compile_status_get(void);
static void NO_COVERAGE test_gl33_shader_handle_addr_get(void);
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
static renderer_result_t gl33_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);
static renderer_result_t gl33_uniform_location_get(renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_result_t gl33_mat4f_uniform_set(renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_);

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
};

const renderer_shader_vtable_t* gl33_shader_vtable_get(void) {
    return &s_gl33_shader_vtable;
}

static renderer_result_t gl33_shader_create(renderer_backend_shader_t** shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_shader_t* tmp = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_shader_create", "*shader_handle_")

    ret = render_mem_allocate(sizeof(renderer_backend_shader_t), (void**)&tmp);
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
    // NOTE: 将来的に仕様変更でrender_mem_allocate成功した後で失敗することを想定し、cleanup漏れ検出を追加
    // ここはカバレッジ到達不可だけど許容する
    if(RENDERER_SUCCESS != ret && NULL != tmp) {
        assert(false);
    }
#endif
    return ret;
}

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

static renderer_result_t gl33_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLenum gl33_shader_type;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;
    GLuint tmp_handle = 0;
    GLuint* handle_addr = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

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
        // gl33_shader_handle_addr_getで既にエラー処理されているため、ここに来ることはないが将来的な変更のために残しておく
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_resolve_target).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_handle = mock_glCreateShader(gl33_shader_type);
    if(0 == tmp_handle) {
        ret = RENDERER_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to create shader object handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダソースをコンパイル
    mock_glShaderSource(tmp_handle, 1, &shader_source_ , NULL);
    mock_glCompileShader(tmp_handle);

    // コンパイル結果をチェック
    mock_glGetShaderiv(tmp_handle, GL_COMPILE_STATUS, &result);   // コンパイル結果正常でresult = GL_TRUE
    mock_glGetShaderiv(tmp_handle, GL_INFO_LOG_LENGTH, &info_log_length); // コンパイル結果正常でinfo_log_length = 0
    if(0 < info_log_length) {
        ret = render_mem_allocate((size_t)info_log_length, (void**)&err_mes);
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

static renderer_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLuint tmp_program_id = 0;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

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
    mock_glGetProgramiv(tmp_program_id, GL_LINK_STATUS, &result);
    mock_glGetProgramiv(tmp_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(0 < info_log_length) {
        ret = render_mem_allocate((size_t)info_log_length, (void**)&err_mes);
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

static renderer_result_t gl33_shader_use(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

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
 * @retval RENDERER_RUNTIME_ERROR ユニフォーム変数の取得に失敗(変数名称誤り?)
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_uniform_location_get(renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_uniform_location_get", "out_location_")

    int32_t tmp_location = mock_glGetUniformLocation(shader_handle_->program_id, name_);
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
 * @note
 * - OpenGL 3.3実装
 * - 現在使用中のシェーダープログラムと、送信対象シェーダープログラムが異なる場合は、使用中のプログラムが送信対象シェーダープログラムに切り替わる
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
static renderer_result_t gl33_mat4f_uniform_set(renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

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
 * @param[in] shader_type_ 判定対象シェーダ種別を指定する
 * @param[in] shader_handle_ シェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE shader_handle_ == NULL
 * @retval SHADER_COMPILE_STATUS_NOT_COMPILED シェーダーオブジェクトは未コンパイル状態
 * @retval SHADER_COMPILE_STATUS_COMPILED シェーダーオブジェクトはコンパイル済み
 * @retval SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE サポート対象外のシェーダー種別
 */
static shader_compile_status_t shader_compile_status_get(shader_type_t shader_type_, const renderer_backend_shader_t* shader_handle_) {
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
    if(s_fail_injection.is_enabled_glDeleteShader_no_op) {
        return;
    } else {
        glDeleteShader(shader_);
    }
#else
    glDeleteShader(shader_);
#endif
}

static void NO_COVERAGE mock_glDeleteProgram(GLuint program_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glDeleteProgram_no_op) {
        return;
    } else {
        glDeleteProgram(program_);
    }
#else
    glDeleteProgram(program_);
#endif
}

static GLuint NO_COVERAGE mock_glCreateShader(GLenum shader_type_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glCreateShader) {
        return s_fail_injection.result_glCreateShader;
    } else {
        return glCreateShader(shader_type_);
    }
#else
    return glCreateShader(shader_type_);
#endif
}

static GLuint NO_COVERAGE mock_glCreateProgram(void) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glCreateProgram) {
        return s_fail_injection.result_glCreateProgram;
    } else {
        return glCreateProgram();
    }
#else
    return glCreateProgram();
#endif
}

static void NO_COVERAGE mock_glShaderSource(GLuint shader_, GLsizei count_, const GLchar **string_, const GLint *length_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glShaderSource_no_op) {
        return;
    } else {
        glShaderSource(shader_, count_, string_, length_);
    }
#else
    glShaderSource(shader_, count_, string_, length_);
#endif
}

static void NO_COVERAGE mock_glCompileShader(GLuint shader_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glCompileShader_no_op) {
        return;
    } else {
        glCompileShader(shader_);
    }
#else
    glCompileShader(shader_);
#endif
}

static void NO_COVERAGE mock_glGetShaderiv(GLuint shader_, GLenum pname_, GLint *params_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glGetShaderiv && GL_COMPILE_STATUS == pname_) {
        *params_ = s_fail_injection.status_glGetShaderiv;
    } else if(s_fail_injection.is_enabled_glGetShaderiv && GL_INFO_LOG_LENGTH == pname_) {
        *params_ = s_fail_injection.length_glGetShaderiv;
    } else {
        glGetShaderiv(shader_, pname_, params_);
    }
#else
    glGetShaderiv(shader_, pname_, params_);
#endif
}

static void NO_COVERAGE mock_glGetShaderInfoLog(GLuint shader_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glGetShaderInfoLog) {
        if(0 >= maxLength_) {
            return;
        }
        GLsizei copy_length = (maxLength_ > TEST_INFO_LOG_LENGTH) ? TEST_INFO_LOG_LENGTH : maxLength_;
        for(GLsizei i = 0; i != (copy_length - 1); ++i) {
            infoLog_[i] = s_fail_injection.log_str_glGetShaderInfoLog[i];
        }
        infoLog_[copy_length - 1] = '\0';
    } else {
        glGetShaderInfoLog(shader_, maxLength_, length_, infoLog_);
    }
#else
    glGetShaderInfoLog(shader_, maxLength_, length_, infoLog_);
#endif
}

static void NO_COVERAGE mock_glAttachShader(GLuint program_, GLuint shader_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glAttachShader_no_op) {
        return;
    } else {
        glAttachShader(program_, shader_);
    }
#else
    glAttachShader(program_, shader_);
#endif
}

static void NO_COVERAGE mock_glLinkProgram(GLuint program_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glLinkProgram_no_op) {
        return;
    } else {
        glLinkProgram(program_);
    }
#else
    glLinkProgram(program_);
#endif
}

static void NO_COVERAGE mock_glGetProgramiv(GLuint program_, GLenum pname_, GLint *params_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glGetProgramiv && GL_LINK_STATUS == pname_) {
        *params_ = s_fail_injection.status_glGetProgramiv;
    } else if(s_fail_injection.is_enabled_glGetProgramiv && GL_INFO_LOG_LENGTH == pname_) {
        *params_ = s_fail_injection.length_glGetProgramiv;
    } else {
        glGetProgramiv(program_, pname_, params_);
    }
#else
    glGetProgramiv(program_, pname_, params_);
#endif
}

static void NO_COVERAGE mock_glGetProgramInfoLog(GLuint program_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glGetProgramInfoLog) {
        if(0 >= maxLength_) {
            return;
        }
        GLsizei copy_length = (maxLength_ > TEST_INFO_LOG_LENGTH) ? TEST_INFO_LOG_LENGTH : maxLength_;
        for(GLsizei i = 0; i != (copy_length - 1); ++i) {
            infoLog_[i] = s_fail_injection.log_str_glGetProgramInfoLog[i];
        }
        infoLog_[copy_length - 1] = '\0';
    } else {
        glGetProgramInfoLog(program_, maxLength_, length_, infoLog_);
    }
#else
    glGetProgramInfoLog(program_, maxLength_, length_, infoLog_);
#endif
}

static void NO_COVERAGE mock_glUseProgram(GLuint program_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glUseProgram_no_op) {
        return;
    } else {
        glUseProgram(program_);
    }
#else
    glUseProgram(program_);
#endif
}

static void NO_COVERAGE mock_glUniformMatrix4fv(GLint location_, GLsizei count_, GLboolean transpose_, const GLfloat *value_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glUniformMatrix4fv_no_op) {
        return;
    } else {
        glUniformMatrix4fv(location_, count_, transpose_, value_);
    }
#else
    glUniformMatrix4fv(location_, count_, transpose_, value_);
#endif
}

static GLint NO_COVERAGE mock_glGetUniformLocation(GLuint program_, const GLchar *name_) {
#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled_glGetUniformLocation) {
        return s_fail_injection.location_glGetUniformLocation;
    } else {
        return glGetUniformLocation(program_, name_);
    }
#else
    return glGetUniformLocation(program_, name_);
#endif
}

#ifdef TEST_BUILD
void gl33_shader_fail_enable(renderer_result_t result_code_) {
    s_fail_injection.is_enabled = true;
    s_fail_injection.result_code = result_code_;
}

void gl33_shader_fail_disable(void) {
    s_fail_injection.is_enabled = false;
    s_fail_injection.result_code = RENDERER_SUCCESS;

    s_fail_injection.result_glCreateShader = 0;
    s_fail_injection.is_enabled_glCreateShader = false;

    s_fail_injection.result_glCreateProgram = 0;
    s_fail_injection.is_enabled_glCreateProgram = false;

    s_fail_injection.is_enabled_glDeleteShader_no_op = false;

    s_fail_injection.is_enabled_glDeleteProgram_no_op = false;

    s_fail_injection.is_enabled_glShaderSource_no_op = false;

    s_fail_injection.is_enabled_glCompileShader_no_op = false;

    s_fail_injection.is_enabled_glGetShaderiv = false;
    s_fail_injection.status_glGetShaderiv = 0;
    s_fail_injection.length_glGetShaderiv = 0;

    s_fail_injection.is_enabled_glGetShaderInfoLog = false;

    s_fail_injection.is_enabled_glAttachShader_no_op = false;

    s_fail_injection.is_enabled_glLinkProgram_no_op = false;

    s_fail_injection.is_enabled_glGetProgramiv = false;
    s_fail_injection.status_glGetProgramiv = 0;
    s_fail_injection.length_glGetProgramiv = 0;

    s_fail_injection.is_enabled_glGetProgramInfoLog = false;

    s_fail_injection.is_enabled_glUseProgram_no_op = false;

    memset(s_fail_injection.log_str_glGetShaderInfoLog, 0, TEST_INFO_LOG_LENGTH);
    memset(s_fail_injection.log_str_glGetProgramInfoLog, 0, TEST_INFO_LOG_LENGTH);
}

void test_gl33_shader(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
    gl33_shader_fail_disable();

    test_gl33_shader_create();
    test_gl33_shader_destroy();
    test_gl33_shader_compile();
    test_gl33_shader_link();
    test_gl33_shader_use();
    test_gl33_shader_resolve_target();
    test_shader_compile_status_get();
    test_gl33_shader_handle_addr_get();

    memory_system_destroy();
    gl33_shader_fail_disable();
}

static void NO_COVERAGE test_gl33_shader_create(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        // 強制エラー出力機能テスト
        s_fail_injection.is_enabled = true;
        s_fail_injection.result_code = RENDERER_LIMIT_EXCEEDED;

        ret = gl33_shader_create(NULL);
        assert(RENDERER_LIMIT_EXCEEDED == ret);

        s_fail_injection.is_enabled = false;
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        ret = gl33_shader_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // *shader_handle_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_shader_t* shader = NULL;
        shader = malloc(sizeof(renderer_backend_shader_t));

        ret = gl33_shader_create(&shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        free(shader);
    }
    {
        // memory_system_allocateがMEMORY_SYSTEM_LIMIT_EXCEEDED -> RENDERER_LIMIT_EXCEEDED
        memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL == shader);
        assert(RENDERER_LIMIT_EXCEEDED == ret);

        memory_system_test_param_reset();
    }
    {
        // 正常系
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->program_id);
        assert(0 == shader->vertex_shader_handle);
        assert(0 == shader->fragment_shader_handle);
        assert(RENDERER_SUCCESS == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
}

static void NO_COVERAGE test_gl33_shader_destroy(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        // shader_handle_ == NULL -> No-op
        gl33_shader_destroy(NULL);
    }
    {
        // *shader_handle_ == NULL -> No-op
        renderer_backend_shader_t* shader = NULL;
        gl33_shader_destroy(&shader);
    }
    {
        // 0 == (*shader_handle_)->vertex_shader_handle
        // 0 == (*shader_handle_)->fragment_shader_handle
        // 0 == (*shader_handle_)->program_id
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL != shader);
        assert(RENDERER_SUCCESS == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        s_fail_injection.is_enabled_glDeleteProgram_no_op = false;
        s_fail_injection.is_enabled_glDeleteShader_no_op = false;
    }
    {
        // 0 != (*shader_handle_)->vertex_shader_handle
        // 0 != (*shader_handle_)->fragment_shader_handle
        // 0 != (*shader_handle_)->program_id
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL != shader);
        assert(RENDERER_SUCCESS == ret);

        shader->fragment_shader_handle = 1;
        shader->program_id = 2;
        shader->vertex_shader_handle = 3;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        s_fail_injection.is_enabled_glDeleteProgram_no_op = false;
        s_fail_injection.is_enabled_glDeleteShader_no_op = false;
    }
}

static void NO_COVERAGE test_gl33_shader_compile(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    const char* shader_source = "dummy shader source";

    {
        // 強制エラー出力機能テスト
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled = true;
        s_fail_injection.result_code = RENDERER_LIMIT_EXCEEDED;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_LIMIT_EXCEEDED == ret);

        gl33_shader_fail_disable();
    }
    {
        // shader_source_ == NULL -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, NULL, shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_fail_disable();
    }
    {
        // 既にコンパイル済み -> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 1;     // compiled扱い
        shader->fragment_shader_handle = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 既にリンク済み(program_id != 0) -> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 99;              // linked扱い
        shader->vertex_shader_handle = 0;
        shader->fragment_shader_handle = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // shader_type_ が既定値外 -> gl33_shader_handle_addr_getで失敗 -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        ret = gl33_shader_compile((shader_type_t)100, shader_source, shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // glCreateShader が 0 を返す -> RENDERER_SHADER_COMPILE_ERROR
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length == 0 && result == GL_TRUE -> SUCCESS
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 123;

        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;

        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_TRUE;
        s_fail_injection.length_glGetShaderiv = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_SUCCESS == ret);
        assert(123 == shader->vertex_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length > 0 && result == GL_TRUE -> WARN経由でSUCCESS
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 124;

        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;

        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_TRUE;
        s_fail_injection.length_glGetShaderiv = 16;

        s_fail_injection.is_enabled_glGetShaderInfoLog = true;
        memset(s_fail_injection.log_str_glGetShaderInfoLog, 0, TEST_INFO_LOG_LENGTH);
        strncpy(s_fail_injection.log_str_glGetShaderInfoLog, "compile warn", TEST_INFO_LOG_LENGTH - 1);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_SUCCESS == ret);
        assert(124 == shader->vertex_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 && result != GL_TRUE -> RENDERER_SHADER_COMPILE_ERROR（cleanupでDeleteShader）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 125;

        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;

        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_FALSE;
        s_fail_injection.length_glGetShaderiv = 16;

        s_fail_injection.is_enabled_glGetShaderInfoLog = true;
        memset(s_fail_injection.log_str_glGetShaderInfoLog, 0, TEST_INFO_LOG_LENGTH);
        strncpy(s_fail_injection.log_str_glGetShaderInfoLog, "compile error", TEST_INFO_LOG_LENGTH - 1);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length == 0 && result != GL_TRUE -> RENDERER_SHADER_COMPILE_ERROR（cleanupでDeleteShader）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 126;

        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;

        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_FALSE;
        s_fail_injection.length_glGetShaderiv = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 で render_mem_allocate 失敗 -> RENDERER_LIMIT_EXCEEDED（cleanupでDeleteShader）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 127;

        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;

        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_TRUE;
        s_fail_injection.length_glGetShaderiv = 16;

        memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(0 == shader->vertex_shader_handle);

        memory_system_test_param_reset();

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_link(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    {
        // 強制エラー出力機能テスト
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled = true;
        s_fail_injection.result_code = RENDERER_LIMIT_EXCEEDED;

        ret = gl33_shader_link(NULL);
        assert(RENDERER_LIMIT_EXCEEDED == ret);

        gl33_shader_fail_disable();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();

        ret = gl33_shader_link(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_fail_disable();
    }
    {
        // program_id != 0（既にリンク済み）-> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 99;
        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 1;

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // vertex 未コンパイル -> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 0;   // 未
        shader->fragment_shader_handle = 1; // 済扱い

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // fragment 未コンパイル -> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 1;   // 済扱い
        shader->fragment_shader_handle = 0; // 未

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // glCreateProgram が 0 -> RENDERER_SHADER_LINK_ERROR
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 10;
        shader->fragment_shader_handle = 11;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length == 0 && LINK_STATUS == GL_TRUE -> SUCCESS
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 20;
        shader->fragment_shader_handle = 21;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 200;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_TRUE;
        s_fail_injection.length_glGetProgramiv = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SUCCESS == ret);
        assert(200 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length > 0 && LINK_STATUS == GL_TRUE -> WARN経由でSUCCESS
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 30;
        shader->fragment_shader_handle = 31;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 201;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_TRUE;
        s_fail_injection.length_glGetProgramiv = 16;

        s_fail_injection.is_enabled_glGetProgramInfoLog = true;
        memset(s_fail_injection.log_str_glGetProgramInfoLog, 0, TEST_INFO_LOG_LENGTH);
        strncpy(s_fail_injection.log_str_glGetProgramInfoLog, "link warn", TEST_INFO_LOG_LENGTH - 1);

        ret = gl33_shader_link(shader);
        assert(RENDERER_SUCCESS == ret);
        assert(201 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 && LINK_STATUS != GL_TRUE -> RENDERER_SHADER_LINK_ERROR（cleanupでDeleteProgram）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 40;
        shader->fragment_shader_handle = 41;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 202;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_FALSE;
        s_fail_injection.length_glGetProgramiv = 16;

        s_fail_injection.is_enabled_glGetProgramInfoLog = true;
        memset(s_fail_injection.log_str_glGetProgramInfoLog, 0, TEST_INFO_LOG_LENGTH);
        strncpy(s_fail_injection.log_str_glGetProgramInfoLog, "link error", TEST_INFO_LOG_LENGTH - 1);

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length == 0 && LINK_STATUS != GL_TRUE -> RENDERER_SHADER_LINK_ERROR（cleanupでDeleteProgram）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 50;
        shader->fragment_shader_handle = 51;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 203;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_FALSE;
        s_fail_injection.length_glGetProgramiv = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 で render_mem_allocate 失敗 -> RENDERER_LIMIT_EXCEEDED（cleanupでDeleteProgram）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 60;
        shader->fragment_shader_handle = 61;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 204;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_TRUE;
        s_fail_injection.length_glGetProgramiv = 16;

        memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        ret = gl33_shader_link(shader);
        assert(RENDERER_LIMIT_EXCEEDED == ret);
        assert(0 == shader->program_id);

        memory_system_test_param_reset();

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_use(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    {
        // 強制エラー出力機能テスト
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled = true;
        s_fail_injection.result_code = RENDERER_LIMIT_EXCEEDED;

        ret = gl33_shader_use(NULL, NULL);
        assert(RENDERER_LIMIT_EXCEEDED == ret);

        gl33_shader_fail_disable();
    }
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();

        uint32_t out_program_id = 0;
        ret = gl33_shader_use(NULL, &out_program_id);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_fail_disable();
    }
    {
        // out_program_id_ == NULL -> RENDERER_INVALID_ARGUMENT
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        ret = gl33_shader_use(shader, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // program_id == 0 -> RENDERER_BAD_OPERATION
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 0;
        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;

        uint32_t out_program_id = 0;
        ret = gl33_shader_use(shader, &out_program_id);
        assert(RENDERER_BAD_OPERATION == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // out_program_id_ が既に program_id と同一 -> SUCCESS（内部でcompiledチェック/UseProgramしない）
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glUseProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 10;
        shader->vertex_shader_handle = 0;    // 未コンパイルでもOK（この分岐では見ない）
        shader->fragment_shader_handle = 0;

        uint32_t out_program_id = 10;
        ret = gl33_shader_use(shader, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(10 == out_program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // vertex 未コンパイル（program_id!=0 かつ out_program_id!=program_id）-> RENDERER_DATA_CORRUPTED
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glUseProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 11;
        shader->vertex_shader_handle = 0;    // 未
        shader->fragment_shader_handle = 1;  // 済扱い

        uint32_t out_program_id = 0;
        ret = gl33_shader_use(shader, &out_program_id);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(0 == out_program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // fragment 未コンパイル（program_id!=0 かつ out_program_id!=program_id）-> RENDERER_DATA_CORRUPTED
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glUseProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 12;
        shader->vertex_shader_handle = 1;    // 済扱い
        shader->fragment_shader_handle = 0;  // 未

        uint32_t out_program_id = 0;
        ret = gl33_shader_use(shader, &out_program_id);
        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(0 == out_program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: program_id!=0 / out_program_id!=program_id / vertex&fragment compiled -> SUCCESS & out_program_id 更新
        gl33_shader_fail_disable();
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        s_fail_injection.is_enabled_glUseProgram_no_op = true;

        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 13;
        shader->vertex_shader_handle = 2;
        shader->fragment_shader_handle = 3;

        uint32_t out_program_id = 0;
        ret = gl33_shader_use(shader, &out_program_id);
        assert(RENDERER_SUCCESS == ret);
        assert(13 == out_program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_resolve_target(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        // out_gl33_type_ == NULL -> RENDERER_INVALID_ARGUMENT
        ret = gl33_shader_resolve_target(SHADER_TYPE_VERTEX, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // shader_type_が既定値外 -> RENDERER_INVALID_ARGUMENT
        // out_gl33_type_への代入を通らないことを確認する
        GLenum type = GL_VERTEX_SHADER;
        ret = gl33_shader_resolve_target(100, &type);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(GL_VERTEX_SHADER == type);
    }
    {
        // 正常系(GL_VERTEX_SHADER)
        GLenum type = GL_VERTEX_SHADER;
        ret = gl33_shader_resolve_target(SHADER_TYPE_VERTEX, &type);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_VERTEX_SHADER == type);
    }
    {
        // 正常系(GL_FRAGMENT_SHADER)
        GLenum type = GL_VERTEX_SHADER;
        ret = gl33_shader_resolve_target(SHADER_TYPE_FRAGMENT, &type);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_FRAGMENT_SHADER == type);
    }
}

static void NO_COVERAGE test_shader_compile_status_get(void) {
    shader_compile_status_t status = SHADER_COMPILE_STATUS_NOT_COMPILED;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    s_fail_injection.is_enabled_glDeleteShader_no_op = true;
    s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
    {
        // shader_handle_ == NULL -> SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE
        status = shader_compile_status_get(SHADER_TYPE_FRAGMENT, NULL);
        assert(SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE == status);
    }
    {
        // vertex_shader_handle == 0, shader_type_ == SHADER_TYPE_VERTEX -> SHADER_COMPILE_STATUS_NOT_COMPILED
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 0;
        status = shader_compile_status_get(SHADER_TYPE_VERTEX, shader);
        assert(SHADER_COMPILE_STATUS_NOT_COMPILED == status);

        gl33_shader_destroy(&shader);
    }
    {
        // vertex_shader_handle != 0, shader_type_ == SHADER_TYPE_VERTEX -> SHADER_COMPILE_STATUS_COMPILED
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        status = shader_compile_status_get(SHADER_TYPE_VERTEX, shader);
        assert(SHADER_COMPILE_STATUS_COMPILED == status);

        gl33_shader_destroy(&shader);
    }
    {
        // vertex_shader_handle == 0, shader_type_ == SHADER_TYPE_FRAGMENT -> SHADER_COMPILE_STATUS_NOT_COMPILED
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->fragment_shader_handle = 0;
        status = shader_compile_status_get(SHADER_TYPE_FRAGMENT, shader);
        assert(SHADER_COMPILE_STATUS_NOT_COMPILED == status);

        gl33_shader_destroy(&shader);
    }
    {
        // vertex_shader_handle != 0, shader_type_ == SHADER_TYPE_FRAGMENT -> SHADER_COMPILE_STATUS_COMPILED
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->fragment_shader_handle = 1;
        status = shader_compile_status_get(SHADER_TYPE_FRAGMENT, shader);
        assert(SHADER_COMPILE_STATUS_COMPILED == status);

        gl33_shader_destroy(&shader);
    }
    {
        // 既定値外のシェーダー種別 -> SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        status = shader_compile_status_get(100, shader);
        assert(SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_TYPE == status);

        gl33_shader_destroy(&shader);
    }
    s_fail_injection.is_enabled_glDeleteProgram_no_op = false;
    s_fail_injection.is_enabled_glDeleteShader_no_op = false;
}

static void NO_COVERAGE test_gl33_shader_handle_addr_get(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    s_fail_injection.is_enabled_glDeleteShader_no_op = true;
    s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
    {
        // shader_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        GLuint* addr = NULL;
        ret = gl33_shader_handle_addr_get(NULL, SHADER_TYPE_VERTEX, &addr);
        assert(NULL == addr);
        assert(RENDERER_INVALID_ARGUMENT);
    }
    {
        // out_handle_addr_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL != shader);
        assert(RENDERER_SUCCESS == ret);

        ret = gl33_shader_handle_addr_get(shader, SHADER_TYPE_VERTEX, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
    {
        // shader_type_が既定値外 -> RENDERER_INVALID_ARGUMENT
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL != shader);
        assert(RENDERER_SUCCESS == ret);

        GLuint* addr = NULL;
        ret = gl33_shader_handle_addr_get(shader, 100, &addr);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == addr);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
    {
        // 正常系
        // 上書きなし
        renderer_backend_shader_t* shader = NULL;
        ret = gl33_shader_create(&shader);
        assert(NULL != shader);
        assert(RENDERER_SUCCESS == ret);
        shader->fragment_shader_handle = 1;
        shader->vertex_shader_handle = 2;

        GLuint* addr = NULL;
        ret = gl33_shader_handle_addr_get(shader, SHADER_TYPE_VERTEX, &addr);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != addr);
        assert(&shader->vertex_shader_handle == addr);
        assert(2 == *addr);

        // 上書き動作確認
        ret = gl33_shader_handle_addr_get(shader, SHADER_TYPE_FRAGMENT, &addr);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != addr);
        assert(&shader->fragment_shader_handle == addr);
        assert(1 == *addr);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
    s_fail_injection.is_enabled_glDeleteShader_no_op = false;
    s_fail_injection.is_enabled_glDeleteProgram_no_op = false;
}

#endif
