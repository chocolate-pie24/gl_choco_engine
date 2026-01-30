/** @ingroup gl33
 *
 * @file gl33_shader.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダープログラムのコンパイル、リンク、使用開始APIの実装
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

#include <GL/glew.h>

#include "engine/renderer/renderer_backend/gl33/gl33_shader.h"

#include "engine/renderer/renderer_core/renderer_memory.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <stdio.h>
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
} fail_injection_t;

static fail_injection_t s_fail_injection;   /**< 上位モジュールテスト用構造体インスタンス */

// テスト用private関数プロトタイプ宣言
static void test_gl33_shader_create(void);
static void test_gl33_shader_destroy(void);
static void test_gl33_shader_compile(void);
static void test_gl33_shader_link(void);
static void test_gl33_shader_use(void);
static void test_gl33_shader_resolve_target(void);
static void test_shader_is_compiled(void);
#endif

/**
 * @brief OpenGL3.3用シェーダーハンドル構造体
 *
 */
struct gl33_shader {
    GLuint program_id;              /**< リンクしたOpenGlシェーダープログラムへのハンドル */
    GLuint vertex_shader_handle;    /**< コンパイルしたバーテックスシェーダープログラムへのハンドル */
    GLuint fragment_shader_handle;  /**< コンパイルしたフラグメントシェーダープログラムへのハンドル */
};

static renderer_result_t gl33_shader_resolve_target(gl33_shader_t* shader_handle_, shader_type_t shader_type_, GLenum* out_gl33_type_, GLuint** out_target_);
static bool shader_is_compiled(shader_type_t shader_type_, gl33_shader_t* shader_handle_);

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

renderer_result_t gl33_shader_create(gl33_shader_t** shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    gl33_shader_t* tmp = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    CHECK_ARG_NULL_GOTO_CLEANUP(shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_create", "shader_handle_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_create", "*shader_handle_")

    ret = render_mem_allocate(sizeof(gl33_shader_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_create(%s) - Failed to allocate memory for shader handle.", renderer_result_to_str(ret));
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

void gl33_shader_destroy(gl33_shader_t** shader_handle_) {
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

    render_mem_free(*shader_handle_, sizeof(gl33_shader_t));
    *shader_handle_ = NULL;
}

renderer_result_t gl33_shader_compile(shader_type_t shader_type_, const char* shader_source_, gl33_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    GLenum gl33_shader_type;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;
    GLuint tmp_handle = 0;
    GLuint* dst_handle = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    CHECK_ARG_NULL_GOTO_CLEANUP(shader_source_, RENDERER_INVALID_ARGUMENT, "gl33_shader_compile", "shader_source_")
    CHECK_ARG_NULL_GOTO_CLEANUP(shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_compile", "shader_handle_")
    if(shader_is_compiled(shader_type_, shader_handle_)) {
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader is already compiled.", renderer_result_to_str(ret));
        goto cleanup;
    } else if(0 != shader_handle_->program_id) {
        // 既にリンク済みなのにコンパイルしようとした
        ret = RENDERER_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader program is already linked.", renderer_result_to_str(ret));
        goto cleanup;
    }
    ret = gl33_shader_resolve_target(shader_handle_, shader_type_, &gl33_shader_type, &dst_handle);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type.", renderer_result_to_str(ret));
        goto cleanup;
    }

    tmp_handle = mock_glCreateShader(gl33_shader_type);
    if(0 == tmp_handle) {
        ret = RENDERER_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to create shader handle.", renderer_result_to_str(ret));
        goto cleanup;
    }

    // シェーダをコンパイル
    mock_glShaderSource(tmp_handle, 1, &shader_source_ , NULL);
    mock_glCompileShader(tmp_handle);

    // シェーダをチェック
    mock_glGetShaderiv(tmp_handle, GL_COMPILE_STATUS, &result);   // コンパイル結果正常でresult = GL_TRUE
    mock_glGetShaderiv(tmp_handle, GL_INFO_LOG_LENGTH, &info_log_length); // コンパイル結果正常でinfo_log_length = 0
    if(0 < info_log_length) {
        ret = render_mem_allocate((size_t)info_log_length, (void**)&err_mes);
        if(RENDERER_SUCCESS != ret) {
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to allocate memory for shader info log.", renderer_result_to_str(ret));
            goto cleanup;
        }
        mock_glGetShaderInfoLog(tmp_handle, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_SHADER_COMPILE_ERROR;
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source: '%s'", renderer_result_to_str(ret), err_mes);
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
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader.", renderer_result_to_str(ret));
        goto cleanup;
    }
    *dst_handle = tmp_handle;

    ret = RENDERER_SUCCESS;
cleanup:
    if(RENDERER_SUCCESS != ret && 0 != tmp_handle) {
        mock_glDeleteShader(tmp_handle);
    }
    return ret;
}

renderer_result_t gl33_shader_link(gl33_shader_t* shader_handle_) {
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

    CHECK_ARG_NULL_GOTO_CLEANUP(shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_link", "shader_handle_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 == shader_handle_->program_id, RENDERER_BAD_OPERATION, "gl33_shader_link", "shader_handle_->program_id")
    // バーテックスシェーダーとフラグメントシェーダーは必須なので、有効な状態でなければエラー
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(shader_is_compiled(SHADER_TYPE_VERTEX, shader_handle_), RENDERER_BAD_OPERATION, "gl33_shader_link", "vertex_shader_handle")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(shader_is_compiled(SHADER_TYPE_FRAGMENT, shader_handle_), RENDERER_BAD_OPERATION, "gl33_shader_link", "fragment_shader_handle")

    // プログラムをリンク
    tmp_program_id = mock_glCreateProgram();
    if(0 == tmp_program_id) {
        ret = RENDERER_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to create shader program.", renderer_result_to_str(ret));
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
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to allocate memory for program info log.", renderer_result_to_str(ret));
            goto cleanup;
        }
        mock_glGetProgramInfoLog(tmp_program_id, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_SHADER_LINK_ERROR;
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program: '%s'", renderer_result_to_str(ret), err_mes);
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
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program.", renderer_result_to_str(ret));
        goto cleanup;
    }
    // バリデーションがprogram_id != 0かつshader_handle == 0でDATA_CORRUPTEDにするため、シェーダーのデストロイは行わない
    shader_handle_->program_id = tmp_program_id;
    ret = RENDERER_SUCCESS;
cleanup:
    if(RENDERER_SUCCESS != ret && 0 != tmp_program_id) {
        mock_glDeleteProgram(tmp_program_id);
    }
    return ret;
}

renderer_result_t gl33_shader_use(gl33_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    CHECK_ARG_NULL_GOTO_CLEANUP(shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_use", "shader_handle_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != shader_handle_->program_id, RENDERER_BAD_OPERATION, "gl33_shader_use", "shader_handle_->program_id")
    if(!shader_is_compiled(SHADER_TYPE_VERTEX, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにvertex_shaderが無効なのは異常
        ret = RENDERER_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Vertex shader is not compiled.", renderer_result_to_str(ret));
        goto cleanup;
    }
    if(!shader_is_compiled(SHADER_TYPE_FRAGMENT, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにfragment_shaderが無効なのは異常
        ret = RENDERER_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Fragment shader is not compiled.", renderer_result_to_str(ret));
        goto cleanup;
    }
    mock_glUseProgram(shader_handle_->program_id);
    ret = RENDERER_SUCCESS;
cleanup:
    return ret;
}

/**
 * @brief shader_type_をOpenGL固有のシェーダー種別に変換し、shader_handle_が保持する対象シェーダー種別のハンドルへのポインタを取得する
 *
 * @param shader_handle_ 取得するハンドルへのポインタを保持するシェーダーハンドル
 * @param shader_type_ シェーダー種別 @ref shader_type_t
 * @param out_gl33_type_ OpenGL固有のシェーダー種別値格納先
 * @param out_target_ シェーダーハンドルへのポインタ格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - out_gl33_type_ == NULL
 * - out_target_ == NULL
 * - *out_target_ != NULL
 * - shader_type_の値が本関数で認識できない値
 * @retval RENDERER_SUCCESS シェーダー種別およびシェーダーハンドルへのポインタの取得に成功し、正常終了
 */
static renderer_result_t gl33_shader_resolve_target(gl33_shader_t* shader_handle_, shader_type_t shader_type_, GLenum* out_gl33_type_, GLuint** out_target_) {
    GLenum tmp_type;
    GLuint* tmp_handle;
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(shader_handle_, RENDERER_INVALID_ARGUMENT, "gl33_shader_resolve_target", "shader_handle_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_gl33_type_, RENDERER_INVALID_ARGUMENT, "gl33_shader_resolve_target", "out_gl33_type_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_target_, RENDERER_INVALID_ARGUMENT, "gl33_shader_resolve_target", "out_target_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_target_, RENDERER_INVALID_ARGUMENT, "gl33_shader_resolve_target", "*out_target_")

    switch(shader_type_) {
    case SHADER_TYPE_VERTEX:
        tmp_type = GL_VERTEX_SHADER;
        tmp_handle = &shader_handle_->vertex_shader_handle;
        break;
    case SHADER_TYPE_FRAGMENT:
        tmp_type = GL_FRAGMENT_SHADER;
        tmp_handle = &shader_handle_->fragment_shader_handle;
        break;
    default:
        ret = RENDERER_INVALID_ARGUMENT;
        goto cleanup;
    }
    *out_gl33_type_ = tmp_type;
    *out_target_ = tmp_handle;

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief shader_handle_が保持するシェーダーハンドルがコンパイル済みかを判定する
 *
 * @param shader_type_ シェーダ種別 @ref shader_type_t
 * @param shader_handle_ シェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval true コンパイル済み
 * @retval false 以下のいずれか
 * - コンパイル済みではない
 * - shader_handle_ == NULL
 * - shader_type_が本関数で未対応の種別
 */
static bool shader_is_compiled(shader_type_t shader_type_, gl33_shader_t* shader_handle_) {
    bool ret = false;
    if(NULL == shader_handle_) {
        ret = false;
    } else {
        switch(shader_type_) {
        case SHADER_TYPE_VERTEX:
            ret = (0 == shader_handle_->vertex_shader_handle) ? false : true;
            break;
        case SHADER_TYPE_FRAGMENT:
            ret = (0 == shader_handle_->fragment_shader_handle) ? false : true;
            break;
        default:
            ret = false;
        }
    }
    return ret;
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
    test_shader_is_compiled();

    memory_system_destroy();
    gl33_shader_fail_disable();
}

static void NO_COVERAGE test_gl33_shader_create(void) {
    {
        // 強制エラー出力（gl33_shader_create先頭のfail_injection分岐のカバレッジ確保）
        gl33_shader_fail_enable(RENDERER_RUNTIME_ERROR);

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_handle_ がNULL
        renderer_result_t ret = gl33_shader_create(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: *shader_handle_ がNULL以外（2重create防止）
        gl33_shader_t dummy;
        gl33_shader_t* shader = &dummy;

        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(&dummy == shader);
    }
    {
        // 異常系: render_mem_allocate失敗
        render_mem_test_param_set(RENDERER_NO_MEMORY);

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == shader);

        render_mem_test_param_reset();
    }
    {
        // 正常系
        gl33_shader_t* shader = NULL;

        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        // 最低限の状態確認
        assert(0 == shader->program_id);
        assert(0 == shader->vertex_shader_handle);
        assert(0 == shader->fragment_shader_handle);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
}

static void NO_COVERAGE test_gl33_shader_destroy(void) {
    {
        // 異常系: shader_handle_ == NULL（何もせずreturn）
        gl33_shader_destroy(NULL);
    }
    {
        // 異常系: *shader_handle_ == NULL（何もせずreturn）
        gl33_shader_t* shader = NULL;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
    {
        // 正常系: 全ハンドルが0（delete系は呼ばれず、freeのみ）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        // create直後の前提（念のため）
        assert(0 == shader->vertex_shader_handle);
        assert(0 == shader->fragment_shader_handle);
        assert(0 == shader->program_id);

        gl33_shader_destroy(&shader);
        assert(NULL == shader);
    }
    {
        // 正常系: ハンドル非0（各delete分岐を踏む）
        // OpenGLコンテキスト無しでも安全に通すため、mock側をNo-opにする
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        // glDeleteShader / glDeleteProgram を実呼び出ししない
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 3;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        // 影響排除
        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_compile(void) {
    const char* dummy_src = "void main() { }";

    {
        // 強制エラー出力（gl33_shader_compile先頭のfail_injection分岐のカバレッジ確保）
        gl33_shader_fail_enable(RENDERER_RUNTIME_ERROR);

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_RUNTIME_ERROR == ret);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_source_ == NULL
        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, NULL, shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        // OpenGLコンテキスト無しでも安全に通すため delete をNo-op
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_handle_ == NULL
        renderer_result_t ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: 既にコンパイル済み（shader_is_compiledがtrueになる状態を直接作る）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 123;  // compiled 扱い
        shader->program_id = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_BAD_OPERATION == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: 既にリンク済み（program_id != 0）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 0;
        shader->fragment_shader_handle = 0;
        shader->program_id = 1;  // linked 扱い

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_BAD_OPERATION == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_type_ が不正（resolve_target失敗）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        ret = gl33_shader_compile((shader_type_t)999, dummy_src, shader);
        assert(RENDERER_INVALID_ARGUMENT == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: glCreateShader が 0 を返す
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 0;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length == 0 かつ compile status が GL_TRUE ではない
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        // OpenGLコンテキスト無しでも安全に通す（実GL呼び出し回避）
        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 1;
        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;
        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_FALSE;
        s_fail_injection.length_glGetShaderiv = 0;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 だが、err_mes確保に失敗（render_mem_allocate失敗）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 2;
        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;
        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_FALSE;
        s_fail_injection.length_glGetShaderiv = 16;
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        render_mem_test_param_set(RENDERER_NO_MEMORY);

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_NO_MEMORY == ret);
        assert(0 == shader->vertex_shader_handle);

        render_mem_test_param_reset();

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 かつ compile status が GL_TRUE ではない（ログ付きエラー）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 3;
        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;
        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_FALSE;
        s_fail_injection.length_glGetShaderiv = 32;

        s_fail_injection.is_enabled_glGetShaderInfoLog = true;
        // 末尾\0はmock側で保証されるが、テスト側でも読みやすいように入れておく
        strcpy(s_fail_injection.log_str_glGetShaderInfoLog, "compile error (test)");

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_SHADER_COMPILE_ERROR == ret);
        assert(0 == shader->vertex_shader_handle);

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length > 0 かつ compile status == GL_TRUE（warningログ経路）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 10;
        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;
        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_TRUE;
        s_fail_injection.length_glGetShaderiv = 24;

        s_fail_injection.is_enabled_glGetShaderInfoLog = true;
        strcpy(s_fail_injection.log_str_glGetShaderInfoLog, "compile warning (test)");

        ret = gl33_shader_compile(SHADER_TYPE_VERTEX, dummy_src, shader);
        assert(RENDERER_SUCCESS == ret);
        assert(10 == shader->vertex_shader_handle);
        assert(0 == shader->program_id);

        // 後始末（OpenGL呼び出しはNo-op）
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length == 0 かつ compile status == GL_TRUE（最短成功経路）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        s_fail_injection.is_enabled_glCreateShader = true;
        s_fail_injection.result_glCreateShader = 11;
        s_fail_injection.is_enabled_glShaderSource_no_op = true;
        s_fail_injection.is_enabled_glCompileShader_no_op = true;
        s_fail_injection.is_enabled_glGetShaderiv = true;
        s_fail_injection.status_glGetShaderiv = GL_TRUE;
        s_fail_injection.length_glGetShaderiv = 0;

        ret = gl33_shader_compile(SHADER_TYPE_FRAGMENT, dummy_src, shader);
        assert(RENDERER_SUCCESS == ret);
        assert(11 == shader->fragment_shader_handle);
        assert(0 == shader->program_id);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_link(void) {
    {
        // 強制エラー出力（gl33_shader_link先頭のfail_injection分岐のカバレッジ確保）
        gl33_shader_fail_enable(RENDERER_RUNTIME_ERROR);

        renderer_result_t ret = gl33_shader_link(NULL);
        assert(RENDERER_RUNTIME_ERROR == ret);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_handle_ == NULL
        renderer_result_t ret = gl33_shader_link(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: program_id != 0（既にリンク済み扱い）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 99; // 既にリンク済み

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        // OpenGLコンテキスト無しでも安全に通すため delete をNo-op
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: vertex_shader が未コンパイル（vertex_shader_handle == 0）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 0;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: fragment_shader が未コンパイル（fragment_shader_handle == 0）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 0;
        shader->program_id = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: glCreateProgram が 0 を返す
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length == 0 かつ link status が GL_TRUE ではない
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        // OpenGLコンテキスト無しでも安全に通す（実GL呼び出し回避）
        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 10;

        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_FALSE;
        s_fail_injection.length_glGetProgramiv = 0;

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 だが、err_mes確保に失敗（render_mem_allocate失敗）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 11;

        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_FALSE;
        s_fail_injection.length_glGetProgramiv = 16;

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        render_mem_test_param_set(RENDERER_NO_MEMORY);

        ret = gl33_shader_link(shader);
        assert(RENDERER_NO_MEMORY == ret);
        assert(0 == shader->program_id);

        render_mem_test_param_reset();

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: info_log_length > 0 かつ link status が GL_TRUE ではない（ログ付きエラー）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 12;

        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_FALSE;
        s_fail_injection.length_glGetProgramiv = 32;

        s_fail_injection.is_enabled_glGetProgramInfoLog = true;
        strcpy(s_fail_injection.log_str_glGetProgramInfoLog, "link error (test)");

        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SHADER_LINK_ERROR == ret);
        assert(0 == shader->program_id);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length > 0 かつ link status == GL_TRUE（warningログ経路）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 20;

        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_TRUE;
        s_fail_injection.length_glGetProgramiv = 24;

        s_fail_injection.is_enabled_glGetProgramInfoLog = true;
        strcpy(s_fail_injection.log_str_glGetProgramInfoLog, "link warning (test)");

        ret = gl33_shader_link(shader);
        assert(RENDERER_SUCCESS == ret);
        assert(20 == shader->program_id);

        // 後始末（OpenGL呼び出しはNo-op）
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: info_log_length == 0 かつ link status == GL_TRUE（最短成功経路）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0;

        s_fail_injection.is_enabled_glCreateProgram = true;
        s_fail_injection.result_glCreateProgram = 21;

        s_fail_injection.is_enabled_glAttachShader_no_op = true;
        s_fail_injection.is_enabled_glLinkProgram_no_op = true;

        s_fail_injection.is_enabled_glGetProgramiv = true;
        s_fail_injection.status_glGetProgramiv = GL_TRUE;
        s_fail_injection.length_glGetProgramiv = 0;

        ret = gl33_shader_link(shader);
        assert(RENDERER_SUCCESS == ret);
        assert(21 == shader->program_id);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_use(void) {
    {
        // 強制エラー出力（gl33_shader_use先頭のfail_injection分岐のカバレッジ確保）
        gl33_shader_fail_enable(RENDERER_RUNTIME_ERROR);

        renderer_result_t ret = gl33_shader_use(NULL);
        assert(RENDERER_RUNTIME_ERROR == ret);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: shader_handle_ == NULL
        renderer_result_t ret = gl33_shader_use(NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: program_id == 0（未リンク）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;
        shader->program_id = 0; // 未リンク

        ret = gl33_shader_use(shader);
        assert(RENDERER_BAD_OPERATION == ret);

        // OpenGLコンテキスト無しでも安全に通すため delete をNo-op
        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: program_id != 0 だが vertex_shader が未コンパイル（データ破損扱い）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 1;            // リンク済み扱い
        shader->vertex_shader_handle = 0;  // 未コンパイル
        shader->fragment_shader_handle = 2;

        ret = gl33_shader_use(shader);
        assert(RENDERER_DATA_CORRUPTED == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 異常系: program_id != 0 だが fragment_shader が未コンパイル（データ破損扱い）
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 1;             // リンク済み扱い
        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 0; // 未コンパイル

        ret = gl33_shader_use(shader);
        assert(RENDERER_DATA_CORRUPTED == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
    {
        // 正常系: program_id != 0 かつ vertex/fragment ともに有効 → glUseProgram 呼び出し経路
        gl33_shader_fail_disable();
        render_mem_test_param_reset();

        gl33_shader_t* shader = NULL;
        renderer_result_t ret = gl33_shader_create(&shader);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != shader);

        shader->program_id = 123;
        shader->vertex_shader_handle = 1;
        shader->fragment_shader_handle = 2;

        // OpenGLコンテキスト無しでも安全に通すため No-op
        s_fail_injection.is_enabled_glUseProgram_no_op = true;

        ret = gl33_shader_use(shader);
        assert(RENDERER_SUCCESS == ret);

        s_fail_injection.is_enabled_glDeleteShader_no_op = true;
        s_fail_injection.is_enabled_glDeleteProgram_no_op = true;

        gl33_shader_destroy(&shader);
        assert(NULL == shader);

        gl33_shader_fail_disable();
    }
}

static void NO_COVERAGE test_gl33_shader_resolve_target(void) {
    {
        // 異常系: shader_handle_ == NULL
        GLenum out_type = 0;
        GLuint* out_target = NULL;

        renderer_result_t ret = gl33_shader_resolve_target(NULL, SHADER_TYPE_VERTEX, &out_type, &out_target);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_target);
    }
    {
        // 異常系: out_gl33_type_ == NULL
        gl33_shader_t shader;
        GLuint* out_target = NULL;

        renderer_result_t ret = gl33_shader_resolve_target(&shader, SHADER_TYPE_VERTEX, NULL, &out_target);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_target);
    }
    {
        // 異常系: out_target_ == NULL
        gl33_shader_t shader;
        GLenum out_type = 0;

        renderer_result_t ret = gl33_shader_resolve_target(&shader, SHADER_TYPE_VERTEX, &out_type, NULL);
        assert(RENDERER_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: *out_target_ != NULL（出力先が既にセット済み）
        gl33_shader_t shader;
        GLenum out_type = 0;

        GLuint dummy = 0;
        GLuint* out_target = &dummy; // *out_target_ が非NULL

        renderer_result_t ret = gl33_shader_resolve_target(&shader, SHADER_TYPE_VERTEX, &out_type, &out_target);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(&dummy == out_target);
    }
    {
        // 異常系: 未対応の shader_type_
        gl33_shader_t shader;
        GLenum out_type = 0;
        GLuint* out_target = NULL;

        renderer_result_t ret = gl33_shader_resolve_target(&shader, (shader_type_t)999, &out_type, &out_target);
        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == out_target);
    }
    {
        // 正常系: VERTEX
        gl33_shader_t shader;
        shader.vertex_shader_handle = 0;
        shader.fragment_shader_handle = 0;

        GLenum out_type = 0;
        GLuint* out_target = NULL;

        renderer_result_t ret = gl33_shader_resolve_target(&shader, SHADER_TYPE_VERTEX, &out_type, &out_target);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_VERTEX_SHADER == out_type);
        assert(&shader.vertex_shader_handle == out_target);

        *out_target = 77;
        assert(77 == shader.vertex_shader_handle);
    }
    {
        // 正常系: FRAGMENT
        gl33_shader_t shader;
        shader.vertex_shader_handle = 0;
        shader.fragment_shader_handle = 0;

        GLenum out_type = 0;
        GLuint* out_target = NULL;

        renderer_result_t ret = gl33_shader_resolve_target(&shader, SHADER_TYPE_FRAGMENT, &out_type, &out_target);
        assert(RENDERER_SUCCESS == ret);
        assert(GL_FRAGMENT_SHADER == out_type);
        assert(&shader.fragment_shader_handle == out_target);

        *out_target = 88;
        assert(88 == shader.fragment_shader_handle);
    }
}

static void NO_COVERAGE test_shader_is_compiled(void) {
    {
        // 異常系: shader_handle_ == NULL は常に false
        bool ret = shader_is_compiled(SHADER_TYPE_VERTEX, NULL);
        assert(false == ret);

        ret = shader_is_compiled(SHADER_TYPE_FRAGMENT, NULL);
        assert(false == ret);

        ret = shader_is_compiled((shader_type_t)999, NULL);
        assert(false == ret);
    }
    {
        // VERTEX: handle == 0 -> false
        gl33_shader_t shader;
        shader.vertex_shader_handle = 0;
        shader.fragment_shader_handle = 0;
        shader.program_id = 0;

        bool ret = shader_is_compiled(SHADER_TYPE_VERTEX, &shader);
        assert(false == ret);
    }
    {
        // VERTEX: handle != 0 -> true
        gl33_shader_t shader;
        shader.vertex_shader_handle = 1;
        shader.fragment_shader_handle = 0;
        shader.program_id = 0;

        bool ret = shader_is_compiled(SHADER_TYPE_VERTEX, &shader);
        assert(true == ret);
    }
    {
        // FRAGMENT: handle == 0 -> false
        gl33_shader_t shader;
        shader.vertex_shader_handle = 0;
        shader.fragment_shader_handle = 0;
        shader.program_id = 0;

        bool ret = shader_is_compiled(SHADER_TYPE_FRAGMENT, &shader);
        assert(false == ret);
    }
    {
        // FRAGMENT: handle != 0 -> true
        gl33_shader_t shader;
        shader.vertex_shader_handle = 0;
        shader.fragment_shader_handle = 2;
        shader.program_id = 0;

        bool ret = shader_is_compiled(SHADER_TYPE_FRAGMENT, &shader);
        assert(true == ret);
    }
    {
        // 異常系: 未対応の shader_type_ -> false
        gl33_shader_t shader;
        shader.vertex_shader_handle = 1;
        shader.fragment_shader_handle = 2;
        shader.program_id = 0;

        bool ret = shader_is_compiled((shader_type_t)999, &shader);
        assert(false == ret);
    }
}

#endif
