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

#include "engine/memory/general_allocator/general_allocator.h"

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

static renderer_backend_result_t gl33_shader_create(renderer_backend_shader_t** out_shader_handle_);
static void gl33_shader_destroy(renderer_backend_shader_t** shader_handle_);
static renderer_backend_result_t gl33_shader_compile(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_shader_use(const renderer_backend_shader_t* shader_handle_);
static renderer_backend_result_t gl33_uniform_location_get(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);
static renderer_backend_result_t gl33_mat4f_uniform_set(int32_t location_, bool should_transpose_, const float* data_);
static renderer_backend_result_t gl33_vec3f_uniform_set(int32_t location_, const float* data_);
static renderer_backend_result_t gl33_float_uniform_set(int32_t location_, float data_);
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
static void mock_glUniform3f(GLint location_, GLfloat r_, GLfloat g_, GLfloat b_);
static void mock_glUniform1f(GLint location_, GLfloat value_);
static void mock_glUniform4fv(GLint location_, GLsizei count_, const GLfloat* value_);
static GLint mock_glGetUniformLocation(GLuint program_, const GLchar *name_);

static bool gl33_shader_is_valid(const renderer_backend_shader_t* shader_);

static const renderer_shader_vtable_t s_gl33_shader_vtable = {
    .renderer_shader_create = gl33_shader_create,
    .renderer_shader_destroy = gl33_shader_destroy,
    .renderer_shader_compile = gl33_shader_compile,
    .renderer_shader_link = gl33_shader_link,
    .renderer_shader_use = gl33_shader_use,
    .renderer_shader_uniform_location_get = gl33_uniform_location_get,
    .renderer_shader_mat4f_uniform_set = gl33_mat4f_uniform_set,
    .renderer_shader_vec3f_uniform_set = gl33_vec3f_uniform_set,
    .renderer_shader_float_uniform_set = gl33_float_uniform_set,
    .renderer_shader_vec4u8_uniform_set = gl33_vec4u8_uniform_set,
    .renderer_shader_is_valid = gl33_shader_is_valid,
};  /**< OpenGL3.3用シェーダー操作仮想関数テーブル */

const renderer_shader_vtable_t* gl33_shader_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_shader_vtable;
}

/**
 * @brief Shader管理Objectを生成し、各Handleをゼロ初期化する
 *
 * @details
 * General Allocatorからrenderer_backend_shader_tのStorageを確保し、
 * 以下のフィールドを0に初期化する。
 *
 * - program_id
 * - vertex_shader_handle
 * - fragment_shader_handle
 *
 * 本関数ではOpenGLのShader ObjectおよびShader Programを生成しない。
 * GPU Resourceは後続のcompile／link処理で生成する。
 *
 * @param[out] out_shader_handle_
 * 生成した管理Objectの格納先。
 * 呼び出し前に*out_shader_handle_はNULLでなければならない。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * 管理Objectの確保と初期化が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * out_shader_handle_がNULL、*out_shader_handle_がNULLではない、
 * またはGeneral Allocatorが引数不正を返した
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * General Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * 管理ObjectのStorageを確保できなかった
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * General Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * General Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * General Allocatorで内部状態の破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * General Allocatorから未定義のResultを受け取った
 */
static renderer_backend_result_t gl33_shader_create(renderer_backend_shader_t** out_shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    renderer_backend_shader_t* tmp_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_create", "out_shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_create", "*out_shader_handle_")

    ret_general_allocator = general_allocator_allocate(sizeof(renderer_backend_shader_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_shader);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = renderer_backend_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("gl33_shader_create(%s) - general_allocator_allocate failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    tmp_shader->program_id = 0;
    tmp_shader->vertex_shader_handle = 0;
    tmp_shader->fragment_shader_handle = 0;
    *out_shader_handle_ = tmp_shader;

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

    general_allocator_free((void**)shader_handle_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

/**
 * @brief 指定されたシェーダーステージのソースコードをコンパイルする
 *
 * @details
 * Vertex / Fragment Shaderのいずれかを対象とする。
 * 指定ステージのハンドルが設定済み、またはプログラムがリンク済みの場合は
 * コンパイルを行わない。
 *
 * OpenGLシェーダーオブジェクトを生成し、ソースコードを設定して
 * コンパイルを実行する。
 * GL_COMPILE_STATUSによってコンパイル結果を確認する。
 *
 * Info Logが存在する場合はGeneral Allocatorで一時領域を確保し、
 * コンパイル失敗時にはError、成功時にはWarningとして出力する。
 *
 * コンパイルに成功した場合のみ、生成されたシェーダーハンドルを
 * shader_handle_の対応フィールドへ設定する。
 * 失敗した場合は、生成途中のシェーダーオブジェクトの削除を要求する。
 *
 * @pre shader_source_はNULL終端された読み取り可能な文字列であること
 *
 * @param[in] shader_stage_ コンパイル対象のシェーダーステージ
 * @param[in] shader_source_ コンパイル対象のシェーダーソースコード
 * @param[in,out] shader_handle_ シェーダーGPUリソース内部状態管理構造体インスタンス
 *
 * @retval RENDERER_BACKEND_SUCCESS コンパイルに成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - shader_source_がNULL
 * - shader_handle_がNULL
 * - shader_stage_が未対応の値
 * - General Allocatorが引数異常を返した
 * @retval RENDERER_BACKEND_BAD_OPERATION 以下のいずれか
 * - 対象ステージのシェーダーハンドルが設定済み
 * - シェーダープログラムがリンク済み
 * - General Allocatorが操作条件違反を返した
 * @retval RENDERER_BACKEND_SHADER_COMPILE_ERROR 以下のいずれか
 * - シェーダーオブジェクトの生成に失敗
 * - シェーダーソースコードのコンパイルに失敗
 * @retval RENDERER_BACKEND_NO_MEMORY 一時領域のメモリ確保に失敗
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED General Allocatorの管理上限を超過
 * @retval RENDERER_BACKEND_OVERFLOW General Allocatorでオーバーフローを検出
 * @retval RENDERER_BACKEND_DATA_CORRUPTED General Allocatorで内部状態の破損を検出
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR General Allocatorから未定義のResultを受け取った
 */
static renderer_backend_result_t gl33_shader_compile(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    GLenum gl33_shader_stage;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;
    GLuint tmp_handle = 0;
    GLuint* handle_addr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_compile", "shader_handle_")

    // シェーダーオブジェクトのコンパイル状況チェック
    if(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(shader_stage_, shader_handle_)) {
        ret = RENDERER_BACKEND_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader object is already compiled.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムのリンク状況チェック
    if(0 != shader_handle_->program_id) {
        ret = RENDERER_BACKEND_BAD_OPERATION;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Shader program is already linked.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // シェーダーオブジェクトハンドルを取得
    ret = gl33_shader_handle_addr_get(shader_handle_, shader_stage_, &handle_addr);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_handle_addr_get).", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // シェーダー種別をOpenGLで使用可能な値に変換
    ret = gl33_shader_resolve_target(shader_stage_, &gl33_shader_stage);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        // NOTE: gl33_shader_handle_addr_getで既にエラー処理されているため、ここに来ることはないが将来的な変更のために残しておく
        ERROR_MESSAGE("gl33_shader_compile(%s) - Unsupported shader type(gl33_shader_resolve_target).", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    tmp_handle = mock_glCreateShader(gl33_shader_stage);
    if(0 == tmp_handle) {
        ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to create shader object handle.", renderer_backend_result_to_str(ret));
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
        ret_general_allocator = general_allocator_allocate((size_t)info_log_length, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&err_mes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = renderer_backend_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("gl33_shader_compile(%s) - general_allocator_allocate failed.", renderer_backend_result_to_str(ret));
            goto cleanup;
        }

        mock_glGetShaderInfoLog(tmp_handle, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
            ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source: '%s'", renderer_backend_result_to_str(ret), err_mes);
            general_allocator_free((void**)&err_mes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_compile - info log: %s", err_mes);
            general_allocator_free((void**)&err_mes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_BACKEND_SHADER_COMPILE_ERROR;
        ERROR_MESSAGE("gl33_shader_compile(%s) - Failed to compile shader source.", renderer_backend_result_to_str(ret));
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
 * @brief コンパイル済みのVertex / Fragment Shaderをリンクする
 *
 * @details
 * Vertex ShaderとFragment ShaderのHandleが設定されており、
 * Shader Programが未リンクであることを確認する。
 *
 * glCreateProgram()でProgramを生成し、
 * 両ShaderをAttachしてglLinkProgram()を実行する。
 *
 * GL_LINK_STATUSを取得し、リンク結果を判定する。
 * Info Logが存在する場合はGeneral Allocatorで一時Bufferを確保し、
 * Log内容を取得する。
 *
 * リンク成功後、生成したProgram Handleを
 * shader_handle_->program_idへ設定する。
 *
 * リンクに失敗した場合は、生成途中のProgram Handleの
 * 削除を要求し、program_idは変更しない。
 *
 * リンク成功後もVertex / Fragment Shader Handleを保持し、
 * これらはdestroy時にProgramとともに破棄する。
 *
 * @param[in,out] shader_handle_
 * コンパイル済みShaderを保持する管理Object
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * Programのリンクに成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * shader_handle_がNULL、またはGeneral Allocatorが引数不正を返した
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Programがリンク済み、Vertex Shaderが未コンパイル、
 * Fragment Shaderが未コンパイル、
 * またはGeneral Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_SHADER_LINK_ERROR
 * Program Handleを生成できない、またはリンクに失敗した
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * Info Log用Storageの確保に失敗した
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * General Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * General Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * General Allocatorで内部状態の破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * General Allocatorから未定義のResultを受け取った
 */
static renderer_backend_result_t gl33_shader_link(renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    GLuint tmp_program_id = 0;
    GLint result = GL_FALSE;
    GLint info_log_length = 0;
    char* err_mes = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_link", "shader_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == shader_handle_->program_id, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_result_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "shader_handle_->program_id")
    // バーテックスシェーダーとフラグメントシェーダーは必須なので、有効な状態でなければエラー
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_STAGE_VERTEX, shader_handle_), ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_result_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "vertex_shader_handle")
    IF_ARG_FALSE_GOTO_CLEANUP(SHADER_COMPILE_STATUS_COMPILED == shader_compile_status_get(SHADER_STAGE_FRAGMENT, shader_handle_), ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_result_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_link", "fragment_shader_handle")

    // プログラムをリンク
    tmp_program_id = mock_glCreateProgram();
    if(0 == tmp_program_id) {
        ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to create shader program.", renderer_backend_result_to_str(ret));
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
        ret_general_allocator = general_allocator_allocate((size_t)info_log_length, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&err_mes);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = renderer_backend_result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("gl33_shader_link(%s) - general_allocator_allocate failed.", renderer_backend_result_to_str(ret));
            goto cleanup;
        }

        mock_glGetProgramInfoLog(tmp_program_id, info_log_length, NULL, err_mes);
        if(GL_TRUE != result) {
            ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
            ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program: '%s'", renderer_backend_result_to_str(ret), err_mes);
            general_allocator_free((void**)&err_mes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
            goto cleanup;
        } else {
            WARN_MESSAGE("gl33_shader_link - info log: %s", err_mes);
            general_allocator_free((void**)&err_mes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
    } else if(GL_TRUE != result) {
        ret = RENDERER_BACKEND_SHADER_LINK_ERROR;
        ERROR_MESSAGE("gl33_shader_link(%s) - Failed to link shader program.", renderer_backend_result_to_str(ret));
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
 * @details
 * program_idが0ではないことを確認する。
 * また、リンク済みProgramに対応するVertex / Fragment Shaderの
 * Handleが両方とも0ではないことを確認する。
 *
 * 条件を満たした場合、glUseProgram()を呼び出す。
 *
 * 本関数はOpenGL側のProgram使用結果を検証しない。
 * Shader Objectが実際にコンパイル済みであるかも、
 * OpenGLへ問い合わせて再検証するわけではない。
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

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_use", "shader_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != shader_handle_->program_id, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_result_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_shader_use", "shader_handle_->program_id")

    if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_STAGE_VERTEX, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにvertex_shaderがコンパイル済みではないのは異常
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Vertex shader object is not compiled.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
    // TODO: 現状の失敗注入では、shader_compile_status_getの連続呼び出しに対して両方とも強制出力をさせることができないため、下のifはテスト不可(失敗注入方式を引数のシェーダー種別に応じて切り替えるように修正する)
    if(SHADER_COMPILE_STATUS_COMPILED != shader_compile_status_get(SHADER_STAGE_FRAGMENT, shader_handle_)) {
        // 既にprogram_idが0ではなく、リンクされているのにfragment_shaderがコンパイル済みではないのは異常
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("gl33_shader_use(%s) - Fragment shader object is not compiled.", renderer_backend_result_to_str(ret));
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

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_uniform_location_get", "out_location_")

    tmp_location = mock_glGetUniformLocation(shader_handle_->program_id, name_);
    if(-1 == tmp_location) {
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("gl33_uniform_location_get(%s) - Failed to get uniform location. name: %s", renderer_backend_result_to_str(ret), name_);
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

    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "data_")

    mock_glUniformMatrix4fv(location_, 1, should_transpose_, data_);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムにvec3f型のユニフォーム変数を送信する
 *
 * @details
 * data_が指す3個のfloat値をglUniform3f()へ渡す。
 *
 * @note OpenGL側の実行結果は検証しない。
 * @warning 本関数の呼び出し前に対象のシェーダープログラムをuse状態にすること
 * @pre data_は3個のfloat値を読み取り可能なメモリ領域を指していること
 *
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] data_ 送信する3個のfloat値へのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_がNULL
 * @retval RENDERER_BACKEND_SUCCESS OpenGLへのユニフォーム設定要求が完了
 */
static renderer_backend_result_t gl33_vec3f_uniform_set(int32_t location_, const float* data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_mat4f_uniform_set", "data_")

    mock_glUniform3f(location_, data_[0], data_[1], data_[2]);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief シェーダープログラムにfloat型のユニフォーム変数を送信する
 *
 * @details
 * 指定されたfloat値をglUniform1f()へ渡す。
 *
 * 現在の実装では、引数の値域やOpenGL側の実行結果を検証せず、
 * OpenGL APIの呼び出し後にSUCCESSを返す。
 *
 * @warning 本関数の呼び出し前に対象のシェーダープログラムをuse状態にすること
 *
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] value_ 送信するfloat値
 *
 * @retval RENDERER_BACKEND_SUCCESS OpenGLへのユニフォーム設定要求が完了
 */
static renderer_backend_result_t gl33_float_uniform_set(int32_t location_, float value_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    mock_glUniform1f(location_, value_);

    ret = RENDERER_BACKEND_SUCCESS;

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

    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vec4u8_uniform_set", "data_")

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

/**
 * @brief 指定されたシェーダーステージのハンドル格納先アドレスを取得する
 *
 * @details
 * shader_handle_が保持するシェーダーハンドルのうち、
 * shader_stage_に対応するフィールドのアドレスを取得する。
 *
 * - SHADER_STAGE_VERTEX: vertex_shader_handle
 * - SHADER_STAGE_FRAGMENT: fragment_shader_handle
 *
 * 取得するアドレスは管理構造体内部のフィールドを指す。
 * 新しいメモリ領域の確保は行わない。
 *
 * @note 取得したアドレスの有効期間はshader_handle_の有効期間に従う。
 * @note エラー時は*out_handle_addr_を変更しない。
 *
 * @param[in,out] shader_handle_ シェーダーGPUリソース内部状態管理構造体インスタンス
 * @param[in] shader_stage_ アドレスを取得するシェーダーステージ
 * @param[out] out_handle_addr_ シェーダーハンドル格納先アドレスの出力先
 *
 * @retval RENDERER_BACKEND_SUCCESS ハンドル格納先アドレスの取得に成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_がNULL
 * - out_handle_addr_がNULL
 * - shader_stage_が未対応の値
 */
static renderer_backend_result_t gl33_shader_handle_addr_get(renderer_backend_shader_t* shader_handle_, shader_stage_t shader_stage_, GLuint** out_handle_addr_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    GLuint* tmp_handle = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(out_handle_addr_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_handle_addr_get", "out_handle_addr_")

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

/**
 * @brief GLCEのシェーダーステージをOpenGL 3.3のシェーダー種別へ変換する
 *
 * @details
 * 次の変換を行う。
 *
 * - SHADER_STAGE_VERTEXからGL_VERTEX_SHADER
 * - SHADER_STAGE_FRAGMENTからGL_FRAGMENT_SHADER
 *
 * 未対応のシェーダーステージでは変換を行わない。
 *
 * @note エラー時は*out_gl33_type_を変更しない。
 *
 * @param[in] shader_stage_ 変換対象のシェーダーステージ
 * @param[out] out_gl33_type_ 変換後のOpenGLシェーダー種別の格納先
 *
 * @retval RENDERER_BACKEND_SUCCESS 変換に成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - out_gl33_type_がNULL
 * - shader_stage_が未対応の値
 */
static renderer_backend_result_t gl33_shader_resolve_target(shader_stage_t shader_stage_, GLenum* out_gl33_type_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    GLenum tmp_type = GL_VERTEX_SHADER;

    IF_ARG_NULL_GOTO_CLEANUP(out_gl33_type_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_shader_resolve_target", "out_gl33_type_")

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

/**
 * @brief 指定されたシェーダーステージのコンパイル管理状態を取得する
 *
 * @details
 * 対象ステージのシェーダーハンドルが0であるかどうかを確認し、
 * コンパイル管理状態を返す。
 *
 * @note 本関数はOpenGLのコンパイル結果を問い合わせない。
 * SHADER_COMPILE_STATUS_COMPILEDは、対応するシェーダーハンドルが
 * 0ではないことを意味する。
 *
 * @param[in] shader_stage_ 検査対象のシェーダーステージ
 * @param[in] shader_handle_ シェーダーGPUリソース内部状態管理構造体インスタンス
 *
 * @return shader_compile_status_t コンパイル管理状態
 *
 * @retval SHADER_COMPILE_STATUS_NOT_COMPILED 対象ステージのハンドルが0
 * @retval SHADER_COMPILE_STATUS_COMPILED 対象ステージのハンドルが0ではない
 * @retval SHADER_COMPILE_STATUS_UNSUPPORTED_SHADER_STAGE 未対応のシェーダーステージ
 * @retval SHADER_COMPILE_STATUS_INVALID_SHADER_HANDLE shader_handle_がNULL
 */
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

static void NO_COVERAGE mock_glUniform3f(GLint location_, GLfloat r_, GLfloat g_, GLfloat b_) {
    glUniform3f(location_, r_, g_, b_);
}

static void NO_COVERAGE mock_glUniform1f(GLint location_, GLfloat value_) {
    glUniform1f(location_, value_);
}

static void NO_COVERAGE mock_glUniform4fv(GLint location_, GLsizei count_, const GLfloat *value_) {
    glUniform4fv(location_, count_, value_);
}

static GLint NO_COVERAGE mock_glGetUniformLocation(GLuint program_, const GLchar *name_) {
    return glGetUniformLocation(program_, name_);
}

/**
 * @brief OpenGL 3.3用シェーダーGPUリソース内部状態管理構造体の妥当性を検証する
 *
 * @details
 * shader_がNULLではないことを確認する。
 *
 * program_idが0ではない場合は、以下の条件も検証する。
 *
 * - vertex_shader_handleが0ではない
 * - fragment_shader_handleが0ではない
 *
 * program_idが0の場合は、両ステージのハンドルが0でも
 * validと判定する。
 * そのため、生成直後および一部のステージだけを
 * コンパイルした状態も許容する。
 *
 * @note OpenGL側のリソース存在確認および実際のコンパイル・リンク状態の
 * 問い合わせは行わない。
 *
 * @param[in] shader_ 検証対象のシェーダーGPUリソース内部状態管理構造体インスタンス
 *
 * @retval true 構造的妥当性を満たす
 * @retval false 構造的妥当性を満たさない
 */
static bool gl33_shader_is_valid(const renderer_backend_shader_t* shader_) {
    if(NULL == shader_) {
        return false;
    }

    if(0 != shader_->program_id) {
        if(0 == shader_->vertex_shader_handle) {
            return false;
        }
        if(0 == shader_->fragment_shader_handle) {
            return false;
        }
    }

    return true;
}
