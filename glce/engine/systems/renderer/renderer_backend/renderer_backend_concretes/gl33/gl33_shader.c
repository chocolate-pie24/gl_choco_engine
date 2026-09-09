// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file gl33_shader.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダーオブジェクト/シェーダープログラム操作関数の実装
 *
 * @date 2026-01-03
 *
 */
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_shader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_shader_vtable.h"

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
    SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_STAGE,     /**< サポート対象外のシェーダーステージ */
    SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE,        /**< 入力されたシェーダーハンドルが不正 */
} shader_compile_status_t;

static renderer_backend_result_t gl33_shader_create(renderer_backend_shader_t** shader_handle_);
static void gl33_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_backend_result_t gl33_shader_compile(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_shader_use(const renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_backend_result_t gl33_mat4f_uniform_set(int32_t location_, bool should_transpose_, const float* data_);
static renderer_backend_result_t gl33_vec4u8_uniform_set(int32_t location_, const uint8_t* data_);

static renderer_backend_result_t gl33_shader_handle_addr_get(renderer_backend_shader_t* shader_handle_, shader_stage_t shader_stage_, GLuint** out_handle_addr_);
static renderer_backend_result_t gl33_shader_resolve_target(shader_stage_t shader_stage_, GLenum* out_gl33_type_);
static shader_compile_status_t shader_compile_status_get(shader_stage_t shader_stage_, const renderer_backend_shader_t* shader_handle_);

static void mock_glDeleteShader(GLuint shader_);
static void mock_glDeleteProgram(GLuint program_);
static GLuint mock_glCreateShader(GLenum shader_stage_);
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
static void mock_glUniform4fv(GLint location_, GLsizei count_, const GLfloat *value_);
static GLint mock_glGetUniformLocation(GLuint program_, const GLchar *name_);

static const renderer_shader_vtable_t s_gl33_shader_vtable = {
    .renderer_shader_create = gl33_shader_create,
    .renderer_shader_destroy = gl33_shader_destroy,
    .renderer_shader_compile = gl33_shader_compile,
    .renderer_shader_link = gl33_shader_link,
    .renderer_shader_use = gl33_shader_use,
    .renderer_shader_uniform_location_get = gl33_uniform_location_get,
    .renderer_shader_mat4f_uniform_set = gl33_mat4f_uniform_set,
    .renderer_shader_vec4u8_uniform_set = gl33_vec4u8_uniform_set,
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
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システム使用可能範囲上限超過
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_shader_create(renderer_backend_shader_t** shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_create", "*shader_handle_")

    ret_memory_system = memory_system_allocate(sizeof(renderer_backend_shader_t), MEMORY_TAG_RENDERER, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("gl33_shader_create(%s) - Failed to allocate memory for shader handle.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->program_id = 0;
    tmp->vertex_shader_handle = 0;
    tmp->fragment_shader_handle = 0;
    *shader_handle_ = tmp;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
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

    memory_system_free(*shader_handle_, sizeof(renderer_backend_shader_t), MEMORY_TAG_RENDERER);
    *shader_handle_ = NULL;
}

static renderer_backend_result_t gl33_shader_compile(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    GLenum gl33_shader_stage;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;
    GLuint tmp_handle = 0;
    GLuint* handle_addr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_compile", "shader_handle_")

    // シェーダーオブジェクトのコンパイル状況チェック
    if(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(shader_stage_, shader_handle_)) {
        ret = RENDERER_BACKEND_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader object is already compiled.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムのリンク状況チェック
    if(0 != shader_handle_->program_id) {
        ret = RENDERER_BACKEND_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader program is already linked.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーオブジェクトハンドルを取得
    ret = gl33_shader_handle_addr_get(shader_handle_, shader_stage_, &handle_addr);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_handle_addr_get).", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダー種別をOpenGLで使用可能な値に変換
    ret = gl33_shader_resolve_target(shader_stage_, &gl33_shader_stage);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        // NOTE: gl33_shader_handle_addr_getで既にエラー処理されているため、ここに来ることはないが将来的な変更のために残しておく
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_resolve_target).", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_handle = mock_glCreateShader(gl33_shader_stage);
    if(0 == tmp_handle) {
        ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to create shader object handle.", renderer_backend_rslt_to_str(ret));
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
        ret_memory_system = memory_system_allocate((size_t)info_log_length, MEMORY_TAG_RENDERER, (void**)&err_mes);
        if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
            ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to allocate memory for shader info log.", renderer_backend_rslt_to_str(ret));
            goto cleanup;
        }
        mock_glGetShaderInfoLog(tmp_handle, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source: '%s'", renderer_backend_rslt_to_str(ret), err_mes);
            memory_system_free(err_mes, (size_t)info_log_length, MEMORY_TAG_RENDERER);
            err_mes = NULL;
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_compile - info log: %s", err_mes);
            memory_system_free(err_mes, (size_t)info_log_length, MEMORY_TAG_RENDERER);
            err_mes = NULL;
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    *handle_addr = tmp_handle;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    if(RENDERER_BACKEND_SUCCESS != ret && 0 != tmp_handle) {
        mock_glDeleteShader(tmp_handle);
    }
    return ret;
}

/**
 * @brief コンパイル済みシェーダーオブジェクトをリンクし、OpenGLシェーダープログラムを生成する
 *
 * @param[in,out] shader_handle_ シェーダー関連リソース管理構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT shader_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムがすでにリンク済み
 * - バーテックスシェーダーが未コンパイル
 * - フラグメントシェーダーが未コンパイル
 * @retval RENDERER_BACKEND_SHADER_LINK_ERROR シェーダーリンクエラー
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    GLuint tmp_program_id = 0;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_link", "shader_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == shader_handle_->program_id, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "shader_handle_->program_id")
    // バーテックスシェーダーとフラグメントシェーダーは必須なので、有効な状態でなければエラー
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_STAGE_VERTEX, shader_handle_), ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "vertex_shader_handle")
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_STAGE_FRAGMENT, shader_handle_), ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "fragment_shader_handle")

    // プログラムをリンク
    tmp_program_id = mock_glCreateProgram();
    if(0 == tmp_program_id) {
        ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to create shader program.", renderer_backend_rslt_to_str(ret));
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
        ret_memory_system = memory_system_allocate((size_t)info_log_length, MEMORY_TAG_RENDERER, (void**)&err_mes);
        if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
            ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to allocate memory for program info log.", renderer_backend_rslt_to_str(ret));
            goto cleanup;
        }
        mock_glGetProgramInfoLog(tmp_program_id, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program: '%s'", renderer_backend_rslt_to_str(ret), err_mes);
            memory_system_free(err_mes, (size_t)info_log_length, MEMORY_TAG_RENDERER);
            err_mes = NULL;
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_link - info log: %s", err_mes);
            memory_system_free(err_mes, (size_t)info_log_length, MEMORY_TAG_RENDERER);
            err_mes = NULL;
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    // use関数呼び出し時に、バリデーション用にprogram_id != 0かつshader_object_handle == 0でDATA_CORRUPTEDにするため、シェーダーオブジェクトのデストロイは行わない(shader_destroy APIでまとめて破棄する)
    shader_handle_->program_id = tmp_program_id;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    if(RENDERER_BACKEND_SUCCESS != ret && 0 != tmp_program_id) {
        mock_glDeleteProgram(tmp_program_id);
    }
    return ret;
}

/**
 * @brief OpenGLシェーダープログラムを切り替える
 *
 * @param[in] shader_handle_ 切り替え先シェーダープログラムを管理する内部状態管理構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT shader_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
 * - program_idが設定されているにもかかわらず、バーテックスシェーダーオブジェクトハンドルが未設定
 * - program_idが設定されているにもかかわらず、フラグメントシェーダーオブジェクトハンドルが未設定
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_shader_use(const renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_use", "shader_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != shader_handle_->program_id, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_use", "shader_handle_->program_id")

    if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_STAGE_VERTEX, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにvertex_shaderがコンパイル済みではないのは異常
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Vertex shader object is not compiled.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    // TODO: 現状の失敗注入では、shader_compile_status_getの連続呼び出しに対して両方とも強制出力をさせることができないため、下のifはテスト不可(失敗注入方式を引数のシェーダー種別に応じて切り替えるように修正する)
    if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_STAGE_FRAGMENT, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにfragment_shaderがコンパイル済みではないのは異常
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Fragment shader object is not compiled.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    mock_glUseProgram(shader_handle_->program_id);

    ret = RENDERER_BACKEND_SUCCESS;

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
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - name_ == NULL
 * - out_location_ == NULL
 * @retval RENDERER_BACKEND_RUNTIME_ERROR ユニフォーム変数が存在しない、未使用として最適化された、またはLocation取得に失敗
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    int32_t tmp_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "out_location_")

    tmp_location = mock_glGetUniformLocation(shader_handle_->program_id, name_);
    if(-1 == tmp_location) {
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("gl33_uniform_location_get(%s) - Failed to get uniform location. name: %s", renderer_backend_rslt_to_str(ret), name_);
        goto cleanup;
    }
    *out_location_ = tmp_location;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
 *
 * @note OpenGL 3.3実装
 * @note 本APIを呼ぶ前に必ずシェーダープログラムをuse状態にすること
 *
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
 * @param[in] data_ 送信データへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_ == NULL
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_mat4f_uniform_set(int32_t location_, bool should_transpose_, const float* data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "data_")

    mock_glUniformMatrix4fv(location_, 1, should_transpose_, data_);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムにvec4u8型のユニフォーム変数を送信する
 *
 * @note OpenGL 3.3実装
 * @note 本APIを呼ぶ前に必ずシェーダープログラムをuse状態にすること
 *
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] data_ 送信データへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_ == NULL
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vec4u8_uniform_set(int32_t location_, const uint8_t* data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    float data_f[4] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vec4u8_uniform_set", "data_")

    // shader側はvec4なので0...1に正規化
    data_f[0] = (float)(data_[0]) / 255.0f;
    data_f[1] = (float)(data_[1]) / 255.0f;
    data_f[2] = (float)(data_[2]) / 255.0f;
    data_f[3] = (float)(data_[3]) / 255.0f;
    mock_glUniform4fv(location_, 1, data_f);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static renderer_backend_result_t gl33_shader_handle_addr_get(renderer_backend_shader_t* shader_handle_, shader_stage_t shader_stage_, GLuint** out_handle_addr_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    GLuint* tmp_handle = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(out_handle_addr_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "out_handle_addr_")

    switch(shader_stage_) {
    case SHADER_STAGE_VERTEX:
        tmp_handle = &shader_handle_->vertex_shader_handle;
        break;
    case SHADER_STAGE_FRAGMENT:
        tmp_handle = &shader_handle_->fragment_shader_handle;
        break;
    default:
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        goto cleanup;
    }
    *out_handle_addr_ = tmp_handle;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static renderer_backend_result_t gl33_shader_resolve_target(shader_stage_t shader_stage_, GLenum* out_gl33_type_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    GLenum tmp_type = GL_VERTEX_SHADER;

    IF_ARG_NULL_GOTO_CLEANUP(out_gl33_type_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_resolve_target", "out_gl33_type_")

    switch(shader_stage_) {
    case SHADER_STAGE_VERTEX:
        tmp_type = GL_VERTEX_SHADER;
        break;
    case SHADER_STAGE_FRAGMENT:
        tmp_type = GL_FRAGMENT_SHADER;
        break;
    default:
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        goto cleanup;
    }
    *out_gl33_type_ = tmp_type;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static shader_compile_status_t shader_compile_status_get(shader_stage_t shader_stage_, const renderer_backend_shader_t* shader_handle_) {
    shader_compile_status_t status = SHADER_COMPILE_STATUS_NOT_COMPILED;

    if(NULL == shader_handle_) {
        status = SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE;
    } else {
        switch(shader_stage_) {
        case SHADER_STAGE_VERTEX:
            status = (0 == shader_handle_->vertex_shader_handle) ? SHADER_COMPILE_STATUS_NOT_COMPILED : SHADER_COMPILE_STATUS_COMPILED;
            break;
        case SHADER_STAGE_FRAGMENT:
            status = (0 == shader_handle_->fragment_shader_handle) ? SHADER_COMPILE_STATUS_NOT_COMPILED : SHADER_COMPILE_STATUS_COMPILED;
            break;
        default:
            status = SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_STAGE;
        }
    }

    return status;
}

static void NO_COVERAGE mock_glDeleteShader(GLuint shader_) {
    glDeleteShader(shader_);
}

static void NO_COVERAGE mock_glDeleteProgram(GLuint program_) {
    glDeleteProgram(program_);
}

static GLuint NO_COVERAGE mock_glCreateShader(GLenum shader_stage_) {
    return glCreateShader(shader_stage_);
}

static GLuint NO_COVERAGE mock_glCreateProgram(void) {
    return glCreateProgram();
}

static void NO_COVERAGE mock_glShaderSource(GLuint shader_, GLsizei count_, const GLchar **string_, const GLint *length_) {
    glShaderSource(shader_, count_, string_, length_);
}

static void NO_COVERAGE mock_glCompileShader(GLuint shader_) {
    glCompileShader(shader_);
}

static void NO_COVERAGE mock_glGetShaderiv(GLuint shader_, GLenum pname_, GLint *params_) {
    glGetShaderiv(shader_, pname_, params_);
}

static void NO_COVERAGE mock_glGetShaderInfoLog(GLuint shader_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
    glGetShaderInfoLog(shader_, maxLength_, length_, infoLog_);
}

static void NO_COVERAGE mock_glAttachShader(GLuint program_, GLuint shader_) {
    glAttachShader(program_, shader_);
}

static void NO_COVERAGE mock_glLinkProgram(GLuint program_) {
    glLinkProgram(program_);
}

static void NO_COVERAGE mock_glGetProgramiv(GLuint program_, GLenum pname_, GLint *params_) {
    glGetProgramiv(program_, pname_, params_);
}

static void NO_COVERAGE mock_glGetProgramInfoLog(GLuint program_, GLsizei maxLength_, GLsizei *length_, GLchar *infoLog_) {
    glGetProgramInfoLog(program_, maxLength_, length_, infoLog_);
}

static void NO_COVERAGE mock_glUseProgram(GLuint program_) {
    glUseProgram(program_);
}

static void NO_COVERAGE mock_glUniformMatrix4fv(GLint location_, GLsizei count_, GLboolean transpose_, const GLfloat *value_) {
    glUniformMatrix4fv(location_, count_, transpose_, value_);
}

static void NO_COVERAGE mock_glUniform4fv(GLint location_, GLsizei count_, const GLfloat *value_) {
    glUniform4fv(location_, count_, value_);
}

static GLint NO_COVERAGE mock_glGetUniformLocation(GLuint program_, const GLchar *name_) {
    return glGetUniformLocation(program_, name_);
}
